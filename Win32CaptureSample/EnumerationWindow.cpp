#include "pch.h"
#include "EnumerationWindow.h"
#include <dwmapi.h>

std::wstring GetClassName(HWND hwnd)
{
    const DWORD TITLE_SIZE = 1024;
    WCHAR windowTitle[TITLE_SIZE];

    ::GetClassName(hwnd, windowTitle, TITLE_SIZE);

    std::wstring title(&windowTitle[0]);
    return title;
}

std::wstring GetWindowText(HWND hwnd)
{
    const DWORD TITLE_SIZE = 1024;
    WCHAR windowTitle[TITLE_SIZE];

    ::GetWindowText(hwnd, windowTitle, TITLE_SIZE);

    std::wstring title(&windowTitle[0]);
    return title;
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

    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    if (!((style & WS_DISABLED) != WS_DISABLED))
    {
        return false;
    }

    DWORD cloaked = FALSE;
    HRESULT hrTemp = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (SUCCEEDED(hrTemp) &&
        cloaked == DWM_CLOAKED_SHELL)
    {
        return false;
    }

    return true;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    auto class_name = GetClassName(hwnd);
    auto title = GetWindowText(hwnd);

    auto window = EnumerationWindow(hwnd, title, class_name);

    if (!IsCapturableWindow(window))
    {
        return TRUE;
    }

    std::vector<EnumerationWindow>& windows = *reinterpret_cast<std::vector<EnumerationWindow>*>(lParam);
    windows.push_back(window);

    return TRUE;
}

const std::vector<EnumerationWindow> EnumerateWindows()
{
    std::vector<EnumerationWindow> windows;
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&windows));

    return windows;
}

std::vector<EnumerationWindow> EnumerationWindow::EnumerateAllWindows()
{
    return EnumerateWindows();
}