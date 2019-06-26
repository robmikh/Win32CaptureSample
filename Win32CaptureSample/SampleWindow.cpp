#include "pch.h"
#include "App.h"
#include "SampleWindow.h"

#include <CommCtrl.h>

using namespace winrt;
using namespace Windows::Graphics::Capture;
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
                        m_app->StartCaptureFromWindowHandle(window.Hwnd());

                        SendMessage(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
                    }
                    else if (hwnd == m_monitorComboBoxHwnd)
                    {
                        auto monitor = m_monitors[index];
                        m_app->StartCaptureFromMonitorHandle(monitor.Hmon());

                        SendMessage(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
                    }
                }
                break;
            case BN_CLICKED:
                {
                    // Because we aren't tracking the async operation, we have no clue if
                    // the user is going to select something from the picker or hit cancel.
                    // To get around that, just stop capture whenever this button is clicked.
                    m_app->StopCapture();
                    auto ignored = m_app->StartCaptureWithPickerAsync();

                    SendMessage(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
                    SendMessage(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
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

void SampleWindow::CreateControls(HINSTANCE instance)
{
    // Create window combo box
    HWND windowComboBoxHwnd = CreateWindow(
        WC_COMBOBOX,
        L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
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
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
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

    // Create button
    HWND buttonHwnd = CreateWindow(
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
    WINRT_VERIFY(buttonHwnd);

    m_windowComboBoxHwnd = windowComboBoxHwnd;
    m_monitorComboBoxHwnd = monitorComboBoxHwnd;
    m_buttonHwnd = buttonHwnd;
}

