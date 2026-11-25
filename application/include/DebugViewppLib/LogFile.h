// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "DebugviewppLib/Colors.h"
#include "DebugviewppLib/ProcessInfo.h"
#include "IndexedStorageLib/IndexedStorage.h"

namespace fusion {
namespace debugviewpp {

struct Message
{
    Message(double time, FILETIME systemTime, DWORD pid, const std::string& processName, const std::string& msg, COLORREF color = Colors::BackGround);

    double time;
    FILETIME systemTime;
    DWORD processId;
    std::string processName;
    std::string text;
    COLORREF color;
    
    // Clarion-specific fields
    std::string clarionThread;
    std::string clarionApp;
    std::string clarionModule;
    std::string clarionProcedure;
    std::string clarionPrefix;
    bool isClarionFormat = false;
    
    // Clarion procedure tracking fields
    enum class ProcedureEvent { None, Open, Close };
    ProcedureEvent procedureEvent = ProcedureEvent::None;
    int callDepth = 0;  // Nesting level of procedure calls
    double procedureStartTime = 0.0;  // Time when procedure was opened
    double procedureDuration = 0.0;  // Duration for closed procedures

    // Queue-specific fields
    std::string queueId;        // From I: parameter
    bool isQueueMessage = false;
    enum class QueueMessageType { None, Begin, Header, Data, End };
    QueueMessageType queueType = QueueMessageType::None;
    int queueRowIndex = -1;      // Position within queue

    // Error message fields
    bool isErrorMessage = false;
    std::string errorMessage;     // The main error message
    std::string errorCode;        // Error code number
    std::string errorDescription; // Error description text
    std::string errorFile;        // Error file path
    std::string fileError;        // File error description
    std::string fileErrorCode;    // File error code
};

// Queue Data Structure
struct QueueData {
    std::string queueId;
    std::vector<std::string> headers;
    std::vector<std::vector<std::string>> rows;
    int startLineIndex;
    int endLineIndex;
    bool isComplete;
    FILETIME firstMessageTime;
    FILETIME lastMessageTime;
};

// Forward declaration
class ClarionProcedureTracker;
class ClarionQueueManager;

class LogFile
{
public:
    bool Empty() const;
    void Clear();
    void Add(const Message& msg);
    void Append(const LogFile& logfile, int beginIndex, int endIndex);
    
    // Parse Clarion format in a message, optionally updating a tracker
    static void ParseClarionMessage(Message& msg, bool modifyText, ClarionProcedureTracker* tracker, int lineIndex = -1);
    int BeginIndex() const;
    int EndIndex() const;
    int Count() const;
    Message operator[](int i) const;
    std::string GetRawMessage(int i) const;  // Get unparsed message text
    int GetHistorySize() const;
    void SetHistorySize(int size);
    std::vector<std::wstring> GetUniqueProcesses() const;

    // Queue management
    ClarionQueueManager* GetQueueManager();
    const ClarionQueueManager* GetQueueManager() const;

private:
    struct InternalMessage
    {
        InternalMessage(double time, FILETIME systemTime, DWORD uid) :
            time(time),
            systemTime(systemTime),
            uid(uid)
        {
        }

        double time;
        FILETIME systemTime;
        DWORD uid;
    };

    std::vector<InternalMessage> m_messages;
    ProcessInfo m_processInfo;
    mutable indexedstorage::SnappyStorage m_storage;
    //    indexedstorage::VectorStorage m_storage;
    int m_historySize = 0;
    std::unique_ptr<ClarionQueueManager> m_queueManager;
};

// Global accessor for the procedure tracker - DEPRECATED (now using per-view trackers)
// class ClarionProcedureTracker;
// ClarionProcedureTracker& GetProcedureTracker();

} // namespace debugviewpp
} // namespace fusion
