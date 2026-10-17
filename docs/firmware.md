# Firmware & controls

## Build and flash

Install PlatformIO Core, then run from the repository root:

```sh
pio run -e tacpad_s3
pio run -e tacpad_s3 -t upload
pio device monitor -b 115200
```

For first flash, hold **BOOT** while connecting the S3's USB-C data cable, then release BOOT and upload. Reconnect or press RESET after upload if it remains in the ROM downloader. USB CDC may enumerate on a different port after the application starts; select that port in PlatformIO. On Windows with the VS Code installation, use `& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"` if `pio` is not on PATH.

The pinned Espressif platform is 6.10.0 / Arduino-ESP32 2.0.17. The reference profile uses `esp32-s3-devkitm-1` with 4 MB flash as a build definition for the S3-Zero family, native USB device mode and CDC enabled. It does not depend on PSRAM, Wi-Fi or BLE. The compiled application contains both keyboard and consumer-control descriptors using [Espressif's USB libraries](https://github.com/espressif/arduino-esp32/tree/2.0.17/libraries/USB).

## Key map

| Key | Legend | USB report / expected host action |
|---:|---|---|
| 1 | COPY | Ctrl+C |
| 2 | PASTE | Ctrl+V |
| 3 | BUILD | Ctrl+Shift+B, e.g. VS Code build task |
| 4 | BLENDER | Ctrl+Alt+Shift+B, **assign this host shortcut yourself** |
| 5 | UNDO | Ctrl+Z |
| 6 | REDO | Ctrl+Y |
| 7 | PLAY | Consumer Play/Pause |
| 8 | MUTE | Consumer Mute |
| 9 | TERMINAL | Ctrl+grave, e.g. VS Code terminal |

The default shortcuts target Windows/Linux applications and a US keyboard layout. USB HID sends usages; the active application and host layout decide their meaning. Blender launch requires a host shortcut or launcher binding; firmware cannot install that binding. Modify [hid_output.h](../include/hid_output.h) to remap actions for another OS or workflow.

Rotate the encoder for relative host volume up/down. Each detent sends one consumer action; the OS chooses its volume increment. There is no host volume feedback, so the USB UI deliberately does not invent a percentage.

Hold the encoder switch for **1.5 seconds** to toggle Macro / XO. Release before another hold. In XO, keys 1–9 select corresponding cells; X starts, O uses Minimax. After a finished round, release and press any key to reset without placing an extra X. Scores persist until reset and saturate at 65,535. No HID key actions are sent during the game.

## Runtime behavior

Inputs use a 20 ms debounce and one action per press. The encoder ISR captures quadrature transitions while the main loop updates the OLED and game. HID reports use a bounded 16-entry queue, 20 ms held presses and at least a 4 ms released interval. There is no blocking delay in the report scheduler. Disconnecting USB or changing mode clears queued actions and releases held reports; detached inputs are discarded.

OLED allocation failure keeps input/HID processing available. Serial logs provide diagnostics rather than being required for macro operation. An absent OLED is therefore not a reason to disable USB actions.

## Wokwi simulation

```sh
pio run -e esp32dev
```

Open the folder in VS Code, install the Wokwi extension, then run **Wokwi: Start Simulator**. [wokwi.toml](../wokwi.toml) points to this legacy build. The original ESP32 diagram uses keys 13/12/14/27/26/25/33/15/16, CLK35, DT34, SW32 and I2C21/22, with external encoder pull-ups on input-only pins. It is a UI/game simulation with Serial macro output, not USB HID validation.

The custom [OLED chip](../chips/primadon-oled.chip.c) accepts SSD1306 I2C commands and displays the real 128 × 32 framebuffer at 2× visual scale. Click the encoder and use Space/arrow keys to operate it. Wokwi extension licensing is managed by Wokwi.

If you edit the custom OLED C source, regenerate its committed WASM using the [Wokwi CLI](https://docs.wokwi.com/guides/custom-chips-to-wasm):

```sh
cd chips
wokwi-cli chip compile primadon-oled.chip.c -o primadon-oled.chip.wasm
```

Then run the host suite to test the resulting binary. The checked-in WASM lets simulator users start without installing a chip compiler.
