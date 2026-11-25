// (C) Copyright ClarionLive 2026.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <map>
#include <optional>
#include <regex>
#include <thread>
#include <utility>

#include "McpTools.h"
#include "MainFrame.h"
#include "LogView.h"
#include "CLogViewTabItem2.h"
#include "DebugViewppLib/LogFile.h"
#include "DebugViewppLib/Filter.h"
#include "DebugViewppLib/FilterType.h"
#include "DebugViewppLib/MatchType.h"
#include "version.h"

namespace fusion {
namespace debugviewpp {

namespace json = boost::json;

// ---------------------------------------------------------------- strings

std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty())
        return {};
    int size = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty())
        return {};
    int size = ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring result(static_cast<size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), size);
    return result;
}

std::string ToUtf8Safe(const std::string& text)
{
    bool ascii = std::all_of(text.begin(), text.end(), [](char c) { return static_cast<unsigned char>(c) < 0x80; });
    if (ascii)
        return text;
    if (::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0) > 0)
        return text;
    // Not UTF-8: OutputDebugStringA text is in the ANSI code page.
    int size = ::MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring wide(static_cast<size_t>(size), L'\0');
    ::MultiByteToWideChar(CP_ACP, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
    return WideToUtf8(wide);
}

namespace {

constexpr int DefaultLimit = 200;
constexpr int MaxLimit = 1000;
constexpr size_t DefaultMaxBytes = 256 * 1024;
constexpr size_t MaxMaxBytes = 1024 * 1024;
constexpr size_t MaxTextBytes = 8 * 1024;
constexpr int MaxScanPerCall = 200000;
constexpr int DefaultWaitSeconds = 30;
constexpr int MaxWaitSeconds = 55;
const wchar_t* const AiTabPrefix = L"AI: ";

std::string Lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::wstring LowerW(std::wstring s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](wchar_t c) { return static_cast<wchar_t>(::towlower(c)); });
    return s;
}

std::string Utf8ToAnsi(const std::string& text)
{
    auto wide = Utf8ToWide(text);
    if (wide.empty())
        return {};
    int size = ::WideCharToMultiByte(CP_ACP, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    std::string result(static_cast<size_t>(size), '\0');
    ::WideCharToMultiByte(CP_ACP, 0, wide.data(), static_cast<int>(wide.size()), result.data(), size, nullptr, nullptr);
    return result;
}

// PWD=..., Password=..., Pwd=..., Passwd=... (connection strings and Clarion owner strings)
std::string MaskSecrets(const std::string& text)
{
    auto lower = Lower(text);
    if (lower.find("pw") == std::string::npos && lower.find("pass") == std::string::npos)
        return text;
    static const std::regex re(R"(((?:\b|_)(?:pwd|password|passwd)\s*=\s*)("[^"]*"|'[^']*'|[^;,\s'")]*))", std::regex::ECMAScript | std::regex::icase);
    return std::regex_replace(text, re, "$1***");
}

std::string TruncateUtf8(const std::string& text, size_t maxBytes, bool& truncated)
{
    truncated = text.size() > maxBytes;
    if (!truncated)
        return text;
    size_t end = maxBytes;
    while (end > 0 && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80)
        --end;
    return text.substr(0, end);
}

std::string IsoTime(const FILETIME& ft)
{
    FILETIME local;
    SYSTEMTIME st;
    if (::FileTimeToLocalFileTime(&ft, &local) == FALSE || ::FileTimeToSystemTime(&local, &st) == FALSE)
        return {};
    char buffer[40];
    std::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02uT%02u:%02u:%02u.%03u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    return buffer;
}

std::string ColorToHex(COLORREF color)
{
    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", GetRValue(color), GetGValue(color), GetBValue(color));
    return buffer;
}

COLORREF HexToColor(const std::string& text, const char* key)
{
    std::string hex = text.size() == 7 && text[0] == '#' ? text.substr(1) : text;
    if (hex.size() != 6 || !std::all_of(hex.begin(), hex.end(), [](unsigned char c) { return std::isxdigit(c) != 0; }))
        throw McpToolError(std::string("'") + key + "' must be a colour like \"#FFFF00\"");
    auto value = std::stoul(hex, nullptr, 16);
    return RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}

// ---------------------------------------------------------------- arguments

const json::value* Arg(const json::object& args, const char* key)
{
    auto* value = args.if_contains(key);
    return value != nullptr && !value->is_null() ? value : nullptr;
}

std::optional<std::string> GetString(const json::object& args, const char* key)
{
    auto* value = Arg(args, key);
    if (value == nullptr)
        return std::nullopt;
    if (!value->is_string())
        throw McpToolError(std::string("'") + key + "' must be a string");
    return std::string(value->as_string());
}

std::string RequireString(const json::object& args, const char* key)
{
    auto value = GetString(args, key);
    if (!value || value->empty())
        throw McpToolError(std::string("'") + key + "' is required");
    return *value;
}

std::optional<long long> GetInt(const json::object& args, const char* key)
{
    auto* value = Arg(args, key);
    if (value == nullptr)
        return std::nullopt;
    if (value->is_int64())
        return value->as_int64();
    if (value->is_uint64())
        return static_cast<long long>(value->as_uint64());
    if (value->is_double() && value->as_double() == static_cast<double>(static_cast<long long>(value->as_double())))
        return static_cast<long long>(value->as_double());
    throw McpToolError(std::string("'") + key + "' must be an integer");
}

std::optional<bool> GetBool(const json::object& args, const char* key)
{
    auto* value = Arg(args, key);
    if (value == nullptr)
        return std::nullopt;
    if (!value->is_bool())
        throw McpToolError(std::string("'") + key + "' must be true or false");
    return value->as_bool();
}

int Clamp(long long value, int low, int high)
{
    return static_cast<int>(std::clamp<long long>(value, low, high));
}

// ---------------------------------------------------------------- line matching

struct LineQuery
{
    std::string containsLower;
    bool hasRegex = false;
    std::regex re;
    std::string processLower;
    std::optional<DWORD> pid;
};

std::regex CompileRegex(const std::string& pattern)
{
    try
    {
        return std::regex(pattern, std::regex::ECMAScript | std::regex::icase);
    }
    catch (const std::regex_error& ex)
    {
        throw McpToolError("Invalid regex '" + pattern + "': " + ex.what());
    }
}

LineQuery ParseQuery(const json::object& args)
{
    LineQuery query;
    if (auto contains = GetString(args, "contains"))
        query.containsLower = Lower(*contains);
    if (auto pattern = GetString(args, "regex"); pattern && !pattern->empty())
    {
        query.re = CompileRegex(*pattern);
        query.hasRegex = true;
    }
    if (auto process = GetString(args, "process"))
        query.processLower = Lower(*process);
    if (auto pid = GetInt(args, "pid"))
        query.pid = static_cast<DWORD>(*pid);
    return query;
}

bool Matches(const LineQuery& query, const Message& msg)
{
    if (query.pid && msg.processId != *query.pid)
        return false;
    if (!query.processLower.empty() && Lower(msg.processName).find(query.processLower) == std::string::npos)
        return false;
    if (!query.containsLower.empty() && Lower(msg.text).find(query.containsLower) == std::string::npos)
        return false;
    if (query.hasRegex && !std::regex_search(msg.text, query.re))
        return false;
    return true;
}

// ---------------------------------------------------------------- schemas

json::object Schema(const char* text)
{
    return json::parse(text).as_object();
}

const char* const TabProperty = R"("tab": {"type": ["string", "integer"], "description": "Tab name (case-insensitive) or 0-based index. Omit to use the whole log."})";

} // namespace

