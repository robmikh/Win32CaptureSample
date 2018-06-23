#pragma once

class SimpleCapture;

class App
{
public:
    App() {}
    ~App() {}

    void Run(
        winrt::Windows::UI::Composition::ContainerVisual root,
        winrt::Windows::Graphics::Capture::GraphicsCapturePicker picker);
private:
    winrt::Windows::Foundation::IAsyncAction StartCaptureAsync();

private:
    winrt::Windows::UI::Composition::Compositor m_compositor{ nullptr };
    winrt::Windows::UI::Composition::SpriteVisual m_root{ nullptr };
    winrt::Windows::UI::Composition::SpriteVisual m_content{ nullptr };
    winrt::Windows::UI::Composition::CompositionSurfaceBrush m_brush{ nullptr };

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    std::unique_ptr<SimpleCapture> m_capture{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCapturePicker m_picker{ nullptr };
};