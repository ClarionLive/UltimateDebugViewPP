// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/json.hpp>

namespace fusion {
namespace debugviewpp {

class CMainFrame;

// Thrown by tool handlers for errors the AI should see (bad arguments, unknown tab, ...).
struct McpToolError : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

// Handed to every tool handler. Handlers run on a worker thread; anything that
// touches the log, views or capture must go through RunOnUi.
class McpContext
{
public:
    virtual ~McpContext() = default;
    virtual boost::json::value RunOnUi(std::function<boost::json::value()> fn) = 0;
    virtual bool Cancelled() const = 0;
};

struct McpTool
{
    std::string name;
    std::string title;
    std::string description;
    boost::json::object inputSchema;
    bool readOnly = true;
    std::function<boost::json::value(const boost::json::object& args, McpContext& ctx)> handler;
};

class McpToolRegistry
{
public:
    void Add(McpTool tool);
    const std::vector<McpTool>& Tools() const;
    const McpTool* Find(const std::string& name) const;

private:
    std::vector<McpTool> m_tools;
};

// Registers the Ultimate DebugView++ tool set. The frame must outlive the server.
void RegisterDebugViewTools(McpToolRegistry& registry, CMainFrame& frame);

// Converts text captured from OutputDebugString (often ANSI) into valid UTF-8.
std::string ToUtf8Safe(const std::string& text);

// Folder of a running process' exe, or empty when it is gone or its image name does not match
// expectedName (Windows reuses pids).
std::wstring ExeFolderOfProcess(unsigned long processId, const std::wstring& expectedName);
// Lower-case process name without ".exe", used to look up remembered exe folders.
std::wstring LowerCaseProcessKey(const std::wstring& processName);
std::string WideToUtf8(const std::wstring& text);
std::wstring Utf8ToWide(const std::string& text);

} // namespace debugviewpp
} // namespace fusion
