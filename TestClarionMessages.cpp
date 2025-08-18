// Test program to send Clarion-formatted messages to DebugView++
#include <windows.h>
#include <string>
#include <sstream>

void SendClarionMessage(const std::string& thread, const std::string& proc, 
                       const std::string& module, const std::string& app,
                       const std::string& prefix, const std::string& message)
{
    std::stringstream ss;
    ss << "[||]X";
    ss << ",T:" << thread;
    ss << ",P:" << proc;
    ss << ",M:" << module;
    ss << ",A:" << app;
    if (!prefix.empty()) {
        ss << ",PRE:" << prefix;
    }
    ss << ",MSG:" << message;
    
    OutputDebugStringA(ss.str().c_str());
}

int main()
{
    // Clear marker messages
    OutputDebugStringA("========== CLARION TEST START ==========");
    
    // Send some test Clarion messages
    SendClarionMessage("1234", "InitDatabase", "MainModule.clw", "MyApp.exe", "DB", 
                      "Connecting to database...");
    
    SendClarionMessage("5678", "CreateWindow", "UIModule.clw", "MyApp.exe", "UI", 
                      "Window created successfully");
    
    SendClarionMessage("3456", "LogEvent", "LogModule.clw", "MyApp.exe", "", 
                      "Standard log entry without prefix");
    
    SendClarionMessage("1234", "ParseData", "DataModule.clw", "MyApp.exe", "DATA", 
                      "Processing: ID=123, Name=John, Value=$456.78");
    
    // Send a regular non-Clarion message
    OutputDebugStringA("This is a regular debug message (not Clarion format)");
    
    SendClarionMessage("9999", "Main", "UltimateRED001.clw", "UltimateRED.EXE", "", 
                      "ROW TYPE Fetching record");
    
    SendClarionMessage("7777", "FolderContents", "UltimateRED007.clw", "UltimateRED.EXE", "FILE", 
                      "Processing folder: C:\\Users\\Documents");
    
    SendClarionMessage("8888", "FindContents", "UltimateRED008.clw", "UltimateRED.EXE", "SEARCH", 
                      "Searching for pattern: *.txt");
    
    OutputDebugStringA("========== CLARION TEST END ==========");
    
    return 0;
}