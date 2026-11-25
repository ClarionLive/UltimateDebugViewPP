// (C) Copyright Gert-Jan de Vos and Jan Wilmans 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <chrono>
#include <Windows.h>

namespace fusion {
namespace debugviewpp {

struct ProcedureNode
{
    std::string name;
    std::string module;
    std::string app;
    double startTime;
    double endTime;
    bool isOpen;
    int depth;
    std::vector<std::shared_ptr<ProcedureNode>> children;
    std::weak_ptr<ProcedureNode> parent;
    
    ProcedureNode(const std::string& name, const std::string& module, const std::string& app, double startTime)
        : name(name), module(module), app(app), startTime(startTime), endTime(0), isOpen(true), depth(0)
    {
    }
};

struct ThreadProcedureState
{
    std::vector<std::shared_ptr<ProcedureNode>> callStack;
    std::vector<std::shared_ptr<ProcedureNode>> rootProcedures;
    int currentDepth = 0;
};

class ClarionProcedureTracker
{
public:
    ClarionProcedureTracker();
    ~ClarionProcedureTracker();
    
    // Track procedure open/close events
    int HandleProcedureOpen(const std::string& threadId, const std::string& procedureName,
                           const std::string& module, const std::string& app, const std::string& processName, double timestamp, int lineIndex = -1);
    int HandleProcedureClose(const std::string& threadId, const std::string& procedureName, const std::string& processName, double timestamp, int lineIndex = -1);
    
    // Get current call depth for a thread
    int GetCallDepth(const std::string& threadId) const;
    
    // Get procedure tree for a thread
    std::vector<std::shared_ptr<ProcedureNode>> GetProcedureTree(const std::string& threadId) const;
    
    // Get all open procedures across all threads
    std::vector<std::pair<std::string, std::shared_ptr<ProcedureNode>>> GetOpenProcedures() const;
    
    // Get all thread IDs that have procedures
    std::vector<std::string> GetAllThreadIds() const;
    
    // Get open procedures for a specific thread
    std::vector<std::shared_ptr<ProcedureNode>> GetOpenProceduresForThread(const std::string& threadId) const;
    
    // Clear tracking data
    void Clear();
    void ClearThread(const std::string& threadId);
    
    // Get statistics
    struct Statistics {
        int totalProceduresCalled;
        int currentlyOpenProcedures;
        int maxCallDepth;
        std::map<std::string, int> procedureCallCounts;
    };
    Statistics GetStatistics() const;

    // New methods for Procedure Stats tab
    struct ProcedureStats {
        int openCount;
        int closeCount;
    };

    // Get aggregated count of currently open procedures (across all threads)
    std::map<std::string, int> GetAggregatedOpenCounts() const;

    // Get lifetime statistics for all procedures
    std::map<std::string, ProcedureStats> GetLifetimeStatistics() const;
    
    // Event tracking for chronological display
    struct Event {
        std::string threadId;
        std::string procedureName;
        std::string module;
        std::string app;
        std::string processName;
        double timestamp;
        bool isOpen;
        int depth;
        double duration;  // For close events
        int lineIndex;    // Source log line index

        Event(const std::string& thread, const std::string& proc, const std::string& mod,
              const std::string& application, const std::string& process, double time, bool open, int d, int lineIdx = -1, double dur = 0.0)
            : threadId(thread), procedureName(proc), module(mod), app(application), processName(process),
              timestamp(time), isOpen(open), depth(d), lineIndex(lineIdx), duration(dur) {}
    };
    
    std::vector<Event> GetChronologicalEvents() const;

private:
    mutable std::mutex m_mutex;
    std::map<std::string, ThreadProcedureState> m_threadStates;
    std::vector<Event> m_events;  // Chronological event log

    // Lifetime statistics tracking
    std::map<std::string, ProcedureStats> m_lifetimeStats;

    ThreadProcedureState& GetOrCreateThreadState(const std::string& threadId);
};

} // namespace debugviewpp
} // namespace fusion