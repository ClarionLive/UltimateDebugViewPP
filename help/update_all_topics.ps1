# PowerShell script to update ALL help topic files with real content based on codebase analysis

$topicsPath = "C:\Dev\UltimateDebugViewPP\bin\Debug\help\topics"

# Helper function to write files
function Write-HelpFile {
    param($filename, $content)
    $content | Set-Content "$topicsPath\$filename" -Encoding UTF8
    Write-Host "Updated: $filename"
}

# Network Sources
Write-HelpFile "network-sources.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Network Sources - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Network Sources</h1>
    <p class="lead">Receive debug messages from remote applications over TCP and UDP connections.</p>

    <h2>Supported Protocols</h2>

    <h3>UDP Socket Listener</h3>
    <p>Listen for UDP messages on configurable ports:</p>
    <ul>
        <li><strong>Default port:</strong> 9999 (configurable)</li>
        <li><strong>Message format:</strong> Plain text or structured</li>
        <li><strong>Multicast support:</strong> Join multicast groups</li>
        <li><strong>Multiple listeners:</strong> Different ports simultaneously</li>
    </ul>

    <h3>TCP DebugView Agent</h3>
    <p>Connect to Sysinternals DebugView Agent:</p>
    <ul>
        <li><strong>Standard port:</strong> 2020</li>
        <li><strong>Protocol:</strong> DebugView network protocol</li>
        <li><strong>Authentication:</strong> Optional password protection</li>
        <li><strong>Compression:</strong> Supports compressed streams</li>
    </ul>

    <h2>Setting Up Network Sources</h2>

    <h3>UDP Listener</h3>
    <ol>
        <li>Click <strong>Home → Add Network → UDP</strong></li>
        <li>Enter port number (default: 9999)</li>
        <li>Optional: Configure bind address</li>
        <li>Click OK to start listening</li>
    </ol>

    <h3>TCP Agent Connection</h3>
    <ol>
        <li>Click <strong>Home → Add Network → TCP Agent</strong></li>
        <li>Enter remote host address</li>
        <li>Port 2020 (standard) or custom</li>
        <li>Enter password if required</li>
        <li>Click Connect</li>
    </ol>

    <h2>Sending Messages to UltimateDebugView++</h2>

    <h3>UDP Example (Python)</h3>
    <pre><code>import socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.sendto(b"Debug message", ("localhost", 9999))</code></pre>

    <h3>UDP Example (PowerShell)</h3>
    <pre><code>$endpoint = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Parse("127.0.0.1"), 9999)
