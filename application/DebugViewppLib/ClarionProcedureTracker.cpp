// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "DebugViewppLib/ClarionProcedureTracker.h"
#include <algorithm>
#include <functional>
#include <Windows.h>

namespace fusion {
namespace debugviewpp {

ClarionProcedureTracker::ClarionProcedureTracker()
{
}

ClarionProcedureTracker::~ClarionProcedureTracker()
{
}

int ClarionProcedureTracker::HandleProcedureOpen(const std::string& threadId,
                                                 const std::string& procedureName,
                                                 const std::string& module,
                                                 const std::string& app,
                                                 const std::string& processName,
                                                 double timestamp,
                                                 int lineIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto& state = GetOrCreateThreadState(threadId);
    
    auto node = std::make_shared<ProcedureNode>(procedureName, module, app, timestamp);
    
    if (!state.callStack.empty()) {
        // Add as child to current procedure
        auto parent = state.callStack.back();
        node->parent = parent;
        node->depth = parent->depth + 1;
        parent->children.push_back(node);
    } else {
        // This is a root procedure
        node->depth = 1;
        state.rootProcedures.push_back(node);
    }
    
    state.callStack.push_back(node);
    state.currentDepth = static_cast<int>(state.callStack.size());
    
    
    // Add to chronological event log - use node's depth, not currentDepth
    m_events.emplace_back(threadId, procedureName, module, app, processName, timestamp, true, node->depth, lineIndex);

    // Update lifetime statistics
    m_lifetimeStats[procedureName].openCount++;

    return state.currentDepth;
}

int ClarionProcedureTracker::HandleProcedureClose(const std::string& threadId,
                                                  const std::string& procedureName,
                                                  const std::string& processName,
                                                  double timestamp,
                                                  int lineIndex)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_threadStates.find(threadId);
    
    // If thread state doesn't exist, create one and use the last known depth or 1
    if (it == m_threadStates.end()) {
        // Still log the close event with depth 1 and preserve the procedure name
        m_events.emplace_back(threadId, procedureName, "", "", processName, timestamp, false, 1, lineIndex, 0.0);
        return 0;
    }
    
    auto& state = it->second;
    
    // If call stack is empty but we have thread state, use last known depth
    if (state.callStack.empty()) {
        int depth = state.currentDepth > 0 ? state.currentDepth : 1;
        m_events.emplace_back(threadId, procedureName, "", "", processName, timestamp, false, depth, lineIndex, 0.0);
        return 0;
    }
    
    // Find the procedure in the call stack (should be the last one with this name)
    std::string module;
    std::string app;
    double duration = 0.0;
    int closingDepth = -1;  // Sentinel value to detect if not found
    bool found = false;
    
    // Debug: Log what's in the stack
    
    for (auto rit = state.callStack.rbegin(); rit != state.callStack.rend(); ++rit) {
        if ((*rit)->name == procedureName) {
            (*rit)->endTime = timestamp;
            (*rit)->isOpen = false;
            
            // Capture the module and app before removing
            module = (*rit)->module;
            app = (*rit)->app;
            duration = timestamp - (*rit)->startTime;
            closingDepth = (*rit)->depth;
            found = true;
            
            // Remove from call stack
            state.callStack.erase(std::next(rit).base());
            break;
        }
    }
    
    // If not found, use sensible defaults based on current stack
    if (!found) {
        // Use depth based on current stack size + 1 (as if it was just removed)
        // This preserves the indentation level
        closingDepth = static_cast<int>(state.callStack.size()) + 1;
        if (closingDepth < 1) closingDepth = 1;
    }
    
    state.currentDepth = static_cast<int>(state.callStack.size());
    
    // Add close event to chronological log - ensure depth is at least 1
    if (closingDepth < 1) closingDepth = 1;
    

    m_events.emplace_back(threadId, procedureName, module, app, processName, timestamp, false, closingDepth, lineIndex, duration);

    // Update lifetime statistics
    m_lifetimeStats[procedureName].closeCount++;

