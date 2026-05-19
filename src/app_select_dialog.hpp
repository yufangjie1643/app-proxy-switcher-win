#pragma once
#include <windows.h>
#include <vector>
#include <string>
#include "app_config.hpp"

class ConfigStore;

class AppSelectDialog {
public:
    static bool Show(HWND parent, ConfigStore& configStore);

private:
    static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void RefreshList(HWND hwndList, ConfigStore& store);
    static void OnAddExe(HWND hwnd, ConfigStore& store);
    static void OnAddMsix(HWND hwnd, ConfigStore& store);
    static void OnAddNpm(HWND hwnd, ConfigStore& store);
    static void OnAddVsCode(HWND hwnd, ConfigStore& store);
    static void OnRemove(HWND hwnd, ConfigStore& store);
    static void OnSelect(HWND hwnd, ConfigStore& store);
};