// ---------------------------------------------------------------- bridge into CMainFrame / CLogView (friend of both)

class McpBridge
{
public:
    // Iterates either the whole log or one tab's filtered lines, in log order.
    struct LineSource
    {
        const LogFile* log = nullptr;
        const std::deque<LogLine>* view = nullptr;

        int Count() const { return view != nullptr ? static_cast<int>(view->size()) : log->Count(); }
        int At(int pos) const { return view != nullptr ? (*view)[pos].line : pos; }
        int LowerBound(int index) const
        {
            if (view == nullptr)
                return std::clamp(index, 0, Count());
            auto it = std::lower_bound(view->begin(), view->end(), index, [](const LogLine& l, int v) { return l.line < v; });
            return static_cast<int>(it - view->begin());
        }
    };

    struct ReadOptions
    {
        std::optional<json::value> tab;
        LineQuery query;
        long long since = -1;  // -1 = start (forward) / no lower bound (tail)
        int limit = DefaultLimit;
        size_t maxBytes = DefaultMaxBytes;
        bool tail = false;
        bool collapse = true;
        bool clarion = true;
    };

    // ---- state

    static long long NextLine(CMainFrame& f) { return f.m_seqBase + f.m_logFile.Count(); }
    static int ClearGeneration(CMainFrame& f) { return f.m_clearGeneration; }

    static json::object CaptureJson(CMainFrame& f)
    {
        return json::object{
            {"paused", f.IsPaused()},
            {"win32", f.m_pLocalReader != nullptr},
            {"global", f.m_pGlobalReader != nullptr},
            {"kernel", f.m_pKernelReader != nullptr}};
    }

    static json::object TabJson(CMainFrame& f, int index)
    {
        auto& view = f.GetView(index);
        auto filters = view.GetFilters();
        auto filterCount = filters.messageFilters.size() + filters.processFilters.size() + filters.prefixFilters.size() + filters.procedureFilters.size();
        return json::object{
            {"index", index},
            {"name", WideToUtf8(view.GetName())},
            {"lineCount", static_cast<std::int64_t>(view.m_logLines.size())},
            {"filterCount", static_cast<std::int64_t>(filterCount)},
            {"active", f.GetTabCtrl().GetCurSel() == index},
            {"createdByAi", IsAiTab(view)},
            {"closable", index != 0}};
    }

    static json::array TabsJson(CMainFrame& f)
    {
        json::array tabs;
        for (int i = 0; i < f.GetActualViewCount(); ++i)
            tabs.push_back(TabJson(f, i));
        return tabs;
    }

    static json::object Status(CMainFrame& f, const std::vector<std::string>& toolNames)
    {
        json::array tools;
        for (auto& name : toolNames)
            tools.push_back(json::value(name));
        int current = f.GetTabCtrl().GetCurSel();
        return json::object{
            {"server", "Ultimate DebugView++"},
            {"version", VERSION_STR},
            {"capture", CaptureJson(f)},
            {"firstLine", f.m_seqBase},
            {"nextLine", NextLine(f)},
            {"linesInLog", f.m_logFile.Count()},
            {"clearGeneration", f.m_clearGeneration},
            {"currentTab", current >= 0 && current < f.GetActualViewCount() ? json::value(TabJson(f, current)) : json::value(nullptr)},
            {"tabs", TabsJson(f)},
            {"maskSecrets", f.m_mcpSettings.maskSecrets},
            {"tools", std::move(tools)}};
    }

    // ---- tabs

    static bool IsAiTab(const CLogView& view) { return view.GetName().rfind(AiTabPrefix, 0) == 0; }

    static std::string TabListText(CMainFrame& f)
    {
        std::string text;
        for (int i = 0; i < f.GetActualViewCount(); ++i)
        {
            if (!text.empty())
                text += ", ";
            text += "[" + std::to_string(i) + "] " + WideToUtf8(f.GetView(i).GetName());
        }
        return text;
    }

    static int ResolveTab(CMainFrame& f, const json::value& tab)
    {
        int count = f.GetActualViewCount();
        if (tab.is_int64() || tab.is_uint64())
        {
            long long index = tab.is_int64() ? tab.as_int64() : static_cast<long long>(tab.as_uint64());
            if (index < 0 || index >= count)
                throw McpToolError("No tab at index " + std::to_string(index) + ". Tabs: " + TabListText(f));
            return static_cast<int>(index);
        }
        if (tab.is_string())
        {
            auto wanted = LowerW(Utf8ToWide(std::string(tab.as_string())));
            for (int i = 0; i < count; ++i)
            {
                if (LowerW(f.GetView(i).GetName()) == wanted)
                    return i;
            }
            for (int i = 0; i < count; ++i)  // allow the name without the "AI: " prefix
            {
                if (IsAiTab(f.GetView(i)) && LowerW(f.GetView(i).GetName().substr(4)) == wanted)
                    return i;
            }
            throw McpToolError("No tab named '" + std::string(tab.as_string()) + "'. Tabs: " + TabListText(f));
        }
        throw McpToolError("'tab' must be a tab name or index");
    }

    static int ResolveTabArg(CMainFrame& f, const json::object& args)
    {
        auto* tab = Arg(args, "tab");
        if (tab == nullptr)
            throw McpToolError("'tab' is required. Tabs: " + TabListText(f));
        return ResolveTab(f, *tab);
    }

