#include "pch.h"
#include "App.h"
#include "SampleWindow.h"
#include "WindowList.h"
#include "MonitorList.h"
#include <CommCtrl.h>

namespace winrt
{
    using namespace Windows::Foundation::Metadata;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::System;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
    using namespace Windows::UI::Composition::Desktop;
    using namespace Windows::Graphics::DirectX;
}

const std::wstring SampleWindow::ClassName = L"Win32CaptureSample";

void SampleWindow::RegisterWindowClass()
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    WNDCLASSEX wcex = { sizeof(wcex) };
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

SampleWindow::SampleWindow(HINSTANCE instance, int cmdShow, std::shared_ptr<App> app)
{
    WINRT_ASSERT(!m_window);
    WINRT_VERIFY(CreateWindowW(ClassName.c_str(), L"Win32CaptureSample", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    ShowWindow(m_window, cmdShow);
    UpdateWindow(m_window);

    auto isAllDisplaysPresent = winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 9);

    m_app = app;
    m_windows = std::make_unique<WindowList>();
    m_monitors = std::make_unique<MonitorList>(isAllDisplaysPresent);
    m_pixelFormats = 
    {
        { L"B8G8R8A8UIntNormalized", winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized },
        { L"R16G16B16A16Float", winrt::DirectXPixelFormat::R16G16B16A16Float }
    };

    CreateControls(instance);
}

SampleWindow::~SampleWindow()
{
    m_windows.reset();
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
                    auto window = m_windows->GetCurrentWindows()[index];
                    m_itemClosedRevoker.revoke();
                    auto item = m_app->StartCaptureFromWindowHandle(window.WindowHandle);
                    m_itemClosedRevoker = item.Closed(winrt::auto_revoke, { this, &SampleWindow::OnCaptureItemClosed });

                    SetSubTitle(std::wstring(item.DisplayName()));
                    SendMessageW(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
                }
                else if (hwnd == m_monitorComboBoxHwnd)
                {
                    auto monitor = m_monitors->GetCurrentMonitors()[index];
                    m_itemClosedRevoker.revoke();
                    auto item = m_app->StartCaptureFromMonitorHandle(monitor.MonitorHandle);
                    m_itemClosedRevoker = item.Closed(winrt::auto_revoke, { this, &SampleWindow::OnCaptureItemClosed });

                    SetSubTitle(std::wstring(item.DisplayName()));
                    SendMessageW(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
                }
                else if (hwnd == m_pixelFormatComboBoxHwnd)
                {
                    auto pixelFormatData = m_pixelFormats[index];
                    m_app->PixelFormat(pixelFormatData.PixelFormat);
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
                    StopCapture();
                }
                else if (hwnd == m_currentSnapshotHwnd)
                {
                    m_app->SnapshotCurrentCapture();
                }
                else if (hwnd == m_snapshotButtonHwnd)
                {
                    OnSnapshotButtonClicked();
                }
            }
            break;
        }
    }
    break;
    case WM_DISPLAYCHANGE:
    {
        m_monitors->Update();
    }
    break;
    default:
        return base_type::MessageHandler(message, wparam, lparam);
        break;
    }

    return 0;
}

winrt::fire_and_forget SampleWindow::OnPickerButtonClicked()
{
    auto selectedItem = co_await m_app->StartCaptureWithPickerAsync();

    if (selectedItem)
    {
        m_itemClosedRevoker.revoke();
        m_itemClosedRevoker = selectedItem.Closed(winrt::auto_revoke, { this, &SampleWindow::OnCaptureItemClosed });
        SetSubTitle(std::wstring(selectedItem.DisplayName()));
        SendMessageW(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
        SendMessageW(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
    }
}

winrt::fire_and_forget SampleWindow::OnSnapshotButtonClicked()
{
    auto file = co_await m_app->TakeSnapshotAsync();
    if (file != nullptr)
    {
        co_await winrt::Launcher::LaunchFileAsync(file);
    }
}

// Not DPI aware but could be by multiplying the constants based on the monitor scale factor
void SampleWindow::CreateControls(HINSTANCE instance)
{
    auto isWin32ProgrammaticPresent = winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 8);
    auto win32ProgrammaticStyle = isWin32ProgrammaticPresent ? 0 : WS_DISABLED;

    // Create window combo box
    HWND windowComboBoxHwnd = CreateWindowW(WC_COMBOBOX, L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | win32ProgrammaticStyle,
        10, 10, 200, 200, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(windowComboBoxHwnd);

    // Populate window combo box and register for updates
    m_windows->RegisterComboBoxForUpdates(windowComboBoxHwnd);

    // Create monitor combo box
    HWND monitorComboBoxHwnd = CreateWindowW(WC_COMBOBOX, L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE | win32ProgrammaticStyle,
        10, 45, 200, 200, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(monitorComboBoxHwnd);

    // Populate monitor combo box
    m_monitors->RegisterComboBoxForUpdates(monitorComboBoxHwnd);

    // Create picker button
    HWND pickerButtonHwnd = CreateWindowW(WC_BUTTON, L"Use Picker",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 80, 200, 30, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(pickerButtonHwnd);

    // Create stop capture button
    HWND stopButtonHwnd = CreateWindowW(WC_BUTTON, L"Stop Capture",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 120, 200, 30, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(stopButtonHwnd);

    // Create current snapshot button
    HWND currentSnapshotButtonHwnd = CreateWindowW(WC_BUTTON, L"Snapshot Current Capture",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 160, 200, 30, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(currentSnapshotButtonHwnd);

    // Create independent snapshot button
    HWND snapshotButtonHwnd = CreateWindowW(WC_BUTTON, L"Take Independent Snapshot",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 200, 200, 30, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(snapshotButtonHwnd);

    // Create pixel format combo box
    HWND pixelFormatComboBox = CreateWindowW(WC_COMBOBOX, L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
        10, 240, 200, 200, m_window, nullptr, instance, nullptr);
    WINRT_VERIFY(pixelFormatComboBox);

    // Populate pixel format combo box
    for (auto& pixelFormat : m_pixelFormats)
    {
        SendMessageW(pixelFormatComboBox, CB_ADDSTRING, 0, (LPARAM)pixelFormat.Name.c_str());
    }
    
    // The default pixel format is BGRA8
    SendMessageW(pixelFormatComboBox, CB_SETCURSEL, 0, 0);

    m_windowComboBoxHwnd = windowComboBoxHwnd;
    m_monitorComboBoxHwnd = monitorComboBoxHwnd;
    m_pickerButtonHwnd = pickerButtonHwnd;
    m_stopButtonHwnd = stopButtonHwnd;
    m_currentSnapshotHwnd = currentSnapshotButtonHwnd;
    m_snapshotButtonHwnd = snapshotButtonHwnd;
    m_pixelFormatComboBoxHwnd = pixelFormatComboBox;
}

void SampleWindow::SetSubTitle(std::wstring const& text)
{
    std::wstring titleText(L"Win32CaptureSample");
    if (!text.empty())
    {
        titleText += (L" - " + text);
    }
    SetWindowTextW(m_window, titleText.c_str());
}

void SampleWindow::StopCapture()
{
    m_app->StopCapture();
    SetSubTitle(L"");
    SendMessageW(m_monitorComboBoxHwnd, CB_SETCURSEL, -1, 0);
    SendMessageW(m_windowComboBoxHwnd, CB_SETCURSEL, -1, 0);
}

void SampleWindow::OnCaptureItemClosed(winrt::GraphicsCaptureItem const&, winrt::Windows::Foundation::IInspectable const&)
{
    StopCapture();
}