$udpClient = New-Object System.Net.Sockets.UdpClient
$bytes = [Text.Encoding]::UTF8.GetBytes("Debug message")
$udpClient.Send($bytes, $bytes.Length, $endpoint)</code></pre>

    <h3>UDP Example (C#)</h3>
    <pre><code>using (var client = new UdpClient())
{
    var data = Encoding.UTF8.GetBytes("Debug message");
    client.Send(data, data.Length, "localhost", 9999);
}</code></pre>

    <h2>Message Format</h2>
    <p>Network messages can include metadata:</p>
    <pre><code>[timestamp]|[hostname]|[process]|message
2024-01-15 10:30:45|server01|app.exe|Application started</code></pre>

    <h2>Advanced Configuration</h2>

    <h3>Firewall Settings</h3>
    <ul>
        <li>Open required ports in Windows Firewall</li>
        <li>Configure inbound rules for listeners</li>
        <li>Allow UltimateDebugView++ through firewall</li>
    </ul>

    <h3>Network Security</h3>
    <ul>
        <li>Use local-only binding for security</li>
        <li>Implement application-level authentication</li>
        <li>Consider VPN for remote debugging</li>
        <li>Use TCP with encryption for sensitive data</li>
    </ul>

    <h3>Performance Tuning</h3>
    <ul>
        <li><strong>Buffer size:</strong> Configure receive buffer</li>
        <li><strong>Thread pool:</strong> Dedicated network threads</li>
        <li><strong>Message batching:</strong> Process in groups</li>
        <li><strong>Rate limiting:</strong> Prevent flooding</li>
    </ul>

    <div class="note">
        <strong>Tip:</strong> Use different ports for different applications or environments to organize network debug streams.
    </div>

    <div class="topic-nav">
        <a href="file-monitoring.html" class="nav-prev">Previous: File Monitoring</a>
        <a href="debugview-agent.html" class="nav-next">Next: DebugView Agent</a>
    </div>
</body>
</html>
'@

# DebugView Agent
Write-HelpFile "debugview-agent.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>DebugView Agent - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>DebugView Agent</h1>
    <p class="lead">Remote debugging using the Sysinternals DebugView Agent protocol.</p>

    <h2>Overview</h2>
    <p>The DebugView Agent allows you to capture debug output from remote machines. UltimateDebugView++ implements the DbgviewReader class for compatibility with the standard DebugView Agent protocol.</p>

    <h2>Agent Setup</h2>

    <h3>On Remote Machine</h3>
    <ol>
        <li>Install Sysinternals DebugView</li>
        <li>Run: <code>Dbgview /a /t</code></li>
        <li>Configure agent options:
            <ul>
                <li>Port: 2020 (default)</li>
                <li>Password: Optional security</li>
                <li>Compression: For bandwidth optimization</li>
            </ul>
        </li>
    </ol>

    <h3>In UltimateDebugView++</h3>
    <ol>
        <li>Click <strong>Home → Remote Agent</strong></li>
        <li>Enter remote machine address</li>
        <li>Provide password if set</li>
        <li>Click Connect</li>
    </ol>

    <h2>Connection Management</h2>
    <ul>
        <li><strong>Auto-reconnect:</strong> Automatic reconnection on disconnect</li>
        <li><strong>Connection timeout:</strong> Configurable timeout values</li>
        <li><strong>Keep-alive:</strong> Maintains connection health</li>
        <li><strong>Multiple connections:</strong> Connect to several agents</li>
    </ul>

    <h2>Security Considerations</h2>
    <ul>
        <li>Use passwords for authentication</li>
        <li>Restrict agent to specific IP addresses</li>
        <li>Use VPN for internet connections</li>
        <li>Monitor agent access logs</li>
    </ul>

    <div class="topic-nav">
        <a href="network-sources.html" class="nav-prev">Previous: Network Sources</a>
        <a href="filtering-basics.html" class="nav-next">Next: Filtering Basics</a>
    </div>
</body>
</html>
'@

# Filtering Basics
Write-HelpFile "filtering-basics.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Filtering Basics - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Filtering Basics</h1>
    <p class="lead">Control message flow with powerful filtering capabilities.</p>

    <h2>Filter Types</h2>
    <p>UltimateDebugView++ provides 10 different filter types:</p>
    <ul>
        <li><strong>Include:</strong> Show only matching messages</li>
        <li><strong>Exclude:</strong> Hide matching messages</li>
        <li><strong>Highlight:</strong> Color-code matching messages</li>
        <li><strong>Token:</strong> Advanced token-based filtering</li>
        <li><strong>Track:</strong> Track specific patterns over time</li>
        <li><strong>Stop:</strong> Stop processing when matched</li>
        <li><strong>Once:</strong> Show only first occurrence</li>
        <li><strong>Bookmark:</strong> Auto-bookmark matches</li>
        <li><strong>Beep:</strong> Audio alert on match</li>
        <li><strong>Clear:</strong> Clear view when matched</li>
    </ul>

    <h2>Match Types</h2>
    <ul>
        <li><strong>Simple:</strong> Basic substring matching</li>
        <li><strong>Wildcard:</strong> Using * and ? patterns</li>
        <li><strong>Regex:</strong> Full regular expression support</li>
        <li><strong>RegexGroups:</strong> Capture groups in regex</li>
    </ul>

    <h2>Creating Filters</h2>
    <ol>
        <li>Navigate to <strong>Filter</strong> tab on ribbon</li>
        <li>Click <strong>Add Filter</strong></li>
        <li>Choose filter type (Include/Exclude/Highlight)</li>
        <li>Enter pattern to match</li>
        <li>Select match type</li>
        <li>Click OK to apply</li>
    </ol>

    <h2>Quick Filtering</h2>
    <ul>
        <li><strong>Right-click message:</strong> Quick filter options</li>
        <li><strong>Select text:</strong> Filter selected text</li>
        <li><strong>Ctrl+I:</strong> Include filter from selection</li>
        <li><strong>Ctrl+E:</strong> Exclude filter from selection</li>
    </ul>

    <h2>Filter Priority</h2>
    <p>Filters are processed in order:</p>
    <ol>
        <li>Exclude filters (remove unwanted)</li>
        <li>Include filters (keep wanted)</li>
        <li>Highlight filters (colorize)</li>
        <li>Action filters (bookmark, beep, etc.)</li>
    </ol>

    <h2>Filter Examples</h2>

    <h3>Show only errors</h3>
    <pre>Type: Include
Pattern: error|exception|fail
Match: Regex (case insensitive)</pre>

    <h3>Hide verbose logging</h3>
    <pre>Type: Exclude
Pattern: TRACE|VERBOSE|DEBUG
Match: Regex</pre>

    <h3>Highlight SQL queries</h3>
    <pre>Type: Highlight
Pattern: SELECT|INSERT|UPDATE|DELETE
Match: Regex
Color: Blue</pre>

    <div class="note">
        <strong>Performance Tip:</strong> Use exclude filters first to reduce the message volume before applying complex include patterns.
    </div>

    <div class="topic-nav">
        <a href="debugview-agent.html" class="nav-prev">Previous: DebugView Agent</a>
        <a href="include-exclude.html" class="nav-next">Next: Include/Exclude Filters</a>
    </div>
</body>
</html>
'@

# Include/Exclude Filters
Write-HelpFile "include-exclude.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Include/Exclude Filters - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Include/Exclude Filters</h1>
    <p class="lead">Fine-tune message visibility with include and exclude filters.</p>

    <h2>Include Filters</h2>
    <p>Show only messages that match specified patterns:</p>
    <ul>
        <li>Focus on specific components</li>
        <li>Monitor particular error types</li>
        <li>Track specific processes</li>
        <li>Filter by message content</li>
    </ul>

    <h3>Include Filter Examples</h3>
    <pre><code>// Show only SQL queries
Pattern: "SELECT|INSERT|UPDATE|DELETE"
Type: Regex

// Show specific process
Pattern: "MyApp.exe"
Type: Simple

// Show error levels
Pattern: "ERROR|CRITICAL|FATAL"
Type: Regex</code></pre>

    <h2>Exclude Filters</h2>
    <p>Hide messages matching patterns:</p>
    <ul>
        <li>Remove noise and clutter</li>
        <li>Hide verbose logging</li>
        <li>Filter out known issues</li>
        <li>Suppress repetitive messages</li>
    </ul>

    <h3>Exclude Filter Examples</h3>
    <pre><code>// Hide trace logging
Pattern: "[TRACE]"
Type: Simple

// Hide heartbeat messages
Pattern: "keepalive|ping|heartbeat"
Type: Regex

// Hide specific DLLs
Pattern: "*.dll"
Type: Wildcard</code></pre>

    <h2>Combining Filters</h2>
    <p>Use both include and exclude for precise control:</p>
    <ol>
        <li>Exclude filters run first (remove unwanted)</li>
        <li>Include filters run second (keep wanted)</li>
        <li>Result: Only desired messages shown</li>
    </ol>

    <h3>Example Combination</h3>
    <pre><code>Exclude: "TRACE|DEBUG"  // Remove verbose
Include: "MyComponent"   // Show only my component
Result: MyComponent messages without TRACE/DEBUG</code></pre>

    <h2>Filter Management</h2>
    <ul>
        <li><strong>Enable/Disable:</strong> Toggle filters without deleting</li>
        <li><strong>Reorder:</strong> Drag filters to change priority</li>
        <li><strong>Edit:</strong> Double-click to modify</li>
        <li><strong>Groups:</strong> Organize filters into sets</li>
    </ul>

    <div class="topic-nav">
        <a href="filtering-basics.html" class="nav-prev">Previous: Filtering Basics</a>
        <a href="highlighting.html" class="nav-next">Next: Message Highlighting</a>
    </div>
</body>
</html>
'@

# Highlighting
Write-HelpFile "highlighting.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Message Highlighting - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Message Highlighting</h1>
    <p class="lead">Use colors and styles to make important messages stand out.</p>

    <h2>Highlight Features</h2>
    <ul>
        <li>16 predefined colors</li>
        <li>Custom RGB color selection</li>
        <li>Background and foreground colors</li>
        <li>Bold and italic text styles</li>
        <li>Multiple highlight rules</li>
    </ul>

    <h2>Creating Highlight Rules</h2>
    <ol>
        <li>Go to <strong>Filter</strong> tab</li>
        <li>Click <strong>Add Highlight</strong></li>
        <li>Enter pattern to match</li>
        <li>Choose colors and style</li>
        <li>Set priority (higher = more important)</li>
        <li>Click OK</li>
    </ol>

    <h2>Common Highlighting Patterns</h2>

    <h3>Error Levels</h3>
    <pre><code>ERROR    → Red background
WARNING  → Yellow background
INFO     → Blue text
DEBUG    → Gray text
SUCCESS  → Green background</code></pre>

    <h3>Component Highlighting</h3>
    <pre><code>[Database]  → Blue
[Network]   → Purple
[Security]  → Red
[UI]        → Green</code></pre>

    <h3>Performance Indicators</h3>
    <pre><code>SLOW|TIMEOUT      → Orange
FAST|OPTIMIZED    → Green
BLOCKED|WAITING   → Red</code></pre>

    <h2>Highlight Priority</h2>
    <p>When multiple rules match:</p>
    <ul>
        <li>Higher priority rules win</li>
        <li>Last matching rule applies</li>
        <li>Drag to reorder rules</li>
        <li>Number indicates priority</li>
    </ul>

    <h2>Tips for Effective Highlighting</h2>
    <ul>
        <li>Use contrasting colors for clarity</li>
        <li>Limit number of colors to avoid confusion</li>
        <li>Reserve red for critical issues</li>
        <li>Use subtle colors for common messages</li>
        <li>Test visibility in both light/dark themes</li>
    </ul>

    <div class="topic-nav">
        <a href="include-exclude.html" class="nav-prev">Previous: Include/Exclude</a>
        <a href="regex-patterns.html" class="nav-next">Next: Regular Expressions</a>
    </div>
</body>
</html>
'@

# Regex Patterns
Write-HelpFile "regex-patterns.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Regular Expression Patterns - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Regular Expression Patterns</h1>
    <p class="lead">Use powerful regex patterns for advanced message filtering and extraction.</p>

    <h2>Regex Support</h2>
    <p>UltimateDebugView++ uses standard regex with these match types:</p>
    <ul>
        <li><strong>Regex:</strong> Standard regular expression</li>
        <li><strong>RegexCase:</strong> Case-sensitive regex</li>
        <li><strong>RegexGroups:</strong> Capture groups for extraction</li>
    </ul>

    <h2>Common Patterns</h2>

    <h3>Error Detection</h3>
    <pre><code>// Any error keyword
\b(ERROR|EXCEPTION|FAIL|FATAL)\b

// Error with code
ERROR\s+(\d+):\s+(.*)

// Stack trace lines
^\s+at\s+[\w\.]+\(.*\)</code></pre>

    <h3>Log Level Extraction</h3>
    <pre><code>// Standard log levels
^\[(TRACE|DEBUG|INFO|WARN|ERROR|FATAL)\]

// Timestamp and level
^(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2})\s+\[(\w+)\]</code></pre>

    <h3>SQL Query Detection</h3>
    <pre><code>// SQL statements
\b(SELECT|INSERT|UPDATE|DELETE|CREATE|DROP)\b.*\bFROM\b

// Parameterized queries
@\w+|:\w+|\?\d*</code></pre>

    <h3>Performance Metrics</h3>
    <pre><code>// Execution time
took\s+(\d+(?:\.\d+)?)\s*(ms|seconds?)

// Memory usage
(\d+(?:\.\d+)?)\s*(KB|MB|GB)\s+used</code></pre>

    <h2>Capture Groups</h2>
    <p>Extract parts of messages using groups:</p>
    <pre><code>// Extract timestamp, level, and message
^(\d{2}:\d{2}:\d{2})\s+\[(\w+)\]\s+(.*)$
Group 1: Timestamp
Group 2: Level
Group 3: Message</code></pre>

    <h2>Advanced Patterns</h2>

    <h3>IP Address Matching</h3>
    <pre><code>// IPv4 address
\b(?:\d{1,3}\.){3}\d{1,3}\b

// IPv4 with port
\b(?:\d{1,3}\.){3}\d{1,3}:\d{1,5}\b</code></pre>

    <h3>File Path Detection</h3>
    <pre><code>// Windows paths
[A-Z]:\\[\w\\]+\.\w+

// Unix paths
\/[\w\/]+\.\w+</code></pre>

    <h3>JSON Detection</h3>
    <pre><code>// JSON object
\{[^{}]*\}

