# Validation record

Version **0.2.0** is a **digital prototype**. Firmware builds, native behavior tests, compiled OLED simulation and CAD geometry checks have been run. No physical assembly, print fit, USB enumeration on a real S3, electrical current measurement or endurance test is claimed.

| Gate | Evidence | Scope |
|---|---|---|
| S3 firmware | PlatformIO `tacpad_s3` | Compiles native keyboard + consumer-control + CDC for 4 MB S3 |
| Legacy simulation | PlatformIO `esp32dev` | Existing Wokwi UI/game target still builds |
| Game engine | `minimax_test.cpp` | 2,097 legal AI positions against full-tree oracle; 521 player paths, zero X wins |
| UI / input behavior | `oled_smoke_test.cpp` | OLED init, text bounds, debounce, volume simulation, hold, game, reset and scores |
| USB behavior | `hid_smoke_test.cpp` | Runs actual firmware with recorded USB doubles; all nine mappings, report release, queue bounds, disconnect, game suppression, timer wrap |
| SSD1306 simulation | `oled_chip_test.cjs` | Exercises the actual compiled WASM callbacks, framebuffer, 2× scale, inversion and power commands |
| Manufacturing geometry | `mechanical/validate.py` | Blender 4.0.2; closed edges, one connected component and positive volume per printable part; nominal travel and selected mesh-intersection checks |
| Project integrity | `scripts/check_repo.py` | Version agreement, BOM arithmetic, internal Markdown links and CAD source hashes |

The [geometry report](../mechanical/validation.json) records source hashes, part quantities, clearances and tested intersection volumes. It checks the case/base/carriers against modeled electronic envelopes, the D-shaft against its bore, the MX stems at rest and keycaps against the case at full press. Intentional support contacts have zero overlapping volume. Passing this gate does not establish injection-molding draft, slicer quality, real switch clip retention, supplier dimensions, solder/wire clearances or printed tolerances.

## Reproduce

```sh
python scripts/check_repo.py
python scripts/check_firmware.py
pio run -e tacpad_s3 -e esp32dev
blender -b --python-exit-code 1 --python mechanical/validate.py
```

Use a C++11 compiler (`g++`, or set `CXX`) and Node.js for the native suite. On Windows the runner also detects PlatformIO's bundled MinGW toolchain. No assertions are disabled by the test runner. The GitHub [Validate workflow](../.github/workflows/validate.yml) runs both firmware profiles and the geometry gate independently.

## First physical build: remaining acceptance checks

1. Confirm the delivered board/module dimensions and pinout; fit the coupon, cap and knob without forcing stems.
2. Verify no 5 V reaches an S3 GPIO and no shorts exist. Confirm the data cable seats fully through the opening.
3. Enumerate the flashed device on a real host, then test all shortcuts in a disposable editor, media controls and encoder direction.
4. Assign the host's Blender launch shortcut. Check unplug/replug and mode switching leave no held modifier.
5. Check every cap over its full 4 mm motion, encoder push/rotation, screw engagement, wire relief and enclosure service access.

Record actual board/module variants, printer/material, host OS and failures before promoting the release beyond prototype status.
