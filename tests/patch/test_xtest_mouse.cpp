#include <catch2/catch_test_macros.hpp>
#include "xtest_mouse_helpers.hpp"
#include <cstring>
#include <vector>
using gamescope::xtest_mouse::Motion;
using gamescope::xtest_mouse::MotionBatch;
namespace {
std::array<uint8_t, 36> Request(int16_t x = 7, int16_t y = -5, bool swapped = false)
{
    std::array<uint8_t, 36> request{};
    request[0] = 132; request[1] = 2; request[4] = 6; request[5] = 1;
    auto word = [&](size_t offset, uint16_t value) {
        if (swapped) value = uint16_t((value >> 8) | (value << 8));
        std::memcpy(request.data() + offset, &value, 2);
    };
    word(2, 9); word(24, uint16_t(x)); word(26, uint16_t(y));
    return request;
}
}
TEST_CASE("XTEST recording forwards immediate focused relative core motion", "[xtest_mouse]")
{
    auto request = Request();
    auto motion = Motion(request, 132, false, true);
    REQUIRE(motion.has_value());
    REQUIRE((*motion)[0] == 7);
    REQUIRE((*motion)[1] == -5);
    REQUIRE_FALSE(Motion(request, 132, false, false));
    REQUIRE_FALSE(Motion(request, 131, false, true));
    REQUIRE_FALSE(Motion(request, 0, false, true));
}
TEST_CASE("XTEST absolute positioning never accumulates as relative motion", "[xtest_mouse]")
{
    auto request = Request(400, 300);
    request[5] = 0;
    for (int i = 0; i < 3; ++i)
        REQUIRE_FALSE(Motion(request, 132, false, true));
    request[5] = 1;
    REQUIRE(Motion(request, 132, false, true).has_value());
}
TEST_CASE("XTEST recording excludes other input and delayed requests", "[xtest_mouse]")
{
    for (const unsigned type : {2, 3, 4, 5}) {
        auto request = Request(); request[4] = type;
        REQUIRE_FALSE(Motion(request, 132, false, true));
    }
    auto request = Request(); request[35] = 4;
    REQUIRE(Motion(request, 132, false, true).has_value()); // core padding is unspecified
    request = Request(); request[4] = 64; request[35] = 4;
    REQUIRE_FALSE(Motion(request, 132, false, true));
    request = Request(); request[8] = 1;
    REQUIRE_FALSE(Motion(request, 132, false, true));
    request = Request(); request[1] = 3;
    REQUIRE_FALSE(Motion(request, 132, false, true));
}
TEST_CASE("XTEST recording decodes signed coordinates in either sender byte order", "[xtest_mouse]")
{
    for (const bool swapped : {false, true}) {
        auto request = Request(-32768, 32767, swapped);
        auto motion = Motion(request, 132, swapped, true);
        REQUIRE(motion.has_value());
        REQUIRE((*motion)[0] == -32768);
        REQUIRE((*motion)[1] == 32767);
        motion = Motion(Request(0, -1, swapped), 132, swapped, true);
        REQUIRE(motion.has_value());
        REQUIRE((*motion)[0] == 0);
        REQUIRE((*motion)[1] == -1);
    }
}
TEST_CASE("XTEST recording rejects malformed requests and stationary motion", "[xtest_mouse]")
{
    REQUIRE_FALSE(Motion({}, 132, false, true));
    auto request = Request();
    REQUIRE_FALSE(Motion(std::span(request).first(35), 132, false, true));
    request[2] = request[3] = 0;
    REQUIRE_FALSE(Motion(request, 132, false, true));
    REQUIRE_FALSE(Motion(Request(0, 0), 132, false, true));
    request = Request(); request[5] = 2;
    REQUIRE_FALSE(Motion(request, 132, false, true));
}

