// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>

#include <windows.h>

namespace fusion {
namespace DebugViewTestMessages {

class TestMessageGenerator
{
private:
    int m_messageCount;
    int m_delayMs;
    bool m_continuous;
    int m_currentMessage;

public:
    TestMessageGenerator(int count = 50, int delayMs = 100, bool continuous = false)
        : m_messageCount(count), m_delayMs(delayMs), m_continuous(continuous), m_currentMessage(0)
    {
    }

    void GenerateMessages()
    {
        std::cout << "Starting DebugView++ Test Message Generator\n";
        std::cout << "Messages: " << m_messageCount << ", Delay: " << m_delayMs << "ms, Continuous: " << (m_continuous ? "Yes" : "No") << "\n\n";

        do {
            m_currentMessage = 0;
            GenerateTestSet();
            
            if (m_continuous) {
                std::cout << "Completed message set, pausing before next iteration...\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            }
        } while (m_continuous);

        std::cout << "\nTest message generation completed.\n";
    }

private:
    void GenerateTestSet()
    {
        // Test the new auto-view creation feature
        SendMessage("@@NEWDEBUGVIEW@@");
        SendMessage("INFO: Auto-view created for this process!");
        
        // Short messages
        SendMessage("INFO: Application started successfully");
        SendMessage("DEBUG: Configuration loaded");
        SendMessage("WARN: Low memory warning");
        SendMessage("ERROR: File not found");
        SendMessage("TRACE: Method entry");
        
        // Medium length messages
        SendMessage("INFO: Database connection established to server 'localhost:5432' with connection pool size 10");
        SendMessage("DEBUG: Processing user request with parameters: userId=12345, action=login, timestamp=2024-01-15T10:30:45");
        SendMessage("WARN: Performance degradation detected - response time exceeded threshold of 500ms (actual: 750ms)");
        SendMessage("ERROR: Authentication failed for user 'admin' from IP address 192.168.1.100 - invalid credentials");
        
        // Long single-line messages (500-1000 chars)
        SendMessage("INFO: User session initialized with comprehensive details including sessionId='sess_abc123def456', userId=98765, userName='john.doe@company.com', clientIP='10.0.0.155', userAgent='Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36', loginTime='2024-01-15T10:30:45.123Z', lastActivity='2024-01-15T10:30:45.123Z', permissions=['read', 'write', 'admin'], organizationId=42, departmentId=7, roleId=3, sessionTimeout=3600, preferredLanguage='en-US', timezone='America/New_York'");
        
        SendMessage("DEBUG: HTTP Request processing pipeline executed with timing breakdown: RouteResolution=15ms, Authentication=25ms, Authorization=8ms, ModelBinding=12ms, ControllerExecution=45ms, ViewRendering=78ms, ResponseSerialization=22ms, TotalTime=205ms for endpoint '/api/v1/users/profile' with method 'GET', contentType='application/json', responseSize=2048bytes, cacheHit=false");
        
        // Very long messages like SQL statements (2000+ chars)
        std::string longSql = "DEBUG: Executing SQL Query: SELECT u.user_id, u.username, u.email, u.first_name, u.last_name, u.created_at, u.last_login, p.profile_id, p.avatar_url, p.bio, p.phone_number, p.address_line1, p.address_line2, p.city, p.state, p.postal_code, p.country, r.role_id, r.role_name, r.permissions, o.org_id, o.org_name, o.org_type, d.dept_id, d.dept_name, s.setting_key, s.setting_value FROM users u LEFT JOIN profiles p ON u.user_id = p.user_id LEFT JOIN user_roles ur ON u.user_id = ur.user_id LEFT JOIN roles r ON ur.role_id = r.role_id LEFT JOIN organizations o ON u.org_id = o.org_id LEFT JOIN departments d ON u.dept_id = d.dept_id LEFT JOIN user_settings s ON u.user_id = s.user_id WHERE u.active = 1 AND u.email_verified = 1 AND (u.created_at BETWEEN '2023-01-01' AND '2024-12-31') AND (r.role_name IN ('admin', 'manager', 'editor') OR u.user_id IN (SELECT DISTINCT user_id FROM special_permissions WHERE permission_type = 'elevated_access' AND expiry_date > NOW())) ORDER BY u.last_login DESC, u.username ASC LIMIT 100 OFFSET 0; -- Query executed at 2024-01-15 10:30:45.123 with parameters: active=1, verified=1, startDate='2023-01-01', endDate='2024-12-31', roles=['admin','manager','editor'], limit=100, offset=0, executionTime=245ms, rowsReturned=87, indexesUsed=['idx_users_active_verified', 'idx_users_created_at', 'idx_user_roles_composite']";
        SendMessage(longSql);
        
        // Multi-line messages
        SendMessage("ERROR: Stack trace for unhandled exception:\n   at Application.ProcessRequest(String requestId)\n   at HttpModule.HandleRequest(HttpContext context)\n   at System.Web.HttpRuntime.ProcessRequestInternal()\n   at System.Threading.ThreadPoolWorkQueue.Dispatch()\n   Inner Exception: ArgumentNullException: Value cannot be null\n   Parameter name: connectionString");
        
        SendMessage("INFO: Configuration file contents:\n{\n  \"database\": {\n    \"host\": \"localhost\",\n    \"port\": 5432,\n    \"name\": \"myapp_db\"\n  },\n  \"logging\": {\n    \"level\": \"DEBUG\",\n    \"file\": \"app.log\"\n  }\n}");
        
        // Messages with tabs and special formatting
        SendMessage("DEBUG: Performance metrics:\nCPU Usage:\t25.4%\nMemory Usage:\t512MB\nDisk I/O:\t\t15.2MB/s\nNetwork I/O:\t1.8MB/s\nThread Count:\t42");
        
        SendMessage("INFO: Database table statistics:\nTable Name\t\tRow Count\tSize (MB)\tLast Updated\nusers\t\t\t50,000\t\t125.4\t\t2024-01-15 10:15:30\nprofiles\t\t\t48,500\t\t89.2\t\t2024-01-15 10:12:45\ntransactions\t\t500,000\t\t1,250.8\t\t2024-01-15 10:30:15");
        
        // Unicode messages using OutputDebugStringW
        SendMessageW(L"INFO: Unicode test - English: Hello World, Chinese: 你好世界, Japanese: こんにちは世界, Russian: Привет мир, Arabic: مرحبا بالعالم");
        SendMessageW(L"DEBUG: Special characters test: ©®™€£¥§¶•‰≈≠≤≥π∞∑√∂∆Ω");
        SendMessageW(L"INFO: File path with Unicode: C:\\Users\\用户\\Documents\\файлы\\документы\\file.txt");
        
        // JSON formatted examples
        SendMessage(R"(INFO: API Response: {"status": "success", "data": {"userId": 12345, "username": "john.doe", "profile": {"firstName": "John", "lastName": "Doe", "email": "john.doe@company.com", "roles": ["user", "admin"], "preferences": {"theme": "dark", "language": "en-US", "notifications": true}}, "metadata": {"requestId": "req_abc123", "timestamp": "2024-01-15T10:30:45.123Z", "responseTime": 156}})");
        
        SendMessage(R"(DEBUG: Configuration update: {"database": {"connectionString": "Server=localhost;Database=MyApp;Integrated Security=true;", "timeout": 30, "poolSize": 100}, "cache": {"provider": "Redis", "connectionString": "localhost:6379", "defaultExpiration": 3600}, "logging": {"level": "INFO", "providers": ["Console", "File", "EventLog"], "file": {"path": "logs/app.log", "maxSize": "100MB", "retention": "30d"}}, "features": {"enableDebugMode": false, "enableProfiling": true, "maxUploadSize": 10485760}})");
        
        // XML formatted examples
        SendMessage("INFO: XML Configuration:\n<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<configuration>\n  <appSettings>\n    <add key=\"DatabaseConnection\" value=\"Server=localhost;Database=MyApp;\" />\n    <add key=\"LogLevel\" value=\"INFO\" />\n    <add key='EnableDebug' value='false' />\n  </appSettings>\n  <system.web>\n    <httpRuntime maxRequestLength=\"51200\" />\n    <compilation debug=\"false\" targetFramework=\"4.8\" />\n  </system.web>\n</configuration>");
        
        // XML with comments containing > character, single quotes, and CDATA
        SendMessage("DEBUG: Complex XML test:\n<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<!-- This comment contains > and < characters: if (a > b && c < d) -->\n<root>\n  <item name='test' value='single quotes' />\n  <description><![CDATA[This is CDATA content with <tags> and special chars: & < > \" ']]></description>\n  <!-- Another comment with comparison: x > 5 -->\n  <config type=\"complex\" enabled='true' />\n</root>");
        
        // Messages with timestamps
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::stringstream timestampMsg;
        timestampMsg << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        timestampMsg << "." << std::setfill('0') << std::setw(3) << ms.count() << "] ";
        timestampMsg << "INFO: Timestamped message with millisecond precision";
        SendMessage(timestampMsg.str());
        
        // Error/warning/info prefixed messages
        SendMessage("[CRITICAL] System is running out of disk space - only 2.1GB remaining on C: drive");
        SendMessage("[ERROR] Failed to connect to external service: timeout after 30 seconds");
        SendMessage("[WARN] Deprecated API endpoint /api/v1/legacy accessed - please migrate to /api/v2/");
        SendMessage("[INFO] Scheduled maintenance completed successfully in 45 minutes");
        SendMessage("[DEBUG] Cache miss for key 'user_profile_12345' - fetching from database");
        SendMessage("[TRACE] Entering method ProcessPayment with parameters: amount=99.99, currency=USD");
        
        // Performance and monitoring messages
        SendMessage("PERF: Method execution time - ProcessOrder: 234ms, ValidatePayment: 45ms, UpdateInventory: 67ms, SendNotification: 23ms");
        SendMessage("MONITOR: System health check - CPU: OK, Memory: OK, Disk: WARNING (85% full), Network: OK, Database: OK");
        SendMessage("AUDIT: User action logged - User: admin@company.com, Action: DELETE, Resource: /api/users/54321, IP: 192.168.1.100, Timestamp: 2024-01-15T10:30:45Z");
        
        // Security messages
        SendMessage("SECURITY: Failed login attempt detected - Username: admin, IP: 203.0.113.45, Attempt: 5/5 (account locked)");
        SendMessage("SECURITY: JWT token expired for user session sess_xyz789 - forcing re-authentication");
        SendMessage("SECURITY: Suspicious file upload blocked - Filename: malware.exe, User: user123, Scanner: ClamAV");
        
        // Business logic messages
        SendMessage("BUSINESS: Order processing pipeline - OrderId: ORD-2024-001234, Status: PROCESSING, Items: 3, Total: $299.97, Payment: AUTHORIZED");
        SendMessage("BUSINESS: Inventory alert - Product SKU-ABC123 stock level below threshold (Current: 5, Minimum: 10, Reorder: 50)");
        SendMessage("BUSINESS: Customer lifecycle event - CustomerId: CUST-98765, Event: SUBSCRIPTION_RENEWED, Plan: PREMIUM, Duration: 12 months");
        
        // Integration messages
        SendMessage("INTEGRATION: Third-party API call - Service: PaymentProcessor, Method: ProcessPayment, Response: SUCCESS, TransactionId: TXN-2024-567890, Duration: 1.2s");
        SendMessage("INTEGRATION: Message queue processing - Queue: order-notifications, Messages processed: 147, Failed: 2, Average processing time: 15ms");
        SendMessage("INTEGRATION: External webhook received - Source: stripe.com, Event: payment.succeeded, ProcessingTime: 89ms, Status: PROCESSED");
        
        // Development and debugging messages
        SendMessage("DEV: Code checkpoint reached - File: OrderService.cs, Line: 245, Method: CalculateDiscount, Variables: originalPrice=100.00, discountPercent=15.0, finalPrice=85.00");
        SendMessage("DEV: Memory allocation report - Objects created: 1,247, Memory allocated: 15.6MB, GC collections: Gen0=5, Gen1=2, Gen2=0");
        SendMessage("DEV: Feature flag evaluation - Flag: enable_new_checkout, User: user123, Result: true, Reason: user_in_beta_group");
        
        // Additional long message examples
        std::string longLogEntry = "INFO: Comprehensive system status report generated at 2024-01-15T10:30:45.123Z - Application Version: 2.3.1, Environment: Production, Server: WEB-01, Uptime: 15d 7h 23m 45s, Total Requests: 2,847,392, Successful: 2,798,441 (98.28%), Failed: 48,951 (1.72%), Average Response Time: 245ms, Peak Response Time: 4.5s, Memory Usage: 2.1GB/8.0GB (26.25%), CPU Usage: 35.7%, Disk Usage: 156GB/500GB (31.2%), Network In: 1.2TB, Network Out: 3.4TB, Active Connections: 1,247, Database Connections: 85/100 (85%), Cache Hit Rate: 94.5%, Error Rate: 0.12%, Top Errors: [NullReferenceException: 234, TimeoutException: 189, ValidationException: 156], Recent Deployments: [v2.3.1: 2024-01-10, v2.3.0: 2024-01-01], Scheduled Maintenance: 2024-01-20T02:00:00Z, Health Check Status: GREEN";
        SendMessage(longLogEntry);

        // Generate remaining messages to reach the target count
        int remainingMessages = m_messageCount - m_currentMessage;
        for (int i = 0; i < remainingMessages; ++i) {
            std::stringstream msg;
            msg << "INFO: Generated test message " << (m_currentMessage + 1) << " of " << m_messageCount;
            msg << " - This is a variable length message with some additional content to test display: ";
            
            // Vary message length
            int extraLength = (m_currentMessage + 1) % 5;
            for (int j = 0; j < extraLength; ++j) {
                msg << "extra_content_part_" << j << " ";
            }
            
            msg << "[Generated at " << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "]";
            
            SendMessage(msg.str());
        }
    }

    void SendMessage(const std::string& message)
    {
        ++m_currentMessage;
        std::cout << "[" << std::setw(3) << m_currentMessage << "/" << m_messageCount << "] Sending: " 
                  << (message.length() > 80 ? message.substr(0, 80) + "..." : message) << std::endl;
        
        OutputDebugStringA(message.c_str());
        
        if (m_delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_delayMs));
        }
    }

