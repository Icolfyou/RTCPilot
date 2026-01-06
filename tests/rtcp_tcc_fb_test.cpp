#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <cstring>
#include <iostream>

#include "net/rtprtcp/rtprtcp_pub.hpp"
#include "net/rtprtcp/rtcp_fb_pub.hpp"
#include "net/rtprtcp/rtcp_tcc_fb.hpp"

using namespace cpp_streamer;

static bool g_verbose = false;

static void dump_basic_header(const uint8_t* buf, size_t len, const char* tag) {
    if (!g_verbose) return;
    auto *rtcp = (const RtcpFbCommonHeader*)buf;
    std::cout << "[" << tag << "] RTCP v=" << int(rtcp->version)
              << ", P=" << int(rtcp->padding)
              << ", FMT=" << int(rtcp->fmt)
              << ", PT=" << int(rtcp->packet_type)
              << ", lenBytes=" << len
              << ", length(words-1)=" << ntohs(rtcp->length)
              << std::endl;
}

static void test_requires_two_deltas() {
    RtcpTccFbPacket pkt;
    pkt.SetSsrc(0x11111111, 0x22222222);
    pkt.SetFbPktCount(7);

    // Only one delta → Serial should fail
    int64_t now = 100000;
    int baseSeq = 1000;
    assert(pkt.InsertPacket(baseSeq, now) == 0);  // first packet
    assert(pkt.InsertPacket(baseSeq + 1, now + 5) == 0); // one delta
    assert(pkt.InsertPacket(baseSeq + 3, now + 6) == 0); // two delta

    uint8_t buf[1500];
    size_t len = sizeof(buf);
    bool ok = pkt.Serial(buf, len);
    assert(ok);
    dump_basic_header(buf, len, "two_deltas");
}

static void test_padding_bit_and_length_21_deltas() {
    RtcpTccFbPacket pkt;
    pkt.SetSsrc(0xA1B2C3D4, 0x01020304);
    pkt.SetFbPktCount(255);

    int64_t now = 123456; // ms
    uint16_t startSeq = 2332;

    // Insert 22 packets → 21 deltas
    assert(pkt.InsertPacket(startSeq, now) == 0);
    for (int i = 1; i <= 21; ++i) {
        now += (i % 5) + 1; // 2..5ms increments
        assert(pkt.InsertPacket(startSeq + i, now) == 0);
    }

    uint8_t buf[1024];
    size_t len = 0;
    bool ok = pkt.Serial(buf, len);
    assert(ok);
    dump_basic_header(buf, len, "21_deltas");

    // Parse RTCP FB common header
    auto *rtcp = (RtcpFbCommonHeader*)buf;
    assert(rtcp->version == 2);
    assert(rtcp->packet_type == RTCP_RTPFB);
    assert(rtcp->fmt == FB_RTP_TCC);
    assert(rtcp->padding == 1); // expect padding because len % 4 != 0 before padding

    // Compute expected length words-minus-one
    size_t wordsMinus1 = (len / 4) - 1;
    assert(ntohs(rtcp->length) == (uint16_t)wordsMinus1);

    // FB header
    auto *fb = (RtcpFbHeader*)(buf + sizeof(RtcpFbCommonHeader));
    assert(ntohl(fb->media_ssrc) == 0x01020304);
    assert(ntohl(fb->sender_ssrc) == 0xA1B2C3D4);

    // Base seq & pkt status count
    uint8_t* p = (uint8_t*)fb + sizeof(RtcpFbHeader);
    uint16_t base_seq = ntohs(*(uint16_t*)p); p += 2;
    uint16_t pkt_cnt  = ntohs(*(uint16_t*)p); p += 2;
    assert(base_seq == startSeq);
    assert(pkt_cnt == 21);

    // Reference time + fb pkt count
    uint32_t ref_and_cnt_net = *(uint32_t*)p; p += 4;
    uint32_t ref_and_cnt = ntohl(ref_and_cnt_net);
    uint8_t fb_cnt = (uint8_t)(ref_and_cnt & 0xFF);
    uint32_t ref_time = (ref_and_cnt >> 8) & 0x00FFFFFF;
    assert(fb_cnt == 255);
    (void)ref_time; // reference time not strictly deterministic here

    // Expect last byte to be padding count
    uint8_t last = buf[len - 1];
    if (g_verbose) {
        std::cout << "[21_deltas] padding_count=" << int(last) << std::endl;
    }
    assert(last == 2); // total padding bytes = 2 (one 0 + count byte)

    // Parse back and verify
    RtcpTccFbPacket* parsed = RtcpTccFbPacket::Parse(buf, len);
    assert(parsed != nullptr);
    assert(parsed->GetSenderSsrc() == 0xA1B2C3D4);
    assert(parsed->GetMediaSsrc()  == 0x01020304);
    assert(parsed->GetBaseSeq()    == startSeq);
    assert(parsed->GetPacketStatusCount() == 21);
    assert(parsed->GetFbPktCount() == 255);
    assert((parsed->GetReferenceTime() & 0xFFFFFF) == (parsed->GetReferenceTime()));

    // Check delta count and reconstructed seq continuity with losses according to chunks
    // Our generator inserted strictly consecutive packets, so 21 received bits, seqs should be startSeq..startSeq+20
    assert(parsed->GetRecvDeltas().size() == 21);
    for (size_t i = 0; i < parsed->GetRecvDeltas().size(); ++i) {
        uint16_t expectSeq = startSeq + (uint16_t)i;
        assert(parsed->GetRecvDeltas()[i].wide_seq_ == expectSeq);
    }
    delete parsed;
}

