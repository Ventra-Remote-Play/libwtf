#include <gtest/gtest.h>

extern "C" {
#include "wtf.h"
}

TEST(PublicApi, VersionAndStringHelpersAreAvailable)
{
    const wtf_version_info_t* version = wtf_get_version();
    ASSERT_NE(version, nullptr);
    EXPECT_NE(version->version, nullptr);
    EXPECT_NE(wtf_result_to_string(WTF_SUCCESS), nullptr);
    EXPECT_NE(wtf_result_to_string(WTF_ERROR_INVALID_PARAMETER), nullptr);
}

TEST(PublicApi, DraftEnumValuesAreStable)
{
    EXPECT_EQ(WTF_WEBTRANSPORT_DRAFT_AUTO, 0);
    EXPECT_EQ(WTF_WEBTRANSPORT_DRAFT_02, 2);
    EXPECT_EQ(WTF_WEBTRANSPORT_DRAFT_07, 7);
    EXPECT_EQ(WTF_WEBTRANSPORT_DRAFT_15, 15);
}

TEST(PublicApi, ZeroInitializersSelectDefaultClientBehavior)
{
    EXPECT_EQ(WTF_CONGESTION_CONTROL_DEFAULT, 0);

    wtf_client_config_t config = {};
    config.url = "https://example.com:443/wt";
    config.draft = WTF_WEBTRANSPORT_DRAFT_15;
    config.allow_pooling = true;
    config.congestion_control = WTF_CONGESTION_CONTROL_LOW_LATENCY;
    config.require_unreliable = true;

    EXPECT_STREQ(config.url, "https://example.com:443/wt");
    EXPECT_EQ(config.draft, WTF_WEBTRANSPORT_DRAFT_15);
    EXPECT_TRUE(config.allow_pooling);
    EXPECT_EQ(config.congestion_control, WTF_CONGESTION_CONTROL_LOW_LATENCY);
    EXPECT_TRUE(config.require_unreliable);
}
