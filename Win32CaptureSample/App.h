#pragma once
#include "SimpleCapture.h"
#include "DirtyRegionVisualizer.h"

class App
{
public:
    App(winrt::Windows::UI::Composition::ContainerVisual root);
    ~App() {}

    winrt::Windows::Graphics::Capture::GraphicsCaptureItem TryStartCaptureFromWindowHandle(HWND hwnd);
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem TryStartCaptureFromMonitorHandle(HMONITOR hmon);
    winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Graphics::Capture::GraphicsCaptureItem> StartCaptureWithPickerAsync();
    winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Storage::StorageFile> TakeSnapshotAsync();
    winrt::Windows::Graphics::DirectX::DirectXPixelFormat PixelFormat() { return m_pixelFormat; }
    void PixelFormat(winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixelFormat);

    bool IsCursorEnabled();
    void IsCursorEnabled(bool value);
    bool IsBorderRequired();
    winrt::fire_and_forget IsBorderRequired(bool value);
    bool IncludeSecondaryWindows();
    void IncludeSecondaryWindows(bool value);

    bool VisualizeDirtyRegions();
    void VisualizeDirtyRegions(bool value);
    winrt::Windows::Graphics::Capture::GraphicsCaptureDirtyRegionMode DirtyRegionMode();
    void DirtyRegionMode(winrt::Windows::Graphics::Capture::GraphicsCaptureDirtyRegionMode value);

    winrt::Windows::Foundation::TimeSpan MinUpdateInterval();
    void MinUpdateInterval(winrt::Windows::Foundation::TimeSpan value);

    void StopCapture();
    void InitializeWithWindow(HWND window);

private:
    void StartCaptureFromItem(winrt::Windows::Graphics::Capture::GraphicsCaptureItem item);
    void InitializeObjectWithWindowHandle(winrt::Windows::Foundation::IUnknown const& object);

private:
    winrt::Windows::System::DispatcherQueue m_mainThread{ nullptr };
    winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
    winrt::Windows::UI::Composition::ContainerVisual m_root{ nullptr };
    winrt::Windows::UI::Composition::SpriteVisual m_content{ nullptr };
    winrt::Windows::UI::Composition::CompositionSurfaceBrush m_brush{ nullptr };

    HWND m_mainWindow = nullptr;

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    std::unique_ptr<SimpleCapture> m_capture{ nullptr };
    winrt::Windows::Graphics::DirectX::DirectXPixelFormat m_pixelFormat = winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized;
    std::shared_ptr<DirtyRegionVisualizer> m_dirtyRegionVisualizer;

    winrt::com_ptr<IWICImagingFactory2> m_wicFactory;
};