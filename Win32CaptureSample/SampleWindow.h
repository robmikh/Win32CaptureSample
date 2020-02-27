#pragma once
#include "util/DesktopWindow.h"

class App;
class WindowList;
class MonitorList;

struct SampleWindow : util::desktop::DesktopWindow<SampleWindow>
{
    static const std::wstring ClassName;
    static void RegisterWindowClass();

    SampleWindow(HINSTANCE instance, int cmdShow, std::shared_ptr<App> app);
    ~SampleWindow();

    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget CreateWindowTarget(winrt::Windows::UI::Composition::Compositor const& compositor)
    {
        return util::desktop::CreateDesktopWindowTarget(compositor, m_window, true);
    }

    void InitializeObjectWithWindowHandle(winrt::Windows::Foundation::IUnknown const& object)
    {
        auto initializer = object.as<util::desktop::IInitializeWithWindow>();
        winrt::check_hresult(initializer->Initialize(m_window));
    }

    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);

private:
    void CreateControls(HINSTANCE instance);
    void SetSubTitle(std::wstring const& text);
    winrt::fire_and_forget OnPickerButtonClicked();
    winrt::fire_and_forget OnSnapshotButtonClicked();
    void StopCapture();
    void OnCaptureItemClosed(winrt::Windows::Graphics::Capture::GraphicsCaptureItem const&, winrt::Windows::Foundation::IInspectable const&);

    struct PixelFormatData
    {
        std::wstring Name;
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat PixelFormat;
    };

private:
    HWND m_windowComboBoxHwnd = nullptr;
    HWND m_monitorComboBoxHwnd = nullptr;
    HWND m_pickerButtonHwnd = nullptr;
    HWND m_stopButtonHwnd = nullptr;
    HWND m_currentSnapshotHwnd = nullptr;
    HWND m_snapshotButtonHwnd = nullptr;
    HWND m_pixelFormatComboBoxHwnd = nullptr;
    std::unique_ptr<WindowList> m_windows;
    std::unique_ptr<MonitorList> m_monitors;
    std::vector<PixelFormatData> m_pixelFormats;
    std::shared_ptr<App> m_app;
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem::Closed_revoker m_itemClosedRevoker;
};