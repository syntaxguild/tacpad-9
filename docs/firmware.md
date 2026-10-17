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

## Profiles and key map

**Tap and release the encoder** in Macro mode to cycle **Coding → Blender → Media → Coding**. The OLED header shows the active profile. A tap acts on the debounced release; the 1.5-second long press switches Macro / XO and never also changes profile. Taps in XO are ignored.

| Profile | Purpose |
|---|---|
| Coding | Original Copy/Paste/Build/Blender launcher/Undo/Redo/Play/Mute/Terminal arrangement |
| Blender | Move, Rotate, Scale, Extrude, Undo, Redo, Edit Mode, Deselect and Render |
| Media | Previous, Play/Pause, Next, Volume Down, Mute, Volume Up, Stop, Rewind and Fast Forward |

The [complete 27-key map](keymap.md) is generated from the compiled [keymap.h](../include/keymap.h). This is the single source for action labels, modifier combinations, keyboard keys and consumer usages. OLED action labels, Serial descriptions and queued USB reports use those same bindings. Edit this header to remap actions, then run `python scripts/check_firmware.py --write-keymap` to refresh the reference; the normal test run checks that the document is current.

The printed key legends show Coding. Use the small printed numbers and the profile reference for other layouts; the OLED shows the current profile and the last pressed action. Profiles are selected manually on the device.

Shortcuts target Windows/Linux applications and a US keyboard layout. Coding's BLENDER key sends Ctrl+Alt+Shift+B: **assign a host launcher binding yourself**. BUILD and TERMINAL assume the application's matching shortcuts, such as VS Code's default bindings. Blender's profile assumes the [default Blender keymap](https://docs.blender.org/manual/en/4.1/interface/keymap/blender_default.html), with [Ctrl+Shift+Z for Redo](https://docs.blender.org/manual/en/latest/interface/undo_redo.html). Give the intended editor focus; Move/Rotate/Scale/Extrude start interactive operations that still need confirmation. Media Rewind/Fast Forward send short consumer pulses; support and seeking behavior depend on the active player. These are not held transport controls.

## Remembering the selection

The active profile ID is stored using the ESP32's built-in [Preferences / NVS library](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/preferences.html), under namespace `tacpad9`, key `profile`. A successful selection commits one byte immediately; loop ticks, key presses, volume detents and game mode do not write settings. The default is Coding, and unknown saved IDs fall back to Coding. Keep existing profile IDs stable when extending the table.

If NVS cannot open or commit the selection, the device remains usable in RAM and the OLED appends `*` to the profile name; Serial explains the failure. After a failed save, the next reboot loads the last successfully stored choice, or Coding if none exists. Restart to retry opening unavailable storage. Erasing flash or writing the full merged firmware bundle can reset the stored selection; ordinary unplug/replug is the intended persistence path. Storage handling is tested with host doubles; real-board power-cycle testing remains pending.

Rotate the encoder for relative host volume up/down. Each detent sends one consumer action; the OS chooses its volume increment. There is no host volume feedback, so the USB UI deliberately does not invent a percentage.

Hold the encoder switch for **1.5 seconds** to toggle Macro / XO. Release before another hold. In XO, keys 1–9 select corresponding cells; X starts, O uses Minimax. After a finished round, release and press any key to reset without placing an extra X. Scores persist until reset and saturate at 65,535. No HID key actions are sent during the game.

## Runtime behavior

Inputs use a 20 ms debounce and one action per press. The encoder ISR captures quadrature transitions while the main loop updates the OLED and game. HID reports use a bounded 16-entry queue, 20 ms held presses and at least a 4 ms released interval. There is no blocking delay in the report scheduler. Disconnecting USB, changing mode or changing profile clears queued actions and releases held reports; detached inputs are discarded. Profile changes also wait for all physical keys to be released before accepting new actions.

Queue entries capture the binding at enqueue time. Changing a profile clears the queue before the synchronous NVS commit, so an old shortcut cannot be reinterpreted as a new profile's action. Flash commit latency has not been measured on real hardware.

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
