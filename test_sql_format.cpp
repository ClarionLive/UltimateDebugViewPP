#include <iostream>
#include <string>
#include <algorithm>

// Simplified version of FormatSQL to test the issue
std::wstring TestFormatSQL(const std::wstring& text) {
    std::wstring upperText = text;
    std::transform(upperText.begin(), upperText.end(), upperText.begin(), ::towupper);
    
    // Find where SQL starts
    size_t sqlStart = upperText.find(L"SELECT");
    if (sqlStart == std::wstring::npos) {
        return text;
    }
    
    std::wstring prefix = text.substr(0, sqlStart);
    std::wstring sqlContent = text.substr(sqlStart);
    std::wstring result = prefix + L"\n";
    
    std::wcout << L"Prefix: [" << prefix << L"]\n";
    std::wcout << L"SQL Content starts with: [" << sqlContent.substr(0, 100) << L"...]\n";
    
    // Check for comment
    size_t commentPos = sqlContent.find(L"--");
    if (commentPos != std::wstring::npos) {
        std::wcout << L"Found comment at position: " << commentPos << L"\n";
        std::wstring beforeComment = sqlContent.substr(0, commentPos);
        std::wstring comment = sqlContent.substr(commentPos);
        std::wcout << L"Before comment: [" << beforeComment.substr(beforeComment.length() - 50) << L"]\n";
        std::wcout << L"Comment: [" << comment.substr(0, 100) << L"...]\n";
    }
    
    return result;
}

int main() {
    std::wstring testSQL = L"DEBUG: Executing SQL Query: SELECT u.user_id FROM users u WHERE u.active = 1; -- Query executed at 2024-01-15 with roles=['admin']";
    TestFormatSQL(testSQL);
    return 0;
}