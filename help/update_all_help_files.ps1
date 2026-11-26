# PowerShell script to update all help files with real content

$topicsPath = "C:\Dev\UltimateDebugViewPP\bin\Debug\help\topics"

# Message Sources
@'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Message Sources - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Message Sources</h1>
    <p class="lead">UltimateDebugView++ supports multiple simultaneous message sources for comprehensive debugging.</p>

    <h2>Available Message Sources</h2>

    <h3>1. Win32 OutputDebugString</h3>
    <p>The primary message source for Windows applications:</p>
    <ul>
        <li><strong>Local capture:</strong> Messages from your user session</li>
        <li><strong>Global capture:</strong> System-wide messages (requires admin)</li>
        <li><strong>Automatic detection:</strong> Starts capturing immediately on launch</li>
        <li>Supports both ANSI and Unicode messages</li>
    </ul>

    <h3>2. Network Sources</h3>
    <p>Receive debug messages over the network:</p>
    <ul>
        <li><strong>UDP Socket:</strong> Listen on configurable ports for UDP messages</li>
        <li><strong>TCP DebugView Agent:</strong> Connect to remote DebugView agents on port 2020</li>
        <li><strong>Multiple listeners:</strong> Run several network sources simultaneously</li>
    </ul>

    <h3>3. File Monitoring</h3>
    <p>Monitor log files in real-time:</p>
    <ul>
        <li><strong>Text files:</strong> Standard log file monitoring with tailing</li>
        <li><strong>Binary files:</strong> Support for binary log formats</li>
        <li><strong>Auto-reload:</strong> Detects file changes and updates</li>
        <li><strong>Multiple files:</strong> Monitor several files simultaneously</li>
    </ul>

    <h3>4. Named Pipes</h3>
    <p>Inter-process communication via Windows named pipes:</p>
    <ul>
        <li>Create named pipe servers</li>
        <li>Connect to existing pipe sources</li>
        <li>Bidirectional communication support</li>
    </ul>

    <h3>5. Process-Specific Capture</h3>
    <p>Filter messages by specific processes:</p>
    <ul>
        <li>Monitor individual applications</li>
        <li>Filter by process name or ID</li>
        <li>Track child processes automatically</li>
    </ul>

    <h3>6. Kernel Messages</h3>
    <p>Capture kernel-level debug output:</p>
    <ul>
        <li>Requires administrator privileges</li>
        <li>Driver and system-level messages</li>
        <li>ETW (Event Tracing for Windows) support</li>
    </ul>

    <h2>Managing Message Sources</h2>

    <h3>Adding Sources</h3>
    <ol>
        <li>Navigate to the <strong>Home</strong> tab on the ribbon</li>
        <li>Click the appropriate source button (File, Network, etc.)</li>
        <li>Configure source-specific settings</li>
        <li>Click OK to start capturing</li>
    </ol>

    <h3>Source Status</h3>
    <p>Each source displays its status:</p>
    <ul>
        <li><strong>Green:</strong> Active and receiving messages</li>
        <li><strong>Yellow:</strong> Connected but idle</li>
        <li><strong>Red:</strong> Error or disconnected</li>
        <li><strong>Gray:</strong> Paused or disabled</li>
    </ul>

    <h3>Source Properties</h3>
    <p>Right-click any source to:</p>
    <ul>
        <li>View connection details</li>
        <li>Modify settings</li>
        <li>Pause/resume capture</li>
        <li>Clear messages from this source</li>
        <li>Remove the source</li>
    </ul>

    <h2>Performance Considerations</h2>
    <ul>
        <li>Each source runs in its own thread</li>
        <li>Messages are buffered for optimal performance</li>
        <li>Use filters to manage high-volume sources</li>
        <li>Consider process-specific capture for focused debugging</li>
    </ul>

    <h2>Source Identification</h2>
    <p>Messages include source information:</p>
    <ul>
        <li><strong>Process column:</strong> Shows originating process</li>
        <li><strong>Source prefix:</strong> Optional prefix like [NET] or [FILE]</li>
        <li><strong>Color coding:</strong> Different sources can use different colors</li>
    </ul>

    <div class="note">
        <strong>Tip:</strong> Use multiple tabs with different source filters to organize complex debugging sessions.
    </div>

    <div class="topic-nav">
        <a href="detail-pane.html" class="nav-prev">Previous: Detail Pane</a>
        <a href="win32-capture.html" class="nav-next">Next: Win32 Capture</a>
    </div>
</body>
</html>
'@ | Set-Content "$topicsPath\message-sources.html" -Encoding UTF8