TEST_CASE("XTEST RECORD batches skip variable XI requests without losing core motion", "[xtest_mouse]")
{
    for (const bool swapped : {false, true}) {
        const auto request = Request(7, -5, swapped);
        std::vector<uint8_t> data(request.begin(), request.end());
        auto xi = Request(400, 300, swapped);
        xi[4] = 64; // XI DeviceMotionNotify
        uint16_t words = swapped ? 0x1100 : 17;
        std::memcpy(xi.data() + 2, &words, 2);
        data.insert(data.end(), xi.begin(), xi.end());
        data.resize(data.size() + 32); // appended DeviceValuator event
        data.insert(data.end(), request.begin(), request.end());
        MotionBatch batch;
        REQUIRE(gamescope::xtest_mouse::DispatchRequests(data, 132, swapped, true, batch));
        REQUIRE(batch.Motions().size() == 2);
        REQUIRE(batch.Motions()[0][0] + batch.Motions()[1][0] == 14);
        REQUIRE(batch.Motions()[0][1] + batch.Motions()[1][1] == -10);
    }
}
TEST_CASE("XTEST RECORD framing handles big requests and rejects malformed data", "[xtest_mouse]")
{
    for (const bool swapped : {false, true}) {
        auto request = Request(7, -5, swapped);
        std::vector<uint8_t> data(request.begin(), request.end());
        data[2] = data[3] = 0;
        uint32_t words = swapped ? 0x0a000000 : 10;
        data.insert(data.begin() + 4, 4, 0);
        std::memcpy(data.data() + 4, &words, 4);
        data.insert(data.end(), request.begin(), request.end());
        MotionBatch batch;
        REQUIRE(gamescope::xtest_mouse::DispatchRequests(data, 132, swapped, true, batch));
        REQUIRE(batch.Motions().size() == 1); // unsupported BIG FakeInput skipped
    }
    MotionBatch batch;
    const auto request = Request();
    REQUIRE_FALSE(gamescope::xtest_mouse::DispatchRequests(std::span(request).first(3), 132, false, true, batch));
    REQUIRE_FALSE(gamescope::xtest_mouse::DispatchRequests(std::span(request).first(35), 132, false, true, batch));
    REQUIRE(batch.Motions().empty());
}

TEST_CASE("XTEST RECORD marker floods never fail the drain", "[xtest_mouse]")
{
    MotionBatch batch;
    for (unsigned i = 0; i < 4 * MotionBatch::kLimit; ++i)
        REQUIRE(gamescope::xtest_mouse::DispatchRecordData(0, {}, 132, false, true, batch));
    REQUIRE(batch.Motions().empty());
    const auto request = Request();
    REQUIRE(gamescope::xtest_mouse::DispatchRecordData(1, request, 132, false, true, batch));
    REQUIRE(batch.Motions().size() == 1);
    REQUIRE_FALSE(gamescope::xtest_mouse::DispatchRecordData(5, {}, 132, false, true, batch));
}
TEST_CASE("XTEST RECORD motion beyond the budget is coalesced, not dropped or failed", "[xtest_mouse]")
{
    MotionBatch batch;
    const auto request = Request(7, -5);
    const unsigned sent = MotionBatch::kLimit + 44;
    for (unsigned i = 0; i < sent; ++i) {
        REQUIRE(gamescope::xtest_mouse::DispatchRecordData(0, {}, 132, false, true, batch));
        REQUIRE(gamescope::xtest_mouse::DispatchRecordData(1, request, 132, false, true, batch));
    }
    const auto motions = batch.Motions();
    REQUIRE(motions.size() == MotionBatch::kLimit);
    double x = 0, y = 0;
    for (const auto &motion : motions) {
        x += motion[0];
        y += motion[1];
    }
    REQUIRE(x == 7.0 * sent);
    REQUIRE(y == -5.0 * sent);
    REQUIRE(motions[0][0] == 7);
    REQUIRE(motions[MotionBatch::kLimit - 2][0] == 7);
    REQUIRE(motions[MotionBatch::kLimit - 1][0] == 7 * 45);
    REQUIRE(motions[MotionBatch::kLimit - 1][1] == -5 * 45);
}
