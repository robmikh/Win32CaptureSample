#pragma once
#include <winrt/Windows.Graphics.Capture.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.capture.h>

// Taken from shobjidl_core.h
struct __declspec(uuid("3E68D4BD-7135-4D10-8018-9FB6D9F33FA1"))
    IInitializeWithWindow : ::IUnknown
{
    virtual HRESULT __stdcall Initialize(HWND hwnd) = 0;
};

inline auto CreateCaptureItemForWindow(HWND hwnd)
{
    auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
    winrt::check_hresult(interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
    return item;
}

inline auto CreateCaptureItemForMonitor(HMONITOR hmon)
{
    auto interop_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
    winrt::check_hresult(interop_factory->CreateForMonitor(hmon, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), winrt::put_abi(item)));
    return item;
}

inline auto CreateCapturePickerForHwnd(HWND hwnd)
{
    auto picker = winrt::Windows::Graphics::Capture::GraphicsCapturePicker();
    winrt::check_hresult(picker.as<IInitializeWithWindow>()->Initialize(hwnd));
    return picker;
}