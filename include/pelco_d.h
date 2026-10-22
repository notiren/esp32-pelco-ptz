#pragma once

#include <stddef.h>
#include <stdint.h>

namespace pelco {

constexpr size_t kFrameSize = 7;
constexpr uint8_t kSync = 0xFF;
constexpr uint8_t kMaxSpeed = 0x3F;

// Each axis: -1, 0 or +1.
//   pan:   -1 left,  +1 right
//   tilt:  -1 down,  +1 up
//   zoom:  -1 wide,  +1 tele
//   focus: -1 near,  +1 far
//   iris:  -1 close, +1 open
struct Motion {
    int8_t pan = 0;
    int8_t tilt = 0;
    int8_t zoom = 0;
    int8_t focus = 0;
    int8_t iris = 0;
    uint8_t panSpeed = 0x20;
    uint8_t tiltSpeed = 0x20;
};

enum class PresetAction : uint8_t {
    Set = 0x03,
    Clear = 0x05,
    Go = 0x07,
};

uint8_t checksum(const uint8_t frame[kFrameSize]);
void buildMotion(uint8_t address, const Motion& motion, uint8_t out[kFrameSize]);
void buildStop(uint8_t address, uint8_t out[kFrameSize]);
void buildPreset(uint8_t address, PresetAction action, uint8_t preset, uint8_t out[kFrameSize]);

}  // namespace pelco
