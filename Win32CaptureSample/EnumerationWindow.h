#pragma once

struct EnumerationWindow
{
public:
    static std::vector<EnumerationWindow> EnumerateAllWindows();

    HWND Hwnd() const noexcept { return m_hwnd; }
    std::wstring Title() const noexcept { return m_title; }
    std::wstring ClassName() const noexcept { return m_className; }

private:
    EnumerationWindow(HWND hwnd, const std::wstring& title, const std::wstring& className) : 
        m_hwnd(hwnd), m_title(title), m_className(className)
    {
    }
    const HWND m_hwnd;
    const std::wstring m_title;
    const std::wstring m_className;
};