    void SendMessageW(const std::wstring& message)
    {
        ++m_currentMessage;
        std::wcout << L"[" << std::setw(3) << m_currentMessage << L"/" << m_messageCount << L"] Sending (Unicode): " 
                   << (message.length() > 80 ? message.substr(0, 80) + L"..." : message) << std::endl;
        
        OutputDebugStringW(message.c_str());
        
        if (m_delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(m_delayMs));
        }
    }
};

void PrintUsage()
{
    std::cout << "DebugView++ Test Message Generator\n"
                 "Usage: DebugViewTestMessages [options]\n\n"
                 "Options:\n"
                 "  --count N      Number of messages to generate (default: 50)\n"
                 "  --delay MS     Delay between messages in milliseconds (default: 100)\n"
                 "  --continuous   Keep generating messages in a loop\n"
                 "  --help         Show this help message\n\n"
                 "This program generates various types of debug messages to test\n"
                 "DebugView++ display capabilities, including:\n"
                 "  - Short and long messages\n"
                 "  - Multi-line messages\n"
                 "  - Messages with special formatting\n"
                 "  - Unicode messages\n"
                 "  - JSON and XML examples\n"
                 "  - SQL statements\n"
                 "  - Stack traces\n"
                 "  - Timestamped messages\n"
                 "  - Various log levels (ERROR, WARN, INFO, DEBUG, etc.)\n";
}

