#include "pch.h"
#include "EnumerationWindow.h"
#include <dwmapi.h>

std::wstring GetClassName(HWND hwnd)
{
    const DWORD TITLE_SIZE = 1024;
    wchar_t windowTitle[TITLE_SIZE];

    ::GetClassNameW(hwnd, windowTitle, TITLE_SIZE);

    std::wstring title(&windowTitle[0]);
    return title;
}

std::wstring GetWindowTextW(HWND hwnd)
{
    const DWORD TITLE_SIZE = 1024;
    wchar_t windowTitle[TITLE_SIZE];

    ::GetWindowTextW(hwnd, windowTitle, TITLE_SIZE);
    return std::wstring(windowTitle);
}

bool IsCapturableWindow(EnumerationWindow const& window)
{
    HWND hwnd = window.Hwnd();
    HWND shellWindow = GetShellWindow();

    auto title = window.Title();
    auto className = window.ClassName();

    if (hwnd == shellWindow)
    {
        return false;
    }

    if (title.length() == 0)
    {
        return false;
    }

    if (!IsWindowVisible(hwnd))
    {
        return false;
    }

    if (GetAncestor(hwnd, GA_ROOT) != hwnd)
    {
        return false;
    }

    LONG style = GetWindowLongW(hwnd, GWL_STYLE);
    if (!((style & WS_DISABLED) != WS_DISABLED))
    {
        return false;
    }

    DWORD cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && (cloaked == DWM_CLOAKED_SHELL))
    {
        return false;
    }

    return true;
}

std::vector<EnumerationWindow> EnumerationWindow::EnumerateAllWindows()
{
    std::vector<EnumerationWindow> windows;
    EnumWindows([](HWND hwnd, LPARAM lParam)
    {
        auto class_name = GetClassNameW(hwnd);
        auto title = GetWindowTextW(hwnd);

        auto window = EnumerationWindow(hwnd, title, class_name);

        if (!IsCapturableWindow(window))
        {
            return TRUE;
        }

        auto& windows = *reinterpret_cast<std::vector<EnumerationWindow>*>(lParam);
        windows.push_back(window);

        return TRUE;
    }, reinterpret_cast<LPARAM>(&windows));

    return windows;
}