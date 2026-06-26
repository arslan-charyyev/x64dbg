# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this fork is for

x64dbg is a Windows-only binary debugger (malware analysis / reverse engineering). This checkout is for the **in-progress Linux/cross-platform port**, which lives **entirely under `src/cross/`**. We are AI-assisted contributors to that port (led by @3rdit; original author is mrexodia).

Priorities, in order, per the maintainers:
1. **Make the Qt GUI widgets reusable cross-platform** — this is mrexodia's only real goal ("otherwise I do not care much about the code/features"). The widgets currently live in the Windows GUI and must compile and run on Linux unchanged.
2. **Grow Linux feature parity with the Windows version** behind that reusable-widget foundation.

The current spike is the GUI: getting more of `src/gui/Src` to build and behave cross-platform, and matching the Windows interface. Do **tech discovery + a brief written plan** before large implementation work — hash out structure and open questions first.

Reference discussions: x64dbg/x64dbg discussions #3838, PR #3839, PR #3883.

## The split: Windows root vs. `src/cross/` (read this first)

- The **root project** (root `cmake.toml`, `src/dbg`, `src/gui`, `src/bridge`, `src/exe`, `src/headless`, `src/launcher`) is **Windows/MSVC only** — Qt5 WinExtras, Win32 libs (Psapi, Wininet, dbghelp, …), TitanEngine/GleeBug. It does **not** build natively on Linux. `docs/COMPILE-linux.md` only describes cross-compiling it to Windows binaries via `msvc-wine` — that is **not** our porting workflow. Do not try to build the root project unless explicitly asked.
- **All Linux/port work happens in `src/cross/`**, which is a *separate* CMake/cmkr project rooted at `src/cross/cmake.toml`.

## Build & run (the cross project)

Canonical configure+build (matches CI in `.github/workflows/cross.yml`):

```bash
cd src/cross
cmake -B build -G Ninja -DCMAKE_UNITY_BUILD=ON
cmake --build build
```

- **Qt5 and Qt6 are both supported and auto-detected** (`src/cross/widgets/Qt.cmake` prefers Qt6, falls back to Qt5). Both are installed locally (Qt 5.15.19 and Qt6). Qt components needed: Widgets, Svg, PrintSupport, WebSockets (+ OpenGLWidgets on Qt6).
- Build one target: `cmake --build build --target debugger` (also `minidump`, `hex_viewer`, `remote_table`, `release_notes`, `ElfBug`, `x64dbg_widgets`).
- Run a built app directly, e.g. `./build/debugger`, `./build/minidump`.
- **AppImage** packaging: `src/cross/debugger/build-appimage.sh` (outputs `src/cross/build-linux/x64dbg.AppImage`). Requires `linuxdeploy` + its Qt plugin (currently **not installed locally** — only needed for AppImage, not normal dev builds) and is normally run inside the `ghcr.io/x64dbg/x64dbg/qt5-appimage` image (see `src/cross/debugger/Dockerfile`, Ubuntu 20.04 + Qt5 + gcc-13).

**If a build fails on a missing system dependency, stop and ask** — do not auto-install or pull toolchains. Likely candidates if something is off: a missing Qt component/module, `linuxdeploy` (AppImage only).

## Build system: cmkr (`cmake.toml` is the source of truth)

