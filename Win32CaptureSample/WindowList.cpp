#include "pch.h"
#include "WindowList.h"

bool inline MatchTitleAndClassName(WindowInfo const& window, std::wstring const& title, std::wstring const& className)
{
    return wcscmp(window.Title.c_str(), title.c_str()) == 0 &&
        wcscmp(window.ClassName.c_str(), className.c_str()) == 0;
}

bool IsKnownBlockedWindow(WindowInfo const& window)
{
    return
        // Task View
        MatchTitleAndClassName(window, L"Task View", L"Windows.UI.Core.CoreWindow") ||
        // XAML Islands
        MatchTitleAndClassName(window, L"DesktopWindowXamlSource", L"Windows.UI.Core.CoreWindow") ||
        // XAML Popups
        MatchTitleAndClassName(window, L"PopupHost", L"Xaml_WindowedPopupClass");
}

bool IsCapturableWindow(WindowInfo const& window)
{
    if (window.Title.empty() || window.WindowHandle == GetShellWindow() ||
        !IsWindowVisible(window.WindowHandle) || GetAncestor(window.WindowHandle, GA_ROOT) != window.WindowHandle)
    {
        return false;
    }

    auto style = GetWindowLongW(window.WindowHandle, GWL_STYLE);
    if (style & WS_DISABLED)
    {
        return false;
    }

    auto exStyle = GetWindowLongW(window.WindowHandle, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW)    // No tooltips
    {
        return false;
    }

    // Check to see if the window is cloaked if it's a UWP
    if (wcscmp(window.ClassName.c_str(), L"Windows.UI.Core.CoreWindow") == 0 ||
        wcscmp(window.ClassName.c_str(), L"ApplicationFrameWindow") == 0)
    {
        DWORD cloaked = FALSE;
        if (SUCCEEDED(DwmGetWindowAttribute(window.WindowHandle, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && (cloaked == DWM_CLOAKED_SHELL))
        {
            return false;
        }
    }

    // Unfortunate work-around. Not sure how to avoid this.
    if (IsKnownBlockedWindow(window))
    {
        return false;
    }

    return true;
}

static thread_local WindowList* WindowListForThread;

WindowList::WindowList()
{
    if (WindowListForThread)
    {
        throw std::exception("WindowList already exists for this thread!");
    }
    WindowListForThread = this;

    EnumWindows([](HWND hwnd, LPARAM lParam)
    {
        if (GetWindowTextLengthW(hwnd) > 0)
        {
            auto window = WindowInfo(hwnd);

            if (!IsCapturableWindow(window))
            {
                return TRUE;
            }

            auto windowList = reinterpret_cast<WindowList*>(lParam);
            windowList->AddWindow(window);
        }
        
        return TRUE;
    }, reinterpret_cast<LPARAM>(this));
    
    m_eventHook.reset(SetWinEventHook(EVENT_OBJECT_DESTROY, /*EVENT_OBJECT_SHOW*/EVENT_OBJECT_UNCLOAKED, nullptr,
        [](HWINEVENTHOOK eventHook, DWORD event, HWND hwnd, LONG objectId, LONG childId, DWORD eventThreadId, DWORD eventTimeInMilliseconds)
        {
            if (event == EVENT_OBJECT_DESTROY && childId == CHILDID_SELF)
            {
                WindowListForThread->RemoveWindow(WindowInfo(hwnd));
                return;
            }

            if (objectId == OBJID_WINDOW && childId == CHILDID_SELF && hwnd != nullptr && GetAncestor(hwnd, GA_ROOT) == hwnd &&
                GetWindowTextLengthW(hwnd) > 0 && (event == EVENT_OBJECT_SHOW || event == EVENT_OBJECT_UNCLOAKED))
            {
                auto window = WindowInfo(hwnd);

                if (IsCapturableWindow(window))
                {
                    WindowListForThread->AddWindow(window);
                }
            }
        }, 0, 0, WINEVENT_OUTOFCONTEXT));
}

WindowList::~WindowList()
{
    m_eventHook.reset();
    WindowListForThread = nullptr;
}

void WindowList::AddWindow(WindowInfo const& info)
{
    auto search = m_seenWindows.find(info.WindowHandle);
    if (search == m_seenWindows.end())
    {
        m_windows.push_back(info);
        m_seenWindows.insert(info.WindowHandle);
        for (auto& comboBox : m_comboBoxes)
        {
            winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBox, CB_ADDSTRING, 0, (LPARAM)info.Title.c_str())));
        }
    }
}

bool WindowList::RemoveWindow(WindowInfo const& info)
{
    auto search = m_seenWindows.find(info.WindowHandle);
    if (search != m_seenWindows.end())
    {
        m_seenWindows.erase(search);
        auto index = 0;
        for (auto& window : m_windows)
        {
            if (window.WindowHandle == info.WindowHandle)
            {
                break;
            }
            index++;
        }
        m_windows.erase(m_windows.begin() + index);
        for (auto& comboBox : m_comboBoxes)
        {
            winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBox, CB_DELETESTRING, index, 0)));
        }
        return true;
    }
    return false;
}

void WindowList::ForceUpdateComboBox(HWND comboBoxHandle)
{
    winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBoxHandle, CB_RESETCONTENT, 0, 0)));
    for (auto& window : m_windows)
    {
        winrt::check_hresult(static_cast<const int32_t>(SendMessageW(comboBoxHandle, CB_ADDSTRING, 0, (LPARAM)window.Title.c_str())));
    }
}