    return state.currentDepth;
}

int ClarionProcedureTracker::GetCallDepth(const std::string& threadId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_threadStates.find(threadId);
    if (it == m_threadStates.end()) {
        return 0;
    }
    
    return it->second.currentDepth;
}

std::vector<std::shared_ptr<ProcedureNode>> ClarionProcedureTracker::GetProcedureTree(const std::string& threadId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_threadStates.find(threadId);
    if (it == m_threadStates.end()) {
        return {};
    }
    
    return it->second.rootProcedures;
}

std::vector<std::pair<std::string, std::shared_ptr<ProcedureNode>>> ClarionProcedureTracker::GetOpenProcedures() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<std::pair<std::string, std::shared_ptr<ProcedureNode>>> result;
    
    for (const auto& [threadId, state] : m_threadStates) {
        for (const auto& proc : state.callStack) {
            if (proc->isOpen) {
                result.push_back({threadId, proc});
            }
        }
    }
    
    return result;
}

void ClarionProcedureTracker::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadStates.clear();
    m_events.clear();
    m_lifetimeStats.clear();
}

void ClarionProcedureTracker::ClearThread(const std::string& threadId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_threadStates.erase(threadId);
    // Note: We don't clear events here as they're chronological history
}

ClarionProcedureTracker::Statistics ClarionProcedureTracker::GetStatistics() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    Statistics stats = {};
    
    for (const auto& [threadId, state] : m_threadStates) {
        stats.currentlyOpenProcedures += static_cast<int>(state.callStack.size());
        stats.maxCallDepth = std::max(stats.maxCallDepth, state.currentDepth);
        
        // Count all procedures (including closed ones)
        std::function<void(const std::shared_ptr<ProcedureNode>&)> countProcs;
        countProcs = [&](const std::shared_ptr<ProcedureNode>& node) {
            stats.totalProceduresCalled++;
            stats.procedureCallCounts[node->name]++;
            for (const auto& child : node->children) {
                countProcs(child);
            }
        };
        
        for (const auto& root : state.rootProcedures) {
            countProcs(root);
        }
    }
    
    return stats;
}

ThreadProcedureState& ClarionProcedureTracker::GetOrCreateThreadState(const std::string& threadId)
{
    return m_threadStates[threadId];
}

std::vector<std::string> ClarionProcedureTracker::GetAllThreadIds() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> threadIds;
    
    for (const auto& [threadId, state] : m_threadStates) {
        if (!state.rootProcedures.empty()) {
            threadIds.push_back(threadId);
        }
    }
    
    return threadIds;
}

std::vector<std::shared_ptr<ProcedureNode>> ClarionProcedureTracker::GetOpenProceduresForThread(const std::string& threadId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<ProcedureNode>> openProcs;
    
    auto it = m_threadStates.find(threadId);
    if (it == m_threadStates.end()) {
        return openProcs;
    }
    
    const auto& state = it->second;
    
    // Collect all open procedures from the tree
    std::function<void(const std::shared_ptr<ProcedureNode>&)> collectOpen;
    collectOpen = [&](const std::shared_ptr<ProcedureNode>& node) {
        if (node->isOpen) {
            openProcs.push_back(node);
        }
        for (const auto& child : node->children) {
            collectOpen(child);
        }
    };
    
    for (const auto& root : state.rootProcedures) {
        collectOpen(root);
    }
    
    return openProcs;
}

std::vector<ClarionProcedureTracker::Event> ClarionProcedureTracker::GetChronologicalEvents() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events;
}

std::map<std::string, int> ClarionProcedureTracker::GetAggregatedOpenCounts() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::map<std::string, int> openCounts;

    // Iterate through all threads and count open procedures
    for (const auto& [threadId, state] : m_threadStates) {
        // Helper to count open procedures recursively
        std::function<void(const std::shared_ptr<ProcedureNode>&)> countOpen;
        countOpen = [&](const std::shared_ptr<ProcedureNode>& node) {
            if (node->isOpen) {
                openCounts[node->name]++;
            }
            for (const auto& child : node->children) {
                countOpen(child);
            }
        };

        // Count from all root procedures
        for (const auto& root : state.rootProcedures) {
            countOpen(root);
        }
    }

    return openCounts;
}

std::map<std::string, ClarionProcedureTracker::ProcedureStats> ClarionProcedureTracker::GetLifetimeStatistics() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lifetimeStats;
}

} // namespace debugviewpp
} // namespace fusion