#include <winsock2.h>
#include <ws2tcpip.h>
#include "../src/app_finder.hpp"
#include "../src/dpi_utils.hpp"
#include "../src/known_agents.hpp"
#include "../src/proxy_detector.hpp"
#include <windows.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

static void Check(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        std::exit(1);
    }
}

static void ParsesSinglePackageJson() {
    const std::string json =
        "{\n"
        "  \"InstallLocation\": \"C:\\\\Program Files\\\\WindowsApps\\\\OpenAI.Codex_1.0.0.0_x64__abc\",\n"
        "  \"PackageFamilyName\": \"OpenAI.Codex_abc\"\n"
        "}";

    auto info = ParseAppxPackageJson(json);
    Check(info.has_value(), "single package JSON should parse");
    Check(info->installLocation == L"C:\\Program Files\\WindowsApps\\OpenAI.Codex_1.0.0.0_x64__abc", "single package install location mismatch");
    Check(info->packageFamilyName == L"OpenAI.Codex_abc", "single package family name mismatch");
}

static void ParsesFirstValidPackageFromArray() {
    const std::string json =
        "[\n"
        "  {\"InstallLocation\": null, \"PackageFamilyName\": \"Broken\"},\n"
        "  {\n"
        "    \"PackageFamilyName\": \"OpenAI.Codex_abc\",\n"
        "    \"InstallLocation\": \"C:\\\\Program Files\\\\WindowsApps\\\\OpenAI.Codex_2.0.0.0_x64__abc\"\n"
        "  }\n"
        "]";

    auto info = ParseAppxPackageJson(json);
    Check(info.has_value(), "array package JSON should parse");
    Check(info->installLocation == L"C:\\Program Files\\WindowsApps\\OpenAI.Codex_2.0.0.0_x64__abc", "array package install location mismatch");
    Check(info->packageFamilyName == L"OpenAI.Codex_abc", "array package family name mismatch");
}

static void RejectsMissingInstallLocation() {
    auto info = ParseAppxPackageJson("{\"PackageFamilyName\":\"OpenAI.Codex_abc\"}");
    Check(!info.has_value(), "missing install location should be rejected");
}

static bool HasString(const std::vector<std::wstring>& values, const std::wstring& expected) {
    for (const auto& value : values) {
        if (value == expected) return true;
    }
    return false;
}

static void NpmPackageNamesResolveToInstalledCommandNames() {
    auto codex = GetNpmCommandCandidatesForPackage(L"@openai/codex");
    auto claude = GetNpmCommandCandidatesForPackage(L"@anthropic-ai/claude-code");
    auto gemini = GetNpmCommandCandidatesForPackage(L"@google/gemini-cli");
    auto qwen = GetNpmCommandCandidatesForPackage(L"@qwen-code/qwen-code");
    auto opencode = GetNpmCommandCandidatesForPackage(L"opencode-ai");
    auto amp = GetNpmCommandCandidatesForPackage(L"@sourcegraph/amp");

    Check(HasString(codex, L"codex"), "@openai/codex should map to codex");
    Check(HasString(claude, L"claude"), "@anthropic-ai/claude-code should map to claude");
    Check(HasString(gemini, L"gemini"), "@google/gemini-cli should map to gemini");
    Check(HasString(qwen, L"qwen"), "@qwen-code/qwen-code should map to qwen");
    Check(HasString(opencode, L"opencode"), "opencode-ai should map to opencode");
    Check(HasString(amp, L"amp"), "@sourcegraph/amp should map to amp");
}

static void ParsesVsCodeExtensionIdentityFromPackageJson() {
    auto identity = ParseVsCodeExtensionPackageJson("{\"publisher\":\"GitHub\",\"name\":\"copilot-chat\"}");
    Check(identity == L"GitHub.copilot-chat", "VSCode extension identity should parse");
    Check(identity != L"GitHub.copilot", "VSCode extension identity should not prefix-match");
}

static bool HasApp(const std::vector<AppConfig>& apps, AppType type, const std::wstring& packageName) {
    for (const auto& app : apps) {
        if (app.type == type && app.packageName == packageName) {
            return true;
        }
    }
    return false;
}

static int CountApp(const std::vector<AppConfig>& apps, AppType type, const std::wstring& packageName) {
    int count = 0;
    for (const auto& app : apps) {
        if (app.type == type && app.packageName == packageName) {
            ++count;
        }
    }
    return count;
}

