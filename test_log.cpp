#include <fstream>
#include <iostream>

int main() {
    std::wofstream log(L"C:\\Dev\\DebugViewPP\\test_write.log", std::ios::app);
    if (log.is_open()) {
        log << L"Test write successful" << std::endl;
        log.close();
        std::wcout << L"Log written successfully" << std::endl;
    } else {
        std::wcout << L"Failed to open log file" << std::endl;
    }
    return 0;
}