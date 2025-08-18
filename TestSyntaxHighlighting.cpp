#include <windows.h>
#include <string>

int main()
{
    // Test JSON syntax highlighting
    OutputDebugStringA("JSON Test:");
    OutputDebugStringA("{ \"name\": \"John Doe\", \"age\": 30, \"active\": true, \"balance\": 1234.56 }");
    
    Sleep(100);
    
    // Test XML syntax highlighting  
    OutputDebugStringA("XML Test:");
    OutputDebugStringA("<person><name>Jane Smith</name><age>25</age><active>false</active></person>");
    
    Sleep(100);
    
    // Test SQL syntax highlighting
    OutputDebugStringA("SQL Test:");
    OutputDebugStringA("SELECT * FROM users WHERE age > 18 AND active = 1 ORDER BY name");
    
    Sleep(100);
    
    // More complex JSON
    OutputDebugStringA("Complex JSON:");
    OutputDebugStringA("{\"users\": [{\"id\": 1, \"name\": \"Alice\"}, {\"id\": 2, \"name\": \"Bob\"}], \"count\": 2}");
    
    Sleep(100);
    
    // More complex XML
    OutputDebugStringA("Complex XML:");
    OutputDebugStringA("<?xml version=\"1.0\"?><root><item id=\"1\">First</item><!-- comment --><item id=\"2\">Second</item></root>");
    
    Sleep(100);
    
    // More complex SQL
    OutputDebugStringA("Complex SQL:");
    OutputDebugStringA("SELECT u.name, COUNT(o.id) as order_count FROM users u LEFT JOIN orders o ON u.id = o.user_id WHERE u.created_at > '2024-01-01' GROUP BY u.name HAVING COUNT(o.id) > 5");
    
    return 0;
}