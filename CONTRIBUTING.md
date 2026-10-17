# Working on Tacpad 9

Keep firmware in `src/` and `include/`, mechanical sources in `mechanical/`, simulation assets in `chips/` and product documentation in `docs/`.

Run the checks in [validation.md](docs/validation.md) for the area changed. Regenerate `mechanical/validation.json` after geometry/reference edits. Regenerate product images if the visible model changes; do not use concept images as proof of fabrication. Keep generated `.blend`, mesh bundles and firmware binaries in Releases rather than source commits.

Edit profile bindings in `include/keymap.h`, keep stored profile IDs stable, and run `python scripts/check_firmware.py --write-keymap` before committing. The standard test run verifies the generated keymap reference agrees with the compiled table.

Use a focused Conventional Commit for each real change. Update `VERSION`, `include/version.h`, `mechanical/reference.json`, README and the changelog together when changing a release version. Prototype tags use GitHub prereleases until a physical test record exists.

Do not commit API keys, personal credentials, `.pio/`, local logs or superseded render bundles. Preserve third-party notices in the Wokwi API header. Primadon branding and the supplied mascot belong to their respective owner; no third-party branding license is implied by this repository.
