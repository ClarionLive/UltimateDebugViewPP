# CLAUDE.md

Guidance for Claude Code (and other agents) working in this repository.

## Project

**Ultimate DebugView++**: ClarionLive's enhanced fork of [DebugView++](https://github.com/CobaltFusion/DebugViewPP), a Windows viewer for `OutputDebugString` and other debug output. On top of the original, it adds:
- a ribbon UI
- a detail pane with SQL/JSON/XML formatting (fsqlf)
- syntax highlighting (Scintilla/Lexilla)
- Clarion integration: the UltimateDebug message format, procedure tracking, timeline and stats, queue dumps
- WebView2 in-app help
- a **built-in MCP server** so AI assistants can drive the viewer

- Repo: https://github.com/ClarionLive/UltimateDebugViewPP (public). `main` is the default branch; work happens on `develop` and task branches.
- Exe: `UltimateDebugviewpp.exe`, version 2.0.0.12 (`application/DebugViewpp/version.h`).
- Deployed copy (the Owner's shortcut): `C:\Utilities\UltimateDebugPP\UltimateDebugViewPP`. It holds the exe, `fsqlf.dll` and `help\`.

## Build

Visual Studio 2022 generator, built from the **repo root** into `build\`. Output goes to `bin\Release` (or `bin\Debug`).

```bash
# configure (first time, or after CMake changes)
cmake -S . -B build -G "Visual Studio 17 2022"
# build only the app (run from Bash; PowerShell mangles "-m:4")
cmake --build build --config Release --target UltimateDebugviewpp -- -m:4 -v:m
```

- **External dependencies live in `C:\Libraries`** (see `cmake/CMakeLists.dependencies.cmake`): `boost_1_80_0` (headers only), `wtl_10_0_10320` and `snappy_1_1_1_7` (with `src/` and `include/`). They are not in the repo. The WebView2 SDK is downloaded into `build\packages` by CMake.
- **Close the viewer before building.** A running `bin\Release\UltimateDebugviewpp.exe` makes the link fail with LNK1104. The Owner often has it open, so ask them to close it; don't kill it.
- A post-build step copies `help\` (the tracked in-app help) to `bin\<config>\help`.
- C++20 (`cxx_std_20`), Unicode, static CRT (`/MT`), `/W4`. Boost.JSON is compiled header-only into `McpServer.cpp`.
- `build.bat`, `build_with_ninja.bat` and `azure-pipelines.yml` come from the original project and are not maintained.
- `build.laptop\` is the Owner's old build tree, kept for reference and ignored by git.

## Layout

| Path | What |
|---|---|
| `application/DebugViewpp/` | The WTL GUI: `MainFrame`, `LogView`, `CLogViewTabItem2` (detail pane), `PreferencesDlg`, `HelpTabView`, Clarion views, MCP server |
| `application/DebugViewppLib/` | Core: log sources (`DBWinReader`, file, socket, pipe), `LogFile`, `Filter`, `SqlFormatter`, `SyntaxHighlighter`, `ClarionProcedureTracker`, `ClarionQueueManager` |
| `application/include/` | Public headers for the libraries |
| `application/CobaltFusion`, `Win32Lib`, `IndexedStorageLib`, `Libraries/` | Utilities, Win32 wrappers, Snappy storage, PropertyGrid / TabbingFramework |
| `scintilla/`, `lexilla/`, `fsqlf_dll/` | Third-party editor, lexers and the SQL formatter DLL (tracked as plain files) |
| `ClarionDebugTemplates/` | The Clarion side: `UltimateDebug` class and `UltimateSQL`, which produce the `[||]X,T:..,P:..,MSG:..` lines the viewer parses |
| `help/` | In-app help (`index.html`, `topics/*.html`), copied next to the exe on build |
| `docs/` | `AI-Assistant-Guide.md` (end-user MCP guide), `mcp.sample.json` |
| root `*.md` / `*.txt` | Historical design notes and session prompts from earlier work, kept for reference |

## Key behaviour

- All capture, filtering and view updates happen on the **UI thread** (`CMainFrame::ProcessLines`). Keep it that way.
- Each tab is a `CLogView` over the shared `LogFile`, with its own `LogFilter` and its own `ClarionProcedureTracker`.
- Settings live in the registry at `HKCU\Software\ClarionLive\UltimateDebugView++` (`CMainFrame::LoadSettings` / `SaveSettings`).

## MCP server (AI assistants)

- Files: `McpServer.*` (HTTP/JSON-RPC), `McpTools.*` (30 tools; `McpBridge` is a friend of `CMainFrame` and `CLogView`), `McpSettings.h`, `McpAgentSetup.*` (the *Set up my AI agents* button: `claude mcp add --scope user` per Claude profile, `codex mcp add --url`), the AI / MCP page in `PreferencesDlg.*`.
- Streamable HTTP on `127.0.0.1:7777/mcp`. It is off by default and Host/Origin are checked. Each connection's peer process is looked up (`GetExtendedTcpTable`) and must run as the same Windows user, not AppContainer, at medium integrity or above. The bearer token is optional (`McpRequireToken`). Tool calls are marshalled to the UI thread (`WM_MCP_CALL`) with claim/cancel and held while a modal dialog is open.
- Registry keys: `McpEnabled`, `McpPort`, `McpRequireToken`, `McpToken`, `McpMaskSecrets`, `McpExportMode`, `McpExportFolder`, `McpExportSubfolder`, `McpExportPerApp`.
- Security rules any change must keep:
  - Save tools write **new files only**, through one verified handle, into the folder the user chose. The AI supplies a plain file name only.
  - Tabs the AI creates are named `AI: …`. User tabs need `allow_user_tab`.
  - Masking is best effort and on by default.
  - The design was reviewed through several security passes; re-review any change to `McpServer.cpp` or the save path.
- User docs: `docs/AI-Assistant-Guide.md` and `help/topics/ai-mcp.html` / `ai-mcp-tools.html`. Keep them in step with tool changes.

## Testing the MCP server against a live instance

The scripted smoke tests set registry values, launch `bin\Release\UltimateDebugviewpp.exe`, call the tools over HTTP and kill the process.
- **Snapshot the Owner's `Mcp*` registry values first and restore them exactly afterwards.** Never just delete them.
- Don't start a second instance while the Owner's viewer is running: capture is exclusive.

## Conventions

- Follow the surrounding code style (WTL message maps, `fusion::debugviewpp` namespace, 4-space indent).
- Commit or push only when asked.
- To push to ClarionLive, use a one-off token: `GH_TOKEN=$(gh auth token --user ClarionLive)` with the real `gh.exe`. Never run `gh auth switch`; it is global and other agents rely on it.
