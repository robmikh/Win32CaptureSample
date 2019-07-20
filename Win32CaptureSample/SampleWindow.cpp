#include "pch.h"
#include "App.h"
#include "SampleWindow.h"
#include <CommCtrl.h>

using namespace winrt;
using namespace Windows::Graphics::Capture;
using namespace Windows::System;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;

SampleWindow::SampleWindow(HINSTANCE instance, int cmdShow, std::shared_ptr<App> app)
{
    WNDCLASSEX wcex = { sizeof(wcex) };
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIconW(instance, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"Win32CaptureSample";
    wcex.hIconSm = LoadIconW(wcex.hInstance, IDI_APPLICATION);
    WINRT_VERIFY(RegisterClassExW(&wcex));
    WINRT_ASSERT(!m_window);

    WINRT_VERIFY(CreateWindowW(L"Win32CaptureSample", L"Win32CaptureSample", WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    ShowWindow(m_window, cmdShow);
    UpdateWindow(m_window);

    m_app = app;
    m_windows = EnumerationWindow::EnumerateAllWindows();
    m_monitors = EnumerationMonitor::EnumerateAllMonitors();

    CreateControls(instance);
}

LRESULT SampleWindow::MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            auto command = HIWORD(wparam);
            auto hwnd = (HWND)lparam;
            switch (command)
            {
            case CBN_SELCHANGE:
                {
                    auto index = SendMessageW(hwnd, CB_GETCURSEL, 0, 0);
                    if (hwnd == m_windowComboBoxHwnd)
                    {
                        auto window = m_windows[index];
                        auto item = m_app->StartCaptureFromWindowHandle(window.Hwnd());

                        SetSubTitle(std::wstring(item.DisplayName()));
                        SendMessage(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
                    }
                    else if (hwnd == m_monitorComboBoxHwnd)
                    {
                        auto monitor = m_monitors[index];
                        auto item = m_app->StartCaptureFromMonitorHandle(monitor.Hmon());

                        SetSubTitle(std::wstring(item.DisplayName()));
                        SendMessageW(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
                    }
                }
                break;
            case BN_CLICKED:
                {
                    if (hwnd == m_pickerButtonHwnd)
                    {
                        OnPickerButtonClicked();
                    }
                    else if (hwnd == m_stopButtonHwnd)
                    {
                        m_app->StopCapture();
                        SetSubTitle(L"");
                        SendMessageW(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
                        SendMessageW(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
                    }
                }
                break;
            }
        }
        break;
    default:
        return base_type::MessageHandler(message, wparam, lparam);
        break;
    }

    return 0;
}

fire_and_forget SampleWindow::OnPickerButtonClicked()
{
    auto selectedItem = co_await m_app->StartCaptureWithPickerAsync();

    if (selectedItem)
    {
        SetSubTitle(std::wstring(selectedItem.DisplayName()));
        SendMessageW(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
        SendMessageW(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
    }
}

// Not DPI aware but could be by multiplying the constants based on the monitor scale factor

void SampleWindow::CreateControls(HINSTANCE instance)
{
    auto isWin32ProgrammaticPresent = winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 8);
    auto win32ProgrammaticStyle = isWin32ProgrammaticPresent ? 0 : WS_DISABLED;

    // Create window combo box
    HWND windowComboBoxHwnd = CreateWindowW(WC_COMBOBOX, L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | win32ProgrammaticStyle,
        10, 10, 200, 200, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(windowComboBoxHwnd);

    // Populate window combo box
    for (auto& window : m_windows)
    {
        SendMessageW(windowComboBoxHwnd, CB_ADDSTRING, 0, (LPARAM)window.Title().c_str());
    }

    // Create monitor combo box
    HWND monitorComboBoxHwnd = CreateWindowW(WC_COMBOBOX, L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | win32ProgrammaticStyle,
        10, 45, 200, 200, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(monitorComboBoxHwnd);

    // Populate monitor combo box
    for (auto& monitor : m_monitors)
    {
        SendMessageW(monitorComboBoxHwnd, CB_ADDSTRING, 0, (LPARAM)monitor.DisplayName().c_str());
    }

    // Create picker button
    HWND pickerButtonHwnd = CreateWindowW(WC_BUTTON, L"Use Picker",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 80, 200, 30, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(pickerButtonHwnd);

    // Create picker button
    HWND stopButtonHwnd = CreateWindowW(WC_BUTTON, L"Stop Capture",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 120, 200, 30, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(stopButtonHwnd);

    m_windowComboBoxHwnd = windowComboBoxHwnd;
    m_monitorComboBoxHwnd = monitorComboBoxHwnd;
    m_pickerButtonHwnd = pickerButtonHwnd;
    m_stopButtonHwnd = stopButtonHwnd;
}

void SampleWindow::SetSubTitle(const std::wstring& text)
{
    std::wstring titleText(L"Win32CaptureSample");
    if (!text.empty())
    {
        titleText += (L" - " + text);
    }
    SetWindowTextW(m_window, titleText.c_str());
}
