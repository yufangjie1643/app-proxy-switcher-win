#pragma once
#include <string>
#include <vector>
#include "app_config.hpp"

struct KnownAgentDefinition {
    std::wstring id;
    std::wstring displayName;
    AppType type = AppType::Msix;
    std::wstring packageName;
};

const std::vector<KnownAgentDefinition>& GetKnownAgentDefinitions();
void EnsureKnownAgentApps(std::vector<AppConfig>& apps, std::wstring& currentAppId);
