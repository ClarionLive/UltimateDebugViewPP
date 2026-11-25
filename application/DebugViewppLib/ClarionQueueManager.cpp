#include "DebugViewppLib/ClarionQueueManager.h"
#include <sstream>
#include <algorithm>

namespace fusion {
namespace debugviewpp {

ClarionQueueManager::ClarionQueueManager()
{
}

ClarionQueueManager::~ClarionQueueManager()
{
}

void ClarionQueueManager::ProcessQueueMessage(const Message& message, int lineIndex)
{
    if (!message.isQueueMessage || message.queueId.empty())
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Get or create Queue data
    auto it = m_queues.find(message.queueId);
    if (it == m_queues.end())
    {
        auto queueData = std::make_shared<QueueData>();
        queueData->queueId = message.queueId;
        queueData->isComplete = false;
        queueData->startLineIndex = lineIndex;
        queueData->firstMessageTime = message.systemTime;
        m_queues[message.queueId] = queueData;
        it = m_queues.find(message.queueId);
    }

    auto queueData = it->second;
    queueData->lastMessageTime = message.systemTime;

    // Map line to Queue ID
    m_lineToQueueMap[lineIndex] = message.queueId;

    // Process based on message type
    switch (message.queueType)
    {
    case Message::QueueMessageType::Begin:
        // Reset Queue data for new begin
        queueData->headers.clear();
        queueData->rows.clear();
        queueData->startLineIndex = lineIndex;
        queueData->firstMessageTime = message.systemTime;
        queueData->isComplete = false;
        break;

    case Message::QueueMessageType::Header:
    case Message::QueueMessageType::Data:  // Data type can be headers or data rows
        // Parse data from MSG field
        {
            size_t msgPos = message.text.find("MSG:");
            if (msgPos != std::string::npos)
            {
                std::string dataLine = message.text.substr(msgPos + 4);

                // If headers are empty, this is the header row
                if (queueData->headers.empty())
                {
                    queueData->headers = ParseCsvLine(dataLine);
                }
                else
                {
                    // This is a data row
                    queueData->rows.push_back(ParseCsvLine(dataLine));
                }
            }
        }
        break;

    case Message::QueueMessageType::End:
        queueData->endLineIndex = lineIndex;
        queueData->isComplete = true;
        break;
    }
}

std::shared_ptr<QueueData> ClarionQueueManager::GetQueue(const std::string& queueId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end())
        return it->second;
    return nullptr;
}

std::string ClarionQueueManager::GetQueueIdForLine(int lineIndex) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_lineToQueueMap.find(lineIndex);
    if (it != m_lineToQueueMap.end())
        return it->second;
    return "";
}

bool ClarionQueueManager::IsQueueComplete(const std::string& queueId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_queues.find(queueId);
    if (it != m_queues.end())
        return it->second->isComplete;
    return false;
}

void ClarionQueueManager::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queues.clear();
    m_lineToQueueMap.clear();
}

std::vector<std::string> ClarionQueueManager::GetAllQueueIds() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> ids;
    for (const auto& pair : m_queues)
    {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<std::string> ClarionQueueManager::ParseCsvLine(const std::string& line)
{
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.length(); ++i)
    {
        char c = line[i];

        if (c == '"')
        {
            if (inQuotes && i + 1 < line.length() && line[i + 1] == '"')
            {
                // Escaped quote
                current += '"';
                ++i;
            }
            else
            {
                // Toggle quote state
                inQuotes = !inQuotes;
            }
        }
        else if (c == ',' && !inQuotes)
        {
            // End of field
            result.push_back(current);
            current.clear();
        }
        else
        {
            current += c;
        }
    }

    // Add last field
    if (!current.empty() || !line.empty())
    {
        result.push_back(current);
    }

    return result;
}

} // namespace debugviewpp
} // namespace fusion