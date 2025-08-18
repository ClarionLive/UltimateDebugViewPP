#include <windows.h>
#include <string>
#include <thread>
#include <chrono>

int main()
{
    // Send the special message to create/switch to a process-specific view
    OutputDebugStringA("@@NEWDEBUGVIEW@@");
    
    // Wait a bit to ensure the view is created
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Now send some regular debug messages
    OutputDebugStringA("Starting test application...");
    
    for (int i = 1; i <= 5; ++i)
    {
        std::string msg = "Test message #" + std::to_string(i);
        OutputDebugStringA(msg.c_str());
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    OutputDebugStringA("Test complete!");
    
    return 0;
}