// JSON array
\[[^\[\]]*\]</code></pre>

    <h2>Regex Tips</h2>
    <ul>
        <li>Use <code>\b</code> for word boundaries</li>
        <li>Make patterns specific to avoid false matches</li>
        <li>Test patterns with sample data first</li>
        <li>Use non-greedy matches (<code>.*?</code>) when appropriate</li>
        <li>Consider performance with complex patterns</li>
    </ul>

    <h2>Testing Regex</h2>
    <ol>
        <li>Open Filter dialog</li>
        <li>Select "Regex" match type</li>
        <li>Enter pattern</li>
        <li>Click "Test" to preview matches</li>
        <li>Adjust pattern as needed</li>
    </ol>

    <div class="note">
        <strong>Performance:</strong> Complex regex patterns can impact performance with high message volumes. Use simpler patterns when possible.
    </div>

    <div class="topic-nav">
        <a href="highlighting.html" class="nav-prev">Previous: Highlighting</a>
        <a href="saved-filters.html" class="nav-next">Next: Saved Filters</a>
    </div>
</body>
</html>
'@

# Saved Filters
Write-HelpFile "saved-filters.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Saved Filters - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Saved Filters</h1>
    <p class="lead">Save and manage filter configurations for different debugging scenarios.</p>

    <h2>Saving Filter Sets</h2>
    <ol>
        <li>Configure your filters</li>
        <li>Click <strong>Filter → Save Filter Set</strong></li>
        <li>Enter a descriptive name</li>
        <li>Optional: Add description</li>
        <li>Click Save</li>
    </ol>

    <h2>Loading Filter Sets</h2>
    <ol>
        <li>Click <strong>Filter → Load Filter Set</strong></li>
        <li>Select from saved sets</li>
        <li>Choose load option:
            <ul>
                <li><strong>Replace:</strong> Clear existing filters</li>
                <li><strong>Append:</strong> Add to existing filters</li>
            </ul>
        </li>
        <li>Click Load</li>
    </ol>

    <h2>Managing Filter Sets</h2>
    <ul>
        <li><strong>Edit:</strong> Modify saved filter sets</li>
        <li><strong>Delete:</strong> Remove unused sets</li>
        <li><strong>Export:</strong> Share with team members</li>
        <li><strong>Import:</strong> Load shared filter sets</li>
        <li><strong>Set as Default:</strong> Auto-load on startup</li>
    </ul>

    <h2>Common Filter Set Examples</h2>

    <h3>Production Monitoring</h3>
    <ul>
        <li>Include: ERROR|CRITICAL|FATAL</li>
        <li>Exclude: DEBUG|TRACE</li>
        <li>Highlight: CRITICAL (Red)</li>
        <li>Beep: FATAL</li>
    </ul>

    <h3>Development Debugging</h3>
    <ul>
        <li>Include: MyComponent</li>
        <li>Highlight: SQL (Blue)</li>
        <li>Highlight: ERROR (Red)</li>
        <li>Bookmark: Exception</li>
    </ul>

    <h3>Performance Analysis</h3>
    <ul>
        <li>Include: PERF|TIMING|SLOW</li>
        <li>Exclude: INFO|DEBUG</li>
        <li>Highlight: >1000ms (Orange)</li>
        <li>Track: Memory usage</li>
    </ul>

    <h2>Filter Set Storage</h2>
    <p>Filter sets are stored in:</p>
    <ul>
        <li>Windows Registry (per-user)</li>
        <li>Export to XML files</li>
        <li>Shareable configuration files</li>
    </ul>

    <div class="topic-nav">
        <a href="regex-patterns.html" class="nav-prev">Previous: Regular Expressions</a>
        <a href="sql-formatting.html" class="nav-next">Next: SQL Formatting</a>
    </div>
</body>
</html>
'@

# SQL Formatting
Write-HelpFile "sql-formatting.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SQL Formatting - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>SQL Formatting</h1>
    <p class="lead">Automatic SQL query beautification powered by the fsqlf formatter.</p>

    <h2>Features</h2>
    <ul>
        <li>Automatic SQL detection in messages</li>
        <li>Real-time query formatting</li>
        <li>Syntax highlighting</li>
        <li>Multiple formatting styles</li>
        <li>Customizable formatting options</li>
    </ul>

    <h2>Automatic Detection</h2>
    <p>SQL queries are detected when messages contain:</p>
    <ul>
        <li>SQL keywords (SELECT, INSERT, UPDATE, DELETE)</li>
        <li>[SQL] prefix in message</li>
        <li>Common SQL patterns</li>
        <li>Database error messages with queries</li>
    </ul>

    <h2>Formatting Options</h2>

    <h3>Keyword Case</h3>
    <ul>
        <li><strong>Upper:</strong> SELECT, FROM, WHERE</li>
        <li><strong>Lower:</strong> select, from, where</li>
        <li><strong>Capitalize:</strong> Select, From, Where</li>
    </ul>

    <h3>Indentation</h3>
    <ul>
        <li><strong>Type:</strong> Spaces or Tabs</li>
        <li><strong>Size:</strong> 2, 4, or 8 spaces</li>
        <li><strong>Nested:</strong> Indent subqueries</li>
    </ul>

    <h3>Line Breaks</h3>
    <ul>
        <li>Before/after keywords</li>
        <li>Comma positioning</li>
        <li>Parentheses style</li>
        <li>Join clause formatting</li>
    </ul>

    <h2>Formatting Examples</h2>

    <h3>Compact Style</h3>
    <pre><code>SELECT id, name FROM users WHERE active = 1</code></pre>

    <h3>Expanded Style</h3>
    <pre><code>SELECT
    id,
    name
FROM
    users
WHERE
    active = 1</code></pre>

    <h3>Standard Style</h3>
    <pre><code>SELECT id, name
FROM users
WHERE active = 1</code></pre>

    <h2>Using SQL Formatting</h2>

    <h3>Automatic Formatting</h3>
    <ol>
        <li>SQL queries are auto-detected</li>
        <li>Formatted in detail pane</li>
        <li>Syntax highlighting applied</li>
    </ol>

    <h3>Manual Formatting</h3>
    <ol>
        <li>Select message with SQL</li>
        <li>Press F4 for detail pane</li>
        <li>Click <strong>Format SQL</strong> button</li>
        <li>Copy formatted result</li>
    </ol>

    <h2>Configuration</h2>
    <p>Configure SQL formatting in Settings:</p>
    <ol>
        <li>Go to <strong>Tools → Settings</strong></li>
        <li>Select <strong>SQL Formatting</strong> tab</li>
        <li>Adjust formatting options</li>
        <li>Preview with sample SQL</li>
        <li>Click Apply</li>
    </ol>

    <h2>Supported SQL Features</h2>
    <ul>
        <li>Standard SQL (SELECT, INSERT, UPDATE, DELETE)</li>
        <li>DDL statements (CREATE, ALTER, DROP)</li>
        <li>Joins (INNER, LEFT, RIGHT, FULL)</li>
        <li>Subqueries and CTEs</li>
        <li>Functions and expressions</li>
        <li>Comments (-- and /* */)</li>
    </ul>

    <div class="note">
        <strong>Tip:</strong> Use the [SQL] prefix in your debug messages to ensure automatic SQL detection and formatting.
    </div>

    <div class="topic-nav">
        <a href="saved-filters.html" class="nav-prev">Previous: Saved Filters</a>
        <a href="sql-detection.html" class="nav-next">Next: SQL Detection</a>
    </div>
</body>
</html>
'@

# SQL Detection
Write-HelpFile "sql-detection.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Automatic SQL Detection - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Automatic SQL Detection</h1>
    <p class="lead">How UltimateDebugView++ identifies and processes SQL statements in debug messages.</p>

    <h2>Detection Methods</h2>

    <h3>1. Keyword Detection</h3>
    <p>Recognizes SQL keywords at message start:</p>
    <ul>
        <li>SELECT, INSERT, UPDATE, DELETE</li>
        <li>CREATE, ALTER, DROP</li>
        <li>EXEC, EXECUTE, CALL</li>
        <li>WITH (CTE queries)</li>
    </ul>

    <h3>2. Prefix Detection</h3>
    <p>Messages with SQL indicators:</p>
    <ul>
        <li>[SQL] prefix</li>
        <li>[QUERY] prefix</li>
        <li>SQL: at start</li>
        <li>Query: at start</li>
    </ul>

    <h3>3. Pattern Recognition</h3>
    <p>Common SQL patterns:</p>
    <ul>
        <li>FROM table_name</li>
        <li>WHERE conditions</li>
        <li>JOIN clauses</li>
        <li>Parameter markers (?, @param, :param)</li>
    </ul>

    <h2>Improving Detection</h2>

    <h3>Best Practices</h3>
    <ul>
        <li>Use [SQL] prefix for guaranteed detection</li>
        <li>Start messages with SQL keywords</li>
        <li>Include complete SQL statements</li>
        <li>Avoid mixing SQL with other text</li>
    </ul>

    <h3>Example Messages</h3>
    <pre><code>// Good - Will be detected
