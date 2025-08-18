// Debug test to check what's being sent
#include <windows.h>
#include <string>

int main()
{
    // Send a simple Clarion message to check format
    std::string msg = "[||]X,T:1234,P:TestProc,M:TestModule.clw,A:TestApp.exe,PRE:TEST,MSG:This is a test message";
    
    OutputDebugStringA("=== DEBUG: About to send Clarion message ===");
    OutputDebugStringA(("DEBUG: Full message: " + msg).c_str());
    OutputDebugStringA(msg.c_str());
    OutputDebugStringA("=== DEBUG: Clarion message sent ===");
    
    return 0;
}