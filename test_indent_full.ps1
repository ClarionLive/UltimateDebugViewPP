# Full test to verify fsqlf.dll indentation with proper spacing configuration

$dllPath = "C:\Dev\UltimateDebugViewPP\bin\Debug\fsqlf.dll"

if (-not (Test-Path $dllPath)) {
    Write-Host "ERROR: fsqlf.dll not found at $dllPath" -ForegroundColor Red
    exit 1
}

Write-Host "Found DLL at: $dllPath" -ForegroundColor Green

$code = @"
using System;
using System.Runtime.InteropServices;

public class Fsqlf {
    [DllImport(@"$dllPath", CallingConvention = CallingConvention.Cdecl)]
    public static extern int fsqlf_init();

    [DllImport(@"$dllPath", CallingConvention = CallingConvention.Cdecl)]
    public static extern int fsqlf_format_string(string sql_in, out IntPtr sql_out);

    [DllImport(@"$dllPath", CallingConvention = CallingConvention.Cdecl)]
    public static extern void fsqlf_free_string(IntPtr str);

    [DllImport(@"$dllPath", CallingConvention = CallingConvention.Cdecl)]
    public static extern void fsqlf_cleanup();

    [DllImport(@"$dllPath", CallingConvention = CallingConvention.Cdecl)]
    public static extern void fsqlf_set_indent_type(int indent_type);

    [DllImport(@"$dllPath", CallingConvention = CallingConvention.Cdecl)]
    public static extern void fsqlf_set_keyword_case(int case_option);

    [DllImport(@"$dllPath", CallingConvention = CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
    public static extern void fsqlf_set_keyword_spacing(string keyword, int before_newlines, int before_indent, int after_newlines, int after_indent);
}
"@

Add-Type -TypeDefinition $code

Write-Host "`nTesting fsqlf.dll native indentation" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan

$testSql = "SELECT a, b, c FROM table1 WHERE x = 1 AND y = 2"
Write-Host "`nTest SQL: $testSql" -ForegroundColor Yellow

# Test each indent type
@(
    @{Type=0; Name="2 spaces"; Pattern="  "},
    @{Type=1; Name="4 spaces"; Pattern="    "},
    @{Type=2; Name="tabs"; Pattern="`t"}
) | ForEach-Object {
    Write-Host "`nTest: $($_.Name) (type=$($_.Type))" -ForegroundColor Green
    Write-Host "-----------------------------------"

    # Reinitialize for each test
    [Fsqlf]::fsqlf_cleanup()
    [Fsqlf]::fsqlf_init()

    # Set indent type FIRST
    [Fsqlf]::fsqlf_set_indent_type($_.Type)

    # Set keyword case
    [Fsqlf]::fsqlf_set_keyword_case(2)  # UPPER

    # Configure spacing for AND/OR to have indentation
    [Fsqlf]::fsqlf_set_keyword_spacing("kw_and", 1, 1, 0, 0)  # newline before, 1 indent level
    [Fsqlf]::fsqlf_set_keyword_spacing("kw_or", 1, 1, 0, 0)   # newline before, 1 indent level

    $result = [IntPtr]::Zero
    if ([Fsqlf]::fsqlf_format_string($testSql, [ref]$result) -eq 0 -and $result -ne [IntPtr]::Zero) {
        $formatted = [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($result)
        [Fsqlf]::fsqlf_free_string($result)

        # Display result with visible tabs
        if ($_.Type -eq 2) {
            $display = $formatted -replace "`t", "[TAB]"
            Write-Host $display
        } else {
            Write-Host $formatted
        }

        # Check for expected indentation before AND
        $searchPattern = $_.Pattern + "AND"
        if ($formatted.Contains($searchPattern)) {
            Write-Host "PASS: Correct indentation detected ($($_.Name))" -ForegroundColor Green
        } else {
            Write-Host "FAIL: Expected indentation not found" -ForegroundColor Red
            Write-Host "Looking for: '$searchPattern'" -ForegroundColor Yellow

            # Debug: show what we got for AND line
            $lines = $formatted -split "`n"
            $andLine = $lines | Where-Object { $_ -match "AND" } | Select-Object -First 1
            if ($andLine) {
                Write-Host "AND line: '$andLine'" -ForegroundColor Yellow
                # Show the bytes at the start
                $bytes = [System.Text.Encoding]::ASCII.GetBytes($andLine)
                $first10 = $bytes | Select-Object -First 10
                Write-Host "First bytes: $($first10 -join ' ')" -ForegroundColor Yellow
            }
        }
    } else {
        Write-Host "ERROR: Failed to format SQL" -ForegroundColor Red
    }
}

[Fsqlf]::fsqlf_cleanup()
Write-Host "`nTest completed!" -ForegroundColor Cyan