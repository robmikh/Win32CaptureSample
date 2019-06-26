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

SampleWindow::SampleWindow(
    HINSTANCE instance,
    int cmdShow,
    std::shared_ptr<App> app)
{
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPLICATION));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = L"Win32CaptureSample";
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
    WINRT_VERIFY(RegisterClassEx(&wcex));
    WINRT_ASSERT(!m_window);

    WINRT_VERIFY(CreateWindow(
        L"Win32CaptureSample",
        L"Win32CaptureSample",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        800,
        600,
        NULL,
        NULL,
        instance,
        this));

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
                    auto index = SendMessage(hwnd, CB_GETCURSEL, 0, 0);
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
                        SendMessage(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
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
                        SendMessage(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
                        SendMessage(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
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
        SendMessage(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
        SendMessage(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
    }
}

void SampleWindow::CreateControls(HINSTANCE instance)
{
    auto isWin32ProgrammaticPresent = winrt::Windows::Foundation::Metadata::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 8);
    auto win32ProgrammaticStyle = isWin32ProgrammaticPresent ? 0 : WS_DISABLED;

    // Create window combo box
    HWND windowComboBoxHwnd = CreateWindow(
        WC_COMBOBOX,
        L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | win32ProgrammaticStyle,
        10,
        10,
        200,
        200,
        m_window,
        NULL,
        instance,
        NULL);
    WINRT_VERIFY(windowComboBoxHwnd);

    // Populate window combo box
    for (auto& window : m_windows)
    {
        SendMessage(windowComboBoxHwnd, CB_ADDSTRING, 0, (LPARAM)window.Title().c_str());
    }

    // Create monitor combo box
    HWND monitorComboBoxHwnd = CreateWindow(
        WC_COMBOBOX,
        L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | win32ProgrammaticStyle,
        10,
        45,
        200,
        200,
        m_window,
        NULL,
        instance,
        NULL);
    WINRT_VERIFY(monitorComboBoxHwnd);

    // Populate monitor combo box
    for (auto& monitor : m_monitors)
    {
        SendMessage(monitorComboBoxHwnd, CB_ADDSTRING, 0, (LPARAM)monitor.DisplayName().c_str());
    }

    // Create picker button
    HWND pickerButtonHwnd = CreateWindow(
        WC_BUTTON,
        L"Use Picker",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10,
        80,
        200,
        30,
        m_window,
        NULL,
        instance,
        NULL);
    WINRT_VERIFY(pickerButtonHwnd);

    // Create picker button
    HWND stopButtonHwnd = CreateWindow(
        WC_BUTTON,
        L"Stop Capture",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10,
        120,
        200,
        30,
        m_window,
        NULL,
        instance,
        NULL);
    WINRT_VERIFY(stopButtonHwnd);

    m_windowComboBoxHwnd = windowComboBoxHwnd;
    m_monitorComboBoxHwnd = monitorComboBoxHwnd;
    m_pickerButtonHwnd = pickerButtonHwnd;
    m_stopButtonHwnd = stopButtonHwnd;
}

void SampleWindow::SetSubTitle(std::wstring text)
{
    std::wstring titleText(L"Win32CaptureSample");
    if (!text.empty())
    {
        titleText += (L" - " + text);
    }
    SetWindowText(m_window, titleText.c_str());
}
