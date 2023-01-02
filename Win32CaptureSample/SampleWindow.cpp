#include "pch.h"
#include "App.h"
#include "SampleWindow.h"
#include "WindowList.h"
#include "MonitorList.h"
#include "ControlsHelper.h"

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

// SPOUT - change Class name from "Win32CaptureSample" to "SpoutWinCapture"
const std::wstring SampleWindow::ClassName = L"SpoutWinCapture";

void SampleWindow::RegisterWindowClass()
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    //
    // robmikh commit - 14-05-22
    // fix window class description init 
    // WNDCLASSEX wcex = { sizeof(wcex) };
    WNDCLASSEX wcex = { };
    wcex.cbSize = sizeof(wcex);
    //
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = instance;
    wcex.hIcon = LoadIconW(instance, IDI_APPLICATION);
    wcex.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    // SPOUT
    // wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.hbrBackground = CreateSolidBrush(0x828484);
    wcex.lpszClassName = ClassName.c_str();
    wcex.hIconSm = LoadIconW(wcex.hInstance, IDI_APPLICATION);
    winrt::check_bool(RegisterClassExW(&wcex));
}

// SPOUT
// Allow for a command line, passed from main.cpp
// SampleWindow::SampleWindow(HINSTANCE instance, int cmdShow, std::shared_ptr<App> app)
SampleWindow::SampleWindow(HINSTANCE instance, LPSTR lpCmdLine, int cmdShow, std::shared_ptr<App> app)
{
    WINRT_ASSERT(!m_window);
    // SPOUT - change title from "Win32CaptureSample" to "SpoutWinCapture"
    WINRT_VERIFY(CreateWindowW(ClassName.c_str(), L"SpoutWinCapture",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

    // SPOUT
    // ============================================
    // Easy way to add an icon without changing the code
    HICON hWindowIcon = reinterpret_cast<HICON>(LoadImage(nullptr, L"Windows.ico", IMAGE_ICON, 16, 16, LR_LOADFROMFILE));
    SendMessage(m_window, WM_SETICON, ICON_BIG, (LPARAM)hWindowIcon);
    SendMessage(m_window, WM_SETICON, ICON_SMALL, (LPARAM)hWindowIcon);
    // Centre the window on the desktop work area
    RECT rc ={0, 0, 800, 600}; // Client size
    GetWindowRect(m_window, &rc);
    RECT WorkArea;
    int WindowPosLeft = 0;
    int WindowPosTop = 0;
    SystemParametersInfo(SPI_GETWORKAREA, 0, (LPVOID)&WorkArea, 0);
    WindowPosLeft += ((WorkArea.right - WorkArea.left) - (rc.right - rc.left)) / 2;
    WindowPosTop += ((WorkArea.bottom - WorkArea.top) - (rc.bottom - rc.top)) / 2;
    MoveWindow(m_window, WindowPosLeft, WindowPosTop, (rc.right - rc.left), (rc.bottom - rc.top), false);
    // ============================================

    // SPOUT
    // ============================================
    // Command line window capture
    // Find the window now so the main window can be minimized
    HWND hwndcapture = nullptr;
    if (lpCmdLine && *lpCmdLine) {
        // Remove double quotes
        std::string str = lpCmdLine;
        str.erase(std::remove(str.begin(), str.end(), '"'), str.end());
        // Find the window handle from it's caption
        hwndcapture = FindWindowA(NULL, str.c_str());
    }
    
    // Minimize the main window if a command line sender was found
    if (hwndcapture > 0)
        ShowWindow(m_window, SW_MINIMIZE);
    else
        ShowWindow(m_window, cmdShow);
    // ============================================

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

    // SPOUT
    // ============================================
    // Command line window capture after starting
    if (hwndcapture > 0) {
        // Restore if minimized 
        if (IsIconic(hwndcapture)) ShowWindow(hwndcapture, SW_RESTORE);
        // TryStartCapture for the window
        auto item = m_app->TryStartCaptureFromWindowHandle(hwndcapture);
        if (item != nullptr) {
            // Capture from this window straight away
            OnCaptureStarted(item, CaptureType::ProgrammaticWindow);
        }
    }
    // ============================================

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
                if (hwnd == m_windowComboBox)
                {
                    auto window = m_windows->GetCurrentWindows()[index];
                    auto item = m_app->TryStartCaptureFromWindowHandle(window.WindowHandle);
                    if (item != nullptr)
                    {
                        OnCaptureStarted(item, CaptureType::ProgrammaticWindow);
                    }
                }
                else if (hwnd == m_monitorComboBox)
                {
                    auto monitor = m_monitors->GetCurrentMonitors()[index];
                    auto item = m_app->TryStartCaptureFromMonitorHandle(monitor.MonitorHandle);
                    if (item != nullptr)
                    {
                        OnCaptureStarted(item, CaptureType::ProgrammaticMonitor);
                    }
                }
            }
            break;

        case BN_CLICKED:
            {
                if (hwnd == m_pickerButton)
                {
                     OnPickerButtonClicked();
                }
                else if (hwnd == m_stopButton)
                {
                    StopCapture();
                }
                else if (hwnd == m_snapshotButton)
                {
                    OnSnapshotButtonClicked();
                }
                else if (hwnd == m_cursorCheckBox)
                {
                    auto value = SendMessageW(m_cursorCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    m_app->IsCursorEnabled(value);
                }
                // SPOUT client area capture
                else if (hwnd == m_clientCheckBox)
                {
                    auto value = SendMessageW(m_clientCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    m_app->IsClientEnabled(value);
                }
                else if (hwnd == m_captureExcludeCheckBox)
                {
                    auto value = SendMessageW(m_captureExcludeCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    winrt::check_bool(SetWindowDisplayAffinity(m_window, value ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE));
                }
        }
            break;
        }
    }
    break;
    case WM_DISPLAYCHANGE:
        m_monitors->Update();
        break;
    case WM_CTLCOLORSTATIC:
    {
        HDC staticColorHdc = reinterpret_cast<HDC>(wparam);
        auto color = static_cast<COLORREF>(GetSysColor(COLOR_WINDOW));
        SetBkColor(staticColorHdc, color);
        SetDCBrushColor(staticColorHdc, color);
        return reinterpret_cast<LRESULT>(GetStockObject(DC_BRUSH));
    }
    break;
    default:
        return base_type::MessageHandler(message, wparam, lparam);
        break;
    }

    return 0;
}

void SampleWindow::OnCaptureStarted(winrt::GraphicsCaptureItem const& item, CaptureType captureType)
{
    m_itemClosedRevoker.revoke();
    m_itemClosedRevoker = item.Closed(winrt::auto_revoke, { this, &SampleWindow::OnCaptureItemClosed });
    SetSubTitle(std::wstring(item.DisplayName()));

    switch (captureType)
    {
    case CaptureType::ProgrammaticWindow:
        SendMessageW(m_monitorComboBox, CB_SETCURSEL, -1, 0);
        break;
    case CaptureType::ProgrammaticMonitor:
        SendMessageW(m_windowComboBox, CB_SETCURSEL, -1, 0);
        break;
    case CaptureType::Picker:
        SendMessageW(m_windowComboBox, CB_SETCURSEL, -1, 0);
        SendMessageW(m_monitorComboBox, CB_SETCURSEL, -1, 0);
        break;
    }

    // SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_UNCHECKED, 0); // SPOUT - default no cursor
    m_app->IsCursorEnabled(false);
    SendMessageW(m_clientCheckBox, BM_SETCHECK, BST_CHECKED, 0); // SPOUT - default send client area

    EnableWindow(m_stopButton, true);
    EnableWindow(m_snapshotButton, true);
}

winrt::fire_and_forget SampleWindow::OnPickerButtonClicked()
{
    auto selectedItem = co_await m_app->StartCaptureWithPickerAsync();
    if (selectedItem)
    {
        OnCaptureStarted(selectedItem, CaptureType::Picker);
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
    // Programmatic capture
    auto isWin32ProgrammaticPresent = winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 8);
    auto win32ProgrammaticStyle = isWin32ProgrammaticPresent ? 0 : WS_DISABLED;

    // Cursor capture
    auto isCursorEnablePresent = winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 9);
    auto cursorEnableStyle = isCursorEnablePresent ? 0 : WS_DISABLED;

    // Window exclusion
    auto isWin32CaptureExcludePresent = winrt::ApiInformation::IsApiContractPresent(L"Windows.Foundation.UniversalApiContract", 9);

    auto controls = StackPanel(m_window, instance, 10, 10, 200);

    auto windowLabel = controls.CreateControl(ControlType::Label, L"Windows:");

    // Create window combo box
    auto windowComboBox = controls.CreateControl(ControlType::ComboBox, L"", win32ProgrammaticStyle);

    // Populate window combo box and register for updates
    m_windows->RegisterComboBoxForUpdates(windowComboBox);

    auto monitorLabel = controls.CreateControl(ControlType::Label, L"Displays:");

    // Create monitor combo box
    auto monitorComboBox = controls.CreateControl(ControlType::ComboBox, L"", win32ProgrammaticStyle);

    // Populate monitor combo box
    m_monitors->RegisterComboBoxForUpdates(monitorComboBox);

    // Create picker button
    auto pickerButton = controls.CreateControl(ControlType::Button, L"Open Picker");

    // Create stop capture button
    auto stopButton = controls.CreateControl(ControlType::Button, L"Stop Capture", WS_DISABLED);

    // Create independent snapshot button
    auto snapshotButton = controls.CreateControl(ControlType::Button, L"Take Snapshot", WS_DISABLED);

    // Create cursor checkbox
    auto cursorCheckBox = controls.CreateControl(ControlType::CheckBox, L"Enable Cursor", cursorEnableStyle);

    // The default state is true for cursor rendering
    // SendMessageW(cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
    // SPOUT - default no cursor
    SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);

    // SPOUT
    // Create send client area checkbox
    auto clientCheckBox = controls.CreateControl(ControlType::CheckBox, L"Send client area", cursorEnableStyle);
    // The default state is true
    SendMessageW(clientCheckBox, BM_SETCHECK, BST_CHECKED, 0);

    // Create capture exclude checkbox
    // NOTE: We don't version check this feature because setting WDA_EXCLUDEFROMCAPTURE is the same as
    //       setting WDA_MONITOR on older builds of Windows. We're changing the label here to try and 
    //       limit any user confusion.
    std::wstring excludeCheckBoxLabel = isWin32CaptureExcludePresent ? L"Exclude this window" : L"Block this window";
    auto captureExcludeCheckBox = controls.CreateControl(ControlType::CheckBox, excludeCheckBoxLabel.c_str());

    // The default state is false for capture exclusion
    SendMessageW(captureExcludeCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);

    m_windowComboBox = windowComboBox;
    m_monitorComboBox = monitorComboBox;
    m_pickerButton = pickerButton;
    m_stopButton = stopButton;
    m_snapshotButton = snapshotButton;
    m_cursorCheckBox = cursorCheckBox;
    m_captureExcludeCheckBox = captureExcludeCheckBox;
    m_clientCheckBox = clientCheckBox; // Add a client/window capture checkbox

}


void SampleWindow::SetSubTitle(std::wstring const& text)
{
    // SPOUT
    // Change name from "Win32CaptureSample" to "SpoutWinCapture"
    std::wstring titleText(L"SpoutWinCapture");
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
    SendMessageW(m_windowComboBox, CB_SETCURSEL, -1, 0);
    SendMessageW(m_monitorComboBox, CB_SETCURSEL, -1, 0);
    // SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_UNCHECKED, 0); // SPOUT - default no cursor
    SendMessageW(m_clientCheckBox, BM_SETCHECK, BST_CHECKED, 0); // SPOUT - default send client area

    EnableWindow(m_stopButton, false);
    EnableWindow(m_snapshotButton, false);
}

void SampleWindow::OnCaptureItemClosed(winrt::GraphicsCaptureItem const&, winrt::Windows::Foundation::IInspectable const&)
{
    StopCapture();
}