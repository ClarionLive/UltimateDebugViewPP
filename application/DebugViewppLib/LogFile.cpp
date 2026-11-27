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
#include "DebugViewppLib/ClarionProcedureTracker.h"
#include "DebugViewppLib/ClarionQueueManager.h"

namespace fusion {
namespace debugviewpp {

// Global procedure tracker instance - DEPRECATED, now using per-view trackers
// static ClarionProcedureTracker g_procedureTracker;

// Forward declaration
class ClarionProcedureTracker;

static std::string TrimSpaces(const std::string& s)
{
    size_t first = s.find_first_not_of(" \t");
    if (first == std::string::npos)
        return std::string();
    size_t last = s.find_last_not_of(" \t");
    return s.substr(first, last - first + 1);
}

// Parses the legacy format emitted by the UltimateDebug class assert hook (ASSERT(0, eqDBG & ...)):
//     [||]X <prefix>,T <line>,P <file>[||]<message>
// Note that T is the source line number (not the thread) and P is the source file (not the procedure),
// so neither is mapped to the thread/procedure fields: that would confuse the procedure tracker.
// Returns false if the text does not match this layout.
static bool ParseLegacyClarionFormat(Message& msg, bool modifyText)
{
    static const std::string marker = "[||]X ";
    static const std::string separator = "[||]";
    if (msg.text.compare(0, marker.size(), marker) != 0)
        return false;

    size_t sepPos = msg.text.find(separator, marker.size());
    if (sepPos == std::string::npos)
        return false;

    // head = "<prefix>,T <line>,P <file>"
    std::string head = msg.text.substr(marker.size(), sepPos - marker.size());
    size_t tPos = head.find(",T ");
    if (tPos == std::string::npos)
        return false;
    size_t pPos = head.find(",P ", tPos + 3);
    if (pPos == std::string::npos)
        return false;

    msg.isClarionFormat = true;
    msg.clarionPrefix = TrimSpaces(head.substr(0, tPos));
    msg.clarionModule = TrimSpaces(head.substr(pPos + 3));
    std::string line = TrimSpaces(head.substr(tPos + 3, pPos - (tPos + 3)));

    if (modifyText)
    {
        std::string text = msg.text.substr(sepPos + separator.size());
        if (!line.empty())
            text = "(L" + line + ") " + text;
        msg.text = text;
    }
    return true;
}

// Helper function to parse Clarion format
static void ParseClarionFormat(Message& msg, bool modifyText = true, ClarionProcedureTracker* tracker = nullptr, int lineIndex = -1)
{
    // Check if message starts with [||]X,
    if (msg.text.size() < 6 || msg.text.substr(0, 6) != "[||]X,")
    {
        ParseLegacyClarionFormat(msg, modifyText);
        return;
    }
    
    msg.isClarionFormat = true;
    
    
    // Parse key:value pairs
    std::string remaining = msg.text.substr(6); // Skip "[||]X,"
    std::string actualMessage;
    
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
            } else if (key == "I") {
                msg.queueId = value;
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
    
    // Check if this is an Error message
    if (msg.clarionPrefix == "[Error]" && !actualMessage.empty()) {
        msg.isErrorMessage = true;

        // Parse error message in format:
        // Message: Open Customers,Errorcode: 52,Error: File Already Open,ErrorFile: n/a,FileError: n/a,FileErrorCode: n/a
        size_t parsePos = 0;
        while (parsePos < actualMessage.length()) {
            // Find the next key-value pair
            size_t colonPos = actualMessage.find(':', parsePos);
            if (colonPos == std::string::npos) break;

            // Find the start of the key (skip any spaces/commas)
            size_t keyStart = parsePos;
            while (keyStart < colonPos && (actualMessage[keyStart] == ' ' || actualMessage[keyStart] == ',')) {
                keyStart++;
            }

            std::string key = actualMessage.substr(keyStart, colonPos - keyStart);

            // Find the value (up to the next comma that precedes a key)
            size_t valueStart = colonPos + 1;
            while (valueStart < actualMessage.length() && actualMessage[valueStart] == ' ') {
                valueStart++;
            }

            // Find the end of the value
            size_t valueEnd = actualMessage.length();
            size_t nextKeyPos = valueStart;
            while (nextKeyPos < actualMessage.length()) {
                size_t nextComma = actualMessage.find(',', nextKeyPos);
                if (nextComma == std::string::npos) break;

                // Check if this comma is followed by a known key
                size_t afterComma = nextComma + 1;
                while (afterComma < actualMessage.length() && actualMessage[afterComma] == ' ') {
                    afterComma++;
                }

                // Check for known keys
                if (afterComma < actualMessage.length()) {
                    if (actualMessage.substr(afterComma, 9) == "Errorcode" ||
                        actualMessage.substr(afterComma, 5) == "Error" ||
                        actualMessage.substr(afterComma, 9) == "ErrorFile" ||
                        actualMessage.substr(afterComma, 9) == "FileError" ||
                        actualMessage.substr(afterComma, 13) == "FileErrorCode") {
                        valueEnd = nextComma;
                        break;
                    }
                }
                nextKeyPos = nextComma + 1;
            }

            std::string value = actualMessage.substr(valueStart, valueEnd - valueStart);

            // Trim trailing spaces from value
            size_t lastNonSpace = value.find_last_not_of(" ");
            if (lastNonSpace != std::string::npos) {
                value = value.substr(0, lastNonSpace + 1);
            }

            // Store the parsed values
            if (key == "Message") {
                msg.errorMessage = value;
            } else if (key == "Errorcode") {
                msg.errorCode = value;
            } else if (key == "Error") {
                msg.errorDescription = value;
            } else if (key == "ErrorFile") {
                msg.errorFile = value;
            } else if (key == "FileError") {
                msg.fileError = value;
            } else if (key == "FileErrorCode") {
                msg.fileErrorCode = value;
            }

            parsePos = valueEnd;
        }
    }
    // Check if this is a Queue message
    else if (msg.clarionPrefix == "[Queue]" && !msg.queueId.empty()) {
        msg.isQueueMessage = true;

        // Determine Queue message type based on MSG content
        if (actualMessage.find("Begin Queue") == 0) {  // Starts with "Begin Queue"
            msg.queueType = Message::QueueMessageType::Begin;
        } else if (actualMessage.find("End Queue") == 0) {  // Starts with "End Queue"
            msg.queueType = Message::QueueMessageType::End;
        } else if (!actualMessage.empty()) {
            // Check if this looks like a header (contains multiple comma-separated values)
            // First data row after Begin Queue is considered the header
            // This will be refined when integrated with the QueueManager
            size_t commaCount = std::count(actualMessage.begin(), actualMessage.end(), ',');
            if (commaCount > 0) {
                // For now, we'll mark it as Data and let the QueueManager determine if it's a header
                msg.queueType = Message::QueueMessageType::Data;
            }
        }
    }

    // Check if this is a procedure tracking message
    // Accept any prefix for now - the OPEN/CLOSE in the message is what matters
    else if (!actualMessage.empty()) {
        // Check if message starts with OPEN or CLOSE
        if (actualMessage.find("OPEN ") == 0) {
            msg.procedureEvent = Message::ProcedureEvent::Open;
            msg.procedureStartTime = msg.time;
            
            // Use P: field if available, otherwise extract from message
            std::string procName = msg.clarionProcedure;
            if (procName.empty() && actualMessage.length() > 5) {
                // Only extract from message if P: field is empty
                procName = actualMessage.substr(5); // Skip "OPEN "
                
                // Trim whitespace and tabs from the extracted name
                size_t start = procName.find_first_not_of(" \t\r\n");
                size_t end = procName.find_last_not_of(" \t\r\n");
                if (start != std::string::npos && end != std::string::npos) {
                    procName = procName.substr(start, end - start + 1);
                }
            }
            
            // Track the procedure open event (only if tracker is provided)
            if (tracker) {
                msg.callDepth = tracker->HandleProcedureOpen(
                    msg.clarionThread, procName,
                    msg.clarionModule, msg.clarionApp, msg.processName, msg.time, lineIndex);
            }
            
            // Store the extracted name back
            msg.clarionProcedure = procName;
            
        } else if (actualMessage.find("CLOSE ") == 0) {
            msg.procedureEvent = Message::ProcedureEvent::Close;
            
            // Use P: field if available, otherwise extract from message
            std::string procName = msg.clarionProcedure;
            if (procName.empty() && actualMessage.length() > 6) {
                // Only extract from message if P: field is empty
                procName = actualMessage.substr(6); // Skip "CLOSE "
                
                // Trim whitespace and tabs from the extracted name
                size_t start = procName.find_first_not_of(" \t\r\n");
                size_t end = procName.find_last_not_of(" \t\r\n");
                if (start != std::string::npos && end != std::string::npos) {
                    procName = procName.substr(start, end - start + 1);
                }
            }
            
            
            // Track the procedure close event (only if tracker is provided)
            if (tracker) {
                msg.callDepth = tracker->HandleProcedureClose(
                    msg.clarionThread, procName, msg.processName, msg.time, lineIndex);
            }
            msg.procedureDuration = msg.time - msg.procedureStartTime;
            
            // Store the extracted name back
            msg.clarionProcedure = procName;
        }
    } else if (msg.isClarionFormat && !msg.clarionThread.empty()) {
        // For regular Clarion messages, get the current call depth
        if (tracker) {
            msg.callDepth = tracker->GetCallDepth(msg.clarionThread);
        }
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

    // Clear Queue manager
    if (m_queueManager)
    {
        m_queueManager->Clear();
    }

    // Global tracker no longer used - clearing is handled per-view
    // g_procedureTracker.Clear();
}

void LogFile::Add(const Message& msg)
{
    auto props = m_processInfo.GetProcessProperties(msg.processId, WStr(msg.processName).str());
    m_messages.emplace_back(InternalMessage(msg.time, msg.systemTime, props.uid));
    m_storage.Add(msg.text);

    // Parse the message to check if it's a Queue message
    Message parsedMsg(msg.time, msg.systemTime, msg.processId, msg.processName, msg.text, msg.color);
    ParseClarionFormat(parsedMsg, false, nullptr);  // Parse without modifying text or tracker

    // Process Queue messages
    if (parsedMsg.isQueueMessage && !parsedMsg.queueId.empty())
    {
        int lineIndex = static_cast<int>(m_messages.size() - 1);
        GetQueueManager()->ProcessQueueMessage(parsedMsg, lineIndex);
    }

    // No longer update global tracker here - this is now handled per-view in CLogView::Add
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
    
    // Parse Clarion format but DON'T update tracker (just retrieving)
    ParseClarionFormat(result, true, nullptr);
    
    return result;
}

Message LogFile::GetUnparsed(int i) const
{
    auto& msg = m_messages[i];
    auto props = m_processInfo.GetProcessProperties(msg.uid);
    return Message(msg.time, msg.systemTime, props.pid, Str(props.name).str(), m_storage[i], props.color);
}

std::string LogFile::GetRawMessage(int i) const
{
    if (i < 0 || i >= Count())
        return "";
    
    // Return the unparsed message directly from storage
    return m_storage[i];
}

int LogFile::GetHistorySize() const
{
    return m_historySize;
}

void LogFile::SetHistorySize(int size)
{
    m_historySize = size;
}

ClarionQueueManager* LogFile::GetQueueManager()
{
    if (!m_queueManager)
    {
        m_queueManager = std::make_unique<ClarionQueueManager>();
    }
    return m_queueManager.get();
}

const ClarionQueueManager* LogFile::GetQueueManager() const
{
    return m_queueManager.get();
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

// This function is deprecated - we now use per-view trackers
// ClarionProcedureTracker& GetProcedureTracker()
// {
//     return g_procedureTracker;
// }

void LogFile::ParseClarionMessage(Message& msg, bool modifyText, ClarionProcedureTracker* tracker, int lineIndex)
{
    ParseClarionFormat(msg, modifyText, tracker, lineIndex);
}

} // namespace debugviewpp
} // namespace fusion
