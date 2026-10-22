#include <unity.h>

#include "pelco_d.h"

namespace {

void assertFrame(const uint8_t expected[pelco::kFrameSize], const uint8_t actual[pelco::kFrameSize]) {
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, actual, pelco::kFrameSize);
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_stop() {
    uint8_t f[pelco::kFrameSize];
    pelco::buildStop(1, f);
    const uint8_t expected[] = {0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01};
    assertFrame(expected, f);
}

void test_pan_right() {
    uint8_t f[pelco::kFrameSize];
    pelco::Motion m;
    m.pan = 1;
    m.panSpeed = 0x20;
    pelco::buildMotion(1, m, f);
    const uint8_t expected[] = {0xFF, 0x01, 0x00, 0x02, 0x20, 0x00, 0x23};
    assertFrame(expected, f);
}

void test_up_left_diagonal() {
    uint8_t f[pelco::kFrameSize];
    pelco::Motion m;
    m.pan = -1;
    m.tilt = 1;
    m.panSpeed = m.tiltSpeed = 0x10;
    pelco::buildMotion(1, m, f);
    const uint8_t expected[] = {0xFF, 0x01, 0x00, 0x0C, 0x10, 0x10, 0x2D};
    assertFrame(expected, f);
}

void test_zoom_tele_has_no_speed() {
    uint8_t f[pelco::kFrameSize];
    pelco::Motion m;
    m.zoom = 1;
    pelco::buildMotion(1, m, f);
    const uint8_t expected[] = {0xFF, 0x01, 0x00, 0x20, 0x00, 0x00, 0x21};
    assertFrame(expected, f);
}

void test_focus_near_and_iris_open() {
    uint8_t f[pelco::kFrameSize];
    pelco::Motion m;
    m.focus = -1;
    m.iris = 1;
    pelco::buildMotion(1, m, f);
    const uint8_t expected[] = {0xFF, 0x01, 0x03, 0x00, 0x00, 0x00, 0x04};
    assertFrame(expected, f);
}

void test_speed_is_clamped() {
    uint8_t f[pelco::kFrameSize];
    pelco::Motion m;
    m.tilt = -1;
    m.tiltSpeed = 0xFF;
    pelco::buildMotion(2, m, f);
    const uint8_t expected[] = {0xFF, 0x02, 0x00, 0x10, 0x00, 0x3F, 0x51};
    assertFrame(expected, f);
}

void test_goto_preset() {
    uint8_t f[pelco::kFrameSize];
    pelco::buildPreset(1, pelco::PresetAction::Go, 3, f);
    const uint8_t expected[] = {0xFF, 0x01, 0x00, 0x07, 0x00, 0x03, 0x0B};
    assertFrame(expected, f);
}

void test_checksum_wraps() {
    uint8_t f[pelco::kFrameSize];
    pelco::buildPreset(0xFF, pelco::PresetAction::Set, 0xFF, f);
    // 0xFF + 0x00 + 0x03 + 0x00 + 0xFF = 0x201 -> 0x01
    TEST_ASSERT_EQUAL_HEX8(0x01, f[6]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_stop);
    RUN_TEST(test_pan_right);
    RUN_TEST(test_up_left_diagonal);
    RUN_TEST(test_zoom_tele_has_no_speed);
    RUN_TEST(test_focus_near_and_iris_open);
    RUN_TEST(test_speed_is_clamped);
    RUN_TEST(test_goto_preset);
    RUN_TEST(test_checksum_wraps);
    return UNITY_END();
}
