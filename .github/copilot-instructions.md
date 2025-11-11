## SleepBuddy — Copilot instructions (concise)

This file captures the minimal, concrete knowledge an AI coding agent needs to contribute safely and productively to this PlatformIO + Arduino (Renesas RA UNO R4 Minima) project.

High-level architecture
- Firmware for an UNO R4 Minima using PlatformIO (`platform = renesas-ra`, `board = uno_r4_minima`) and the Arduino framework. See `platformio.ini`.
- UI is a small LCD-driven menu system built around the external libraries: `LiquidCrystal_I2C`, `LcdMenu`/`LcdMenu`-style macros and adapters. The code uses an adapter/renderer pattern:
  - `LiquidCrystal_I2C` (display driver) -> `LiquidCrystal_I2CAdapter` -> `CharacterDisplayRenderer` -> `LcdMenu` (menu engine).
  - Input is abstracted via `KeyboardAdapter` (look in `src/renderer/` and `src/input/` for adapters).

Files of interest (quick map)
- `platformio.ini` — build environment, declared libs via `lib_deps` (LiquidCrystal_I2C, LcdMenu, Button). Use this to add or pin dependencies.
- `src/main.cpp` — top-level initialization: `renderer.begin()`, `menu.setScreen(...)`, `menu.poll(1000)`. Menu screens are declared with `MENU_SCREEN` macro.
- `include/` — project headers belong here.
- `lib/` — local libraries (private code) go here if you need to add a reusable module.

Project-specific patterns and conventions
- Menu definitions are created with macro invocations (see `src/main.cpp`):
  - Example: `MENU_SCREEN(mainScreen, mainItems, ITEM_BASIC("DATE (Placeholder)"), ITEM_SUBMENU("Options", optionsScreen));`
  - New screens follow the same macro pattern. Keep screen and item names as globals (the library expects static data at file-scope).
- Adapter & renderer pattern: don't bypass the renderer/adapter layers. Create a new adapter if you add a different display/input device and wire it into `CharacterDisplayRenderer` or `LcdMenu` rather than modifying library internals.
- Minimal runtime: initialization belongs in `setup()` (Serial.begin, renderer.begin(), menu.setScreen); the main loop calls `menu.poll(...)` with a timeout. Avoid long blocking calls in `loop()`.
- Serial debugging uses `Serial.begin(9600)`; debugging prints should be conservative (embedded device, limited I/O).

Build / test / upload workflows (practical commands)
- Build (local): from repo root, run PlatformIO CLI (PowerShell):
  - pio run
  - pio run -e uno_r4_minima  # explicitly build the declared environment
- Upload to device (PowerShell):
  - pio run -e uno_r4_minima -t upload
- Monitor serial output (make sure the device is on correct COM port):
  - pio device monitor --port COM3 --baud 9600  # replace COM3 with detected port
- Tests: there are no unit tests currently; see `test/README` for the test folder. If you add tests, follow PlatformIO unit testing conventions (`pio test`).

How to modify UI/menu safely (concrete example)
- To add a new submenu called "Alarms":
  1. Add a new MENU_SCREEN declaration following existing style near other screens in `src/main.cpp`.
  2. Use `ITEM_SUBMENU("Alarms", alarmsScreen)` inside parent screen's items.
  3. Call `menu.setScreen(...)` only from `setup()`; avoid calling from within user callbacks.

Integration points & external deps
- External libs are declared in `platformio.ini` under `lib_deps`. Typical integration points in code:
  - `LiquidCrystal_I2C` instance construction (address and size are currently `0x20, 16, 2`). If you change wiring/address, update the constructor and adapter instantiation.
  - `LcdMenu` consumes the `CharacterDisplayRenderer` object; follow current constructor order if adding other renderers.

Things to preserve / watch for
- Keep the global menu structures in `src` (not static inside functions) — the LcdMenu macros create references expected at link time.
- Avoid heavy dynamic allocation; keep stack/heap use minimal for UNO-class devices.
- When adding libraries, prefer `lib_deps` entries in `platformio.ini` so CI and collaborators get the same deps.

Example references in repo
- `platformio.ini` — shows `renesas-ra` platform and `lib_deps` entries.
- `src/main.cpp` — menu macro usage and adapter/renderer wiring (primary pattern to follow).

If something is unclear
- Ask where to place a new screen or adapter. Provide a one-file patch for small changes (e.g., add new MENU_SCREEN in `src/main.cpp`) rather than sweeping refactors.

Next steps for Copilot agents
- Prefer small, incremental edits. Add unit tests only when small, self-contained logic is added (non-hardware-bound). For hardware behavior, prefer simulator/mocks or clearly-scoped integration steps.

---
If you want, I can also add a short CONTRIBUTING.md with the same build and run commands and a checklist for adding new menu screens or adapters.
