# Phaselink

A Qt desktop application for phased-array / ultrasonic NDT inspection, built on
top of the Phaselink device SDK.

## Overview

- **Data path** — `DataDispatch::dataPacketCallback(data, length, deviceId)`
  receives raw packets from the SDK and fans them out to registered
  `DataProcessor` instances and to lightweight "direct sinks" created with
  `addDirectSink`, so extra consumers (extra views, packet logging) can be
  attached without modifying the processing classes.
- **Inspection widgets** — A-scan / UT widgets and a phase-array widget under
  `UI/UT/`, plus a gate widget, an ACG/TCG widget, an essential-parameters widget
  and a scanning view.
- **Views and threading** — `dialog/viewwidget`, `dialog/viewmodel` and
  `dialog/viewworker` keep the rendering work off the UI thread.
- **Measurement and tooling dialogs** — colour dialog, amplitude palette,
  measure widget, parameter manager, ruler widget, packet data saver, and
  device/group configuration dialogs.
- **Replay** — `App::Replay` and `App::currentPosition` in `app.h` drive
  frame-stepping playback when live data is not available.
- **Appearance** — palettes in `PAL/` (Eintik `.pal` set) and Qt style sheets in
  `UI/Qss/`; `NdtBase` in `ndtbase.h` switches between day and night mode.
- **Localization** — `phaselink_CN.ts` / `phaselink_EN.ts`, with the compiled
  `.qm` files committed as well.

## Repository layout

| Path | Purpose |
| --- | --- |
| `main.cpp`, `mainwindow.{h,cpp,ui}` | Application entry point and main window |
| `app.{h,cpp}` | Singleton holding global application state (replay, resolutions, scan length) |
| `datadispatch.{h,cpp}` | Packet fan-out to processors and direct sinks |
| `ndtbase.{h,cpp}` | Base widget for the custom NDT widgets (show mode, style, value access) |
| `UI/UT/` | A-scan/UT, phase-array, gate and ACG/TCG widgets |
| `UI/scanning.{h,cpp,ui}` | Scanning view |
| `dialog/` | Views, measurement, palette, colour, ruler, parameter and device dialogs |
| `PAL/` | Device colour palettes |
| `Qss.qrc`, `UI/Qss/` | Qt style sheets |
| `Images.qrc` | Application images |
| `Phaselink.pro` | qmake project (links the vendor `client` SDK) |

## Requirements

- Qt 6 with `core gui network charts widgets xml`, plus `axcontainer`, `opengl`
  and `openglwidgets` (Windows)
- The Phaselink client SDK: headers in `C:\Phaselink\include` and
  `client.lib` in `C:\Phaselink\lib`. These paths are hard-coded in
  `Phaselink.pro` and must be adjusted to your installation.
- A device / PC address; the current value used by the application is stored in
  the `IP_adress` file (`192.168.50.205`).

## Build and run

Build with qmake and the MSVC toolchain (Qt Creator, or `qmake` followed by
`nmake`/`jom`). A note in `Phaselink.pro` states that the vendor `client.dll`
supports the Release CRT/Qt only and that a Debug build crashes at start-up when
a heap block is freed across CRTs. Use a Release build — `CONFIG +=
force_debug_info` is already set, so Release output still carries PDB symbols if
you need to attach a debugger.

## Notes

- `phaselink_CN.qm` / `phaselink_EN.qm` are generated from the `.ts` files and
  are checked in; regenerate them with `lrelease` after editing translations.
- `build/release/release/default_config.json` is a sample/leftover build output
  kept in the repository.

## License

No license file is present in this repository. Contact the repository owner for
licensing terms.
