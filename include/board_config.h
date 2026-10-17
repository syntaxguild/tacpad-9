#pragma once
#include <cstdint>

#ifndef TACPAD_USB_HID
#define TACPAD_USB_HID 0
#endif
#ifndef TACPAD_REFERENCE_BOARD
#define TACPAD_REFERENCE_BOARD 0
#endif

namespace TacpadBoard {
#if TACPAD_REFERENCE_BOARD
// S3-Zero: leave USB GPIO19/20, BOOT GPIO0, strap GPIO3 and RGB GPIO21 free.
constexpr uint8_t KeyPins[9] = {1, 2, 4, 5, 6, 7, 8, 9, 10};
constexpr uint8_t EncoderDT = 12, EncoderCLK = 11, EncoderSW = 13;
constexpr uint8_t Sda = 17, Scl = 18;
constexpr bool EncoderPullups = true;
#else
constexpr uint8_t KeyPins[9] = {13, 12, 14, 27, 26, 25, 33, 15, 16};
constexpr uint8_t EncoderDT = 34, EncoderCLK = 35, EncoderSW = 32;
constexpr uint8_t Sda = 21, Scl = 22;
constexpr bool EncoderPullups = false;
#endif
}
