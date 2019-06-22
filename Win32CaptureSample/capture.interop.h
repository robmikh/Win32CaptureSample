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
    auto activation_factory = winrt::get_activation_factory<winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
    auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item = { nullptr };
    interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(item)));
    return item;
}

inline auto CreateCapturePickerForHwnd(HWND hwnd)
{
    auto picker = winrt::Windows::Graphics::Capture::GraphicsCapturePicker();
    auto initializer = picker.as<IInitializeWithWindow>();
    winrt::check_hresult(initializer->Initialize(hwnd));
    return picker;
}