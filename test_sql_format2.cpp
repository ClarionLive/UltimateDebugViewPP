#include <iostream>
#include <string>
#include "application/DebugViewppLib/SqlFormatter.cpp"

int main() {
    std::wstring sql = L"SELECT u.user_id, u.username FROM users u WHERE u.active = 1 -- This is a comment";
    
    std::wcout << L"Original SQL:\n" << sql << L"\n\n";
    
    std::wstring formatted = fusion::debugviewpp::FormatSQL(sql);
    
    std::wcout << L"Formatted SQL:\n" << formatted << L"\n";
    
    return 0;
}