    static void RequireWritable(CMainFrame& f, int index, const json::object& args)
    {
        if (IsAiTab(f.GetView(index)) || GetBool(args, "allow_user_tab").value_or(false))
            return;
        throw McpToolError("Tab '" + WideToUtf8(f.GetView(index).GetName()) +
                           "' belongs to the user. Only change it if the user asked you to, and then pass allow_user_tab: true.");
    }

    static std::wstring AiTabName(const std::string& name)
    {
        auto wide = Utf8ToWide(name);
        return wide.rfind(AiTabPrefix, 0) == 0 ? wide : AiTabPrefix + wide;
    }

    static json::object CreateTab(CMainFrame& f, const std::string& name, const LogFilter& filter, bool select)
    {
        int previous = f.GetTabCtrl().GetCurSel();
        f.AddFilterView(AiTabName(name), filter);
        int index = f.GetActualViewCount() - 1;
        if (!select && previous >= 0 && previous < f.GetActualViewCount())
            f.SwitchToView(previous);
        return TabJson(f, index);
    }

    static void CloseTab(CMainFrame& f, int index)
    {
        if (index == 0)
            throw McpToolError("The first tab cannot be closed");
        f.CloseView(index);
    }

    static void SwitchTab(CMainFrame& f, int index) { f.SwitchToView(index); }

    static void RenameTab(CMainFrame& f, int index, std::wstring name)
    {
        if (IsAiTab(f.GetView(index)) && name.rfind(AiTabPrefix, 0) != 0)
            name = AiTabPrefix + name;
        if (auto* pTabItem = f.GetTabCtrl().GetItem(index))
            pTabItem->SetText(name.c_str());
        f.GetTabCtrl().UpdateLayout();
        f.GetTabCtrl().Invalidate();
        f.GetView(index).SetName(name);
    }

    static LogFilter GetFilters(CMainFrame& f, int index) { return f.GetView(index).GetFilters(); }
    static void SetFilters(CMainFrame& f, int index, const LogFilter& filter) { f.GetView(index).SetFilters(filter); }

    // ---- capture

    static void Clear(CMainFrame& f) { f.ClearLog(); }

    static void Pause(CMainFrame& f)
    {
        if (!f.IsPaused())
            f.Pause();
        f.UpdateStatusBar();
    }

    static void Resume(CMainFrame& f)
    {
        if (f.IsPaused())
            f.Resume();
        f.UpdateStatusBar();
    }

    static long long InsertMarker(CMainFrame& f, const std::string& text)
    {
        long long cursor = NextLine(f);
        f.m_logSources.AddMessage(Utf8ToAnsi(text));
        return cursor;
    }

    // ---- lines

    static json::object LineJson(CMainFrame& f, int index, bool clarion, int repeat)
    {
        const LogFile& log = f.m_logFile;
        Message msg = log[index];
        auto text = ToUtf8Safe(log.GetRawMessage(index));
        if (f.m_mcpSettings.maskSecrets)
            text = MaskSecrets(text);
        bool truncated = false;
        text = TruncateUtf8(text, MaxTextBytes, truncated);

        json::object obj{
            {"line", f.m_seqBase + index},
            {"time", msg.time},
            {"systemTime", IsoTime(msg.systemTime)},
            {"pid", static_cast<std::int64_t>(msg.processId)},
            {"process", ToUtf8Safe(msg.processName)},
            {"text", text}};
        if (truncated)
            obj["textTruncated"] = true;
        if (repeat > 1)
            obj["repeat"] = repeat;
        if (clarion && msg.isClarionFormat)
        {
            obj["clarion"] = json::object{
                {"thread", ToUtf8Safe(msg.clarionThread)},
                {"app", ToUtf8Safe(msg.clarionApp)},
                {"module", ToUtf8Safe(msg.clarionModule)},
                {"procedure", ToUtf8Safe(msg.clarionProcedure)},
                {"prefix", ToUtf8Safe(msg.clarionPrefix)}};
        }
        return obj;
    }

    static LineSource Source(CMainFrame& f, const std::optional<json::value>& tab, json::value& tabJson)
    {
        LineSource source;
        source.log = &f.m_logFile;
        if (tab)
        {
            int index = ResolveTab(f, *tab);
            source.view = &f.GetView(index).m_logLines;
            tabJson = json::object{{"index", index}, {"name", WideToUtf8(f.GetView(index).GetName())}};
        }
        return source;
    }

    // Maps a cursor to a log index; reports lines lost to earlier clears.
    static int StartIndex(CMainFrame& f, long long since, long long& lost)
    {
        lost = 0;
        if (since < 0)
            return 0;
        if (since < f.m_seqBase)
        {
            lost = f.m_seqBase - since;
            return 0;
        }
        return static_cast<int>(std::min<long long>(since - f.m_seqBase, f.m_logFile.Count()));
    }