static void test_no_padding_when_aligned_22_deltas() {
    RtcpTccFbPacket pkt;
    pkt.SetSsrc(0xDEADBEEF, 0xFEEDC0DE);
    pkt.SetFbPktCount(42);

    int64_t now = 987654;
    uint16_t startSeq = 5000;

    // Insert 23 packets → 22 deltas
    assert(pkt.InsertPacket(startSeq, now) == 0);
    for (int i = 1; i <= 22; ++i) {
        now += (i % 3) + 1;
        assert(pkt.InsertPacket(startSeq + i, now) == 0);
    }

    uint8_t buf[1024];
    size_t len = 0;
    bool ok = pkt.Serial(buf, len);
    assert(ok);
    dump_basic_header(buf, len, "22_deltas");

    // With 22 deltas, predicted len before padding: 20 + 4 + 44 = 68 → aligned
    auto *rtcp = (RtcpFbCommonHeader*)buf;
    assert(rtcp->padding == 0);

    size_t wordsMinus1 = (len / 4) - 1;
    assert(ntohs(rtcp->length) == (uint16_t)wordsMinus1);

    // Parse back and verify
    RtcpTccFbPacket* parsed = RtcpTccFbPacket::Parse(buf, len);
    assert(parsed != nullptr);
    assert(parsed->GetSenderSsrc() == 0xDEADBEEF);
    assert(parsed->GetMediaSsrc()  == 0xFEEDC0DE);
    assert(parsed->GetBaseSeq()    == startSeq);
    assert(parsed->GetPacketStatusCount() == 22);
    assert(parsed->GetFbPktCount() == 42);
    assert(parsed->GetRecvDeltas().size() == 22);
    for (size_t i = 0; i < parsed->GetRecvDeltas().size(); ++i) {
        uint16_t expectSeq = startSeq + (uint16_t)i;
        assert(parsed->GetRecvDeltas()[i].wide_seq_ == expectSeq);
    }
    delete parsed;
}

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    const char* env = std::getenv("TCC_TEST_VERBOSE");
    if (env && env[0] != '\0' && env[0] != '0') g_verbose = true;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--verbose") g_verbose = true;
    }

    if (g_verbose) std::cout << "Running rtcp_tcc_fb tests in verbose mode" << std::endl;
    test_requires_two_deltas();
    test_padding_bit_and_length_21_deltas();
    test_no_padding_when_aligned_22_deltas();
    std::puts("rtcp_tcc_fb tests: ALL PASSED");
    return 0;
}