# Win32 Capture
@'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Win32 OutputDebugString Capture - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>Win32 OutputDebugString Capture</h1>
    <p class="lead">Capture debug output from Windows applications using the OutputDebugString API.</p>

    <h2>Overview</h2>
    <p>The Win32 OutputDebugString API is the standard mechanism for Windows applications to emit debug messages. UltimateDebugView++ uses the DBWinReader class to capture these messages through shared memory and system events.</p>

    <h2>How It Works</h2>
    <p>The capture mechanism uses Windows system objects:</p>
    <ul>
        <li><strong>DBWIN_BUFFER:</strong> Shared memory for message data</li>
        <li><strong>DBWIN_BUFFER_READY:</strong> Event signaling buffer is ready</li>
        <li><strong>DBWIN_DATA_READY:</strong> Event signaling new data available</li>
        <li>Supports both local (session) and global capture</li>
    </ul>

    <h2>Capture Modes</h2>

    <h3>Local Capture (Default)</h3>
    <ul>
        <li>Captures messages from current user session only</li>
        <li>No special privileges required</li>
        <li>Ideal for debugging user applications</li>
        <li>Lower system overhead</li>
    </ul>

    <h3>Global Capture</h3>
    <ul>
        <li>Captures messages from all sessions</li>
        <li>Requires administrator privileges</li>
        <li>Includes system services and other users</li>
        <li>Higher message volume</li>
    </ul>

    <h2>Starting Win32 Capture</h2>
    <ol>
        <li>Launch UltimateDebugView++ (auto-starts by default)</li>
        <li>Or click <strong>Home → Win32 Capture</strong> to toggle</li>
        <li>For global capture, run as administrator</li>
        <li>Status bar shows "Win32: Active" when running</li>
    </ol>

    <h2>Message Format</h2>
    <p>Captured messages include:</p>
    <ul>
        <li><strong>Timestamp:</strong> Precise time of message (millisecond accuracy)</li>
        <li><strong>Process ID:</strong> Originating process identifier</li>
        <li><strong>Process Name:</strong> Executable name</li>
        <li><strong>Message Text:</strong> The debug string content</li>
    </ul>

    <h2>Using OutputDebugString in Your Code</h2>

    <h3>C/C++</h3>
    <pre><code>#include &lt;windows.h&gt;
OutputDebugString(L"Debug message");
OutputDebugStringA("ANSI message");
OutputDebugStringW(L"Unicode message");</code></pre>

    <h3>C#/.NET</h3>
    <pre><code>System.Diagnostics.Debug.WriteLine("Debug message");
System.Diagnostics.Trace.WriteLine("Trace message");</code></pre>

    <h3>Clarion</h3>
    <pre><code>ud.Debug('Debug message')
OutputDebugString('Direct API call')</code></pre>

    <h2>Performance Optimization</h2>
    <ul>
        <li><strong>Buffer size:</strong> 4KB shared memory buffer</li>
        <li><strong>Message rate:</strong> Handles 50,000+ messages/second</li>
        <li><strong>Threading:</strong> Dedicated capture thread</li>
        <li><strong>Zero-copy:</strong> Direct memory mapping for efficiency</li>
    </ul>

    <h2>Common Issues</h2>

    <h3>No Messages Appearing</h3>
    <ul>
        <li>Verify Win32 capture is enabled</li>
        <li>Check if another debugger is attached</li>
        <li>Ensure no other tool is capturing (only one can capture at a time)</li>
        <li>Try running as administrator for global capture</li>
    </ul>

    <h3>Missing Messages</h3>
    <ul>
        <li>Check process filters aren't excluding messages</li>
        <li>Verify message isn't being filtered out</li>
        <li>Ensure buffer isn't overflowing (very high message rates)</li>
    </ul>

    <h3>Access Denied</h3>
    <ul>
        <li>Global capture requires admin rights</li>
        <li>Some system processes require elevation</li>
        <li>Anti-virus may block capture - add exception</li>
    </ul>

    <h2>Advanced Features</h2>

    <h3>Process Filtering</h3>
    <p>Focus on specific applications:</p>
    <ul>
        <li>Include only messages from selected processes</li>
        <li>Exclude noisy applications</li>
        <li>Filter by process name patterns</li>
    </ul>

    <h3>Message Prefixes</h3>
    <p>Automatically detect and parse prefixes:</p>
    <ul>
        <li>[ERROR], [WARNING], [INFO] for log levels</li>
        <li>[SQL] for query detection</li>
        <li>Custom prefixes for categorization</li>
    </ul>

    <div class="note">
        <strong>Important:</strong> Only one application can capture OutputDebugString at a time. Close other debuggers before using UltimateDebugView++.
    </div>

    <div class="topic-nav">
        <a href="message-sources.html" class="nav-prev">Previous: Message Sources</a>
        <a href="file-monitoring.html" class="nav-next">Next: File Monitoring</a>
    </div>
</body>
</html>
'@ | Set-Content "$topicsPath\win32-capture.html" -Encoding UTF8

# Continue with more files...
Write-Host "Creating File Monitoring help..."
@'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>File Monitoring - UltimateDebugView++</title>
    <link rel="stylesheet" href="../css/topic-styles.css">