    static json::object Read(CMainFrame& f, const ReadOptions& o)
    {
        const LogFile& log = f.m_logFile;
        json::value tabJson = nullptr;
        LineSource source = Source(f, o.tab, tabJson);
        long long lost = 0;
        int startIndex = StartIndex(f, o.since, lost);

        struct Group
        {
            int pos;
            int index;
            int repeat;
        };
        std::vector<Group> groups;
        int end = source.Count();
        int pos = 0;
        bool olderAvailable = false;

        if (!o.tail)
        {
            pos = source.LowerBound(startIndex);
            std::string lastText;
            DWORD lastPid = 0;
            for (int scanned = 0; pos < end && scanned < MaxScanPerCall; ++scanned)
            {
                int index = source.At(pos);
                Message msg = log.GetUnparsed(index);
                if (Matches(o.query, msg))
                {
                    if (o.collapse && !groups.empty() && msg.processId == lastPid && msg.text == lastText)
                    {
                        ++groups.back().repeat;
                    }
                    else
                    {
                        if (static_cast<int>(groups.size()) == o.limit)
                            break;  // this line starts the next page
                        groups.push_back({pos, index, 1});
                        lastText = msg.text;
                        lastPid = msg.processId;
                    }
                }
                ++pos;
            }
        }
        else
        {
            int lowest = source.LowerBound(startIndex);
            int p = end - 1;
            std::string lastText;
            DWORD lastPid = 0;
            for (int scanned = 0; p >= lowest && scanned < MaxScanPerCall; ++scanned, --p)
            {
                int index = source.At(p);
                Message msg = log.GetUnparsed(index);
                if (!Matches(o.query, msg))
                    continue;
                if (o.collapse && !groups.empty() && msg.processId == lastPid && msg.text == lastText)
                {
                    ++groups.back().repeat;
                    groups.back().pos = p;
                    groups.back().index = index;
                    continue;
                }
                if (static_cast<int>(groups.size()) == o.limit)
                {
                    olderAvailable = true;
                    break;
                }
                groups.push_back({p, index, 1});
                lastText = msg.text;
                lastPid = msg.processId;
            }
            if (p >= lowest)
                olderAvailable = true;
            pos = end;
        }

        json::array lines;
        size_t bytes = 0;
        bool truncated = false;
        int nextPos = pos;
        if (!o.tail)
        {
            for (auto& group : groups)
            {
                auto obj = LineJson(f, group.index, o.clarion, group.repeat);
                size_t size = json::serialize(obj).size();
                if (!lines.empty() && bytes + size > o.maxBytes)
                {
                    truncated = true;
                    nextPos = group.pos;
                    break;
                }
                bytes += size;
                lines.push_back(std::move(obj));
            }
        }
        else
        {
            // groups are newest first; keep the newest that fit, then return oldest first
            std::vector<json::object> kept;
            for (auto& group : groups)
            {
                auto obj = LineJson(f, group.index, o.clarion, group.repeat);
                size_t size = json::serialize(obj).size();
                if (!kept.empty() && bytes + size > o.maxBytes)
                {
                    truncated = true;
                    olderAvailable = true;
                    break;
                }
                bytes += size;
                kept.push_back(std::move(obj));
            }
            for (auto it = kept.rbegin(); it != kept.rend(); ++it)
                lines.push_back(std::move(*it));
        }

        long long nextLine = nextPos < source.Count() ? f.m_seqBase + source.At(nextPos) : NextLine(f);
        json::object result{
            {"lines", std::move(lines)},
            {"nextLine", nextLine},
            {"more", !o.tail && nextPos < source.Count()},
            {"truncated", truncated},
            {"clearGeneration", f.m_clearGeneration}};
        if (o.tail)
            result["olderAvailable"] = olderAvailable;
        if (lost > 0)
            result["lost"] = lost;
        if (!tabJson.is_null())
            result["tab"] = tabJson;
        return result;
    }

    static json::object Search(CMainFrame& f, const std::optional<json::value>& tab, const LineQuery& query, long long since, int context, int limit, size_t maxBytes)
    {
        const LogFile& log = f.m_logFile;
        json::value tabJson = nullptr;
        LineSource source = Source(f, tab, tabJson);
        long long lost = 0;
        int pos = source.LowerBound(StartIndex(f, since, lost));
        int end = source.Count();

        auto contextLines = [&](int from, int to) {
            json::array array;
            for (int p = std::max(from, 0); p < std::min(to, end); ++p)
                array.push_back(LineJson(f, source.At(p), false, 1));
            return array;
        };

        json::array matches;
        size_t bytes = 0;
        bool truncated = false;
        int scanned = 0;
        for (; pos < end && scanned < MaxScanPerCall; ++pos, ++scanned)
        {
            Message msg = log.GetUnparsed(source.At(pos));
            if (!Matches(query, msg))
                continue;
            if (static_cast<int>(matches.size()) == limit)
                break;
            json::object match{
                {"match", LineJson(f, source.At(pos), true, 1)},
                {"before", contextLines(pos - context, pos)},
                {"after", contextLines(pos + 1, pos + 1 + context)}};
            size_t size = json::serialize(match).size();
            if (!matches.empty() && bytes + size > maxBytes)
            {
                truncated = true;
                break;
            }
            bytes += size;
            matches.push_back(std::move(match));
        }

        long long nextLine = pos < end ? f.m_seqBase + source.At(pos) : NextLine(f);
        json::object result{
            {"matches", std::move(matches)},
            {"nextLine", nextLine},
            {"more", pos < end},
            {"truncated", truncated},
            {"clearGeneration", f.m_clearGeneration}};
        if (lost > 0)
            result["lost"] = lost;
        if (!tabJson.is_null())
            result["tab"] = tabJson;
        return result;
    }

    static json::object Processes(CMainFrame& f)
    {
        struct Stats
        {
            long long count = 0;
            int first = 0;
            int last = 0;
            FILETIME firstTime{};
            FILETIME lastTime{};
        };
        const LogFile& log = f.m_logFile;
        int count = log.Count();
        constexpr int MaxScan = 500000;
        int start = std::max(0, count - MaxScan);
        std::map<std::pair<DWORD, std::string>, Stats> stats;
        for (int i = start; i < count; ++i)
        {
            Message msg = log.GetUnparsed(i);
            auto& s = stats[{msg.processId, msg.processName}];
            if (s.count++ == 0)
            {
                s.first = i;
                s.firstTime = msg.systemTime;
            }
            s.last = i;
            s.lastTime = msg.systemTime;
        }

        std::vector<std::pair<std::pair<DWORD, std::string>, Stats>> sorted(stats.begin(), stats.end());
        std::sort(sorted.begin(), sorted.end(), [](auto& a, auto& b) { return a.second.last > b.second.last; });
        json::array processes;
        for (auto& [key, s] : sorted)
        {
            if (processes.size() >= 200)
                break;
            processes.push_back(json::object{
                {"pid", static_cast<std::int64_t>(key.first)},
                {"process", ToUtf8Safe(key.second)},
                {"lineCount", s.count},
                {"firstLine", f.m_seqBase + s.first},
                {"lastLine", f.m_seqBase + s.last},
                {"firstSeen", IsoTime(s.firstTime)},
                {"lastSeen", IsoTime(s.lastTime)}});
        }
        json::object result{{"processes", std::move(processes)}, {"scannedFromLine", f.m_seqBase + start}, {"nextLine", NextLine(f)}};
        return result;
    }
};

// ---------------------------------------------------------------- filters

