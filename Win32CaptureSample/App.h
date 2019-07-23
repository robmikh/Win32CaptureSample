#pragma once
#include "SimpleCapture.h"

class App
{
public:
    App(winrt::Windows::UI::Composition::ContainerVisual root,
        winrt::Windows::Graphics::Capture::GraphicsCapturePicker capturePicker,
		winrt::Windows::Storage::Pickers::FileSavePicker savePicker);
    ~App() {}

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem StartCaptureFromWindowHandle(HWND hwnd);
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem StartCaptureFromMonitorHandle(HMONITOR hmon);
    winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Graphics::Capture::GraphicsCaptureItem> StartCaptureWithPickerAsync();
	winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFile> TakeSnapshotAsync();

    void StopCapture();

private:
    void StartCaptureFromItem(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item);

private:
    winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
    winrt::Windows::UI::Composition::ContainerVisual m_root{ nullptr };
    winrt::Windows::UI::Composition::SpriteVisual m_content{ nullptr };
    winrt::Windows::UI::Composition::CompositionSurfaceBrush m_brush{ nullptr };

    winrt::Windows::Graphics::Capture::GraphicsCapturePicker m_capturePicker{ nullptr };
	winrt::Windows::Storage::Pickers::FileSavePicker m_savePicker{ nullptr };

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    std::unique_ptr<SimpleCapture> m_capture{ nullptr };

    winrt::com_ptr<ID2D1Factory1> m_d2dFactory;
    winrt::com_ptr<ID2D1Device> m_d2dDevice;
    winrt::com_ptr<ID2D1DeviceContext> m_d2dContext;
};