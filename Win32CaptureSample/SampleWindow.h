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

    void InitializeObejctWithWindowHandle(winrt::Windows::Foundation::IUnknown const& object)
    {
        auto initializer = object.as<IInitializeWithWindow>();
        winrt::check_hresult(initializer->Initialize(m_window));
    }

    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);

private:
    void CreateControls(HINSTANCE instance);
    void SetSubTitle(const std::wstring& text);
    winrt::fire_and_forget OnPickerButtonClicked();

    HWND m_windowComboBoxHwnd = nullptr;
    HWND m_monitorComboBoxHwnd = nullptr;
    HWND m_pickerButtonHwnd = nullptr;
    HWND m_stopButtonHwnd = nullptr;
    std::vector<EnumerationWindow> m_windows;
    std::vector<EnumerationMonitor> m_monitors;
    std::shared_ptr<App> m_app;
};