namespace {

const char* const Categories[] = {"message", "process", "prefix", "procedure"};

std::vector<Filter>& Category(LogFilter& filter, const std::string& category)
{
    if (category == "message")
        return filter.messageFilters;
    if (category == "process")
        return filter.processFilters;
    if (category == "prefix")
        return filter.prefixFilters;
    if (category == "procedure")
        return filter.procedureFilters;
    throw McpToolError("'category' must be one of message, process, prefix, procedure");
}

MatchType::type ParseMatchType(const std::string& text)
{
    auto wanted = Lower(text);
    std::string names;
    for (int i = 0; i <= 4; ++i)
    {
        auto name = Lower(MatchTypeToString(IntToMatchType(i)));
        if (name == wanted)
            return IntToMatchType(i);
        names += (names.empty() ? "" : ", ") + name;
    }
    throw McpToolError("'matchType' must be one of " + names);
}

FilterType::type ParseFilterType(const std::string& text)
{
    auto wanted = Lower(text);
    std::string names;
    for (int i = 0; i <= 10; ++i)
    {
        auto name = Lower(FilterTypeToString(IntToFilterType(i)));
        if (name == wanted)
            return IntToFilterType(i);
        names += (names.empty() ? "" : ", ") + name;
    }
    throw McpToolError("'filterType' must be one of " + names);
}

json::object FilterJson(const Filter& filter, const std::string& category, size_t index)
{
    return json::object{
        {"category", category},
        {"index", static_cast<std::int64_t>(index)},
        {"text", ToUtf8Safe(filter.text)},
        {"matchType", Lower(MatchTypeToString(filter.matchType))},
        {"filterType", Lower(FilterTypeToString(filter.filterType))},
        {"bgColor", ColorToHex(filter.bgColor)},
        {"fgColor", ColorToHex(filter.fgColor)},
        {"enabled", filter.enable}};
}

json::object FiltersJson(LogFilter filter)
{
    json::array list;
    for (auto* category : Categories)
    {
        auto& filters = Category(filter, category);
        for (size_t i = 0; i < filters.size(); ++i)
            list.push_back(FilterJson(filters[i], category, i));
    }
    return json::object{{"filters", std::move(list)}};
}

// Parses {category, text, matchType, filterType, bgColor, fgColor, enabled}; returns category + filter.
std::pair<std::string, Filter> ParseFilter(const json::object& obj)
{
    auto category = Lower(GetString(obj, "category").value_or("message"));
    LogFilter probe;
    Category(probe, category);  // throws on an unknown category
    auto text = RequireString(obj, "text");
    auto matchType = ParseMatchType(GetString(obj, "matchType").value_or("simple"));
    auto filterType = ParseFilterType(GetString(obj, "filterType").value_or("include"));
    bool colouring = filterType == FilterType::Highlight || filterType == FilterType::Token || filterType == FilterType::MatchColor;
    COLORREF bg = colouring ? RGB(255, 255, 0) : RGB(255, 255, 255);
    COLORREF fg = RGB(0, 0, 0);
    if (auto value = GetString(obj, "bgColor"))
        bg = HexToColor(*value, "bgColor");
    if (auto value = GetString(obj, "fgColor"))
        fg = HexToColor(*value, "fgColor");
    bool enabled = GetBool(obj, "enabled").value_or(true);
    try
    {
        return {category, Filter(Utf8ToAnsi(text), matchType, filterType, bg, fg, enabled)};
    }
    catch (const std::regex_error& ex)
    {
        throw McpToolError("Invalid filter pattern '" + text + "': " + ex.what());
    }
}

LogFilter ParseFilterList(const json::value* value)
{
    LogFilter filter;
    if (value == nullptr)
        return filter;
    if (!value->is_array())
        throw McpToolError("'filters' must be an array of filter objects");
    for (auto& item : value->as_array())
    {
        if (!item.is_object())
            throw McpToolError("each filter must be an object with at least 'text'");
        auto [category, parsed] = ParseFilter(item.as_object());
        Category(filter, category).push_back(std::move(parsed));
    }
    return filter;
}

std::pair<std::string, size_t> FilterAddress(const json::object& args)
{
    auto category = Lower(RequireString(args, "category"));
    auto index = GetInt(args, "index");
    if (!index || *index < 0)
        throw McpToolError("'index' is required (see get_filters)");
    return {category, static_cast<size_t>(*index)};
}

// ---------------------------------------------------------------- registration helpers

void AddTool(McpToolRegistry& registry, const char* name, const char* title, bool readOnly, const char* schema, const char* description,
             std::function<json::value(const json::object&, McpContext&)> handler)
{
    McpTool tool;
    tool.name = name;
    tool.title = title;
    tool.readOnly = readOnly;
    tool.inputSchema = Schema(schema);
    tool.description = description;
    tool.handler = std::move(handler);
    registry.Add(std::move(tool));
}

std::string Props(std::initializer_list<const char*> parts)
{
    std::string text = R"({"type": "object", "properties": {)";
    bool first = true;
    for (auto* part : parts)
    {
        if (!first)
            text += ", ";
        text += part;
        first = false;
    }
    return text + "}";
}

std::string Close(std::string text, const char* required = nullptr)
{
    if (required != nullptr)
        text += std::string(R"(, "required": )") + required;
    return text + ", \"additionalProperties\": false}";
}

const char* const P_Since = R"("since_line": {"type": "integer", "minimum": 0, "description": "Cursor: return lines with line >= since_line. Use nextLine from a previous result, get_status or insert_marker."})";
const char* const P_Contains = R"("contains": {"type": "string", "description": "Case-insensitive substring the line text must contain."})";
const char* const P_Regex = R"("regex": {"type": "string", "description": "Case-insensitive ECMAScript regex the line text must match."})";
const char* const P_Process = R"("process": {"type": "string", "description": "Case-insensitive substring of the process name, e.g. \"myapp.exe\"."})";
const char* const P_Pid = R"("pid": {"type": "integer", "description": "Only lines from this process id."})";
const char* const P_MaxBytes = R"("max_bytes": {"type": "integer", "minimum": 1024, "maximum": 1048576, "description": "Response size budget in bytes (default 262144). Lines beyond it are left for the next call (truncated: true)."})";
const char* const P_AllowUser = R"("allow_user_tab": {"type": "boolean", "description": "Required to change a tab you did not create (tabs you create are named \"AI: ...\"). Only set it when the user asked you to change that tab."})";
const char* const P_FilterFields =
    R"("category": {"type": "string", "enum": ["message", "process", "prefix", "procedure"], "description": "What the filter matches: message text (default), process name, Clarion prefix or Clarion procedure."},
       "text": {"type": "string", "description": "Pattern text."},
       "matchType": {"type": "string", "enum": ["simple", "wildcard", "regex", "regexgroups", "regexcase"], "description": "How text is matched (default simple = substring)."},
       "filterType": {"type": "string", "enum": ["include", "exclude", "highlight", "token", "stop", "track", "once", "clear", "beep", "matchcolor", "bookmark"], "description": "What a match does (default include)."},
       "bgColor": {"type": "string", "description": "Background colour #RRGGBB (highlight/token default #FFFF00)."},
       "fgColor": {"type": "string", "description": "Text colour #RRGGBB."},
       "enabled": {"type": "boolean", "description": "Default true."})";

std::string FilterObjectSchema()
{
    return std::string(R"({"type": "object", "properties": {)") + P_FilterFields + R"(}, "required": ["text"]})";
}

