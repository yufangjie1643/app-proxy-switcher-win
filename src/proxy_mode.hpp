#pragma once

enum class ProxyMode {
    EnvVar = 0,     // 环境变量注入 (HTTP_PROXY/HTTPS_PROXY)
    SystemProxy = 1 // 系统代理 (Windows Internet Options 注册表)
};