This project uses [cmkr](https://build-cpp.github.io/cmkr). Most `CMakeLists.txt` files are **generated** and carry a `DO NOT EDIT` banner (also `linguist-generated` in `.gitattributes`). Edit the sibling **`cmake.toml`** instead; the cmkr bootstrap (`cmake/cmkr.cmake`) regenerates `CMakeLists.txt` on the next configure.

- cmkr-managed: root `cmake.toml`, `src/cross/cmake.toml`, `src/cross/vendor/cmake.toml`, `src/cross/ElfBug/tests/cmake.toml`.
- **Exception — hand-written:** `src/cross/widgets/CMakeLists.txt` has no `cmake.toml`. It is maintained by hand and is **where you add ported widget sources** (see architecture below).

## Tests (ElfBug only)

Disabled by default. Linux-only. Catch2-based (fetched at configure time).

```bash
cd src/cross
cmake -B build -G Ninja -DELFBUG_BUILD_TESTS=ON
cmake --build build
cd build && ctest --output-on-failure        # or run ./build/tests/ElfBug_tests directly
# single test: ./build/tests/ElfBug_tests "<test name or tag>"
```

Tests live in `src/cross/ElfBug/tests/` and drive ElfBug against small ELF fixtures (`targets/*.cpp`, built `-pie -g -O0` into `build/tests/targets/`). There is currently **no test suite for the widgets/GUI side.**

## Formatting (enforced in CI)

AStyle, Allman style (`align-pointer=type`, `align-reference=middle`, convert-tabs). The cross-platform entry point is a `uv`-run script (`uv` is installed):

```bash
uv run --script .github/format/AStyleHelper.py Check     # verify (CI uses this logic)
uv run --script .github/format/AStyleHelper.py Silent     # apply formatting
```

CI job `format.yml` fails the build if code is not formatted. Excludes `src/cross/vendor` and `src/gui/Src/ThirdPartyLibs/md4c`. Run it before proposing commits that touch C++.

**Line endings: `.cpp`/`.h` are CRLF** (`.editorconfig` sets `crlf` for `[*]`; `.gitattributes` `* -text` disables normalization). Preserve CRLF when editing source files — do not convert to LF. CMake/`.cmake`/`.toml` are LF.

## Architecture: how the Linux port reuses the real Windows GUI

The port does **not** reimplement the GUI. It compiles the *actual* Windows widget source files against a reimplemented, platform-neutral Bridge shim. The five moving parts:

1. **`src/gui/Src/`** — the canonical Windows Qt GUI (unchanged home of the widgets). On Windows it talks to the debugger through the Bridge (`src/bridge/`) → `dbg` DLL (`src/dbg/`, the TitanEngine/GleeBug-based core).

2. **`src/cross/widgets/` → `x64dbg::widgets`** (static lib, the heart of the port). Its hand-written `CMakeLists.txt` compiles a **curated subset of `src/gui/Src/*` files in place** (via `widgets_SOURCE_DIR = ../../gui/Src`) — not copies. It pairs them with a **cross-platform Bridge shim** (`widgets/Bridge.{h,cpp}`, `Configuration.cpp`, `Types.h`, `RegisterContext.h`) that re-implements the same `Dbg*` / `Bridge*` / `Gui*` free-function API the widgets expect — but backed by a `MemoryProvider` abstract interface + a breakpoint-query hook instead of the Windows `dbg` DLL. **Many shim functions are deliberate stubs/TODOs** (e.g. `DbgEval` is a hex-only parser, `DbgGetLabelAt`/`DbgGetStringAt` return `false`). Builds under Qt5 or Qt6. Also pulls in two shared lower-level libs from the Windows tree as subdirectories: `src/zydis_wrapper` (Zydis disassembler) and `src/gui/Src/ThirdPartyLibs/md4c` (markdown).

   **The core porting loop:** to bring another widget cross-platform, add its `src/gui/Src/...` files to `widgets_SOURCES` in `widgets/CMakeLists.txt`, build, then satisfy whatever Bridge/Dbg API it newly references by implementing or stubbing it in the shim. Prefer making widgets *Bridge-API-driven* over sprinkling `#ifdef`. Remember those same `src/gui/Src/*` files **still build on Windows** — guard platform-specific code; don't break the Windows build.

3. **`src/cross/ElfBug/` → `ElfBug`** (static lib, **Linux-only**). A native ptrace-based ELF debugger engine (x86-64 + i386) — the Linux analogue of the Windows `dbg`+TitanEngine core, far smaller and younger. Public C API: `ElfBug/ElfBug/api/elfbug_api.h` (`ElfBugCreate`, `ElfBugStart` = blocking debug loop, `ElfBugContinue/StepInto/Pause/Stop` = thread-safe, memory R/W, registers, breakpoints, memory-map & breakpoint enumeration, module-from-addr). Internals: `core/Debugger*` (event loop split across `Loop.Process/Signal/Thread`), `process/` (Memory, Breakpoint, ProcessArch), `thread/` (Registers, HardwareBreakpoint, Thread).

4. **`src/cross/debugger/` → `debugger`** (Qt app, **Linux-only**) — the actual Linux x64dbg. `gui/MainWindow.cpp` assembles the CPU tab (Disassembly + RegistersView + HexDump + CPUStack, all from `x64dbg::widgets`) and wires it to **`core/DbgAdapter`**, the glue class: it implements the widgets' `MemoryProvider` interface **and** drives `ElfBug` via its callback struct, translating ElfBug events → Qt signals (`processCreated`, `registersUpdated`, `stopped`, …). The blocking `ElfBugStart` runs on a dedicated `QThread`. The Memory Map tab is a wired-up `MemoryMapView`; the Breakpoints, Call Stack, and Threads tabs are still placeholders ("not yet implemented").

5. **`src/cross/{minidump,hex_viewer,remote_table,release_notes}/`** — standalone cross-platform Qt example apps that consume `x64dbg::widgets` to **prove the widgets are reusable outside the debugger** (mrexodia's actual goal). They pull extra vendored deps via `src/cross/vendor/` (cmkr `[fetch-content]`: nlohmann_json, cpp-httplib, linux-pe, mrexodia/PatternLanguage `libpl`, udmp-parser).

**Runtime data flow:** widget calls `DbgMemRead`/`DbgMemFindBaseAddr`/`DbgGetBpxTypeAt`/… → cross Bridge shim routes to the registered provider → `DbgAdapter` → ElfBug C API → ptrace. Wire it up with `DbgSetMemoryProvider(adapter)` and `DbgSetBreakpointQuery(fn)`; tear down with `DbgSetMemoryProvider(nullptr)` on process exit.

## Conventions & gotchas specific to this work

- Edit `cmake.toml`, never the generated `CMakeLists.txt` — **except** `src/cross/widgets/CMakeLists.txt`, which is hand-written.
- Touching `src/gui/Src/*` affects the Windows build too; keep it compiling on Windows.
- Growing the Bridge shim (replacing stubs with real implementations — often backed by ElfBug, or a real expression evaluator for `DbgEval`) is expected, normal work. Grep `src/cross/widgets/Bridge.cpp` for `TODO`.
- Preserve CRLF in `.cpp`/`.h`.
- `build*/` and `cmake-build*/` are gitignored; the cross build dir (`src/cross/build`) won't show in `git status`.
- This guidance lives at `.claude/CLAUDE.md` (tracked) — Claude Code auto-loads project memory from `<repo>/.claude/CLAUDE.md`, so there is no repo-root `CLAUDE.md` to keep in sync.
