#pragma once

class App;
class WindowList;
class MonitorList;
struct BorderWindow;

struct SampleWindow : robmikh::common::desktop::DesktopWindow<SampleWindow>
{
    static const std::wstring ClassName;

    SampleWindow(int width, int height, std::shared_ptr<App> app);
    ~SampleWindow();

    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);

private:
    struct PixelFormatData
    {
        std::wstring Name;
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat PixelFormat;
    };

    enum class CaptureType
    {
        ProgrammaticWindow,
        ProgrammaticMonitor,
        Picker,
    };

    static void RegisterWindowClass();
    static LRESULT SubClassWndProc(HWND window, UINT const message, WPARAM const wparam, LPARAM const lparam);
    void CreateControls(HINSTANCE instance);
    void SetSubTitle(std::wstring const& text);
    winrt::fire_and_forget OnPickerButtonClicked();
    winrt::fire_and_forget OnSnapshotButtonClicked();
    void StopCapture();
    void OnCaptureItemClosed(winrt::Windows::Graphics::Capture::GraphicsCaptureItem const&, winrt::Windows::Foundation::IInspectable const&);
    void OnCaptureStarted(
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item, 
        CaptureType captureType);
    LRESULT WindowSelectionButtonMessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);
    LRESULT MonitorSelectionButtonMessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);
    void OnWindowSelected(HWND window);
    void OnMonitorSelected(HMONITOR monitor);

private:
    HWND m_windowComboBox = nullptr;
    HWND m_windowSelectButton = nullptr;
    WNDPROC m_windowSelectionButtonWndProc = nullptr;
    HWND m_monitorComboBox = nullptr;
    HWND m_monitorSelectButton = nullptr;
    WNDPROC m_monitorSelectionButtonWndProc = nullptr;
    HWND m_pickerButton = nullptr;
    HWND m_stopButton = nullptr;
    HWND m_snapshotButton = nullptr;
    HWND m_pixelFormatComboBox = nullptr;
    HWND m_cursorCheckBox = nullptr;
    HWND m_captureExcludeCheckBox = nullptr;
    HWND m_borderRequiredCheckBoxHwnd = nullptr;
    std::unique_ptr<WindowList> m_windows;
    std::unique_ptr<MonitorList> m_monitors;
    std::vector<PixelFormatData> m_pixelFormats;
    std::shared_ptr<App> m_app;
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem::Closed_revoker m_itemClosedRevoker;
    std::unique_ptr<BorderWindow> m_borderWindow;
    HWND m_currentPickerWindow = nullptr;
    HWND m_currentPickerCandidateWindow = nullptr;
    HMONITOR m_currentPickerMonitor = nullptr;
    HMONITOR m_currentPickerCandidateMonitor = nullptr;
    bool m_cursorCaptured = false;
};