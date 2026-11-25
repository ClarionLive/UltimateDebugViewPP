// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Asio must be included before anything that pulls in <windows.h>.
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include <atomic>
#include <cctype>
#include <chrono>
#include <future>
#include <list>
#include <mutex>
#include <optional>
#include <thread>

#include "McpServer.h"
#include "McpTools.h"

namespace fusion {
namespace debugviewpp {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;
using tcp = net::ip::tcp;

namespace {

const char* const SupportedProtocolVersions[] = {"2025-06-18", "2025-03-26", "2024-11-05"};
constexpr std::uint64_t MaxBodyBytes = 1024 * 1024;
constexpr std::size_t MaxConnections = 16;
constexpr DWORD IdleReceiveTimeoutMs = 5 * 60 * 1000;
constexpr auto UiCallTimeout = std::chrono::seconds(15);

const char* const ServerInstructions =
    "Ultimate DebugView++ captures Windows debug output (OutputDebugString), including Clarion debug messages.\n"
    "- Line numbers are absolute indices into the captured log. Pass a result's nextLine back as since_line to read only new output.\n"
    "- clearGeneration changes whenever the log is cleared (by you or the user); when it changes, restart from since_line 0.\n"
    "- Typical test loop: insert_marker before running the app, then wait_for_messages for the line you expect, then read_messages from the marker.\n"
    "- Tabs are filtered views of the same log; read_messages with tab reads only what that tab shows.";

bool StartsWith(std::string_view s, std::string_view prefix)
{
    return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool IsDigits(std::string_view s)
{
    if (s.empty())
        return false;
    for (char c : s)
    {
        if (std::isdigit(static_cast<unsigned char>(c)) == 0)
            return false;
    }
    return true;
}

// host is "localhost", "127.0.0.1" or "[::1]", optionally followed by ":port".
bool IsLoopbackHost(std::string_view host)
{
    for (std::string_view name : {"localhost", "127.0.0.1", "[::1]"})
    {
        if (!StartsWith(host, name))
            continue;
        auto rest = host.substr(name.size());
        if (rest.empty() || (rest[0] == ':' && IsDigits(rest.substr(1))))
            return true;
    }
    return false;
}

bool IsLoopbackOrigin(std::string_view origin)
{
    for (std::string_view scheme : {"http://", "https://"})
    {
        if (StartsWith(origin, scheme))
            return IsLoopbackHost(origin.substr(scheme.size()));
    }
    return false;
}

bool ConstantTimeEquals(std::string_view a, std::string_view b)
{
    unsigned char diff = static_cast<unsigned char>(a.size() != b.size());
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        diff |= static_cast<unsigned char>(a[i] ^ (i < b.size() ? b[i] : 0));
    }
    return diff == 0;
}

json::object RpcError(const json::value& id, int code, const std::string& message)
{
    return json::object{{"jsonrpc", "2.0"}, {"id", id}, {"error", json::object{{"code", code}, {"message", message}}}};
}

json::object RpcResult(const json::value& id, json::value result)
{
    return json::object{{"jsonrpc", "2.0"}, {"id", id}, {"result", std::move(result)}};
}

} // namespace

struct McpServer::Impl
{
    explicit Impl(McpToolRegistry& tools) :
        tools(tools)
    {
    }

    struct Connection
    {
        std::shared_ptr<tcp::socket> socket;
        std::shared_ptr<std::atomic<bool>> done;
        std::thread thread;
    };

    class RequestContext : public McpContext
    {
    public:
        explicit RequestContext(Impl& impl) :
            m_impl(impl)
        {
        }
        json::value RunOnUi(std::function<json::value()> fn) override { return m_impl.RunOnUi(std::move(fn)); }
        bool Cancelled() const override { return m_impl.stopping.load(); }

    private:
        Impl& m_impl;
    };

    McpToolRegistry& tools;
    McpServerConfig config;
    net::io_context ioc;
    std::unique_ptr<tcp::acceptor> acceptor;
    std::thread acceptThread;
    std::atomic<bool> running{false};
    std::atomic<bool> stopping{false};
    unsigned short port = 0;
    std::mutex connectionsMutex;
    std::list<Connection> connections;

