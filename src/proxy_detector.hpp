#pragma once

#include "config_store.hpp"
#include <string>
#include <vector>

struct ProxyScanResult {
    bool currentOpen = false;
    std::vector<int> openPorts;
    std::vector<int> scannedPorts;
};

const std::vector<int>& GetCommonProxyPorts();
bool IsTcpPortOpen(const std::wstring& host, int port, int timeoutMs = 250);
std::vector<int> ScanOpenProxyPorts(const std::wstring& host, const std::vector<int>& ports, int timeoutMs = 180);
ProxyScanResult CheckProxyAvailability(const ProxySettings& settings, int timeoutMs = 180);
std::wstring DescribeProxyScanResult(const ProxySettings& settings, const ProxyScanResult& result);