int Main(int argc, char* argv[])
{
    std::cout << "DebugView++ Test Message Generator - PID: " << GetCurrentProcessId() << std::endl;

    int messageCount = 50;
    int delayMs = 100;
    bool continuous = false;

    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        
        if (arg == "--help" || arg == "-h") {
            PrintUsage();
            return 0;
        }
        else if (arg == "--count" && i + 1 < argc) {
            messageCount = std::stoi(argv[++i]);
            if (messageCount <= 0) {
                std::cerr << "Error: Count must be a positive number\n";
                return 1;
            }
        }
        else if (arg == "--delay" && i + 1 < argc) {
            delayMs = std::stoi(argv[++i]);
            if (delayMs < 0) {
                std::cerr << "Error: Delay must be non-negative\n";
                return 1;
            }
        }
        else if (arg == "--continuous") {
            continuous = true;
        }
        else {
            std::cerr << "Error: Unknown argument '" << arg << "'\n";
            PrintUsage();
            return 1;
        }
    }

    // Validate parameters
    if (messageCount > 10000) {
        std::cout << "Warning: Large message count (" << messageCount << ") - this may take a while\n";
    }

    try {
        TestMessageGenerator generator(messageCount, delayMs, continuous);
        generator.GenerateMessages();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

} // namespace DebugViewTestMessages
} // namespace fusion

int main(int argc, char* argv[])
{
    try {
        return fusion::DebugViewTestMessages::Main(argc, argv);
    }
    catch (const std::exception& ex) {
        std::cerr << "Unhandled exception: " << ex.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
}