    void AcceptLoop();
    void ReapFinished();
    void Serve(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::atomic<bool>> done);
    http::response<http::string_body> HandleHttp(const http::request<http::string_body>& req);
    std::optional<json::value> HandleRpc(const json::object& message);
    json::value CallTool(const json::object& params);
    json::value RunOnUi(std::function<json::value()> fn);
};

void McpServer::Impl::AcceptLoop()
{
    while (!stopping)
    {
        auto socket = std::make_shared<tcp::socket>(ioc);
        boost::system::error_code ec;
        acceptor->accept(*socket, ec);
        if (stopping)
            break;
        if (ec)
        {
            if (ec == net::error::operation_aborted || !acceptor->is_open())
                break;
            continue;
        }

        ReapFinished();
        std::lock_guard<std::mutex> lock(connectionsMutex);
        if (connections.size() >= MaxConnections)
        {
            socket->close(ec);
            continue;
        }

        DWORD timeout = IdleReceiveTimeoutMs;
        ::setsockopt(socket->native_handle(), SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));

        auto done = std::make_shared<std::atomic<bool>>(false);
        Connection connection;
        connection.socket = socket;
        connection.done = done;
        connection.thread = std::thread([this, socket, done] { Serve(socket, done); });
        connections.push_back(std::move(connection));
    }
}

void McpServer::Impl::ReapFinished()
{
    std::lock_guard<std::mutex> lock(connectionsMutex);
    for (auto it = connections.begin(); it != connections.end();)
    {
        if (it->done->load())
        {
            if (it->thread.joinable())
                it->thread.join();
            it = connections.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void McpServer::Impl::Serve(std::shared_ptr<tcp::socket> socket, std::shared_ptr<std::atomic<bool>> done)
{
    beast::flat_buffer buffer;
    boost::system::error_code ec;
    while (!stopping)
    {
        http::request_parser<http::string_body> parser;
        parser.body_limit(MaxBodyBytes);
        http::read(*socket, buffer, parser, ec);
        if (ec)
            break;

        auto req = parser.release();
        http::response<http::string_body> res;
        try
        {
            res = HandleHttp(req);
        }
        catch (const std::exception& ex)
        {
            res = http::response<http::string_body>(http::status::internal_server_error, req.version());
            res.set(http::field::content_type, "text/plain");
            res.body() = ex.what();
        }
        res.keep_alive(req.keep_alive());
        res.prepare_payload();
        http::write(*socket, res, ec);
        if (ec || !res.keep_alive())
            break;
    }
    socket->shutdown(tcp::socket::shutdown_both, ec);
    socket->close(ec);
    done->store(true);
}

http::response<http::string_body> McpServer::Impl::HandleHttp(const http::request<http::string_body>& req)
{
    auto reply = [&req](http::status status, const std::string& contentType, std::string body) {
        http::response<http::string_body> res(status, req.version());
        res.set(http::field::server, "UltimateDebugViewPP-MCP");
        if (!contentType.empty())
            res.set(http::field::content_type, contentType);
        res.body() = std::move(body);
        return res;
    };
    auto replyJson = [&reply](http::status status, const json::value& value) {
        return reply(status, "application/json", json::serialize(value));
    };

    std::string_view target(req.target().data(), req.target().size());
    target = target.substr(0, target.find('?'));
    if (target != "/mcp" && target != "/")
        return reply(http::status::not_found, "text/plain", "Not found. The MCP endpoint is /mcp");

    // DNS-rebinding guard: only loopback hosts and origins.
    auto host = req[http::field::host];
    if (!host.empty() && !IsLoopbackHost(std::string_view(host.data(), host.size())))
        return reply(http::status::forbidden, "text/plain", "Forbidden host");
    auto origin = req[http::field::origin];
    if (!origin.empty() && !IsLoopbackOrigin(std::string_view(origin.data(), origin.size())))
        return reply(http::status::forbidden, "text/plain", "Forbidden origin");

    if (!config.token.empty())
    {
        auto auth = req[http::field::authorization];
        std::string expected = "Bearer " + config.token;
        if (!ConstantTimeEquals(std::string_view(auth.data(), auth.size()), expected))
        {
            auto res = reply(http::status::unauthorized, "text/plain", "Missing or wrong bearer token");
            res.set(http::field::www_authenticate, "Bearer");
            return res;
        }
    }

    if (req.method() != http::verb::post)
    {
        auto res = reply(http::status::method_not_allowed, "text/plain", "Use POST (this server does not offer an SSE stream)");
        res.set(http::field::allow, "POST");
        return res;
    }

    boost::system::error_code ec;
    json::value body = json::parse(req.body(), ec);
    if (ec)
        return replyJson(http::status::bad_request, RpcError(nullptr, -32700, "Parse error: " + ec.message()));
    if (body.is_array())
        return replyJson(http::status::bad_request, RpcError(nullptr, -32600, "JSON-RPC batches are not supported"));
    if (!body.is_object())
        return replyJson(http::status::bad_request, RpcError(nullptr, -32600, "Invalid request"));

    auto result = HandleRpc(body.as_object());
    if (!result)
        return reply(http::status::accepted, "", "");
    return replyJson(http::status::ok, *result);
}

std::optional<json::value> McpServer::Impl::HandleRpc(const json::object& message)
{
    auto* method = message.if_contains("method");
    auto* idPtr = message.if_contains("id");
    if (method == nullptr || !method->is_string())
    {
        // A response from the client, or junk: nothing to answer unless it carries an id.
        if (idPtr == nullptr || message.contains("result") || message.contains("error"))
            return std::nullopt;
        return json::value(RpcError(*idPtr, -32600, "Invalid request: missing method"));
    }
    if (idPtr == nullptr)
        return std::nullopt; // notification (initialized, cancelled, ...)

    const json::value& id = *idPtr;
    std::string name(method->as_string());
    json::object params;
    if (auto* p = message.if_contains("params"); p != nullptr && p->is_object())
        params = p->as_object();

    if (name == "initialize")
    {
        std::string requested;
        if (auto* v = params.if_contains("protocolVersion"); v != nullptr && v->is_string())
            requested = std::string(v->as_string());
        std::string version = SupportedProtocolVersions[0];
        for (auto* supported : SupportedProtocolVersions)
        {
            if (requested == supported)
                version = supported;
        }
        json::object result{
            {"protocolVersion", version},
            {"capabilities", json::object{{"tools", json::object{{"listChanged", false}}}}},
            {"serverInfo", json::object{{"name", "ultimate-debugviewpp"}, {"title", "Ultimate DebugView++"}, {"version", config.serverVersion}}},
            {"instructions", ServerInstructions}};
        return json::value(RpcResult(id, std::move(result)));
    }
    if (name == "ping")
        return json::value(RpcResult(id, json::object{}));
    if (name == "tools/list")
    {
        json::array list;
        for (auto& tool : tools.Tools())
        {
            list.push_back(json::object{
                {"name", tool.name},
                {"title", tool.title},
                {"description", tool.description},
                {"inputSchema", tool.inputSchema},
                {"annotations", json::object{{"title", tool.title}, {"readOnlyHint", tool.readOnly}, {"openWorldHint", false}}}});
        }
        return json::value(RpcResult(id, json::object{{"tools", std::move(list)}}));
    }
    if (name == "tools/call")
    {
        auto* toolName = params.if_contains("name");
        if (toolName == nullptr || !toolName->is_string())
            return json::value(RpcError(id, -32602, "tools/call needs a tool name"));
        if (tools.Find(std::string(toolName->as_string())) == nullptr)
            return json::value(RpcError(id, -32602, "Unknown tool: " + std::string(toolName->as_string())));
        return json::value(RpcResult(id, CallTool(params)));
    }
    return json::value(RpcError(id, -32601, "Method not found: " + name));
}

json::value McpServer::Impl::CallTool(const json::object& params)
{
    const McpTool* tool = tools.Find(std::string(params.at("name").as_string()));
    json::object args;
    if (auto* a = params.if_contains("arguments"); a != nullptr && a->is_object())
        args = a->as_object();

    auto errorResult = [](const std::string& message) {
        return json::object{
            {"content", json::array{json::object{{"type", "text"}, {"text", message}}}},
            {"isError", true}};
    };

    try
    {
        RequestContext context(*this);
        json::value value = tool->handler(args, context);
        json::object result{
            {"content", json::array{json::object{{"type", "text"}, {"text", json::serialize(value)}}}},
            {"isError", false}};
        if (value.is_object())
            result["structuredContent"] = value;
        return result;
    }
    catch (const McpToolError& ex)
    {
        return errorResult(ex.what());
    }
    catch (const std::exception& ex)
    {
        return errorResult(std::string("Internal error: ") + ex.what());
    }
}

json::value McpServer::Impl::RunOnUi(std::function<json::value()> fn)
{
    auto promise = std::make_shared<std::promise<json::value>>();
    auto future = promise->get_future();
    bool posted = config.postToUi && config.postToUi([promise, fn = std::move(fn)] {
        try
        {
            promise->set_value(fn());
        }
        catch (...)
        {
            promise->set_exception(std::current_exception());
        }
    });
    if (!posted)
        throw McpToolError("The viewer window is not available");

    auto deadline = std::chrono::steady_clock::now() + UiCallTimeout;
    while (future.wait_for(std::chrono::milliseconds(50)) != std::future_status::ready)
    {
        if (stopping)
            throw McpToolError("The MCP server is shutting down");
        if (std::chrono::steady_clock::now() > deadline)
            throw McpToolError("The viewer did not respond within 15 s (a modal dialog may be open)");
    }
    return future.get();
}

McpServer::McpServer(McpToolRegistry& tools) :
    m_impl(std::make_unique<Impl>(tools))
{
}

McpServer::~McpServer()
{
    Stop();
}

bool McpServer::Start(const McpServerConfig& config, std::string& error)
{
    Stop();
    auto& impl = *m_impl;
    impl.config = config;
    impl.stopping = false;
    try
    {
        tcp::endpoint endpoint(net::ip::make_address("127.0.0.1"), config.port);
        auto acceptor = std::make_unique<tcp::acceptor>(impl.ioc);
        acceptor->open(endpoint.protocol());
        // Exclusive use: never share the port with another listener.
        BOOL exclusive = TRUE;
        ::setsockopt(acceptor->native_handle(), SOL_SOCKET, SO_EXCLUSIVEADDRUSE, reinterpret_cast<const char*>(&exclusive), sizeof(exclusive));
        acceptor->bind(endpoint);
        acceptor->listen(net::socket_base::max_listen_connections);
        impl.port = acceptor->local_endpoint().port();
        impl.acceptor = std::move(acceptor);
    }
    catch (const std::exception& ex)
    {
        error = ex.what();
        impl.acceptor.reset();
        return false;
    }

    impl.running = true;
    impl.acceptThread = std::thread([&impl] { impl.AcceptLoop(); });
    return true;
}

void McpServer::Stop()
{
    auto& impl = *m_impl;
    if (!impl.running)
        return;

    impl.stopping = true;
    boost::system::error_code ec;
    if (impl.acceptor)
    {
        impl.acceptor->cancel(ec);
        impl.acceptor->close(ec);
    }
    if (impl.acceptThread.joinable())
        impl.acceptThread.join();
    impl.acceptor.reset();

    std::list<Impl::Connection> connections;
    {
        std::lock_guard<std::mutex> lock(impl.connectionsMutex);
        connections.swap(impl.connections);
    }
    for (auto& connection : connections)
    {
        connection.socket->shutdown(tcp::socket::shutdown_both, ec);
        connection.socket->close(ec);
    }
    for (auto& connection : connections)
    {
        if (connection.thread.joinable())
            connection.thread.join();
    }
    impl.running = false;
    impl.port = 0;
}

bool McpServer::IsRunning() const
{
    return m_impl->running;
}

unsigned short McpServer::Port() const
{
    return m_impl->port;
}

void McpToolRegistry::Add(McpTool tool)
{
    m_tools.push_back(std::move(tool));
}

const std::vector<McpTool>& McpToolRegistry::Tools() const
{
    return m_tools;
}

const McpTool* McpToolRegistry::Find(const std::string& name) const
{
    for (auto& tool : m_tools)
    {
        if (tool.name == name)
            return &tool;
    }
    return nullptr;
}

} // namespace debugviewpp
} // namespace fusion

// Boost.JSON is used header-only; its implementation is compiled into this one translation unit.
#include <boost/json/src.hpp>
