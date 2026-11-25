# PowerShell script to test fsqlf.dll indentation functionality

$code = @'
using System;
using System.Runtime.InteropServices;
using System.Text;

public class FsqlfTest {
    [DllImport("fsqlf.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int fsqlf_init();

    [DllImport("fsqlf.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int fsqlf_format_string(string sql_in, out IntPtr sql_out);

    [DllImport("fsqlf.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void fsqlf_free_string(IntPtr str);

    [DllImport("fsqlf.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void fsqlf_cleanup();

    [DllImport("fsqlf.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void fsqlf_set_indent_type(int indent_type);

    [DllImport("fsqlf.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void fsqlf_set_keyword_case(int case_option);

    public static string FormatSql(string sql, int indentType) {
        fsqlf_cleanup();
        fsqlf_init();
        fsqlf_set_indent_type(indentType);
        fsqlf_set_keyword_case(2); // UPPER case

        IntPtr result;
        if (fsqlf_format_string(sql, out result) == 0 && result != IntPtr.Zero) {
            string formatted = Marshal.PtrToStringAnsi(result);
            fsqlf_free_string(result);
            return formatted;
        }
        return null;
    }
}
'@

# Change to the Debug directory where the DLL is located
Set-Location "C:\Dev\UltimateDebugViewPP\bin\Debug"

Add-Type -TypeDefinition $code

Write-Host "Testing fsqlf.dll native indentation support" -ForegroundColor Cyan
Write-Host "=============================================" -ForegroundColor Cyan
Write-Host ""

$testSql = "SELECT a, b, c FROM table1 WHERE x = 1 AND y = 2"
Write-Host "Original SQL:" -ForegroundColor Yellow
Write-Host $testSql
Write-Host ""

# Test 1: 2 spaces
Write-Host "Test 1: 2-space indentation (indent_type=0)" -ForegroundColor Green
Write-Host "--------------------------------------------"
$result = [FsqlfTest]::FormatSql($testSql, 0)
if ($result) {
    Write-Host $result
    if ($result -match "  AND") {
        Write-Host "✓ 2-space indentation detected" -ForegroundColor Green
    } else {
        Write-Host "✗ 2-space indentation not detected" -ForegroundColor Red
    }
}
Write-Host ""

# Test 2: 4 spaces
Write-Host "Test 2: 4-space indentation (indent_type=1)" -ForegroundColor Green
Write-Host "--------------------------------------------"
$result = [FsqlfTest]::FormatSql($testSql, 1)
if ($result) {
    Write-Host $result
    if ($result -match "    AND") {
        Write-Host "✓ 4-space indentation detected" -ForegroundColor Green
    } else {
        Write-Host "✗ 4-space indentation not detected" -ForegroundColor Red
    }
}
Write-Host ""

# Test 3: tabs
Write-Host "Test 3: Tab indentation (indent_type=2)" -ForegroundColor Green
Write-Host "-----------------------------------------"
$result = [FsqlfTest]::FormatSql($testSql, 2)
if ($result) {
    # Display tabs as [TAB] for visibility
    $display = $result -replace "`t", "[TAB]"
    Write-Host $display
    if ($result.Contains("`tAND")) {
        Write-Host "✓ Tab indentation detected" -ForegroundColor Green
    } else {
        Write-Host "✗ Tab indentation not detected" -ForegroundColor Red
    }
}
Write-Host ""

Write-Host "Test completed!" -ForegroundColor Cyan