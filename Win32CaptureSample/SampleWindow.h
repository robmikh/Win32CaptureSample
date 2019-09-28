#pragma once
#include "DesktopWindow.h"
#include "EnumerationMonitor.h"

class App;
class WindowList;

struct SampleWindow : DesktopWindow<SampleWindow>
{
    static const std::wstring ClassName;
    static void RegisterWindowClass();

    SampleWindow(HINSTANCE instance, int cmdShow, std::shared_ptr<App> app);
    ~SampleWindow();

    winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget CreateWindowTarget(winrt::Windows::UI::Composition::Compositor const& compositor)
    {
        return CreateDesktopWindowTarget(compositor, m_window, true);
    }

    void InitializeObjectWithWindowHandle(winrt::Windows::Foundation::IUnknown const& object)
    {
        auto initializer = object.as<IInitializeWithWindow>();
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

private:
    HWND m_windowComboBoxHwnd = nullptr;
    HWND m_monitorComboBoxHwnd = nullptr;
    HWND m_pickerButtonHwnd = nullptr;
    HWND m_stopButtonHwnd = nullptr;
    HWND m_currentSnapshotHwnd = nullptr;
    HWND m_snapshotButtonHwnd = nullptr;
    std::unique_ptr<WindowList> m_windowList;
    std::vector<EnumerationMonitor> m_monitors;
    std::shared_ptr<App> m_app;
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem::Closed_revoker m_itemClosedRevoker;
};