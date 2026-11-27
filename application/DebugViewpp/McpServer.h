// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <functional>
#include <memory>
#include <string>

namespace fusion {
namespace debugviewpp {

class McpToolRegistry;

struct McpServerConfig
{
    unsigned short port = 7777;
    bool requireToken = false;  // also require "Authorization: Bearer <token>" (16+ characters)
    std::string token;
    std::string serverVersion;  // reported in initialize
    // Runs fn on the UI thread (asynchronously). Returns false if it could not be queued.
    std::function<bool(std::function<void()>)> postToUi;
};

// Model Context Protocol server (Streamable HTTP transport, JSON responses only)
// listening on 127.0.0.1. Tool handlers run on worker threads and reach the UI
// through McpContext::RunOnUi.
class McpServer
{
public:
    explicit McpServer(McpToolRegistry& tools);
    ~McpServer();

    McpServer(const McpServer&) = delete;
    McpServer& operator=(const McpServer&) = delete;

    // Binds synchronously; on failure returns false and fills error.
    bool Start(const McpServerConfig& config, std::string& error);
    void Stop();
    bool IsRunning() const;
    unsigned short Port() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace debugviewpp
} // namespace fusion
