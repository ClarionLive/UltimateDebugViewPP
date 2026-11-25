# Simple test to verify fsqlf.dll indentation

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
}
"@

Add-Type -TypeDefinition $code

Write-Host "`nTesting fsqlf.dll native indentation" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan

$testSql = "SELECT a, b, c FROM table1 WHERE x = 1 AND y = 2"
Write-Host "`nTest SQL: $testSql" -ForegroundColor Yellow

# Initialize fsqlf
[Fsqlf]::fsqlf_init()

# Test each indent type
@(
    @{Type=0; Name="2 spaces"; Expected="  "},
    @{Type=1; Name="4 spaces"; Expected="    "},
    @{Type=2; Name="tabs"; Expected="`t"}
) | ForEach-Object {
    Write-Host "`nTest: $($_.Name) (type=$($_.Type))" -ForegroundColor Green

    [Fsqlf]::fsqlf_cleanup()
    [Fsqlf]::fsqlf_init()
    [Fsqlf]::fsqlf_set_indent_type($_.Type)
    [Fsqlf]::fsqlf_set_keyword_case(2)  # UPPER

    $result = [IntPtr]::Zero
    if ([Fsqlf]::fsqlf_format_string($testSql, [ref]$result) -eq 0 -and $result -ne [IntPtr]::Zero) {
        $formatted = [System.Runtime.InteropServices.Marshal]::PtrToStringAnsi($result)
        [Fsqlf]::fsqlf_free_string($result)

        # Display result with visible tabs
        $display = $formatted -replace "`t", "[TAB]"
        Write-Host $display

        # Check for expected indentation
        $searchPattern = $_.Expected + "AND"
        if ($formatted.Contains($searchPattern)) {
            Write-Host "PASS: Correct indentation detected" -ForegroundColor Green
        } else {
            Write-Host "FAIL: Expected indentation not found" -ForegroundColor Red
        }
    } else {
        Write-Host "ERROR: Failed to format SQL" -ForegroundColor Red
    }
}

[Fsqlf]::fsqlf_cleanup()
Write-Host "`nTest completed!" -ForegroundColor Cyan