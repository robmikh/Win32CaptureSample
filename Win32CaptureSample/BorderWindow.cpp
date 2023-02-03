#include "pch.h"
#include "BorderWindow.h"

namespace util
{
    using namespace robmikh::common::desktop;
}

const std::wstring BorderWindow::ClassName = L"Win32CaptureSample.BorderWindow";
std::once_flag BorderWindowClassRegistration;

void BorderWindow::RegisterWindowClass()
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(wcex);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIconW(instance, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = ClassName.c_str();
    wcex.hIconSm = LoadIconW(wcex.hInstance, IDI_APPLICATION);
    winrt::check_bool(RegisterClassExW(&wcex));
}

BorderWindow::BorderWindow(winrt::Windows::UI::Composition::Compositor const& compositor)
{
    std::call_once(BorderWindowClassRegistration, []()
        {
            RegisterWindowClass();
        });

    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    winrt::check_bool(CreateWindowExW(WS_EX_NOREDIRECTIONBITMAP | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW | WS_EX_LAYERED | WS_EX_TOPMOST, ClassName.c_str(), L"", WS_POPUP,
        CW_USEDEFAULT, CW_USEDEFAULT, 5, 5, nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    m_colorBrush = compositor.CreateColorBrush(winrt::Windows::UI::Color{ 255, 255, 0, 0 }); // ARGB
    m_nineGridBrush = compositor.CreateNineGridBrush();
    m_nineGridBrush.SetInsets(m_thickness);
    m_nineGridBrush.IsCenterHollow(true);
    m_nineGridBrush.Source(m_colorBrush);

    auto visual = compositor.CreateSpriteVisual();
    visual.RelativeSizeAdjustment({ 1, 1 });
    visual.Brush(m_nineGridBrush);
    visual.Opacity(0.5f);

    m_target = util::CreateDesktopWindowTarget(compositor, m_window, false);
    m_target.Root(visual);
}

void BorderWindow::BorderColor(winrt::Windows::UI::Color const& value)
{
    m_colorBrush.Color(value);
}

void BorderWindow::BorderThickness(float value)
{
    m_thickness = value;
    m_nineGridBrush.SetInsets(m_thickness);
}

void BorderWindow::PositionOver(HWND otherWindowHandle)
{
    if (m_lastWindowOver != otherWindowHandle && otherWindowHandle != m_window)
    {
        RECT rect = {};
        winrt::check_hresult(DwmGetWindowAttribute(otherWindowHandle, DWMWA_EXTENDED_FRAME_BOUNDS, reinterpret_cast<void*>(&rect), sizeof(rect)));
        MoveAndResize(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
        m_lastWindowOver = otherWindowHandle;
        m_lastMonitorOver = nullptr;
    }
}

void BorderWindow::PositionOver(HMONITOR monitorHandle)
{
    if (m_lastMonitorOver != monitorHandle)
    {
        RECT rect = {};
        MONITORINFO info = {};
        info.cbSize = sizeof(info);
        winrt::check_bool(GetMonitorInfoW(monitorHandle, &info));
        rect = info.rcMonitor;
        MoveAndResize(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
        m_lastWindowOver = nullptr;
        m_lastMonitorOver = monitorHandle;
    }
}

void BorderWindow::MoveTo(int32_t x, int32_t y)
{
    SetWindowPos(m_window, HWND_TOPMOST, x, y, 0, 0, SWP_NOACTIVATE | SWP_NOSIZE);
}

void BorderWindow::Resize(int32_t width, int32_t height)
{
    SetWindowPos(m_window, HWND_TOPMOST, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE);
}

void BorderWindow::MoveAndResize(int32_t x, int32_t y, int32_t width, int32_t height)
{
    SetWindowPos(m_window, HWND_TOPMOST, x, y, width, height, SWP_NOACTIVATE);
}

void BorderWindow::Show()
{
    ShowWindow(m_window, SW_SHOW);
    UpdateWindow(m_window);
}

void BorderWindow::Hide()
{
    ShowWindow(m_window, SW_HIDE);
    UpdateWindow(m_window);
}

void BorderWindow::Close()
{
    DestroyWindow(m_window);
}

LRESULT BorderWindow::MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam)
{
    return DefWindowProcW(m_window, message, wparam, lparam);
}