#include "pch.h"
#include "App.h"
#include "SampleWindow.h"
#include "WindowList.h"
#include "MonitorList.h"

namespace winrt
{
    using namespace Windows::Foundation::Metadata;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::System;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
    using namespace Windows::UI::Composition::Desktop;
}

namespace util
{
    using namespace robmikh::common::desktop::controls;
}

const std::wstring SampleWindow::ClassName = L"Win32CaptureSample.SampleWindow";
std::once_flag SampleWindowClassRegistration;

void SampleWindow::RegisterWindowClass()
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));
    WNDCLASSEX wcex = {};
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

SampleWindow::SampleWindow(int width, int height, std::shared_ptr<App> app)
{
    auto instance = winrt::check_pointer(GetModuleHandleW(nullptr));

    std::call_once(SampleWindowClassRegistration, []() { RegisterWindowClass(); });

    auto exStyle = 0;
    auto style = WS_OVERLAPPEDWINDOW;

    RECT rect = { 0, 0, width, height };
    winrt::check_bool(AdjustWindowRectEx(&rect, style, false, exStyle));
    auto adjustedWidth = rect.right - rect.left;
    auto adjustedHeight = rect.bottom - rect.top;

    winrt::check_bool(CreateWindowExW(exStyle, ClassName.c_str(), L"Win32CaptureSample", style,
        CW_USEDEFAULT, CW_USEDEFAULT, adjustedWidth, adjustedHeight, nullptr, nullptr, instance, this));
    WINRT_ASSERT(m_window);

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

    ShowWindow(m_window, SW_SHOW);
    UpdateWindow(m_window);
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
                else if (hwnd == m_pixelFormatComboBox)
                {
                    auto pixelFormatData = m_pixelFormats[index];
                    m_app->PixelFormat(pixelFormatData.PixelFormat);
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
                else if (hwnd == m_captureExcludeCheckBox)
                {
                    auto value = SendMessageW(m_captureExcludeCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    winrt::check_bool(SetWindowDisplayAffinity(m_window, value ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE));
                }
                else if (hwnd == m_borderRequiredCheckBoxHwnd)
                {
                    auto value = SendMessageW(m_borderRequiredCheckBoxHwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
                    m_app->IsBorderRequired(value);
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
        return util::StaticControlColorMessageHandler(wparam, lparam);
    default:
        return base_type::MessageHandler(message, wparam, lparam);
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
    SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(m_borderRequiredCheckBoxHwnd, BM_SETCHECK, BST_CHECKED, 0);
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

    // Border configuration
    auto isBorderRequiredPresent = winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(winrt::name_of<winrt::GraphicsCaptureSession>(), L"IsBorderRequired");
    auto borderEnableSytle = isBorderRequiredPresent ? 0 : WS_DISABLED;

    auto controls = util::StackPanel(m_window, instance, 10, 10, 40, 200, 30);

    auto windowLabel = controls.CreateControl(util::ControlType::Label, L"Windows:");

    // Create window combo box
    auto windowComboBox = controls.CreateControl(util::ControlType::ComboBox, L"", win32ProgrammaticStyle);

    // Populate window combo box and register for updates
    m_windows->RegisterComboBoxForUpdates(windowComboBox);

    auto monitorLabel = controls.CreateControl(util::ControlType::Label, L"Displays:");

    // Create monitor combo box
    auto monitorComboBox = controls.CreateControl(util::ControlType::ComboBox, L"", win32ProgrammaticStyle);

    // Populate monitor combo box
    m_monitors->RegisterComboBoxForUpdates(monitorComboBox);

    // Create picker button
    auto pickerButton = controls.CreateControl(util::ControlType::Button, L"Open Picker");

    // Create stop capture button
    auto stopButton = controls.CreateControl(util::ControlType::Button, L"Stop Capture", WS_DISABLED);

    // Create independent snapshot button
    auto snapshotButton = controls.CreateControl(util::ControlType::Button, L"Take Snapshot", WS_DISABLED);

    auto pixelFormatLabel = controls.CreateControl(util::ControlType::Label, L"Pixel Format:");

    // Create pixel format combo box
    auto pixelFormatComboBox = controls.CreateControl(util::ControlType::ComboBox, L"");

    // Populate pixel format combo box
    for (auto& pixelFormat : m_pixelFormats)
    {
        SendMessageW(pixelFormatComboBox, CB_ADDSTRING, 0, (LPARAM)pixelFormat.Name.c_str());
    }
    
    // The default pixel format is BGRA8
    SendMessageW(pixelFormatComboBox, CB_SETCURSEL, 0, 0);
  
    // Create cursor checkbox
    auto cursorCheckBox = controls.CreateControl(util::ControlType::CheckBox, L"Enable Cursor", cursorEnableStyle);

    // The default state is true for cursor rendering
    SendMessageW(cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);

    // Create capture exclude checkbox
    // NOTE: We don't version check this feature because setting WDA_EXCLUDEFROMCAPTURE is the same as
    //       setting WDA_MONITOR on older builds of Windows. We're changing the label here to try and 
    //       limit any user confusion.
    std::wstring excludeCheckBoxLabel = isWin32CaptureExcludePresent ? L"Exclude this window" : L"Block this window";
    auto captureExcludeCheckBox = controls.CreateControl(util::ControlType::CheckBox, excludeCheckBoxLabel.c_str());

    // The default state is false for capture exclusion
    SendMessageW(captureExcludeCheckBox, BM_SETCHECK, BST_UNCHECKED, 0);

    // Border required checkbox
    HWND borderRequiredCheckBoxHwnd = controls.CreateControl(util::ControlType::CheckBox, L"Border required", borderEnableSytle);

    // The default state is false for border required checkbox
    SendMessageW(borderRequiredCheckBoxHwnd, BM_SETCHECK, BST_CHECKED, 0);

    m_windowComboBox = windowComboBox;
    m_monitorComboBox = monitorComboBox;
    m_pickerButton = pickerButton;
    m_stopButton = stopButton;
    m_snapshotButton = snapshotButton;
    m_cursorCheckBox = cursorCheckBox;
    m_captureExcludeCheckBox = captureExcludeCheckBox;
    m_pixelFormatComboBox = pixelFormatComboBox;
    m_borderRequiredCheckBoxHwnd = borderRequiredCheckBoxHwnd;
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
    SendMessageW(m_windowComboBox, CB_SETCURSEL, -1, 0);
    SendMessageW(m_monitorComboBox, CB_SETCURSEL, -1, 0);
    SendMessageW(m_cursorCheckBox, BM_SETCHECK, BST_CHECKED, 0);
    SendMessageW(m_borderRequiredCheckBoxHwnd, BM_SETCHECK, BST_CHECKED, 0);
    EnableWindow(m_stopButton, false);
    EnableWindow(m_snapshotButton, false);
}

void SampleWindow::OnCaptureItemClosed(winrt::GraphicsCaptureItem const&, winrt::Windows::Foundation::IInspectable const&)
{
    StopCapture();
}