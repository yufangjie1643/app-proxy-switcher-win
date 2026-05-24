#include "proxy_detector.hpp"
#include "utils.hpp"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <algorithm>
#include <string>

namespace {

struct WinsockSession {
    WinsockSession() {
        WSADATA data = {};
        ok = WSAStartup(MAKEWORD(2, 2), &data) == 0;
    }

    ~WinsockSession() {
        if (ok) {
            WSACleanup();
        }
    }

    bool ok = false;
};

bool TryConnectAddress(addrinfo* address, int timeoutMs) {
    SOCKET sock = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (sock == INVALID_SOCKET) {
        return false;
    }

    u_long nonBlocking = 1;
    ioctlsocket(sock, FIONBIO, &nonBlocking);

    int rc = connect(sock, address->ai_addr, static_cast<int>(address->ai_addrlen));
    if (rc == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK) {
        closesocket(sock);
        return false;
    }

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(sock, &writeSet);

    timeval timeout = {};
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    bool open = false;
    rc = select(0, nullptr, &writeSet, nullptr, &timeout);
    if (rc > 0 && FD_ISSET(sock, &writeSet)) {
        int socketError = 0;
        int len = sizeof(socketError);
        if (getsockopt(sock, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &len) == 0) {
            open = socketError == 0;
        }
    }

    closesocket(sock);
    return open;
}

std::wstring JoinPorts(const std::vector<int>& ports) {
    std::wstring text;
    for (int port : ports) {
        if (!text.empty()) {
            text += L", ";
        }
        text += L"127.0.0.1:" + std::to_wstring(port);
    }
    return text;
}

}

const std::vector<int>& GetCommonProxyPorts() {
    static const std::vector<int> ports = {
        7890, 7891, 7892, 7897, 7899,
        1080, 10808, 10809,
        2080, 2081,
        20170, 20171,
        8080, 8118, 8888, 9090
    };
    return ports;
}

bool IsTcpPortOpen(const std::wstring& host, int port, int timeoutMs) {
    if (host.empty() || port <= 0 || port > 65535) {
        return false;
    }

    static WinsockSession winsock;
    if (!winsock.ok) {
        return false;
    }

    std::string hostUtf8 = WideToUtf8(host);
    std::string portText = std::to_string(port);

    addrinfo hints = {};
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    addrinfo* result = nullptr;
    int rc = getaddrinfo(hostUtf8.c_str(), portText.c_str(), &hints, &result);
    if (rc != 0 || !result) {
        return false;
    }

    bool open = false;
    for (addrinfo* address = result; address != nullptr; address = address->ai_next) {
        if (TryConnectAddress(address, timeoutMs)) {
            open = true;
            break;
        }
    }

    freeaddrinfo(result);
    return open;
}

std::vector<int> ScanOpenProxyPorts(const std::wstring& host, const std::vector<int>& ports, int timeoutMs) {
    std::vector<int> openPorts;
    std::vector<int> checked;

    for (int port : ports) {
        if (std::find(checked.begin(), checked.end(), port) != checked.end()) {
            continue;
        }
        checked.push_back(port);

        if (IsTcpPortOpen(host, port, timeoutMs)) {
            openPorts.push_back(port);
        }
    }

    return openPorts;
}

ProxyScanResult CheckProxyAvailability(const ProxySettings& settings, int timeoutMs) {
    ProxyScanResult result;
    result.scannedPorts = GetCommonProxyPorts();
    result.currentOpen = IsTcpPortOpen(settings.host, settings.port, timeoutMs);

    std::vector<int> ports = result.scannedPorts;
    if (std::find(ports.begin(), ports.end(), settings.port) == ports.end()) {
        ports.insert(ports.begin(), settings.port);
    }

    result.openPorts = ScanOpenProxyPorts(L"127.0.0.1", ports, timeoutMs);
    return result;
}

std::wstring DescribeProxyScanResult(const ProxySettings& settings, const ProxyScanResult& result) {
    std::wstring current = FormatProxyUrl(settings.scheme, settings.host, settings.port);
    if (result.currentOpen) {
        return L"当前代理可用：" + current;
    }

    if (!result.openPorts.empty()) {
        return L"当前代理不可用，发现本机可用端口：" + JoinPorts(result.openPorts);
    }

    return L"当前代理不可用，且未发现常见本地代理端口。请先打开代理软件，或手动配置代理地址。";
}