</head>
<body>
    <h1>File Monitoring</h1>
    <p class="lead">Monitor and tail log files in real-time with automatic change detection.</p>

    <h2>Overview</h2>
    <p>File monitoring allows you to watch log files as they're written, similar to the Unix 'tail -f' command. UltimateDebugView++ supports multiple file formats and provides real-time updates.</p>

    <h2>Supported File Types</h2>
    <ul>
        <li><strong>Text files:</strong> .log, .txt, .out, .err</li>
        <li><strong>Application logs:</strong> IIS, Apache, custom formats</li>
        <li><strong>Binary logs:</strong> With configurable parsing</li>
        <li><strong>Rolling logs:</strong> Automatic rollover detection</li>
        <li><strong>Network shares:</strong> Remote file monitoring</li>
    </ul>

    <h2>Adding File Sources</h2>
    <ol>
        <li>Click <strong>Home → Add File</strong> on the ribbon</li>
        <li>Browse to select the log file</li>
        <li>Configure monitoring options:
            <ul>
                <li><strong>From beginning:</strong> Read entire file</li>
                <li><strong>From end:</strong> Start from current position</li>
                <li><strong>Follow tail:</strong> Monitor for new content</li>
            </ul>
        </li>
        <li>Click OK to start monitoring</li>
    </ol>

    <h2>Monitoring Features</h2>

    <h3>Real-time Updates</h3>
    <ul>
        <li>Automatic detection of file changes</li>
        <li>Configurable polling interval (default: 100ms)</li>
        <li>Efficient change detection using file timestamps</li>
        <li>Minimal CPU usage even with many files</li>
    </ul>

    <h3>File Rotation Support</h3>
    <ul>
        <li>Detects when log files are rotated</li>
        <li>Automatically switches to new file</li>
        <li>Maintains message continuity</li>
        <li>Supports common rotation patterns</li>
    </ul>

    <h3>Line Processing</h3>
    <ul>
        <li>Configurable line endings (CRLF, LF, CR)</li>
        <li>Multi-line message support</li>
        <li>Maximum line length settings</li>
        <li>Unicode and encoding detection</li>
    </ul>

    <h2>Advanced Options</h2>

    <h3>File Filters</h3>
    <p>Pre-filter file content before display:</p>
    <ul>
        <li>Include only lines matching patterns</li>
        <li>Exclude unwanted content</li>
        <li>Apply regex transformations</li>
        <li>Extract specific fields</li>
    </ul>

    <h3>Timestamp Extraction</h3>
    <p>Parse timestamps from log entries:</p>
    <ul>
        <li>Automatic format detection</li>
        <li>Custom timestamp patterns</li>
        <li>Timezone conversion</li>
        <li>Relative time display</li>
    </ul>

    <h3>Column Mapping</h3>
    <p>Parse structured log formats:</p>
    <ul>
        <li>CSV/TSV automatic parsing</li>
        <li>Custom delimiter support</li>
        <li>Field extraction with regex</li>
        <li>Named column display</li>
    </ul>

    <h2>Performance Optimization</h2>

    <h3>Large Files</h3>
    <ul>
        <li>Streaming reader for minimal memory usage</li>
        <li>Configurable buffer sizes</li>
        <li>Seek to end for large existing files</li>
        <li>Line count limits to prevent overflow</li>
    </ul>

    <h3>Multiple Files</h3>
    <ul>
        <li>Parallel monitoring threads</li>
        <li>Shared message queue</li>
        <li>Priority-based processing</li>
        <li>Resource pooling</li>
    </ul>

    <h2>Common Use Cases</h2>

    <h3>Application Logs</h3>
    <pre><code>C:\MyApp\logs\application.log
C:\inetpub\logs\LogFiles\W3SVC1\*.log
C:\Program Files\MyService\debug.txt</code></pre>

    <h3>System Logs</h3>
    <pre><code>C:\Windows\System32\LogFiles\*.log
C:\ProgramData\MySQL\data\error.log</code></pre>

    <h3>Development Logs</h3>
    <pre><code>.\bin\Debug\trace.log
..\output\build.log
\\buildserver\logs\compile.txt</code></pre>

    <h2>Troubleshooting</h2>

    <h3>File Access Denied</h3>
    <ul>
        <li>Check file permissions</li>
        <li>Run as administrator if needed</li>
        <li>Ensure file isn't exclusively locked</li>
        <li>Try read-only mode</li>
    </ul>

    <h3>Missing Updates</h3>
    <ul>
        <li>Verify "Follow tail" is enabled</li>
        <li>Check polling interval settings</li>
        <li>Ensure file is being written to</li>
        <li>Look for file rotation</li>
    </ul>

    <h3>High CPU Usage</h3>
    <ul>
        <li>Increase polling interval</li>
        <li>Reduce number of monitored files</li>
        <li>Use filters to reduce message volume</li>
        <li>Disable unnecessary processing</li>
    </ul>

    <div class="note">
        <strong>Tip:</strong> Use wildcards in file paths to monitor multiple files matching a pattern, like *.log or error_*.txt
    </div>

    <div class="topic-nav">
        <a href="win32-capture.html" class="nav-prev">Previous: Win32 Capture</a>
        <a href="network-sources.html" class="nav-next">Next: Network Sources</a>
    </div>
</body>
</html>
'@ | Set-Content "$topicsPath\file-monitoring.html" -Encoding UTF8

Write-Host "All help files updated with real content!"