#pragma once

#include "DebugViewppLib/LogFile.h"
#include <unordered_map>
#include <memory>
#include <mutex>

namespace fusion {
namespace debugviewpp {

class ClarionQueueManager
{
public:
    ClarionQueueManager();
    ~ClarionQueueManager();

    // Process a Queue message and update internal state
    void ProcessQueueMessage(const Message& message, int lineIndex);

    // Retrieve Queue data by ID
    std::shared_ptr<QueueData> GetQueue(const std::string& queueId) const;

    // Get Queue ID for a given line index
    std::string GetQueueIdForLine(int lineIndex) const;

    // Check if a Queue is complete
    bool IsQueueComplete(const std::string& queueId) const;

    // Clear all Queue data
    void Clear();

    // Get all Queue IDs
    std::vector<std::string> GetAllQueueIds() const;

    // Parse CSV line (handles quoted values)
    static std::vector<std::string> ParseCsvLine(const std::string& line);

private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<QueueData>> m_queues;
    std::unordered_map<int, std::string> m_lineToQueueMap;
};

} // namespace debugviewpp
} // namespace fusion