void ReadOptionsFromArgs(const json::object& args, McpBridge::ReadOptions& o)
{
    if (auto* tab = Arg(args, "tab"))
        o.tab = *tab;
    o.query = ParseQuery(args);
    o.since = GetInt(args, "since_line").value_or(-1);
    o.limit = Clamp(GetInt(args, "limit").value_or(DefaultLimit), 1, MaxLimit);
    o.maxBytes = static_cast<size_t>(Clamp(GetInt(args, "max_bytes").value_or(static_cast<long long>(DefaultMaxBytes)), 1024, static_cast<int>(MaxMaxBytes)));
    o.tail = GetBool(args, "tail").value_or(false);
    o.collapse = GetBool(args, "collapse_repeats").value_or(true);
}

} // namespace

void RegisterDebugViewTools(McpToolRegistry& registry, CMainFrame& frame)
{
    CMainFrame* f = &frame;
    using B = McpBridge;

    // ------------------------------------------------ status and reading

    AddTool(registry, "get_status", "Get viewer status", true, R"({"type": "object", "properties": {}, "additionalProperties": false})",
            "Server version, available tools, capture state (paused, win32/global/kernel), the current cursor (nextLine), "
            "clearGeneration and all tabs. Call this first; pass nextLine as since_line later to read only newer output.",
            [f, &registry](const json::object&, McpContext& ctx) {
                std::vector<std::string> names;
                for (auto& tool : registry.Tools())
                    names.push_back(tool.name);
                return ctx.RunOnUi([f, names] { return json::value(B::Status(*f, names)); });
            });

    AddTool(registry, "read_messages", "Read debug messages", true,
            Close(Props({TabProperty, P_Since, P_Contains, P_Regex, P_Process, P_Pid,
                         R"("limit": {"type": "integer", "minimum": 1, "maximum": 1000, "description": "Max lines (default 200). Folded repeats count once."})",
                         R"("tail": {"type": "boolean", "description": "Return the newest lines instead of reading forward from since_line."})",
                         R"("collapse_repeats": {"type": "boolean", "description": "Fold consecutive identical lines from the same process into one with a repeat count (default true)."})",
                         P_MaxBytes}))
                .c_str(),
            "Read captured lines from the whole log or one tab, oldest first. Each line has line (cursor number), time, systemTime, pid, process, text "
            "and Clarion fields when present. Returns nextLine: pass it as since_line next time to get only new output. more: true means another page is "
            "waiting; truncated: true means the byte budget was hit. lost > 0 means a clear removed lines you had not read yet. "
            "Passwords such as PWD=... are masked unless the user turned masking off.",
            [f](const json::object& args, McpContext& ctx) {
                B::ReadOptions o;
                ReadOptionsFromArgs(args, o);
                return ctx.RunOnUi([f, o] { return json::value(B::Read(*f, o)); });
            });

    AddTool(registry, "search_messages", "Search with context", true,
            Close(Props({TabProperty, P_Since, P_Process, P_Pid,
                         R"("pattern": {"type": "string", "description": "Text to find (case-insensitive substring, or a regex when regex is true)."})",
                         R"("regex": {"type": "boolean", "description": "Treat pattern as an ECMAScript regex (default false)."})",
                         R"("context": {"type": "integer", "minimum": 0, "maximum": 10, "description": "Lines of context before and after each match (default 2)."})",
                         R"("limit": {"type": "integer", "minimum": 1, "maximum": 100, "description": "Max matches (default 20)."})",
                         P_MaxBytes}),
                  R"(["pattern"])")
                .c_str(),
            "Find lines matching a pattern and return each with surrounding lines (context comes from the same tab when tab is given). "
            "Use nextLine as since_line to continue after the last match.",
            [f](const json::object& args, McpContext& ctx) {
                auto pattern = RequireString(args, "pattern");
                json::object queryArgs;
                if (GetBool(args, "regex").value_or(false))
                    queryArgs["regex"] = pattern;
                else
                    queryArgs["contains"] = pattern;
                if (auto* v = Arg(args, "process"))
                    queryArgs["process"] = *v;
                if (auto* v = Arg(args, "pid"))
                    queryArgs["pid"] = *v;
                LineQuery query = ParseQuery(queryArgs);
                std::optional<json::value> tab;
                if (auto* v = Arg(args, "tab"))
                    tab = *v;
                long long since = GetInt(args, "since_line").value_or(-1);
                int context = Clamp(GetInt(args, "context").value_or(2), 0, 10);
                int limit = Clamp(GetInt(args, "limit").value_or(20), 1, 100);
                auto maxBytes = static_cast<size_t>(Clamp(GetInt(args, "max_bytes").value_or(static_cast<long long>(DefaultMaxBytes)), 1024, static_cast<int>(MaxMaxBytes)));
                return ctx.RunOnUi([f, tab, query, since, context, limit, maxBytes] { return json::value(B::Search(*f, tab, query, since, context, limit, maxBytes)); });
            });

    AddTool(registry, "list_processes", "List processes in the log", true, R"({"type": "object", "properties": {}, "additionalProperties": false})",
            "Processes that wrote to the log (newest first): pid, process name, lineCount, first/last line and first/last seen time. "
            "Windows reuses PIDs, so a restarted app can share a pid with an old run; the viewer does not record process start times, so use "
            "firstSeen/firstLine to tell runs apart. Scans at most the newest 500,000 lines.",
            [f](const json::object&, McpContext& ctx) { return ctx.RunOnUi([f] { return json::value(B::Processes(*f)); }); });

    // ------------------------------------------------ control

    AddTool(registry, "wait_for_messages", "Wait for messages", true,
            Close(Props({TabProperty, P_Since, P_Contains, P_Regex, P_Process, P_Pid,
                         R"("count": {"type": "integer", "minimum": 1, "maximum": 100, "description": "Return once this many matching lines have arrived (default 1)."})",
                         R"("timeout_seconds": {"type": "integer", "minimum": 0, "maximum": 55, "description": "Give up after this long (default 30, max 55). On timeout just call again with the returned nextLine."})"}))
                .c_str(),
            "Block until matching lines arrive, without freezing the viewer. Pass since_line from insert_marker or get_status taken BEFORE you launch "
            "the app, so a line that arrives before this call is not missed; without since_line it only waits for lines newer than this call. "
            "Returns matched, timedOut, lines, and nextLine to continue from.",
            [f](const json::object& args, McpContext& ctx) {
                B::ReadOptions o;
                if (auto* tab = Arg(args, "tab"))
                    o.tab = *tab;
                o.query = ParseQuery(args);
                o.collapse = false;
                int count = Clamp(GetInt(args, "count").value_or(1), 1, 100);
                int timeout = Clamp(GetInt(args, "timeout_seconds").value_or(DefaultWaitSeconds), 0, MaxWaitSeconds);
                long long cursor = GetInt(args, "since_line").value_or(-1);
                if (cursor < 0)
                    cursor = ctx.RunOnUi([f] { return json::value(B::NextLine(*f)); }).as_int64();

                auto start = std::chrono::steady_clock::now();
                auto deadline = start + std::chrono::seconds(timeout);
                json::array found;
                long long lost = 0;
                json::value generation = nullptr;
                for (;;)
                {
                    o.since = cursor;
                    o.limit = count - static_cast<int>(found.size());
                    auto result = ctx.RunOnUi([f, o] { return json::value(B::Read(*f, o)); }).as_object();
                    for (auto& line : result["lines"].as_array())
                        found.push_back(line);
                    cursor = result["nextLine"].as_int64();
                    generation = result["clearGeneration"];
                    if (auto* l = result.if_contains("lost"))
                        lost += l->as_int64();

                    bool matched = static_cast<int>(found.size()) >= count;
                    bool timedOut = !matched && std::chrono::steady_clock::now() >= deadline;
                    if (matched || timedOut)
                    {
                        json::object reply{
                            {"matched", matched},
                            {"timedOut", timedOut},
                            {"lines", std::move(found)},
                            {"nextLine", cursor},
                            {"clearGeneration", generation},
                            {"waitedMs", std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count()}};
                        if (lost > 0)
                            reply["lost"] = lost;
                        return json::value(std::move(reply));
                    }
                    if (ctx.Cancelled())
                        throw McpToolError("The MCP server is shutting down");
                    if (!result["more"].as_bool())
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            });

    AddTool(registry, "insert_marker", "Insert a marker line", false,
            Close(Props({R"("text": {"type": "string", "maxLength": 2000, "description": "Marker text, e.g. \"=== test 3: open BINDOC ===\"."})"}), R"(["text"])").c_str(),
            "Write a marker line into the log to bracket a test run. Returns cursor: the marker appears at or after it, so pass cursor as since_line to "
            "wait_for_messages / read_messages to see only what happened after the marker.",
            [f](const json::object& args, McpContext& ctx) {
                auto text = RequireString(args, "text");
                if (text.size() > 2000)
                    throw McpToolError("'text' is limited to 2000 characters");
                return ctx.RunOnUi([f, text] {
                    long long cursor = B::InsertMarker(*f, text);
                    return json::value(json::object{{"cursor", cursor}, {"text", text}});
                });
            });

    AddTool(registry, "clear_log", "Clear the log", false, R"({"type": "object", "properties": {}, "additionalProperties": false})",
            "Clear all captured lines from the log and every tab (same as the Clear button). Cursors keep counting up; the result's nextLine is the "
            "new starting cursor and clearGeneration increases.",
            [f](const json::object&, McpContext& ctx) {
                return ctx.RunOnUi([f] {
                    B::Clear(*f);
                    return json::value(json::object{{"cleared", true}, {"nextLine", B::NextLine(*f)}, {"clearGeneration", B::ClearGeneration(*f)}});
                });
            });

    AddTool(registry, "pause_capture", "Pause capture", false, R"({"type": "object", "properties": {}, "additionalProperties": false})",
            "Stop capturing new debug output (same as the Pause button). Lines already captured stay.",
            [f](const json::object&, McpContext& ctx) {
                return ctx.RunOnUi([f] {
                    B::Pause(*f);
                    return json::value(json::object{{"capture", B::CaptureJson(*f)}});
                });
            });

    AddTool(registry, "resume_capture", "Resume capture", false, R"({"type": "object", "properties": {}, "additionalProperties": false})",
            "Start capturing debug output again. If another debug viewer holds the capture, the viewer shows an error to the user and this call times out.",
            [f](const json::object&, McpContext& ctx) {
                return ctx.RunOnUi([f] {
                    B::Resume(*f);
                    return json::value(json::object{{"capture", B::CaptureJson(*f)}});
                });
            });

    // ------------------------------------------------ tabs

    AddTool(registry, "list_tabs", "List tabs", true, R"({"type": "object", "properties": {}, "additionalProperties": false})",
            "All tabs with index, name, lineCount (lines the tab shows), filterCount, active, createdByAi and closable.",
            [f](const json::object&, McpContext& ctx) { return ctx.RunOnUi([f] { return json::value(json::object{{"tabs", B::TabsJson(*f)}}); }); });

    AddTool(registry, "create_tab", "Create a filtered tab", false,
            Close(Props({R"("name": {"type": "string", "description": "Tab name; \"AI: \" is prefixed so the user can tell your tabs apart."})",
                         (std::string(R"("filters": {"type": "array", "description": "Optional filters for the new tab.", "items": )") + FilterObjectSchema() + "}").c_str(),
                         R"("select": {"type": "boolean", "description": "Switch to the new tab (default true)."})"}),
                  R"(["name"])")
                .c_str(),
            "Create a new tab showing the log through its own filters, e.g. name \"SQL errors\" with an include filter on \"SQLSTATE\". "
            "Tabs you create are named \"AI: ...\" and you may change them freely.",
            [f](const json::object& args, McpContext& ctx) {
                auto name = RequireString(args, "name");
                auto filter = ParseFilterList(Arg(args, "filters"));
                bool select = GetBool(args, "select").value_or(true);
                return ctx.RunOnUi([f, name, filter, select] {
                    auto tab = B::CreateTab(*f, name, filter, select);
                    json::object result{{"tab", tab}};
                    result["filters"] = FiltersJson(B::GetFilters(*f, static_cast<int>(tab["index"].as_int64())))["filters"];
                    return json::value(std::move(result));
                });
            });

    AddTool(registry, "close_tab", "Close a tab", false, Close(Props({TabProperty, P_AllowUser}), R"(["tab"])").c_str(),
            "Close a tab. Only tabs you created (\"AI: ...\") unless allow_user_tab is true. The first tab cannot be closed.",
            [f](const json::object& args, McpContext& ctx) {
                return ctx.RunOnUi([f, args] {
                    int index = B::ResolveTabArg(*f, args);
                    B::RequireWritable(*f, index, args);
                    B::CloseTab(*f, index);
                    return json::value(json::object{{"closed", true}, {"tabs", B::TabsJson(*f)}});
                });
            });

    AddTool(registry, "switch_tab", "Switch to a tab", false, Close(Props({TabProperty}), R"(["tab"])").c_str(),
            "Make a tab the visible one, e.g. to show the user what you found.",
            [f](const json::object& args, McpContext& ctx) {
                return ctx.RunOnUi([f, args] {
                    int index = B::ResolveTabArg(*f, args);
                    B::SwitchTab(*f, index);
                    return json::value(json::object{{"tab", B::TabJson(*f, index)}});
                });
            });

    AddTool(registry, "rename_tab", "Rename a tab", false,
            Close(Props({TabProperty, R"("name": {"type": "string", "description": "New name. Your tabs keep their \"AI: \" prefix."})", P_AllowUser}), R"(["tab", "name"])").c_str(),
            "Rename a tab you created (or a user tab with allow_user_tab: true).",
            [f](const json::object& args, McpContext& ctx) {
                auto name = RequireString(args, "name");
                return ctx.RunOnUi([f, args, name] {
                    int index = B::ResolveTabArg(*f, args);
                    B::RequireWritable(*f, index, args);
                    B::RenameTab(*f, index, Utf8ToWide(name));
                    return json::value(json::object{{"tab", B::TabJson(*f, index)}});
                });
            });

    // ------------------------------------------------ filters

    AddTool(registry, "get_filters", "Get a tab's filters", true, Close(Props({TabProperty}), R"(["tab"])").c_str(),
            "List a tab's filters. Each has category, index (position within its category, used by remove_filter/enable_filter), text, matchType, "
            "filterType, colours and enabled.",
            [f](const json::object& args, McpContext& ctx) {
                return ctx.RunOnUi([f, args] {
                    int index = B::ResolveTabArg(*f, args);
                    auto result = FiltersJson(B::GetFilters(*f, index));
                    result["tab"] = B::TabJson(*f, index);
                    return json::value(std::move(result));
                });
            });

    AddTool(registry, "set_filters", "Replace a tab's filters", false,
            Close(Props({TabProperty, (std::string(R"("filters": {"type": "array", "items": )") + FilterObjectSchema() + "}").c_str(), P_AllowUser}), R"(["tab", "filters"])").c_str(),
            "Replace all filters of a tab (an empty array removes them all). Only tabs you created unless allow_user_tab is true.",
            [f](const json::object& args, McpContext& ctx) {
                auto filter = ParseFilterList(Arg(args, "filters"));
                if (Arg(args, "filters") == nullptr)
                    throw McpToolError("'filters' is required (use [] to remove all filters)");
                return ctx.RunOnUi([f, args, filter] {
                    int index = B::ResolveTabArg(*f, args);
                    B::RequireWritable(*f, index, args);
                    B::SetFilters(*f, index, filter);
                    auto result = FiltersJson(B::GetFilters(*f, index));
                    result["tab"] = B::TabJson(*f, index);
                    return json::value(std::move(result));
                });
            });

    AddTool(registry, "add_filter", "Add a filter to a tab", false,
            (std::string(R"({"type": "object", "properties": {)") + TabProperty + ", " + P_FilterFields + ", " + P_AllowUser +
             R"(}, "required": ["tab", "text"], "additionalProperties": false})")
                .c_str(),
            "Add one filter to a tab. filterType include shows only matching lines, exclude hides them, highlight colours them. "
            "Only tabs you created unless allow_user_tab is true.",
            [f](const json::object& args, McpContext& ctx) {
                auto [category, parsed] = ParseFilter(args);
                return ctx.RunOnUi([f, args, category = category, parsed = parsed] {
                    int index = B::ResolveTabArg(*f, args);
                    B::RequireWritable(*f, index, args);
                    auto filter = B::GetFilters(*f, index);
                    Category(filter, category).push_back(parsed);
                    B::SetFilters(*f, index, filter);
                    auto result = FiltersJson(B::GetFilters(*f, index));
                    result["tab"] = B::TabJson(*f, index);
                    return json::value(std::move(result));
                });
            });

    const std::string addressProps = std::string(TabProperty) +
                                     R"(, "category": {"type": "string", "enum": ["message", "process", "prefix", "procedure"]}, "index": {"type": "integer", "minimum": 0, "description": "Index within the category, from get_filters."})";

    AddTool(registry, "remove_filter", "Remove a filter", false,
            (std::string(R"({"type": "object", "properties": {)") + addressProps + ", " + P_AllowUser +
             R"(}, "required": ["tab", "category", "index"], "additionalProperties": false})")
                .c_str(),
            "Remove one filter (identified by category + index from get_filters). Only tabs you created unless allow_user_tab is true.",
            [f](const json::object& args, McpContext& ctx) {
                auto [category, position] = FilterAddress(args);
                return ctx.RunOnUi([f, args, category = category, position = position] {
                    int index = B::ResolveTabArg(*f, args);
                    B::RequireWritable(*f, index, args);
                    auto filter = B::GetFilters(*f, index);
                    auto& filters = Category(filter, category);
                    if (position >= filters.size())
                        throw McpToolError("No " + category + " filter at index " + std::to_string(position));
                    filters.erase(filters.begin() + static_cast<std::ptrdiff_t>(position));
                    B::SetFilters(*f, index, filter);
                    auto result = FiltersJson(B::GetFilters(*f, index));
                    result["tab"] = B::TabJson(*f, index);
                    return json::value(std::move(result));
                });
            });

    AddTool(registry, "enable_filter", "Enable or disable a filter", false,
            (std::string(R"({"type": "object", "properties": {)") + addressProps + R"(, "enabled": {"type": "boolean"}, )" + P_AllowUser +
             R"(}, "required": ["tab", "category", "index", "enabled"], "additionalProperties": false})")
                .c_str(),
            "Turn one filter on or off without removing it. Only tabs you created unless allow_user_tab is true.",
            [f](const json::object& args, McpContext& ctx) {
                auto [category, position] = FilterAddress(args);
                auto enabled = GetBool(args, "enabled");
                if (!enabled)
                    throw McpToolError("'enabled' is required");
                return ctx.RunOnUi([f, args, category = category, position = position, enabled = *enabled] {
                    int index = B::ResolveTabArg(*f, args);
                    B::RequireWritable(*f, index, args);
                    auto filter = B::GetFilters(*f, index);
                    auto& filters = Category(filter, category);
                    if (position >= filters.size())
                        throw McpToolError("No " + category + " filter at index " + std::to_string(position));
                    filters[position].enable = enabled;
                    B::SetFilters(*f, index, filter);
                    auto result = FiltersJson(B::GetFilters(*f, index));
                    result["tab"] = B::TabJson(*f, index);
                    return json::value(std::move(result));
                });
            });
}

} // namespace debugviewpp
} // namespace fusion
