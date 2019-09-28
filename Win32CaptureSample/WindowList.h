#pragma once

struct WindowInfo
{
    WindowInfo(HWND windowHandle)
    {
        WindowHandle = windowHandle;
        auto titleLength = GetWindowTextLengthW(WindowHandle);
        if (titleLength > 0)
        {
            titleLength++;
        }
        std::wstring title(titleLength, 0);
        GetWindowTextW(WindowHandle, title.data(), titleLength);
        Title = title;
        auto classNameLength = 256;
        std::wstring className(classNameLength, 0);
        GetClassNameW(WindowHandle, className.data(), classNameLength);
        ClassName = className;
    }

    HWND WindowHandle;
    std::wstring Title;
    std::wstring ClassName;

    bool operator==(const WindowInfo& info) { return WindowHandle == info.WindowHandle; }
    bool operator!=(const WindowInfo& info) { return !(*this == info); }
};

class WindowList
{
public:
    WindowList();
    ~WindowList();

    void RegisterComboBoxForUpdates(HWND comboBoxHandle) { m_comboBoxes.push_back(comboBoxHandle); ForceUpdateComboBox(comboBoxHandle); }
    void UnregisterComboBox(HWND comboBoxHandle) { m_comboBoxes.erase(std::remove(m_comboBoxes.begin(), m_comboBoxes.end(), comboBoxHandle), m_comboBoxes.end()); }
    const std::vector<WindowInfo> GetCurrentWindows() { return m_windows; }

private:
    void AddWindow(WindowInfo const& info);
    bool RemoveWindow(WindowInfo const& info);
    void ForceUpdateComboBox(HWND comboBoxHandle);

private:
    std::vector<HWND> m_comboBoxes;
    std::vector<WindowInfo> m_windows;
    std::unordered_set<HWND> m_seenWindows;
    wil::unique_hwineventhook m_eventHook;
};