#include "pelco_d.h"

namespace pelco {

namespace {

// Command 1 bits
constexpr uint8_t kFocusNear = 0x01;
constexpr uint8_t kIrisOpen = 0x02;
constexpr uint8_t kIrisClose = 0x04;

// Command 2 bits
constexpr uint8_t kRight = 0x02;
constexpr uint8_t kLeft = 0x04;
constexpr uint8_t kUp = 0x08;
constexpr uint8_t kDown = 0x10;
constexpr uint8_t kZoomTele = 0x20;
constexpr uint8_t kZoomWide = 0x40;
constexpr uint8_t kFocusFar = 0x80;

uint8_t clampSpeed(uint8_t speed) {
    return speed > kMaxSpeed ? kMaxSpeed : speed;
}

void fill(uint8_t address, uint8_t cmd1, uint8_t cmd2, uint8_t data1, uint8_t data2,
          uint8_t out[kFrameSize]) {
    out[0] = kSync;
    out[1] = address;
    out[2] = cmd1;
    out[3] = cmd2;
    out[4] = data1;
    out[5] = data2;
    out[6] = checksum(out);
}

}  // namespace

uint8_t checksum(const uint8_t frame[kFrameSize]) {
    uint16_t sum = 0;
    for (size_t i = 1; i < kFrameSize - 1; ++i) {
        sum += frame[i];
    }
    return static_cast<uint8_t>(sum & 0xFF);
}

void buildMotion(uint8_t address, const Motion& m, uint8_t out[kFrameSize]) {
    uint8_t cmd1 = 0;
    uint8_t cmd2 = 0;

    if (m.pan > 0) cmd2 |= kRight;
    if (m.pan < 0) cmd2 |= kLeft;
    if (m.tilt > 0) cmd2 |= kUp;
    if (m.tilt < 0) cmd2 |= kDown;
    if (m.zoom > 0) cmd2 |= kZoomTele;
    if (m.zoom < 0) cmd2 |= kZoomWide;
    if (m.focus > 0) cmd2 |= kFocusFar;
    if (m.focus < 0) cmd1 |= kFocusNear;
    if (m.iris > 0) cmd1 |= kIrisOpen;
    if (m.iris < 0) cmd1 |= kIrisClose;

    const uint8_t panSpeed = m.pan != 0 ? clampSpeed(m.panSpeed) : 0;
    const uint8_t tiltSpeed = m.tilt != 0 ? clampSpeed(m.tiltSpeed) : 0;
    fill(address, cmd1, cmd2, panSpeed, tiltSpeed, out);
}

void buildStop(uint8_t address, uint8_t out[kFrameSize]) {
    fill(address, 0, 0, 0, 0, out);
}

void buildPreset(uint8_t address, PresetAction action, uint8_t preset,
                 uint8_t out[kFrameSize]) {
    fill(address, 0, static_cast<uint8_t>(action), 0, preset, out);
}

}  // namespace pelco
