#pragma once
#include <string>

std::wstring Utf8ToWide(const std::string& utf8);
std::string WideToUtf8(const std::wstring& wide);
std::wstring ExpandEnvironmentStringsWrapper(const std::wstring& input);
bool FileExists(const std::wstring& path);
bool DirectoryExists(const std::wstring& path);
std::wstring GetAppDataPath();
std::wstring GetExecutableDirectory();
std::string Trim(const std::string& s);
std::wstring FormatProxyUrl(const std::wstring& scheme, const std::wstring& host, int port);
