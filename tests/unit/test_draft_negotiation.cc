#include <gtest/gtest.h>

extern "C" {
#include "draft.h"
#include "settings.h"
}

namespace {

wtf_settings PeerSettings()
{
    wtf_settings settings = {};
    wtf_settings_init(&settings);
    return settings;
}

}  // namespace

TEST(DraftNegotiationRegression, AutoPrefersDraft15WhenExplicitlyAdvertised)
{
    wtf_settings peer = PeerSettings();
    peer.enable_webtransport_draft15 = true;
    peer.enable_webtransport_draft07 = true;
    peer.enable_webtransport_draft02 = true;
    peer.h3_datagram_rfc_enabled = true;
    peer.h3_datagram_draft04_enabled = true;

    EXPECT_EQ(wtf_draft_select(WTF_ENDPOINT_CLIENT, WTF_WEBTRANSPORT_DRAFT_AUTO, &peer),
              WTF_WEBTRANSPORT_DRAFT_15);
}

TEST(DraftNegotiationRegression, AutoFallsBackToDraft07BeforeDraft02)
{
    wtf_settings peer = PeerSettings();
    peer.enable_webtransport_draft07 = true;
    peer.enable_webtransport_draft02 = true;
    peer.h3_datagram_rfc_enabled = true;
    peer.h3_datagram_draft04_enabled = true;

    EXPECT_EQ(wtf_draft_select(WTF_ENDPOINT_CLIENT, WTF_WEBTRANSPORT_DRAFT_AUTO, &peer),
              WTF_WEBTRANSPORT_DRAFT_07);
}

TEST(DraftNegotiationRegression, ChromeDraft02SettingsDoNotSelectDraft15)
{
    wtf_settings peer = PeerSettings();
    peer.enable_webtransport_draft02 = true;
    peer.h3_datagram_rfc_enabled = true;
    peer.h3_datagram_draft04_enabled = true;

    EXPECT_EQ(wtf_draft_select(WTF_ENDPOINT_SERVER, WTF_WEBTRANSPORT_DRAFT_AUTO, &peer),
              WTF_WEBTRANSPORT_DRAFT_02);
}

TEST(DraftNegotiationRegression, ServerMayInferDraft15OnlyWithoutLegacyWebTransportSettings)
{
    wtf_settings peer = PeerSettings();
    peer.h3_datagram_rfc_enabled = true;

    EXPECT_EQ(wtf_draft_select(WTF_ENDPOINT_SERVER, WTF_WEBTRANSPORT_DRAFT_AUTO, &peer),
              WTF_WEBTRANSPORT_DRAFT_15);
}

TEST(DraftNegotiationRegression, ClientDoesNotInferDraft15FromDatagramOnly)
{
    wtf_settings peer = PeerSettings();
    peer.h3_datagram_rfc_enabled = true;

    EXPECT_EQ(wtf_draft_select(WTF_ENDPOINT_CLIENT, WTF_WEBTRANSPORT_DRAFT_AUTO, &peer),
              WTF_WEBTRANSPORT_DRAFT_NONE);
}

TEST(DraftNegotiationRegression, ExplicitDraftRequiresMatchingPeerSupport)
{
    wtf_settings peer = PeerSettings();
    peer.enable_webtransport_draft02 = true;
    peer.h3_datagram_draft04_enabled = true;

    EXPECT_EQ(wtf_draft_select(WTF_ENDPOINT_CLIENT, WTF_WEBTRANSPORT_DRAFT_02, &peer),
              WTF_WEBTRANSPORT_DRAFT_02);
    EXPECT_EQ(wtf_draft_select(WTF_ENDPOINT_CLIENT, WTF_WEBTRANSPORT_DRAFT_07, &peer),
              WTF_WEBTRANSPORT_DRAFT_NONE);
    EXPECT_EQ(wtf_draft_select(WTF_ENDPOINT_CLIENT, WTF_WEBTRANSPORT_DRAFT_15, &peer),
              WTF_WEBTRANSPORT_DRAFT_NONE);
}

TEST(DraftNegotiationRegression, NullPeerSettingsSelectNoDraft)
{
    EXPECT_EQ(wtf_draft_select(WTF_ENDPOINT_CLIENT, WTF_WEBTRANSPORT_DRAFT_AUTO, nullptr),
              WTF_WEBTRANSPORT_DRAFT_NONE);
}
