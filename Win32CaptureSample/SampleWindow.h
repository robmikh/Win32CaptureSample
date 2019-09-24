#pragma once
#include "DesktopWindow.h"
#include "EnumerationWindow.h"
#include "EnumerationMonitor.h"

class App;

struct SampleWindow : DesktopWindow<SampleWindow>
{
    SampleWindow(HINSTANCE instance, int cmdShow, std::shared_ptr<App> app);

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
    std::vector<EnumerationWindow> m_windows;
    std::vector<EnumerationMonitor> m_monitors;
    std::vector<PixelFormatData> m_pixelFormats;
    std::shared_ptr<App> m_app;
};