"[SQL] SELECT * FROM users"
"SELECT id, name FROM customers WHERE active = 1"
"SQL: INSERT INTO logs (message) VALUES (?)"

// Poor - May not be detected
"Running query: select data"
"The SELECT statement failed"
"Error in: SELECT * FROM..."</code></pre>

    <h2>Detection Settings</h2>
    <p>Configure detection sensitivity:</p>
    <ul>
        <li><strong>Strict:</strong> Only explicit SQL markers</li>
        <li><strong>Normal:</strong> Keywords and patterns</li>
        <li><strong>Aggressive:</strong> Any SQL-like content</li>
    </ul>

    <h2>Custom Detection Rules</h2>
    <p>Add custom patterns:</p>
    <ol>
        <li>Settings → SQL Detection</li>
        <li>Add custom prefix or pattern</li>
        <li>Test with sample messages</li>
        <li>Apply to enable</li>
    </ol>

    <div class="topic-nav">
        <a href="sql-formatting.html" class="nav-prev">Previous: SQL Formatting</a>
        <a href="sql-options.html" class="nav-next">Next: Formatting Options</a>
    </div>
</body>
</html>
'@

# SQL Options
Write-HelpFile "sql-options.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SQL Formatting Options - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>SQL Formatting Options</h1>
    <p class="lead">Comprehensive SQL formatting preferences powered by fsqlf.</p>

    <h2>Available Options</h2>

    <h3>Keyword Formatting</h3>
    <ul>
        <li><strong>Case:</strong> Upper, Lower, Capitalize</li>
        <li><strong>Line breaks:</strong> Before/After keywords</li>
        <li><strong>Alignment:</strong> Left, Right</li>
    </ul>

    <h3>Indentation Settings</h3>
    <ul>
        <li><strong>Type:</strong> Spaces or Tabs</li>
        <li><strong>Size:</strong> 2, 4, 8 characters</li>
        <li><strong>Nested queries:</strong> Additional indent</li>
        <li><strong>Join clauses:</strong> Indent level</li>
    </ul>

    <h3>Comma Positioning</h3>
    <ul>
        <li><strong>Trailing:</strong> item1, item2, item3</li>
        <li><strong>Leading:</strong> item1 ,item2 ,item3</li>
        <li><strong>Aligned:</strong> Column alignment</li>
    </ul>

    <h3>Parentheses Style</h3>
    <ul>
        <li><strong>Same line:</strong> WHERE (condition)</li>
        <li><strong>New line:</strong> WHERE<br>    (condition)</li>
        <li><strong>Indent content:</strong> Internal indentation</li>
    </ul>

    <h3>Function Formatting</h3>
    <ul>
        <li><strong>Compact:</strong> COUNT(*)</li>
        <li><strong>Expanded:</strong> COUNT( * )</li>
        <li><strong>Multi-line:</strong> Complex functions</li>
    </ul>

    <h2>Style Presets</h2>

    <h3>Compact</h3>
    <pre><code>SELECT id, name FROM users WHERE active = 1 ORDER BY name</code></pre>

    <h3>Standard</h3>
    <pre><code>SELECT id, name
FROM users
WHERE active = 1
ORDER BY name</code></pre>

    <h3>Expanded</h3>
    <pre><code>SELECT
    id,
    name
FROM
    users
WHERE
    active = 1
ORDER BY
    name</code></pre>

    <h2>Configuring Options</h2>
    <ol>
        <li>Tools → Settings → SQL Formatting</li>
        <li>Choose preset or customize</li>
        <li>Preview with sample SQL</li>
        <li>Save as custom preset</li>
        <li>Apply to all SQL formatting</li>
    </ol>

    <h2>Advanced Settings</h2>
    <ul>
        <li><strong>Alias alignment:</strong> Table and column aliases</li>
        <li><strong>Comment preservation:</strong> Keep SQL comments</li>
        <li><strong>Case preservation:</strong> Maintain identifier case</li>
        <li><strong>Max line length:</strong> Auto-wrap long lines</li>
    </ul>

    <div class="topic-nav">
        <a href="sql-detection.html" class="nav-prev">Previous: SQL Detection</a>
        <a href="clarion-overview.html" class="nav-next">Next: Clarion Overview</a>
    </div>
</body>
</html>
'@

# Continue with remaining files...
Write-Host "Creating Clarion Integration files..."

# Clarion Overview
Write-HelpFile "clarion-overview.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Clarion Integration Overview - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Clarion Integration Overview</h1>
    <p class="lead">Enhanced debugging support specifically designed for Clarion developers.</p>

    <h2>Clarion-Specific Features</h2>
    <ul>
        <li><strong>UltimateDebug Class:</strong> Comprehensive debugging API</li>
        <li><strong>Procedure Tracking:</strong> Call stack visualization</li>
        <li><strong>Queue/Group Display:</strong> Formatted data structures</li>
        <li><strong>Event Monitoring:</strong> Windows event tracking</li>
        <li><strong>SQL Tracing:</strong> Database query logging</li>
        <li><strong>Thread Management:</strong> Multi-thread debugging</li>
    </ul>

    <h2>Message Format</h2>
    <p>Clarion messages use a structured format:</p>
    <pre><code>[||]X,T:&lt;thread&gt;,P:&lt;procedure&gt;,M:&lt;module&gt;,A:&lt;app&gt;,PRE:&lt;prefix&gt;,MSG:&lt;message&gt;</code></pre>

    <h3>Format Components</h3>
    <ul>
        <li><strong>[||]:</strong> Clarion message marker</li>
        <li><strong>X:</strong> Message type indicator</li>
        <li><strong>T:</strong> Thread ID</li>
        <li><strong>P:</strong> Current procedure</li>
        <li><strong>M:</strong> Module name</li>
        <li><strong>A:</strong> Application name</li>
        <li><strong>PRE:</strong> Message prefix/category</li>
        <li><strong>MSG:</strong> Actual message content</li>
    </ul>

    <h2>Special Prefixes</h2>
    <p>UltimateDebugView++ recognizes Clarion prefixes:</p>
    <ul>
        <li><strong>[Open]:</strong> Procedure entry</li>
        <li><strong>[Close]:</strong> Procedure exit</li>
        <li><strong>[Queue]:</strong> Queue data display</li>
        <li><strong>[Group]:</strong> Group data display</li>
        <li><strong>[Events]:</strong> Windows events</li>
        <li><strong>[Assert]:</strong> Assertion messages</li>
        <li><strong>[ERROR]:</strong> Error conditions</li>
        <li><strong>[SQL]:</strong> SQL queries</li>
        <li><strong>[Trace]:</strong> Execution trace</li>
    </ul>

    <h2>Procedure Tracking</h2>
    <p>Advanced call stack visualization:</p>
    <ul>
        <li>Automatic procedure entry/exit detection</li>
        <li>Call hierarchy display</li>
        <li>Execution timing statistics</li>
        <li>Thread-aware tracking</li>
        <li>Performance profiling</li>
    </ul>

    <h2>Getting Started</h2>
    <ol>
        <li>Include UltimateDebug class in your application</li>
        <li>Initialize in program setup:
            <pre><code>ud.Init()
ud.SetApplicationName(AppName)</code></pre>
        </li>
        <li>Add procedure tracking:
            <pre><code>ud.Init(ProcedureName)
! Your code here
ud.EndOfProcedure()</code></pre>
        </li>
        <li>Use debug methods throughout code</li>
    </ol>

    <h2>Benefits for Clarion Developers</h2>
    <ul>
        <li>Rich data structure visualization</li>
        <li>SQL query formatting and analysis</li>
        <li>Multi-threaded application debugging</li>
        <li>Performance profiling capabilities</li>
        <li>Event-driven programming support</li>
        <li>Integration with Clarion IDE</li>
    </ul>

    <div class="note">
        <strong>Note:</strong> UltimateDebugView++ was originally created specifically for Clarion development and includes many Clarion-optimized features.
    </div>

    <div class="topic-nav">
        <a href="sql-options.html" class="nav-prev">Previous: SQL Options</a>
        <a href="clarion-api.html" class="nav-next">Next: UltimateDebug Class</a>
    </div>
</body>
</html>
'@

# Clarion API
Write-HelpFile "clarion-api.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>UltimateDebug Class API - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>UltimateDebug Class API</h1>
    <p class="lead">Complete API reference for the Clarion UltimateDebug class.</p>

    <h2>Core Methods</h2>

    <h3>Debug(string Message)</h3>
    <p>Output a debug message:</p>
    <pre><code>ud.Debug(''Customer ID: '' & CUS:ID)</code></pre>

    <h3>Init(string ProcedureName)</h3>
    <p>Initialize procedure tracking:</p>
    <pre><code>ud.Init(''UpdateCustomer'')
