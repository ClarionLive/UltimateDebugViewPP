# Using Ultimate DebugView++ with an AI assistant

Ultimate DebugView++ can let an AI coding assistant (Claude Code, Codex or any other MCP client) **read, wait for, search, filter and save your debug output** while it builds and tests your application. You no longer need to copy messages out of the viewer or send screenshots: the assistant can put a marker in the log, run your program, wait for the line it expects and read what happened.

It works through the **Model Context Protocol (MCP)**, built into the viewer. The server only listens on your own computer, is off until you turn it on, and only lets in programs that run as your Windows user. Setup is one button; there are no files to edit.

---

## Contents

1. [Turn it on](#1-turn-it-on)
2. [Connect your AI assistant](#2-connect-your-ai-assistant)
3. [Things to ask for](#3-things-to-ask-for)
4. [What the assistant can and cannot do](#4-what-the-assistant-can-and-cannot-do)
5. [Saved logs: where they go](#5-saved-logs-where-they-go)
6. [Privacy and security](#6-privacy-and-security)
7. [Troubleshooting](#7-troubleshooting)
8. [Tool reference](#8-tool-reference)

---

## 1. Turn it on

1. Open **Preferences** and choose the **AI / MCP** tab.
2. Press **Set up my AI agents**. This ticks **Enable MCP server** and connects your assistants (see below).
3. Leave **Mask passwords and tokens** ticked (recommended).
4. Press **OK**.

The status bar now shows `Ready | MCP :7777`. If it shows `MCP off: …`, the reason follows. Usually another program already uses the port, so pick another one and press **Set up my AI agents** again.

## 2. Connect your AI assistant

**Set up my AI agents** registers the viewer with every assistant it finds for your Windows user, using their own setup commands:

- **Claude Code**, for every folder and every session: the terminals in MultiTerminal, the Clarion Clone assistant and any `claude` you start yourself. It is added to your default Claude profile and to any other Claude profile folder (`%USERPROFILE%\.claude-…`).
- **Codex**, if it is installed.

The result is shown under the button, for example *Claude Code: registered for every folder (default profile, .claude-work). Codex: not installed.* Assistants that are already running pick the server up the next time they start. In Claude Code, type `/mcp` to see **ultimate-debugview** connected with 30 tools.

Pressing the button again is harmless; do it after you change the port. **Remove** takes the entry out of the same assistants again.

> A Claude profile kept somewhere other than `%USERPROFILE%\.claude-…` is not found automatically. Add the server to it from a terminal, with `CLAUDE_CONFIG_DIR` set to that profile's folder:
> `set CLAUDE_CONFIG_DIR=D:\Profiles\claude-client` and then `claude mcp add --scope user --transport http ultimate-debugview http://127.0.0.1:7777/mcp`

### Other assistants

Any MCP client that supports the **Streamable HTTP** transport can connect to `http://127.0.0.1:7777/mcp`. It needs no token, as long as it runs as your Windows user.

### Optional: an access token as well

If you want a client to prove itself with a secret too, tick **Also require an access token**. A token is created for you (**Generate** makes a new one). Then every client must send `Authorization: Bearer <token>`. **Set up my AI agents** adds the token to Claude Code for you, and **Copy .mcp.json entry** copies an entry with it for clients you set up yourself. Codex cannot be set up this way while the token is required.

## 3. Things to ask for

You talk to your assistant normally; it picks the tools. Some examples that work well:

**Watch a test run**
> "Put a marker in the debug viewer, then I'll start the app. Wait for a line containing *Main entered* or *ERROR* and tell me what was logged after the marker."

**Find SQL problems**
> "Find the most recent SQL error in the debug viewer, show me the statement formatted, and jump to that line."

**Organise the output**
> "Create a tab called SQL that only shows SELECT, UPDATE and INSERT lines, and highlight SQLSTATE in red."

**Clarion procedure problems**
> "Which Clarion procedures were opened more often than they were closed? What is still open on each thread?"
> "Show me the procedure timeline for BrowseDocs since the last marker."

**Look at a queue**
> "List the queues that were dumped to the log and show me the first 20 rows of the Customers queue."

**Save a log**
> "Save a debug log for PFBExec as test1.dblog."

**Tidy up**
> "Clear the log", "Pause capture", "Close the tabs you created."

A good habit for repeatable tests: ask the assistant to **insert a marker before each run**, for example `=== test 3: open BINDOC ===`. Then it can read exactly what that run produced.

## 4. What the assistant can and cannot do

| It can | It cannot |
|---|---|
| Read and search everything captured, from any tab | Start programs or run commands (the viewer has no such tool) |
| Wait for lines to appear, without freezing the viewer | Change **your** tabs' filters, rename or close **your** tabs, unless you ask it to |
| Create its own tabs (always named **AI: …**) and change their filters | Close the first tab |
| Clear the log, pause and resume capture | Choose where files are saved, or replace an existing file |
| Bookmark lines and scroll the viewer to a line | See passwords and tokens when masking is on |
| Save logs as **new** files in the folder you chose | Connect from another computer |

**Your tabs are protected.** If you ask *"add an exclude filter to my View tab"*, that request is your permission, and the assistant may change that tab. It will not touch your tabs on its own initiative.

**While a dialog is open** (Filter, Preferences, a message box), the assistant's actions wait until you close it. If you leave the dialog open for more than 15 seconds, the action is cancelled and nothing happens later.

## 5. Saved logs: where they go

On the **AI / MCP** tab, under **Logs the AI saves**:

- **My Documents** (default) or **This folder:** any folder you pick with **Browse…**.
- **Sub-folder:** for example `DebugLogs`. It is created when needed. In My Documents, an empty sub-folder means `UltimateDebugView`; with **This folder:**, an empty sub-folder saves straight into that folder.
- **A folder per program:** each program's logs go into a folder named after its exe, for example `Documents\DebugLogs\PFBExec\test1.dblog`.

How saving behaves:

- The assistant only chooses the **file name** (`.dblog`, `.log`, `.txt` or `.csv`); the folder always comes from your settings.
- A file is never replaced. If the name exists, the assistant must pick another.
- Saving *for a program* (for example "save a log for PFBExec") keeps only that program's lines.
- With **a folder per program** on and several programs in the log, the assistant shows you the list (with line counts) and asks which one you mean.
- `.dblog` files open in the viewer with **File > Open**.

## 6. Privacy and security

- **Local only:** the server listens on `127.0.0.1`, and requests from web pages are refused, even pages served from your own computer.
- **Your Windows user only:** for every connection the viewer checks which program is on the other end. Programs running as another Windows user are refused, and so are sandboxed programs running as you (low integrity or AppContainer, such as a browser's web page process). Any ordinary program you run yourself can connect, the same as it could read your files.
- **Access token (optional):** tick **Also require an access token** if you also want a secret.
- **Masking:** on by default. Values of `PWD=`, `Password=` / `"password":`, `secret`, `token`, `api_key`, `Bearer …` and passwords in URLs (`user:pass@host`) reach the assistant as `***`. Its searches only see the masked text. This is **best effort**; it cannot recognise every kind of secret. Your own view of the log is never changed.
- **Saved files are not masked:** they contain the log as captured and stay on your computer.
- **What the assistant reads goes to its AI service.** Only enable the server while you use it, and keep masking on.
- **Safe file writing:** saves never follow links or junctions and never overwrite files, even if another Windows user can write to the target folder.

## 7. Troubleshooting

| Symptom | What to do |
|---|---|
| Status bar shows `MCP off: …` | The port is in use. Choose another port, press **Set up my AI agents** again, then OK. |
| `/mcp` doesn't list **ultimate-debugview** | The assistant was already running. Restart it. Check the result under **Set up my AI agents**. |
| `/mcp` shows the server as failed | Make sure the viewer is running with the server on (status bar `MCP :7777`). |
| **403** "only programs running as the same Windows user" | The assistant runs as another user, or sandboxed. Run it as yourself, not elevated as another account. |
| **401** | **Also require an access token** is on and the client's token doesn't match. Press **Set up my AI agents** again, or copy the entry again. |
| An old project `.mcp.json` still has an `ultimate-debugview` entry | It is no longer needed; the one-click setup covers every folder. You can delete that entry. |
| Nothing is captured | Capture may be paused. Press Resume, or ask the assistant to resume capture. Another debug viewer (Sysinternals DebugView, Visual Studio's debugger) may also hold the capture. |
| "The viewer was busy for 15 s…" | A dialog was open in the viewer. Close it and ask again; nothing was changed. |
| The assistant can't find a line it "should" see | It may be reading one tab whose filters hide the line. Ask it to read the whole log. |
| "File already exists" | Saves never replace files. Ask for another name. |
| "No program by that name…" | That program hasn't written to the log (since the last clear). The error lists the programs that have. |

---

## 8. Tool reference

Every line in the log has a **line number** that only goes up, even after the log is cleared. Most read tools return `nextLine`. Pass it back as `since_line` to get only what is new.

### Status and reading

| Tool | What it does | Main parameters |
|---|---|---|
| `get_status` | Version, tool list, capture state, current cursor (`nextLine`), tabs, masking and save-folder settings. | none |
| `read_messages` | Reads lines, oldest first, from the whole log or one tab. Consecutive identical lines are folded into one with `repeat`. | `tab`, `since_line`, `limit` (≤1000), `tail`, `contains`, `regex`, `process`, `pid`, `collapse_repeats`, `max_bytes` |
| `search_messages` | Finds lines and returns each with the lines around it. | `pattern`, `regex`, `context` (0–10, default 2), `limit` (1–100, default 20), `tab`, `since_line`, `process`, `pid` |
| `wait_for_messages` | Waits until matching lines arrive (default 30 s, at most 55 s per call). Returns `matched`, `timedOut`, the lines and `nextLine`. | `since_line`, `contains`, `regex`, `process`, `pid`, `tab`, `count` (1–100, default 1), `timeout_seconds` |
| `list_processes` | Programs that wrote to the log: pid, name, line count, first and last line and time. | none |
| `get_message_detail` | Everything about one line: Clarion fields (thread, procedure, module, call depth, open/close), queue, error fields, detected format, and formatted text (SQL with your formatting options). | `line` |

Each line in a result has: `line`, `time`, `systemTime`, `pid`, `process`, `text`, plus `clarion` (thread, app, module, procedure, prefix) for UltimateDebug messages and `repeat` when folded.

### Control

| Tool | What it does | Main parameters |
|---|---|---|
| `insert_marker` | Writes a marker line and returns `cursor`. Use it as `since_line` afterwards. | `text` |
| `clear_log` | Clears the log and every tab, like the Clear button. | none |
| `pause_capture` / `resume_capture` | Like the Pause button. | none |

### Tabs and filters

| Tool | What it does | Main parameters |
|---|---|---|
| `list_tabs` | All tabs: index, name, lines shown, filter count, whether the assistant created it. | none |
| `create_tab` | New tab named **AI: <name>**, optionally with filters. | `name`, `filters`, `select` |
| `switch_tab` | Shows a tab. | `tab` |
| `rename_tab` / `close_tab` | For the assistant's own tabs (or yours with `allow_user_tab` when you asked). | `tab`, `name`, `allow_user_tab` |
| `get_filters` | A tab's filters. | `tab` |
| `set_filters` / `add_filter` / `remove_filter` / `enable_filter` | Replace, add, remove, or switch filters on and off. | `tab`, filter fields, `category` + `index`, `enabled`, `allow_user_tab` |

A filter has `text`, `matchType` (`simple`, `wildcard`, `regex`, `regexgroups`, `regexcase`), `filterType` (`include`, `exclude`, `highlight`, `token`, `stop`, `track`, `once`, `clear`, `beep`, `matchcolor`, `bookmark`), `category` (`message`, `process`, `prefix`, `procedure`), `bgColor`, `fgColor` (`#RRGGBB`) and `enabled`.

### Pointing things out

| Tool | What it does | Main parameters |
|---|---|---|
| `jump_to_line` | Scrolls the viewer to a line and selects it, switching tabs if needed. | `line`, `tab` |
| `set_bookmark` / `list_bookmarks` | Sets, clears or lists bookmarks in a tab. | `line`, `bookmarked`, `tab` |

### Clarion

| Tool | What it does | Main parameters |
|---|---|---|
| `get_procedure_stats` | Open and close counts per procedure, most unbalanced first, plus what is open now on each thread. `unbalanced > 0` means more opens than closes. | `tab`, `procedure`, `unbalanced_only`, `limit` |
| `get_procedure_timeline` | Opens and closes in order, with call depth and (for closes) duration. | `tab`, `since_line`, `procedure`, `thread`, `process`, `limit` |
| `list_queues` / `get_queue` | Clarion queues dumped to the log by the UltimateDebug class: list, then headers and rows. | `queue_id`, `offset`, `limit` |
| `create_procedure_view` | A tab (**AI: Proc: <name>**) showing one procedure's lines. | `procedure`, `process` |

### Saving

| Tool | What it does | Main parameters |
|---|---|---|
| `save_log` | Saves the log (or one program's lines) to a **new** file in your chosen folder. Returns the full path. | `path` (file name only), `process` |
| `save_view` | Saves the lines a tab shows. | `path`, `tab`, `process` |

If the result has `needsChoice: true`, the assistant shows you the `apps` list, asks which program, and calls again with `process`.