static void KnownAgentsIncludeClassicNpmAndVsCodeEntries() {
    std::vector<AppConfig> apps;
    std::wstring currentId;

    EnsureKnownAgentApps(apps, currentId);

    bool hasClaudeDesktopExe = false;
    for (const auto& app : apps) {
        if (app.id == L"claude_default") {
            Check(app.type == AppType::Exe, "Claude Desktop should be EXE");
            Check(app.packageName == L"Claude Desktop", "Claude Desktop packageName should be shortcut search name");
            Check(app.IsValid(), "Claude Desktop EXE app should be valid without fixed exe path");
            hasClaudeDesktopExe = true;
        }
    }
    Check(hasClaudeDesktopExe, "known agents should include Claude Desktop");

    Check(HasApp(apps, AppType::Npm, L"@openai/codex"), "known agents should include @openai/codex");
    Check(HasApp(apps, AppType::Npm, L"@anthropic-ai/claude-code"), "known agents should include Claude Code npm");
    Check(HasApp(apps, AppType::Npm, L"@google/gemini-cli"), "known agents should include Gemini CLI");
    Check(HasApp(apps, AppType::Npm, L"@qwen-code/qwen-code"), "known agents should include Qwen Code");
    Check(HasApp(apps, AppType::Npm, L"opencode-ai"), "known agents should include OpenCode");
    Check(HasApp(apps, AppType::Npm, L"@sourcegraph/amp"), "known agents should include Amp");

    Check(HasApp(apps, AppType::VsCodeExt, L"openai.chatgpt"), "known agents should include OpenAI Codex VSCode");
    Check(HasApp(apps, AppType::VsCodeExt, L"GitHub.copilot-chat"), "known agents should include Copilot Chat");
    Check(HasApp(apps, AppType::VsCodeExt, L"Continue.continue"), "known agents should include Continue");
    Check(HasApp(apps, AppType::VsCodeExt, L"saoudrizwan.claude-dev"), "known agents should include Cline");
    Check(HasApp(apps, AppType::VsCodeExt, L"RooVeterinaryInc.roo-cline"), "known agents should include Roo Code");
    Check(HasApp(apps, AppType::VsCodeExt, L"kilocode.Kilo-Code"), "known agents should include Kilo Code");
    Check(HasApp(apps, AppType::VsCodeExt, L"Google.geminicodeassist"), "known agents should include Gemini Code Assist");
    Check(HasApp(apps, AppType::VsCodeExt, L"AmazonWebServices.amazon-q-vscode"), "known agents should include Amazon Q");
    Check(HasApp(apps, AppType::VsCodeExt, L"augment.vscode-augment"), "known agents should include Augment");
    Check(HasApp(apps, AppType::VsCodeExt, L"Codeium.CodeiumVS"), "known agents should include Windsurf/Codeium");
    Check(HasApp(apps, AppType::VsCodeExt, L"sourcegraph.cody-ai"), "known agents should include Cody");
}

static void KnownAgentMergeDeduplicatesAndPreservesCustomExe() {
    AppConfig oldCodex;
    oldCodex.id = L"old_codex";
    oldCodex.displayName = L"Old Codex";
    oldCodex.type = AppType::Npm;
    oldCodex.packageName = L"@openai/codex";

    AppConfig duplicateCodex = oldCodex;
    duplicateCodex.id = L"duplicate_codex";

    AppConfig customExe;
    customExe.id = L"custom";
    customExe.displayName = L"Custom Agent";
    customExe.type = AppType::Exe;
    customExe.exePath = L"C:\\Tools\\agent.exe";

    std::vector<AppConfig> apps = { oldCodex, duplicateCodex, customExe };
    std::wstring currentId = duplicateCodex.id;

    EnsureKnownAgentApps(apps, currentId);

    Check(CountApp(apps, AppType::Npm, L"@openai/codex") == 1, "known agent merge should deduplicate npm apps");
    Check(HasApp(apps, AppType::Exe, L""), "known agent merge should preserve custom exe app");
    Check(currentId == oldCodex.id, "current id should migrate to kept duplicate");
}

static void KnownAgentMergeMigratesOldClaudeMsixEntry() {
    AppConfig oldClaude;
    oldClaude.id = L"claude_default";
    oldClaude.displayName = L"Claude Desktop";
    oldClaude.type = AppType::Msix;
    oldClaude.packageName = L"Anthropic.Claude";

    std::vector<AppConfig> apps = { oldClaude };
    std::wstring currentId = oldClaude.id;

    EnsureKnownAgentApps(apps, currentId);

    Check(CountApp(apps, AppType::Exe, L"Claude Desktop") == 1, "old Claude MSIX should migrate to EXE shortcut app");
    Check(CountApp(apps, AppType::Msix, L"Anthropic.Claude") == 0, "old Claude MSIX should be removed");
    Check(currentId == L"claude_default", "current id should remain on migrated Claude app");
}