! Procedure code
ud.EndOfProcedure()</code></pre>

    <h3>EndOfProcedure()</h3>
    <p>Mark procedure exit (pairs with Init).</p>

    <h2>Data Structure Methods</h2>

    <h3>DebugQueue(*QUEUE Q)</h3>
    <p>Display queue contents:</p>
    <pre><code>ud.DebugQueue(CustomerQueue)</code></pre>

    <h3>DebugGroup(*GROUP G)</h3>
    <p>Display group structure:</p>
    <pre><code>ud.DebugGroup(CustomerRecord)</code></pre>

    <h3>DebugRecord()</h3>
    <p>Display current file record:</p>
    <pre><code>GET(Customer, CUS:ID)
ud.DebugRecord()</code></pre>

    <h2>Event Methods</h2>

    <h3>DebugEvent()</h3>
    <p>Log Windows event:</p>
    <pre><code>CASE EVENT()
OF EVENT:Accepted
    ud.DebugEvent()
END</code></pre>

    <h3>SetEventsToDebug(string Events)</h3>
    <p>Filter events to log:</p>
    <pre><code>ud.SetEventsToDebug(''EVENT:Accepted|EVENT:Selected'')</code></pre>

    <h2>Configuration Methods</h2>

    <h3>SetPrefix(string Prefix)</h3>
    <p>Set message prefix:</p>
    <pre><code>ud.SetPrefix(''[MODULE]'')</code></pre>

    <h3>SetApplicationName(string Name)</h3>
    <p>Set application identifier:</p>
    <pre><code>ud.SetApplicationName(''CustomerManager'')</code></pre>

    <h3>SetModuleName(string Name)</h3>
    <p>Set module identifier:</p>
    <pre><code>ud.SetModuleName(''Billing'')</code></pre>

    <h2>SQL Methods</h2>

    <h3>SQL(string Query)</h3>
    <p>Log formatted SQL:</p>
    <pre><code>ud.SQL(''SELECT * FROM Customers WHERE Active = 1'')</code></pre>

    <h3>EnableSQLTrace()</h3>
    <p>Enable automatic SQL logging.</p>

    <h2>Utility Methods</h2>

    <h3>ClearDebugView()</h3>
    <p>Clear debug output window:</p>
    <pre><code>ud.ClearDebugView()</code></pre>

    <h3>Error(string Message)</h3>
    <p>Log error with [ERROR] prefix:</p>
    <pre><code>ud.Error(''Failed to save: '' & ERROR())</code></pre>

    <h3>Assert(BYTE Condition, string Message)</h3>
    <p>Conditional assertion:</p>
    <pre><code>ud.Assert(CUS:ID > 0, ''Invalid customer ID'')</code></pre>

    <h2>Properties</h2>

    <h3>Enabled</h3>
    <p>Enable/disable all output:</p>
    <pre><code>ud.Enabled = TRUE  ! Enable debugging
ud.Enabled = FALSE ! Disable for production</code></pre>

    <h3>ShowThreadID</h3>
    <p>Include thread ID in messages:</p>
    <pre><code>ud.ShowThreadID = TRUE</code></pre>

    <h3>ShowTimestamp</h3>
    <p>Include timestamps:</p>
    <pre><code>ud.ShowTimestamp = TRUE</code></pre>

    <h2>Advanced Features</h2>

    <h3>Conditional Debugging</h3>
    <pre><code>ud.AddCategoryToDebug(''CustomerUpdate'')
ud.DebugIf(''CustomerUpdate'', ''Updating ID: '' & CUS:ID)</code></pre>

    <h3>Performance Timing</h3>
    <pre><code>ud.StartTimer(''QueryExecution'')
! Execute query
ud.StopTimer(''QueryExecution'')</code></pre>

    <div class="topic-nav">
        <a href="clarion-overview.html" class="nav-prev">Previous: Clarion Overview</a>
        <a href="clarion-messages.html" class="nav-next">Next: Message Format</a>
    </div>
</body>
</html>
'@

# Continue with remaining files
Write-Host "Creating remaining help files..."

# Clarion Messages
Write-HelpFile "clarion-messages.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Clarion Message Format - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Clarion Message Format</h1>
    <p class="lead">Understanding the structured format of Clarion debug messages.</p>

    <h2>Message Structure</h2>
    <pre><code>[||]X,T:123,P:UpdateCustomer,M:Customer,A:MyApp,PRE:[Info],MSG:Processing record</code></pre>

    <h3>Components Explained</h3>
    <ul>
        <li><strong>[||]:</strong> Clarion message identifier</li>
        <li><strong>X:</strong> Message type (O=Open, C=Close, M=Message)</li>
        <li><strong>T:123:</strong> Thread ID</li>
        <li><strong>P:UpdateCustomer:</strong> Current procedure</li>
        <li><strong>M:Customer:</strong> Module name</li>
        <li><strong>A:MyApp:</strong> Application name</li>
        <li><strong>PRE:[Info]:</strong> Message prefix/category</li>
        <li><strong>MSG:Processing record:</strong> Actual message</li>
    </ul>

    <h2>Message Types</h2>
    <ul>
        <li><strong>O (Open):</strong> Procedure entry</li>
        <li><strong>C (Close):</strong> Procedure exit</li>
        <li><strong>M (Message):</strong> Regular debug message</li>
        <li><strong>E (Event):</strong> Windows event</li>
        <li><strong>Q (Queue):</strong> Queue data</li>
        <li><strong>G (Group):</strong> Group data</li>
        <li><strong>S (SQL):</strong> SQL query</li>
    </ul>

    <h2>Parsing in UltimateDebugView++</h2>
    <p>Messages are automatically parsed to extract:</p>
    <ul>
        <li>Thread information for filtering</li>
        <li>Procedure names for call stack</li>
        <li>Module/Application for categorization</li>
        <li>Prefixes for highlighting</li>
        <li>Content for formatting</li>
    </ul>

    <h2>Custom Message Formatting</h2>
    <p>Create custom formatted messages:</p>
    <pre><code>ud.SetFormat(''[||]X,T:{{thread}},APP:{{app}},MSG:{{message}}'')</code></pre>

    <div class="topic-nav">
        <a href="clarion-api.html" class="nav-prev">Previous: UltimateDebug API</a>
        <a href="clarion-examples.html" class="nav-next">Next: Code Examples</a>
    </div>
</body>
</html>
'@

# Clarion Examples
Write-HelpFile "clarion-examples.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Clarion Code Examples - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Clarion Code Examples</h1>
    <p class="lead">Practical examples of using UltimateDebug in Clarion applications.</p>

    <h2>Basic Setup</h2>
    <pre><code>! Global data section
INCLUDE(''UltimateDebug.inc''),ONCE
ud  UltimateDebug

! Program initialization
ud.Init()
ud.SetApplicationName(''CustomerManager'')
ud.Enabled = TRUE</code></pre>

    <h2>Procedure Tracking</h2>
    <pre><code>UpdateCustomer PROCEDURE
CODE
    ud.Init(''UpdateCustomer'')
    ud.Debug(''Customer ID: '' & CUS:ID)

    ! Update logic here

    ud.EndOfProcedure()
    RETURN</code></pre>

    <h2>Error Handling</h2>
    <pre><code>IF ERRORCODE()
    ud.Error(''Database error: '' & ERROR())
    ud.Debug(''Failed on Customer: '' & CUS:ID)
    MESSAGE(ERROR())
END</code></pre>

    <h2>Queue Debugging</h2>
    <pre><code>! Display queue contents
