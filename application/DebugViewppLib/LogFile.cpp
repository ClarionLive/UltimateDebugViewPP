// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <vector>
#include <set>
#include <algorithm>
#include <sstream>
#include "CobaltFusion/Str.h"
#include "Win32/Utilities.h"
#include "DebugViewppLib/LogFile.h"

namespace fusion {
namespace debugviewpp {

// Helper function to parse Clarion format
static void ParseClarionFormat(Message& msg, bool modifyText = true)
{
    // Check if message starts with [||]X,
    if (msg.text.size() < 6 || msg.text.substr(0, 6) != "[||]X,")
        return;
    
    msg.isClarionFormat = true;
    
    // Parse key:value pairs
    std::string remaining = msg.text.substr(6); // Skip "[||]X,"
    std::string actualMessage;
    
    // Debug: Let's see what we're parsing
    // OutputDebugStringA(("Parsing Clarion: " + remaining).c_str());
    
    size_t pos = 0;
    while (pos < remaining.length()) {
        // Find the next comma or end of string
        size_t nextComma = remaining.find(',', pos);
        if (nextComma == std::string::npos) {
            nextComma = remaining.length();
        }
        
        std::string pair = remaining.substr(pos, nextComma - pos);
        size_t colonPos = pair.find(':');
        
        if (colonPos != std::string::npos) {
            std::string key = pair.substr(0, colonPos);
            std::string value = pair.substr(colonPos + 1);
            
            if (key == "T") {
                msg.clarionThread = value;
            } else if (key == "P") {
                msg.clarionProcedure = value;
            } else if (key == "M") {
                msg.clarionModule = value;
            } else if (key == "A") {
                msg.clarionApp = value;
            } else if (key == "PRE") {
                msg.clarionPrefix = value;
            } else if (key == "MSG") {
                // MSG is the last field and can contain commas
                actualMessage = value;
                if (nextComma < remaining.length()) {
                    // Append the rest of the string as it's part of the message
                    actualMessage += remaining.substr(nextComma);
                }
                break;
            }
        }
        
        pos = nextComma + 1;
    }
    
    // Only replace the text if we're displaying (not storing)
    if (modifyText && !actualMessage.empty()) {
        msg.text = actualMessage;
    }
}

Message::Message(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& msg, COLORREF color) :
    time(time),
    systemTime(systemTime),
    processId(pid),
    processName(processName),
    text(msg),
    color(color)
{
    // Don't parse here - we'll parse when retrieving from storage
    // This preserves the original text for storage
}

bool LogFile::Empty() const
{
    return m_messages.empty();
}

void LogFile::Clear()
{
    m_messages.clear();
    m_messages.shrink_to_fit();
    m_storage.Clear();
    m_storage.shrink_to_fit();
    m_processInfo.Clear();
}

void LogFile::Add(const Message& msg)
{
    auto props = m_processInfo.GetProcessProperties(msg.processId, WStr(msg.processName).str());
    m_messages.emplace_back(InternalMessage(msg.time, msg.systemTime, props.uid));
    m_storage.Add(msg.text);
}

int LogFile::BeginIndex() const
{
    return 0;
}

int LogFile::EndIndex() const
{
    return static_cast<int>(m_messages.size());
}

int LogFile::Count() const
{
    return static_cast<int>(m_messages.size());
}

Message LogFile::operator[](int i) const
{
    auto& msg = m_messages[i];
    auto props = m_processInfo.GetProcessProperties(msg.uid);
    // Create the message with stored data
    Message result(msg.time, msg.systemTime, props.pid, Str(props.name).str(), m_storage[i], props.color);
    
    // Now parse Clarion format if applicable
    ParseClarionFormat(result);
    
    // Debug: Check if we parsed anything
    /*
    if (result.isClarionFormat) {
        std::string debug = "Clarion parsed - Thread: " + result.clarionThread + 
                          ", App: " + result.clarionApp + 
                          ", Module: " + result.clarionModule +
                          ", Proc: " + result.clarionProcedure;
        OutputDebugStringA(debug.c_str());
    } else if (m_storage[i].find("[||]X") != std::string::npos) {
        OutputDebugStringA(("Found [||]X but not parsed: " + m_storage[i].substr(0, 100)).c_str());
    }
    */
    
    return result;
}

int LogFile::GetHistorySize() const
{
    return m_historySize;
}

void LogFile::SetHistorySize(int size)
{
    m_historySize = size;
}

void LogFile::Append(const LogFile& logfile, int beginIndex, int endIndex)
{
    for (int i = beginIndex; i <= endIndex; ++i)
    {
        Add(logfile[i]);
    }
}

std::vector<std::wstring> LogFile::GetUniqueProcesses() const
{
    std::set<std::wstring> uniqueProcesses;
    
    // Iterate through all messages and collect unique process names
    for (size_t i = 0; i < m_messages.size(); ++i)
    {
        auto props = m_processInfo.GetProcessProperties(m_messages[i].uid);
        if (!props.name.empty())
        {
            uniqueProcesses.insert(props.name);
        }
    }
    
    // Convert set to vector and return (already sorted due to std::set)
    return std::vector<std::wstring>(uniqueProcesses.begin(), uniqueProcesses.end());
}

} // namespace debugviewpp
} // namespace fusion