static void WindowSizeForClientUsesRequestedDpi() {
    using AdjustWindowRectExForDpiFn = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    auto adjustForDpi = user32
        ? reinterpret_cast<AdjustWindowRectExForDpiFn>(GetProcAddress(user32, "AdjustWindowRectExForDpi"))
        : nullptr;
    if (!adjustForDpi) {
        return;
    }

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    DWORD exStyle = WS_EX_DLGMODALFRAME;
    unsigned int dpi = 144;
    int clientWidth = MulDiv(560, static_cast<int>(dpi), 96);
    int clientHeight = MulDiv(420, static_cast<int>(dpi), 96);

    RECT expected = { 0, 0, clientWidth, clientHeight };
    BOOL ok = adjustForDpi(&expected, style, FALSE, exStyle, dpi);
    Check(ok != FALSE, "AdjustWindowRectExForDpi should calculate expected window size");

    SIZE actual = GetWindowSizeForClientDpi(clientWidth, clientHeight, style, exStyle, dpi);
    Check(actual.cx == expected.right - expected.left, "window width should use requested DPI for non-client frame");
    Check(actual.cy == expected.bottom - expected.top, "window height should use requested DPI for non-client frame");
}

static bool HasInt(const std::vector<int>& values, int expected) {
    return std::find(values.begin(), values.end(), expected) != values.end();
}

struct WinsockForTest {
    WinsockForTest() {
        WSADATA data = {};
        ok = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }

    ~WinsockForTest() {
        if (ok) {
            WSACleanup();
        }
    }

    bool ok = false;
};

struct LoopbackListener {
    explicit LoopbackListener(WinsockForTest& winsock) {
        if (!winsock.ok) return;

        socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (socket == INVALID_SOCKET) return;

        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;

        if (bind(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            closesocket(socket);
            socket = INVALID_SOCKET;
            return;
        }

        if (listen(socket, 1) != 0) {
            closesocket(socket);
            socket = INVALID_SOCKET;
            return;
        }

        int len = sizeof(addr);
        if (getsockname(socket, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
            closesocket(socket);
            socket = INVALID_SOCKET;
            return;
        }

        port = ntohs(addr.sin_port);
    }

    ~LoopbackListener() {
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
        }
    }

    SOCKET socket = INVALID_SOCKET;
    int port = 0;
};

static void CommonProxyPortsIncludeClassicPorts() {
    const auto& ports = GetCommonProxyPorts();

    Check(HasInt(ports, 7890), "common proxy ports should include Clash default 7890");
    Check(HasInt(ports, 7897), "common proxy ports should include current default 7897");
    Check(HasInt(ports, 1080), "common proxy ports should include SOCKS default 1080");
    Check(HasInt(ports, 10808), "common proxy ports should include 10808");
    Check(HasInt(ports, 10809), "common proxy ports should include 10809");
    Check(HasInt(ports, 8080), "common proxy ports should include 8080");
    Check(HasInt(ports, 8118), "common proxy ports should include 8118");
    Check(HasInt(ports, 8888), "common proxy ports should include 8888");
}

static void TcpPortProbeDetectsOpenLoopbackListener() {
    WinsockForTest winsock;
    LoopbackListener listener(winsock);
    Check(listener.port > 0, "test listener should bind an ephemeral loopback port");

    Check(IsTcpPortOpen(L"127.0.0.1", listener.port, 300), "TCP probe should detect open loopback listener");
}

static void ScanProxyPortsFindsInjectedOpenPort() {
    WinsockForTest winsock;
    LoopbackListener listener(winsock);
    Check(listener.port > 0, "test listener should bind an ephemeral loopback port");

    auto openPorts = ScanOpenProxyPorts(L"127.0.0.1", { listener.port, 1 }, 300);
    Check(HasInt(openPorts, listener.port), "proxy scan should return open injected port");
}

int main() {
    ParsesSinglePackageJson();
    ParsesFirstValidPackageFromArray();
    RejectsMissingInstallLocation();
    NpmPackageNamesResolveToInstalledCommandNames();
    ParsesVsCodeExtensionIdentityFromPackageJson();
    KnownAgentsIncludeClassicNpmAndVsCodeEntries();
    KnownAgentMergeDeduplicatesAndPreservesCustomExe();
    KnownAgentMergeMigratesOldClaudeMsixEntry();
    WindowSizeForClientUsesRequestedDpi();
    CommonProxyPortsIncludeClassicPorts();
    TcpPortProbeDetectsOpenLoopbackListener();
    ScanProxyPortsFindsInjectedOpenPort();
    return 0;
}