LOOP I# = 1 TO RECORDS(MyQueue)
    GET(MyQueue, I#)
    ud.Debug(''Queue['' & I# & '']: '' & MyQueue.Field)
END

! Or use built-in method
ud.DebugQueue(MyQueue)</code></pre>

    <h2>SQL Logging</h2>
    <pre><code>! Log SQL queries
SQLString = ''SELECT * FROM Customers WHERE Active = 1''
ud.SQL(SQLString)
PROP:SQL = SQLString

! Enable automatic SQL trace
ud.EnableSQLTrace()</code></pre>

    <h2>Event Debugging</h2>
    <pre><code>! In window procedure
OF EVENT:Accepted
    ud.DebugEvent()
    CASE FIELD()
    OF ?OK
        ud.Debug(''OK button clicked'')
    OF ?Cancel
        ud.Debug(''Cancel button clicked'')
    END</code></pre>

    <h2>Conditional Debugging</h2>
    <pre><code>! Debug only specific operations
IF DEBUG:DetailedLogging
    ud.AddCategoryToDebug(''DatabaseOperations'')
END

! Later in code
ud.DebugIf(''DatabaseOperations'', ''Executing query: '' & SQLString)</code></pre>

    <h2>Performance Profiling</h2>
    <pre><code>! Time critical operations
ud.StartTimer(''DatabaseQuery'')
! ... database operation ...
ud.StopTimer(''DatabaseQuery'')

! Manual timing
StartTime = CLOCK()
! ... operation ...
ud.Debug(''Operation took: '' & (CLOCK() - StartTime) & '' ms'')</code></pre>

    <div class="topic-nav">
        <a href="clarion-messages.html" class="nav-prev">Previous: Message Format</a>
        <a href="syntax-highlighting.html" class="nav-next">Next: Syntax Highlighting</a>
    </div>
</body>
</html>
'@

# Advanced Features
Write-HelpFile "syntax-highlighting.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Syntax Highlighting - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Syntax Highlighting</h1>
    <p class="lead">Automatic syntax highlighting powered by Scintilla and Lexilla.</p>

    <h2>Supported Languages</h2>
    <ul>
        <li><strong>SQL:</strong> Keywords, functions, strings</li>
        <li><strong>JSON:</strong> Keys, values, structures</li>
        <li><strong>XML/HTML:</strong> Tags, attributes, content</li>
        <li><strong>C/C++:</strong> Keywords, types, preprocessor</li>
        <li><strong>C#:</strong> .NET keywords, LINQ</li>
        <li><strong>JavaScript:</strong> ES6+ syntax</li>
        <li><strong>Python:</strong> Keywords, decorators</li>
        <li><strong>Clarion:</strong> Specialized Clarion syntax</li>
        <li><strong>Log Files:</strong> Levels, timestamps</li>
    </ul>

    <h2>Automatic Detection</h2>
    <p>Content type is detected by:</p>
    <ul>
        <li>Message prefixes ([SQL], [JSON], etc.)</li>
        <li>Content patterns (SELECT, {, <, etc.)</li>
        <li>File extensions in paths</li>
        <li>Explicit type markers</li>
    </ul>

    <h2>Color Themes</h2>
    <ul>
        <li><strong>Light Theme:</strong> Optimized for bright displays</li>
        <li><strong>Dark Theme:</strong> Reduced eye strain</li>
        <li><strong>Custom:</strong> Define your own colors</li>
    </ul>

    <h2>Customization</h2>
    <p>Configure highlighting in Settings:</p>
    <ol>
        <li>Tools → Settings → Syntax Colors</li>
        <li>Select language</li>
        <li>Customize colors for each element</li>
        <li>Preview changes</li>
        <li>Save theme</li>
    </ol>

    <div class="topic-nav">
        <a href="clarion-examples.html" class="nav-prev">Previous: Clarion Examples</a>
        <a href="bookmarks.html" class="nav-next">Next: Bookmarks</a>
    </div>
</body>
</html>
'@

# Bookmarks
Write-HelpFile "bookmarks.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Bookmarks - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Bookmarks</h1>
    <p class="lead">Mark important messages for quick navigation.</p>

    <h2>Creating Bookmarks</h2>
    <ul>
        <li><strong>Manual:</strong> Ctrl+F2 on selected message</li>
        <li><strong>Right-click:</strong> Toggle Bookmark</li>
        <li><strong>Auto-bookmark:</strong> Filter-based bookmarking</li>
    </ul>

    <h2>Navigation</h2>
    <ul>
        <li><strong>Next bookmark:</strong> F2</li>
        <li><strong>Previous bookmark:</strong> Shift+F2</li>
        <li><strong>Bookmark list:</strong> Ctrl+Shift+B</li>
        <li><strong>Clear all:</strong> Ctrl+Shift+F2</li>
    </ul>

    <h2>Auto-Bookmarking</h2>
    <p>Create filters that automatically bookmark:</p>
    <ol>
        <li>Add filter with "Bookmark" action</li>
        <li>Define pattern (e.g., "ERROR")</li>
        <li>Messages automatically bookmarked</li>
    </ol>

    <h2>Bookmark Management</h2>
    <ul>
        <li>Export bookmarks to file</li>
        <li>Add notes to bookmarks</li>
        <li>Group related bookmarks</li>
        <li>Search within bookmarks</li>
    </ul>

    <div class="topic-nav">
        <a href="syntax-highlighting.html" class="nav-prev">Previous: Syntax Highlighting</a>
        <a href="export-options.html" class="nav-next">Next: Export Options</a>
    </div>
</body>
</html>
'@

# Export Options
Write-HelpFile "export-options.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Export Options - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Export Options</h1>
    <p class="lead">Save debug messages in various formats for analysis and sharing.</p>

    <h2>Export Formats</h2>
    <ul>
        <li><strong>Text (.txt):</strong> Plain text with formatting</li>
        <li><strong>CSV (.csv):</strong> Comma-separated for Excel</li>
        <li><strong>XML (.xml):</strong> Structured data format</li>
        <li><strong>JSON (.json):</strong> JavaScript object notation</li>
        <li><strong>HTML (.html):</strong> Formatted with highlighting</li>
        <li><strong>RTF (.rtf):</strong> Rich text with colors</li>
    </ul>

    <h2>Export Methods</h2>

    <h3>Export All Messages</h3>
    <ol>
        <li>File → Export → All Messages</li>
        <li>Choose format</li>
        <li>Select destination</li>
        <li>Configure options</li>
        <li>Click Export</li>
    </ol>

    <h3>Export Selection</h3>
    <ol>
        <li>Select messages</li>
        <li>Right-click → Export Selection</li>
        <li>Choose format and save</li>
    </ol>

    <h3>Export Filtered View</h3>
    <ol>
        <li>Apply filters</li>
        <li>File → Export → Visible Messages</li>
        <li>Exports only filtered results</li>
    </ol>

    <h2>Export Options</h2>
    <ul>
        <li><strong>Include timestamps:</strong> Add time information</li>
        <li><strong>Include process info:</strong> Add process details</li>
        <li><strong>Preserve colors:</strong> Keep highlighting (HTML/RTF)</li>
        <li><strong>Line numbers:</strong> Add line references</li>
        <li><strong>Date range:</strong> Export specific time period</li>
    </ul>

    <h2>Automation</h2>
    <ul>
        <li>Auto-export on buffer full</li>
        <li>Scheduled exports</li>
        <li>Command-line export</li>
        <li>Rolling file exports</li>
    </ul>

    <div class="topic-nav">
        <a href="bookmarks.html" class="nav-prev">Previous: Bookmarks</a>
        <a href="performance.html" class="nav-next">Next: Performance Tips</a>
    </div>
</body>
</html>
'@

# Performance
Write-HelpFile "performance.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Performance Tips - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Performance Tips</h1>
    <p class="lead">Optimize UltimateDebugView++ for high-volume debugging scenarios.</p>

    <h2>High-Volume Handling</h2>
    <p>Capable of processing 50,000+ messages per second with:</p>
    <ul>
        <li>Lock-free circular buffers</li>
        <li>Multi-threaded architecture</li>
        <li>Efficient memory management</li>
        <li>Optimized rendering</li>
    </ul>

    <h2>Performance Settings</h2>

    <h3>Buffer Management</h3>
    <ul>
        <li><strong>Buffer size:</strong> Increase for high volume (default: 100,000)</li>
        <li><strong>Circular buffer:</strong> Auto-discard old messages</li>
        <li><strong>Memory limit:</strong> Set maximum memory usage</li>
    </ul>

    <h3>Display Optimization</h3>
    <ul>
        <li><strong>Virtual scrolling:</strong> Only render visible messages</li>
        <li><strong>Update frequency:</strong> Batch UI updates</li>
        <li><strong>Disable auto-scroll:</strong> When reviewing messages</li>
        <li><strong>Limit visible columns:</strong> Hide unnecessary data</li>
    </ul>

    <h2>Filter Performance</h2>
    <ul>
        <li>Use exclude filters first</li>
        <li>Prefer simple over regex when possible</li>
        <li>Limit active filter count</li>
        <li>Use process filters for focus</li>
    </ul>

    <h2>Best Practices</h2>
    <ul>
        <li>Close unused tabs</li>
        <li>Disable syntax highlighting for very long messages</li>
        <li>Use time-based filtering</li>
        <li>Archive old messages regularly</li>
        <li>Monitor memory usage in status bar</li>
    </ul>

    <h2>Troubleshooting Slowdowns</h2>
    <ul>
        <li>Check filter complexity</li>
        <li>Reduce buffer size if memory constrained</li>
        <li>Disable auto-formatting for SQL</li>
        <li>Close detail pane when not needed</li>
        <li>Restart to clear memory</li>
    </ul>

    <div class="topic-nav">
        <a href="export-options.html" class="nav-prev">Previous: Export Options</a>
        <a href="remote-debugging.html" class="nav-next">Next: Remote Debugging</a>
    </div>
</body>
</html>
'@

# Remote Debugging
Write-HelpFile "remote-debugging.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Remote Debugging - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Remote Debugging</h1>
    <p class="lead">Debug applications on remote machines using network connections.</p>

    <h2>Remote Debugging Methods</h2>
    <ul>
        <li><strong>DebugView Agent:</strong> TCP connection to remote agent</li>
        <li><strong>UDP Broadcasting:</strong> Network-wide message collection</li>
        <li><strong>File Sharing:</strong> Monitor remote log files</li>
        <li><strong>Custom Protocols:</strong> Application-specific remote debugging</li>
    </ul>

    <h2>Setup Remote Debugging</h2>

    <h3>Using DebugView Agent</h3>
    <ol>
        <li>Install agent on remote machine</li>
        <li>Configure firewall (port 2020)</li>
        <li>Start agent service</li>
        <li>Connect from UltimateDebugView++</li>
    </ol>

    <h3>Using UDP</h3>
    <ol>
        <li>Configure application to send UDP</li>
        <li>Set up UDP listener in UltimateDebugView++</li>
        <li>Ensure network connectivity</li>
        <li>Start capturing</li>
    </ol>

    <h2>Security Considerations</h2>
    <ul>
        <li>Use VPN for internet connections</li>
        <li>Implement authentication</li>
        <li>Encrypt sensitive data</li>
        <li>Restrict access by IP</li>
        <li>Use dedicated debugging ports</li>
    </ul>

    <h2>Network Configuration</h2>
    <ul>
        <li>Open firewall ports</li>
        <li>Configure router forwarding</li>
        <li>Set up static IPs for servers</li>
        <li>Test connectivity first</li>
    </ul>

    <div class="topic-nav">
        <a href="performance.html" class="nav-prev">Previous: Performance Tips</a>
        <a href="keyboard-shortcuts.html" class="nav-next">Next: Keyboard Shortcuts</a>
    </div>
</body>
</html>
'@

# Command Line
Write-HelpFile "command-line.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Command Line Options - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Command Line Options</h1>
    <p class="lead">Launch UltimateDebugView++ with specific configurations.</p>

    <h2>Basic Usage</h2>
    <pre><code>UltimateDebugviewpp.exe [options] [filename]</code></pre>

    <h2>Available Options</h2>
    <ul>
        <li><strong>/min:</strong> Start minimized</li>
        <li><strong>/max:</strong> Start maximized</li>
        <li><strong>/config:file:</strong> Load configuration</li>
        <li><strong>/filter:file:</strong> Load filter set</li>
        <li><strong>/log:file:</strong> Start logging to file</li>
        <li><strong>/udp:port:</strong> Start UDP listener</li>
        <li><strong>/tcp:host:port:</strong> Connect to TCP agent</li>
    </ul>

    <h2>File Loading</h2>
    <p>Load saved debug logs:</p>
    <pre><code>UltimateDebugviewpp.exe "C:\Logs\debug.log"</code></pre>

    <h2>Multiple Files</h2>
    <p>Open multiple files:</p>
    <pre><code>UltimateDebugviewpp.exe file1.log file2.log file3.log</code></pre>

    <h2>Automation Examples</h2>
    <pre><code># Start with UDP listener
UltimateDebugviewpp.exe /udp:9999

# Load configuration and filter
UltimateDebugviewpp.exe /config:production.xml /filter:errors.filter

# Start minimized with logging
UltimateDebugviewpp.exe /min /log:output.txt</code></pre>

    <div class="topic-nav">
        <a href="keyboard-shortcuts.html" class="nav-prev">Previous: Keyboard Shortcuts</a>
        <a href="configuration.html" class="nav-next">Next: Configuration</a>
    </div>
</body>
</html>
'@

# Configuration
Write-HelpFile "configuration.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuration Settings - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Configuration Settings</h1>
    <p class="lead">Customize UltimateDebugView++ preferences and behavior.</p>

    <h2>Settings Categories</h2>

    <h3>General</h3>
    <ul>
        <li>Startup behavior</li>
        <li>Default capture sources</li>
        <li>Buffer sizes</li>
        <li>Auto-save options</li>
    </ul>

    <h3>Appearance</h3>
    <ul>
        <li>Theme selection (System/Light/Dark)</li>
        <li>Font settings</li>
        <li>Color schemes</li>
        <li>Window layout</li>
    </ul>

    <h3>Filters</h3>
    <ul>
        <li>Default filter sets</li>
        <li>System prefix filters</li>
        <li>Auto-create rules</li>
        <li>Filter persistence</li>
    </ul>

    <h3>SQL Formatting</h3>
    <ul>
        <li>Formatting style</li>
        <li>Keyword case</li>
        <li>Indentation</li>
        <li>Auto-format options</li>
    </ul>

    <h3>Performance</h3>
    <ul>
        <li>Message buffer limits</li>
        <li>Update frequency</li>
        <li>Thread pool size</li>
        <li>Memory limits</li>
    </ul>

    <h2>Configuration Storage</h2>
    <p>Settings are stored in:</p>
    <ul>
        <li><strong>Registry:</strong> HKEY_CURRENT_USER\Software\UltimateDebugViewPP</li>
        <li><strong>Files:</strong> %APPDATA%\UltimateDebugViewPP\</li>
    </ul>

    <h2>Import/Export</h2>
    <ul>
        <li>Export all settings to XML</li>
        <li>Import settings from file</li>
        <li>Share configurations with team</li>
        <li>Backup/restore settings</li>
    </ul>

    <div class="topic-nav">
        <a href="command-line.html" class="nav-prev">Previous: Command Line</a>
        <a href="troubleshooting.html" class="nav-next">Next: Troubleshooting</a>
    </div>
</body>
</html>
'@

# Troubleshooting
Write-HelpFile "troubleshooting.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Troubleshooting - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Troubleshooting</h1>
    <p class="lead">Solutions to common issues and problems.</p>

    <h2>No Messages Appearing</h2>
    <ul>
        <li>Verify capture is enabled (check status bar)</li>
        <li>Ensure no other debugger is attached</li>
        <li>Check filters aren''t hiding all messages</li>
        <li>Run as administrator for global capture</li>
        <li>Verify application is using OutputDebugString</li>
    </ul>

    <h2>Missing Messages</h2>
    <ul>
        <li>Check exclude filters</li>
        <li>Verify buffer isn''t full</li>
        <li>Look for process filters</li>
        <li>Check time range filters</li>
        <li>Ensure high message rate isn''t causing drops</li>
    </ul>

    <h2>High CPU Usage</h2>
    <ul>
        <li>Reduce message volume with filters</li>
        <li>Increase update interval</li>
        <li>Disable syntax highlighting</li>
        <li>Close unused tabs</li>
        <li>Reduce buffer size</li>
    </ul>

    <h2>Connection Issues</h2>
    <ul>
        <li>Check firewall settings</li>
        <li>Verify network connectivity</li>
        <li>Ensure correct port numbers</li>
        <li>Check agent is running</li>
        <li>Test with telnet/nc first</li>
    </ul>

    <h2>File Access Errors</h2>
    <ul>
        <li>Check file permissions</li>
        <li>Ensure file isn''t locked</li>
        <li>Verify path is correct</li>
        <li>Try running as administrator</li>
        <li>Check antivirus isn''t blocking</li>
    </ul>

    <h2>Display Issues</h2>
    <ul>
        <li>Reset window layout (View → Reset)</li>
        <li>Clear corrupted settings</li>
        <li>Update graphics drivers</li>
        <li>Try different theme</li>
        <li>Disable hardware acceleration</li>
    </ul>

    <h2>Getting Help</h2>
    <ul>
        <li>Check this documentation</li>
        <li>Visit GitHub issues page</li>
        <li>Enable debug logging</li>
        <li>Collect system information</li>
        <li>Provide reproduction steps</li>
    </ul>

    <div class="topic-nav">
        <a href="configuration.html" class="nav-prev">Previous: Configuration</a>
        <a href="faq.html" class="nav-next">Next: FAQ</a>
    </div>
</body>
</html>
'@

# FAQ
Write-HelpFile "faq.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>FAQ - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Frequently Asked Questions</h1>
    <p class="lead">Answers to common questions about UltimateDebugView++.</p>

    <h2>General</h2>

    <h3>Q: What is UltimateDebugView++?</h3>
    <p>A: An enhanced debug message viewer for Windows with modern UI, SQL formatting, and Clarion integration.</p>

    <h3>Q: Is it compatible with Sysinternals DebugView?</h3>
    <p>A: Yes, it captures the same OutputDebugString messages and supports DebugView Agent protocol.</p>

    <h3>Q: Can multiple instances run simultaneously?</h3>
    <p>A: No, only one application can capture OutputDebugString at a time.</p>

    <h2>Features</h2>

    <h3>Q: What makes it "Ultimate"?</h3>
    <p>A: Advanced features like SQL formatting, Clarion support, syntax highlighting, and modern ribbon UI.</p>

    <h3>Q: Can it monitor remote machines?</h3>
    <p>A: Yes, via DebugView Agent, UDP, or network file monitoring.</p>

    <h3>Q: Does it support large log files?</h3>
    <p>A: Yes, with streaming readers and efficient memory management.</p>

    <h2>Performance</h2>

    <h3>Q: How many messages can it handle?</h3>
    <p>A: 50,000+ messages per second with optimized settings.</p>

    <h3>Q: What affects performance?</h3>
    <p>A: Complex filters, syntax highlighting, and number of active tabs.</p>

    <h2>Clarion</h2>

    <h3>Q: Do I need Clarion to use it?</h3>
    <p>A: No, it works with any Windows application using OutputDebugString.</p>

    <h3>Q: What Clarion versions are supported?</h3>
    <p>A: All modern Clarion versions with the UltimateDebug class.</p>

    <h2>Troubleshooting</h2>

    <h3>Q: Why no messages from my application?</h3>
    <p>A: Check if another debugger is attached or if running in release mode.</p>

    <h3>Q: Can''t connect to remote agent?</h3>
    <p>A: Verify firewall settings and network connectivity.</p>

    <div class="topic-nav">
        <a href="troubleshooting.html" class="nav-prev">Previous: Troubleshooting</a>
        <a href="api-methods.html" class="nav-next">Next: API Methods</a>
    </div>
</body>
</html>
'@

# API Methods
Write-HelpFile "api-methods.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>API Method Reference - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>API Method Reference</h1>
    <p class="lead">Complete reference of all UltimateDebug API methods.</p>

    <h2>Core Methods</h2>
    <table>
        <tr><th>Method</th><th>Description</th></tr>
        <tr><td>Debug(string)</td><td>Output debug message</td></tr>
        <tr><td>Init(string)</td><td>Initialize procedure tracking</td></tr>
        <tr><td>EndOfProcedure()</td><td>Mark procedure exit</td></tr>
        <tr><td>ClearDebugView()</td><td>Clear output window</td></tr>
    </table>

    <h2>Data Methods</h2>
    <table>
        <tr><th>Method</th><th>Description</th></tr>
        <tr><td>DebugQueue(*QUEUE)</td><td>Display queue contents</td></tr>
        <tr><td>DebugGroup(*GROUP)</td><td>Display group structure</td></tr>
        <tr><td>DebugRecord()</td><td>Display current record</td></tr>
        <tr><td>DebugFile(*FILE)</td><td>Display file structure</td></tr>
    </table>

    <h2>Event Methods</h2>
    <table>
        <tr><th>Method</th><th>Description</th></tr>
        <tr><td>DebugEvent()</td><td>Log Windows event</td></tr>
        <tr><td>SetEventsToDebug(string)</td><td>Filter events</td></tr>
        <tr><td>AddEventToDebug(long)</td><td>Add single event</td></tr>
    </table>

    <h2>Configuration Methods</h2>
    <table>
        <tr><th>Method</th><th>Description</th></tr>
        <tr><td>SetPrefix(string)</td><td>Set message prefix</td></tr>
        <tr><td>SetApplicationName(string)</td><td>Set app name</td></tr>
        <tr><td>SetModuleName(string)</td><td>Set module name</td></tr>
        <tr><td>Enable()</td><td>Enable output</td></tr>
        <tr><td>Disable()</td><td>Disable output</td></tr>
    </table>

    <h2>Utility Methods</h2>
    <table>
        <tr><th>Method</th><th>Description</th></tr>
        <tr><td>Error(string)</td><td>Log error message</td></tr>
        <tr><td>Warning(string)</td><td>Log warning</td></tr>
        <tr><td>Info(string)</td><td>Log info message</td></tr>
        <tr><td>Assert(byte, string)</td><td>Conditional assert</td></tr>
        <tr><td>SQL(string)</td><td>Log SQL query</td></tr>
    </table>

    <div class="topic-nav">
        <a href="faq.html" class="nav-prev">Previous: FAQ</a>
        <a href="api-properties.html" class="nav-next">Next: Properties Reference</a>
    </div>
</body>
</html>
'@

# API Properties
Write-HelpFile "api-properties.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>API Properties Reference - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>API Properties Reference</h1>
    <p class="lead">Complete reference of all UltimateDebug API properties.</p>

    <h2>Control Properties</h2>
    <table>
        <tr><th>Property</th><th>Type</th><th>Description</th></tr>
        <tr><td>Enabled</td><td>BYTE</td><td>Enable/disable all output</td></tr>
        <tr><td>ShowThreadID</td><td>BYTE</td><td>Include thread ID</td></tr>
        <tr><td>ShowTimestamp</td><td>BYTE</td><td>Include timestamps</td></tr>
        <tr><td>ShowProcedure</td><td>BYTE</td><td>Include procedure name</td></tr>
        <tr><td>ShowModule</td><td>BYTE</td><td>Include module name</td></tr>
    </table>

    <h2>Configuration Properties</h2>
    <table>
        <tr><th>Property</th><th>Type</th><th>Description</th></tr>
        <tr><td>ApplicationName</td><td>STRING</td><td>Application identifier</td></tr>
        <tr><td>ModuleName</td><td>STRING</td><td>Module identifier</td></tr>
        <tr><td>DefaultPrefix</td><td>STRING</td><td>Default message prefix</td></tr>
        <tr><td>MessageFormat</td><td>STRING</td><td>Output format template</td></tr>
    </table>

    <h2>Performance Properties</h2>
    <table>
        <tr><th>Property</th><th>Type</th><th>Description</th></tr>
        <tr><td>BufferSize</td><td>LONG</td><td>Internal buffer size</td></tr>
        <tr><td>FlushInterval</td><td>LONG</td><td>Auto-flush milliseconds</td></tr>
        <tr><td>MaxMessageLength</td><td>LONG</td><td>Maximum message size</td></tr>
    </table>

    <h2>Statistics Properties</h2>
    <table>
        <tr><th>Property</th><th>Type</th><th>Description</th></tr>
        <tr><td>MessageCount</td><td>LONG</td><td>Total messages sent</td></tr>
        <tr><td>ErrorCount</td><td>LONG</td><td>Total errors logged</td></tr>
        <tr><td>LastError</td><td>STRING</td><td>Last error message</td></tr>
    </table>

    <div class="topic-nav">
        <a href="api-methods.html" class="nav-prev">Previous: Method Reference</a>
        <a href="about.html" class="nav-next">Next: About</a>
    </div>
</body>
</html>
'@

# About
Write-HelpFile "about.html" @'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>About UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>About UltimateDebugView++</h1>
    <p class="lead">Enhanced debug viewer for Windows with modern features.</p>

    <h2>Version Information</h2>
    <ul>
        <li><strong>Version:</strong> 2.0</li>
        <li><strong>Build:</strong> Ultimate Edition</li>
        <li><strong>Platform:</strong> Windows 7+ (x64)</li>
        <li><strong>Framework:</strong> C++23, WTL, Scintilla</li>
    </ul>

    <h2>Features</h2>
    <ul>
        <li>Modern Ribbon UI</li>
        <li>SQL formatting with fsqlf</li>
        <li>Syntax highlighting via Scintilla/Lexilla</li>
        <li>Clarion integration</li>
        <li>Multiple message sources</li>
        <li>Advanced filtering</li>
        <li>High-performance capture (50,000+ msg/sec)</li>
    </ul>

    <h2>Credits</h2>
    <ul>
        <li><strong>Original DebugView++:</strong> CobaltFusion</li>
        <li><strong>Scintilla:</strong> Neil Hodgson</li>
        <li><strong>fsqlf:</strong> SQL formatter library</li>
        <li><strong>WTL:</strong> Windows Template Library</li>
        <li><strong>Boost:</strong> C++ libraries</li>
    </ul>

    <h2>License</h2>
    <p>This is a streamlined version of DebugView++ focused on the Ultimate variant.</p>

    <h2>Support</h2>
    <ul>
        <li><strong>Documentation:</strong> This help system</li>
        <li><strong>Source:</strong> GitHub repository</li>
        <li><strong>Issues:</strong> GitHub issue tracker</li>
    </ul>

    <h2>System Requirements</h2>
    <ul>
        <li>Windows 7 or later</li>
        <li>Visual C++ 2022 Redistributables</li>
        <li>Administrator rights (for global capture)</li>
        <li>2GB RAM minimum</li>
    </ul>

    <div class="topic-nav">
        <a href="api-properties.html" class="nav-prev">Previous: Properties Reference</a>
        <a href="welcome.html" class="nav-next">Back to Welcome</a>
    </div>
</body>
</html>
'@

Write-Host "All help files have been updated with real content!"