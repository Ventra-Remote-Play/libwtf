#include <msquic.h>

#include "log.h"
#include "types.h"
#include "utils.h"
#include "wtf.h"

#define WTF_MAX_WORKER_PROCESSORS 256u

#ifdef WTF_EXTERNAL_MSQUIC
typedef QUIC_EXECUTION_CONFIG wtf_quic_execution_config_t;
#define WTF_QUIC_EXECUTION_CONFIG_FLAGS_NONE QUIC_EXECUTION_CONFIG_FLAG_NONE
#define WTF_QUIC_EXECUTION_CONFIG_MIN_SIZE QUIC_EXECUTION_CONFIG_MIN_SIZE
#else
typedef QUIC_GLOBAL_EXECUTION_CONFIG wtf_quic_execution_config_t;
#define WTF_QUIC_EXECUTION_CONFIG_FLAGS_NONE QUIC_GLOBAL_EXECUTION_CONFIG_FLAG_NONE
#define WTF_QUIC_EXECUTION_CONFIG_MIN_SIZE QUIC_GLOBAL_EXECUTION_CONFIG_MIN_SIZE
#endif

#define WTF_EXECUTION_CONFIG_STORAGE_SIZE \
    (WTF_QUIC_EXECUTION_CONFIG_MIN_SIZE + WTF_MAX_WORKER_PROCESSORS * sizeof(uint16_t))

// QUIC_PARAM_GLOBAL_EXECUTION_CONFIG is process-global. MsQuic keeps using the
// supplied processor list after SetParam returns, so the backing storage must
// live for the lifetime of the MsQuic library, not merely for context creation.
static uint64_t wtf_execution_config_storage[
    (WTF_EXECUTION_CONFIG_STORAGE_SIZE + sizeof(uint64_t) - 1) / sizeof(uint64_t)];

static QUIC_STATUS wtf_apply_execution_config(const QUIC_API_TABLE* quic_api,
                                              const wtf_context_config_t* config)
{
    if (config->worker_thread_count == 0) {
        return QUIC_STATUS_SUCCESS;
    }

    uint32_t processor_count = config->enable_load_balancing
                                   ? config->worker_thread_count
                                   : 1u;
    if (processor_count > WTF_MAX_WORKER_PROCESSORS) {
        processor_count = WTF_MAX_WORKER_PROCESSORS;
    }
    uint32_t processor_offset = config->worker_processor_offset;
    if (processor_offset >= WTF_MAX_WORKER_PROCESSORS) {
        return QUIC_STATUS_INVALID_PARAMETER;
    }
    if (processor_count > WTF_MAX_WORKER_PROCESSORS - processor_offset) {
        processor_count = WTF_MAX_WORKER_PROCESSORS - processor_offset;
    }

    uint32_t config_size = WTF_QUIC_EXECUTION_CONFIG_MIN_SIZE +
                           processor_count * sizeof(uint16_t);
    memset(wtf_execution_config_storage, 0, WTF_EXECUTION_CONFIG_STORAGE_SIZE);
    wtf_quic_execution_config_t* execution_config =
        (wtf_quic_execution_config_t*)wtf_execution_config_storage;
    execution_config->Flags = WTF_QUIC_EXECUTION_CONFIG_FLAGS_NONE;
    execution_config->PollingIdleTimeoutUs = 0;
    execution_config->ProcessorCount = processor_count;
    for (uint32_t i = 0; i < processor_count; ++i) {
        execution_config->ProcessorList[i] = (uint16_t)(processor_offset + i);
    }

    return quic_api->SetParam(
        NULL,
        QUIC_PARAM_GLOBAL_EXECUTION_CONFIG,
        config_size,
        execution_config);
}

