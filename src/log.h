#ifndef WTF_LOG_H
#define WTF_LOG_H

#ifdef WTF_ENABLE_LOGGING
    #include "types.h"
    #include "wtf.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WTF_ENABLE_LOGGING

void wtf_log_internal(wtf_context* ctx, wtf_log_level_t level, const char* component,
                      const char* file, int line, const char* format, ...);

    #define WTF_LOG(ctx, level, component, ...)                                                    \
        do {                                                                                       \
            wtf_context* wtf_log_ctx_ = (ctx);                                                     \
            if (wtf_log_ctx_ && wtf_log_ctx_->log_callback                                         \
                && (level) >= wtf_log_ctx_->log_level) {                                           \
                wtf_log_internal(wtf_log_ctx_, level, component, __FILE__, __LINE__, __VA_ARGS__); \
            }                                                                                      \
        } while (0)

    #define WTF_LOG_TRACE(ctx, component, ...)                                                     \
        WTF_LOG(ctx, WTF_LOG_LEVEL_TRACE, component, __VA_ARGS__)
    #define WTF_LOG_DEBUG(ctx, component, ...)                                                     \
        WTF_LOG(ctx, WTF_LOG_LEVEL_DEBUG, component, __VA_ARGS__)
    #define WTF_LOG_INFO(ctx, component, ...)                                                      \
        WTF_LOG(ctx, WTF_LOG_LEVEL_INFO, component, __VA_ARGS__)
    #define WTF_LOG_WARN(ctx, component, ...)                                                      \
        WTF_LOG(ctx, WTF_LOG_LEVEL_WARN, component, __VA_ARGS__)
    #define WTF_LOG_ERROR(ctx, component, ...)                                                     \
        WTF_LOG(ctx, WTF_LOG_LEVEL_ERROR, component, __VA_ARGS__)
    #define WTF_LOG_CRITICAL(ctx, component, ...)                                                  \
        WTF_LOG(ctx, WTF_LOG_LEVEL_CRITICAL, component, __VA_ARGS__)

#else

    #define WTF_LOG(ctx, level, component, ...) do { } while (0)
    #define WTF_LOG_TRACE(ctx, component, ...) do { } while (0)
    #define WTF_LOG_DEBUG(ctx, component, ...) do { } while (0)
    #define WTF_LOG_INFO(ctx, component, ...) do { } while (0)
    #define WTF_LOG_WARN(ctx, component, ...) do { } while (0)
    #define WTF_LOG_ERROR(ctx, component, ...) do { } while (0)
    #define WTF_LOG_CRITICAL(ctx, component, ...) do { } while (0)

#endif

#ifdef __cplusplus
}
#endif
#endif  // WTF_LOG_H
