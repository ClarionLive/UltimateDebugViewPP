# UltimateDebugViewPP

Enhanced debug viewer for Windows with modern Ribbon UI, SQL formatting, and syntax highlighting.

## Overview

UltimateDebugViewPP is a streamlined version of DebugView++ focused solely on the Ultimate variant with:
- Modern Ribbon interface
- Scintilla-based text editor with syntax highlighting
- SQL formatting capabilities
- Support for multiple log sources (Win32 OutputDebugString, files, sockets, UDP)
- Advanced filtering with regex support
- High-performance message handling (50,000+ messages/second)

## Project Structure

```
UltimateDebugViewPP/
├── application/           # Source code
│   ├── DebugViewpp/      # Main application (28 files)
│   ├── DebugViewppLib/   # Core library (40+ files)
│   ├── CobaltFusion/     # Utilities
│   ├── Win32Lib/         # Windows utilities
│   ├── Libraries/        # UI components (PropertyGrid, TabbingFramework)
│   └── include/          # Headers
├── scintilla/            # Text editor component
├── lexilla/              # Syntax highlighting
├── fsqlf_dll/            # SQL formatter
└── build/                # Build output (generated)
```

## Dependencies

All dependencies are centralized to avoid duplication across projects:

- **Boost 1.80.0**: `C:\Libraries\boost_1_80_0`
- **WTL 10.0.10320**: `C:\Libraries\wtl_10_0_10320`
- **Snappy 1.1.1.7**: `C:\Libraries\snappy_1_1_1_7`

These are shared across all C++ projects on the system.

## Build Instructions

### Prerequisites
- Visual Studio 2022
- CMake (install via `choco install cmake` or download from https://cmake.org)
- Windows 10/11 SDK

### Building

#### Quick Build (One-liner in PowerShell)
```powershell
cd C:\Dev\UltimateDebugViewPP; cmake -B build -G 'Visual Studio 17 2022' .; cmake --build build --config Release --target UltimateDebugviewpp
```

#### Step-by-Step Build

1. **Generate Visual Studio solution**:
   ```powershell
   cd C:\Dev\UltimateDebugViewPP
   cmake -B build -G 'Visual Studio 17 2022' .
   ```

2. **Build the executable**:
   ```powershell
   cmake --build build --config Release --target UltimateDebugviewpp
   ```

3. **Output location**:
   - Executable: `C:\Dev\UltimateDebugViewPP\bin\Release\UltimateDebugviewpp.exe`

### Alternative: Build with Visual Studio GUI

1. Open `C:\Dev\UltimateDebugViewPP\build\UltimateDebugViewPP.sln`
2. Set configuration to Release
3. Build UltimateDebugviewpp project

## Quick Rebuild

For subsequent builds after code changes:
```powershell
cd C:\Dev\UltimateDebugViewPP
cmake --build build --config Release
```

## Clean Build

To start fresh:
```powershell
cd C:\Dev\UltimateDebugViewPP
if (Test-Path build) { Remove-Item -Recurse -Force build }
cmake -B build -G 'Visual Studio 17 2022' .
cmake --build build --config Release
```

## Development Notes

- **C++ Standard**: C++23
- **Runtime**: Static (/MT)
- **Character Set**: Unicode only
- **Minimum Windows**: Windows 7 (0x0601)

## Key Features

- **Multiple Log Sources**: DBWin, files, TCP/UDP, pipes
- **Advanced Filtering**: Include/exclude/highlight with regex
- **Syntax Highlighting**: C++, SQL, JSON, XML, and more via Scintilla/Lexilla
- **SQL Formatting**: Built-in SQL beautifier
- **High Performance**: Lock-free circular buffers for message handling
- **Process Tracking**: Monitor specific processes
- **Tabbed Interface**: Multiple filtered views of the same data

## AI Assistants (MCP Server)

UltimateDebugViewPP has a built-in [Model Context Protocol](https://modelcontextprotocol.io) server so an AI coding assistant (Claude Code, Codex, ...) can read, wait for and filter debug output while it builds and tests your app.

- **Enable:** Preferences > **AI / MCP** > *Enable MCP server* (off by default). Default port `7777`; a bearer token is required (generated automatically); best-effort password/token masking on by default; save tools write new files only, into a folder you choose (Documents\UltimateDebugView by default, or Documents / a fixed folder plus a sub-folder such as `DebugLogs`, optionally with a folder per program, e.g. `Documents\DebugLogs\PFBExec`). With several programs in the log the AI asks which one to save.
- **Endpoint:** `http://127.0.0.1:7777/mcp` (Streamable HTTP, loopback only; non-local `Host`/`Origin` headers are rejected).
- **Connect Claude Code:** use *Copy .mcp.json entry* on the AI / MCP tab, or see [`docs/mcp.sample.json`](docs/mcp.sample.json), or run
  `claude mcp add --transport http ultimate-debugview http://127.0.0.1:7777/mcp --header "Authorization: Bearer <token>"`.
- **Tools:** `get_status`, `read_messages`, `wait_for_messages`, `search_messages`, `list_processes`, `insert_marker`, `clear_log`, `pause_capture`, `resume_capture`, `list_tabs`, `create_tab`, `close_tab`, `switch_tab`, `rename_tab`, `get_filters`, `set_filters`, `add_filter`, `remove_filter`, `enable_filter`, `get_message_detail`, `jump_to_line`, `set_bookmark`, `list_bookmarks`, `get_procedure_stats`, `get_procedure_timeline`, `list_queues`, `get_queue`, `create_procedure_view`, `save_log`, `save_view`.
- **Cursors:** every line has a `line` number that only increases (also across clears). Pass `nextLine` back as `since_line` to read only new output.
- **Your tabs are protected:** tabs the AI creates are named `AI: ...`; changing other tabs needs `allow_user_tab: true`.

User guide: [`docs/AI-Assistant-Guide.md`](docs/AI-Assistant-Guide.md). In-app: Help > Advanced Features > *AI Assistants (MCP)* (`help/topics/ai-mcp.html`). Code: `application/DebugViewpp/McpServer.*`, `McpTools.*`.

## Troubleshooting

### Missing Dependencies
If build fails with missing headers:
1. Verify centralized libraries exist:
   - `C:\Libraries\boost_1_80_0`
   - `C:\Libraries\wtl_10_0_10320`
   - `C:\Libraries\snappy_1_1_1_7`

2. These were set up from the original DebugViewPP project's NuGet packages

### Build Errors
- Ensure Visual Studio 2022 is installed with C++ workload
- Check that Windows SDK is installed
- If CMake is not found, install it via Chocolatey: `choco install cmake`
- After installing CMake, restart your terminal for PATH changes to take effect

## Original Project

This is a cleaned-up version of [DebugView++](https://github.com/CobaltFusion/DebugViewPP) containing only the UltimateDebugViewPP executable without test projects or console variants.