wtf_result_t wtf_context_create(const wtf_context_config_t* config, wtf_context_t** context)
{
    if (!config || !context) {
        return WTF_ERROR_INVALID_PARAMETER;
    }

    wtf_context_t* ctx = malloc(sizeof(wtf_context_t));
    if (!ctx) {
        return WTF_ERROR_OUT_OF_MEMORY;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->config = *config;
    ctx->log_level = config->log_level;
    ctx->log_callback = config->log_callback;
    ctx->log_user_context = config->log_user_context;

    if (mtx_init(&ctx->mutex, mtx_plain) != thrd_success) {
        free(ctx);
        return WTF_ERROR_INTERNAL;
    }

    QUIC_STATUS status = MsQuicOpen2(&ctx->quic_api);
    if (QUIC_FAILED(status)) {
        WTF_LOG_CRITICAL(ctx, "context", "MsQuicOpen2 failed: 0x%x", status);
        mtx_destroy(&ctx->mutex);
        free(ctx);
        return wtf_quic_status_to_result(status);
    }

    status = wtf_apply_execution_config(ctx->quic_api, config);
    if (QUIC_FAILED(status)) {
        WTF_LOG_CRITICAL(ctx, "context", "MsQuic execution configuration failed: 0x%x", status);
        MsQuicClose(ctx->quic_api);
        mtx_destroy(&ctx->mutex);
        free(ctx);
        return wtf_quic_status_to_result(status);
    }

    QUIC_EXECUTION_PROFILE execution_profile = QUIC_EXECUTION_PROFILE_LOW_LATENCY;
    if (config->execution_profile == WTF_EXECUTION_PROFILE_MAX_THROUGHPUT) {
        execution_profile = QUIC_EXECUTION_PROFILE_TYPE_MAX_THROUGHPUT;
    } else if (config->execution_profile == WTF_EXECUTION_PROFILE_REAL_TIME) {
        execution_profile = QUIC_EXECUTION_PROFILE_TYPE_REAL_TIME;
    } else if (config->execution_profile == WTF_EXECUTION_PROFILE_SCAVENGER) {
        execution_profile = QUIC_EXECUTION_PROFILE_TYPE_SCAVENGER;
    }

    const QUIC_REGISTRATION_CONFIG reg_config = {.AppName = "libwtf",
                                                 .ExecutionProfile = execution_profile};

    status = ctx->quic_api->RegistrationOpen(&reg_config, &ctx->registration);
    if (QUIC_FAILED(status)) {
        WTF_LOG_CRITICAL(ctx, "context", "RegistrationOpen failed: 0x%x", status);
        MsQuicClose(ctx->quic_api);
        mtx_destroy(&ctx->mutex);
        free(ctx);
        return wtf_quic_status_to_result(status);
    }

    WTF_LOG_INFO(ctx, "context", "WebTransport context created successfully");

    *context = ctx;
    return WTF_SUCCESS;
}

void wtf_context_destroy(wtf_context_t* context)
{
    if (!context) {
        return;
    }

    wtf_context* ctx = context;

    WTF_LOG_INFO(ctx, "context", "Destroying WebTransport context");

    wtf_server* server = NULL;
    wtf_client* client = NULL;

    mtx_lock(&ctx->mutex);
    server = ctx->server;
    client = ctx->client;
    ctx->server = NULL;
    ctx->client = NULL;
    mtx_unlock(&ctx->mutex);

    if (server) {
        wtf_server_destroy((wtf_server_t*)server);
    }
    if (client) {
        wtf_client_destroy((wtf_client_t*)client);
    }

    mtx_lock(&ctx->mutex);

    if (ctx->registration) {
        ctx->quic_api->RegistrationClose(ctx->registration);
        ctx->registration = NULL;
    }

    if (ctx->quic_api) {
        MsQuicClose(ctx->quic_api);
        ctx->quic_api = NULL;
    }

    mtx_unlock(&ctx->mutex);
    mtx_destroy(&ctx->mutex);

    free(context);
}

wtf_result_t wtf_context_set_log_level(wtf_context_t* context, wtf_log_level_t level)
{
    if (!context) {
        return WTF_ERROR_INVALID_PARAMETER;
    }

    wtf_context* ctx = context;

    mtx_lock(&ctx->mutex);
    ctx->log_level = level;
    mtx_unlock(&ctx->mutex);

    return WTF_SUCCESS;
}
