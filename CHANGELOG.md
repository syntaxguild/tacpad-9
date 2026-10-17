# Changelog

## 0.3.0 — 2026-10-17 · workflow profiles

- Centralize labels, modifier/key combinations and consumer usages in `include/keymap.h`; OLED, Serial and HID share the same bindings.
- Add Coding, Blender and Media profiles with 27 total mappings. Tap/release the encoder to cycle; retain the 1.5-second Macro/XO hold.
- Store the selected profile with Preferences/NVS once per change, with Coding fallback and a visible `*` when saving is unavailable.
- Release queued/held USB actions during profile changes and require physical keys to be released before accepting new-profile input.
- Check all 27 expected reports, profile gestures, saved/invalid values, failed storage, held-key gating and timer rollover using actual firmware with host doubles.
- Generate the keymap reference from compiled bindings and reject documentation drift in the existing host test runner.
- Add a portfolio case study connecting the brief, decisions, implementation and evidence. Industrial geometry and product renders are carried forward from v0.2.0.

Real-board USB, flash retention/timing and physical fit remain pending.

## 0.2.0 — 2026-10-17 · digital prototype

- Add ESP32-S3 native USB keyboard and consumer controls, with disconnect/mode-change release and bounded nonblocking report scheduling.
- Keep the original ESP32 Wokwi UI/game profile and test its compiled OLED chip.
- Add a rear USB cable opening, case screw bosses, removable OLED carrier, controller cradle/retainer, hollow MX caps, D-shaft bore and fit coupon.
- Check printable mesh integrity, reference-envelope intersections and full key travel in Blender 4.0.2.
- Organize manufacturing source, documentation, tests and release outputs; add sourced ฿500 purchasing plan and automated validation.
- Refresh Primadon product photography and add genuine exploded/service CAD views.

Hardware fit and real-host USB testing remain pending. This release makes no claim of a fabricated device.

## Initial design history

- `63afe15`: parametric enclosure and mesh exports.
- `bc9ed56`: studio lighting, product finish and rendered controls.
- `8d9121f`: rename product to Tacpad 9 / Model Nine.
- `35c3f7c`: Primadon mascot, final product branding and square/wide photography.

Earlier exports remain recoverable from Git history. Current release downloads contain the revised serviceable design.
