#include "known_agents.hpp"
#include <cwctype>
#include <string>

namespace {

std::wstring ToLower(std::wstring value) {
    for (auto& ch : value) {
        ch = static_cast<wchar_t>(std::towlower(ch));
    }
    return value;
}

std::wstring AppKey(const AppConfig& app) {
    switch (app.type) {
        case AppType::Msix:
            return L"msix:" + ToLower(app.packageName);
        case AppType::Npm:
            return L"npm:" + ToLower(app.packageName);
        case AppType::VsCodeExt:
            return L"vscode:" + ToLower(app.packageName);
        case AppType::Exe:
            return !app.exePath.empty()
                ? L"exe:" + ToLower(app.exePath)
                : L"exe-shortcut:" + ToLower(app.packageName);
    }
    return L"id:" + app.id;
}

bool HasKey(const std::vector<std::wstring>& keys, const std::wstring& key, size_t* index = nullptr) {
    for (size_t i = 0; i < keys.size(); ++i) {
        if (keys[i] == key) {
            if (index) *index = i;
            return true;
        }
    }
    return false;
}

AppConfig MakeApp(const KnownAgentDefinition& def) {
    AppConfig app;
    app.id = def.id;
    app.displayName = def.displayName;
    app.type = def.type;
    app.packageName = def.packageName;
    app.useProxy = true;
    return app;
}

void CanonicalizeKnownAlias(AppConfig& app) {
    if (app.type == AppType::Npm) {
        if (_wcsicmp(app.packageName.c_str(), L"codex") == 0) {
            app.packageName = L"@openai/codex";
            app.displayName = L"OpenAI Codex CLI (npm)";
        } else if (_wcsicmp(app.packageName.c_str(), L"claude") == 0) {
            app.packageName = L"@anthropic-ai/claude-code";
            app.displayName = L"Claude Code (npm)";
        }
    } else if (app.type == AppType::VsCodeExt) {
        if (_wcsicmp(app.packageName.c_str(), L"openai.codex-vscode") == 0) {
            app.packageName = L"openai.chatgpt";
            app.displayName = L"Codex - OpenAI coding agent (VSCode)";
        }
    } else if (app.type == AppType::Msix) {
        if (_wcsicmp(app.packageName.c_str(), L"Anthropic.Claude") == 0) {
            app.type = AppType::Exe;
            app.packageName = L"Claude Desktop";
            app.exePath.clear();
            app.displayName = L"Claude Desktop";
        }
    }
}

}

const std::vector<KnownAgentDefinition>& GetKnownAgentDefinitions() {
    static const std::vector<KnownAgentDefinition> definitions = {
        { L"codex_default", L"OpenAI Codex Desktop", AppType::Msix, L"OpenAI.Codex" },
        { L"claude_default", L"Claude Desktop", AppType::Exe, L"Claude Desktop" },

        { L"codex_npm_openai", L"OpenAI Codex CLI (npm)", AppType::Npm, L"@openai/codex" },
        { L"claude_code_npm", L"Claude Code (npm)", AppType::Npm, L"@anthropic-ai/claude-code" },
        { L"gemini_cli_npm", L"Gemini CLI (npm)", AppType::Npm, L"@google/gemini-cli" },
        { L"qwen_code_npm", L"Qwen Code (npm)", AppType::Npm, L"@qwen-code/qwen-code" },
        { L"opencode_npm", L"OpenCode (npm)", AppType::Npm, L"opencode-ai" },
        { L"amp_npm", L"Sourcegraph Amp (npm)", AppType::Npm, L"@sourcegraph/amp" },

        { L"codex_vscode_openai", L"Codex - OpenAI coding agent (VSCode)", AppType::VsCodeExt, L"openai.chatgpt" },
        { L"copilot_chat_vscode", L"GitHub Copilot Chat (VSCode)", AppType::VsCodeExt, L"GitHub.copilot-chat" },
        { L"copilot_vscode", L"GitHub Copilot (VSCode)", AppType::VsCodeExt, L"GitHub.copilot" },
        { L"continue_vscode", L"Continue (VSCode)", AppType::VsCodeExt, L"Continue.continue" },
        { L"cline_vscode", L"Cline (VSCode)", AppType::VsCodeExt, L"saoudrizwan.claude-dev" },
        { L"roo_code_vscode", L"Roo Code (VSCode)", AppType::VsCodeExt, L"RooVeterinaryInc.roo-cline" },
        { L"kilo_code_vscode", L"Kilo Code (VSCode)", AppType::VsCodeExt, L"kilocode.Kilo-Code" },
        { L"gemini_code_assist_vscode", L"Gemini Code Assist (VSCode)", AppType::VsCodeExt, L"Google.geminicodeassist" },
        { L"amazon_q_vscode", L"Amazon Q (VSCode)", AppType::VsCodeExt, L"AmazonWebServices.amazon-q-vscode" },
        { L"augment_vscode", L"Augment Code (VSCode)", AppType::VsCodeExt, L"augment.vscode-augment" },
        { L"windsurf_codeium_vscode", L"Windsurf / Codeium (VSCode)", AppType::VsCodeExt, L"Codeium.CodeiumVS" },
        { L"cody_vscode", L"Sourcegraph Cody (VSCode)", AppType::VsCodeExt, L"sourcegraph.cody-ai" },
        { L"tabnine_vscode", L"Tabnine (VSCode)", AppType::VsCodeExt, L"TabNine.tabnine-vscode" }
    };
    return definitions;
}

void EnsureKnownAgentApps(std::vector<AppConfig>& apps, std::wstring& currentAppId) {
    std::vector<AppConfig> uniqueApps;
    std::vector<std::wstring> keys;

    for (auto app : apps) {
        CanonicalizeKnownAlias(app);

        if (!app.IsValid()) {
            continue;
        }

        std::wstring key = AppKey(app);
        size_t existingIndex = 0;
        if (HasKey(keys, key, &existingIndex)) {
            if (currentAppId == app.id) {
                currentAppId = uniqueApps[existingIndex].id;
            }
            continue;
        }

        keys.push_back(key);
        uniqueApps.push_back(app);
    }

    for (const auto& def : GetKnownAgentDefinitions()) {
        AppConfig app = MakeApp(def);
        std::wstring key = AppKey(app);
        if (!HasKey(keys, key)) {
            keys.push_back(key);
            uniqueApps.push_back(app);
        }
    }

    apps.swap(uniqueApps);

    bool currentExists = false;
    for (const auto& app : apps) {
        if (app.id == currentAppId) {
            currentExists = true;
            break;
        }
    }

    if (!currentExists && !apps.empty()) {
        currentAppId = apps[0].id;
    }
}
