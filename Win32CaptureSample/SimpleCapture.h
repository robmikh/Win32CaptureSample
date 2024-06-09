#pragma once
#include "DirtyRegionVisualizer.h"

class SimpleCapture
{
public:
    SimpleCapture(
        winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice const& device,
        std::shared_ptr<DirtyRegionVisualizer> const& dirtyRegionVisualizer,
        winrt::Windows::Graphics::Capture::GraphicsCaptureItem const& item,
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixelFormat);
    ~SimpleCapture() { Close(); }

    void StartCapture();
    winrt::Windows::UI::Composition::ICompositionSurface CreateSurface(
        winrt::Windows::UI::Composition::Compositor const& compositor);

    bool IsCursorEnabled() { CheckClosed(); return m_session.IsCursorCaptureEnabled(); }
	void IsCursorEnabled(bool value) { CheckClosed(); m_session.IsCursorCaptureEnabled(value); }
    bool IsBorderRequired() { CheckClosed(); return m_session.IsBorderRequired(); }
    void IsBorderRequired(bool value) { CheckClosed(); m_session.IsBorderRequired(value); }
    bool IncludeSecondaryWindows() { CheckClosed(); return m_session.IncludeSecondaryWindows(); }
    void IncludeSecondaryWindows(bool value) { CheckClosed(); m_session.IncludeSecondaryWindows(value); }
    winrt::Windows::Graphics::Capture::GraphicsCaptureDirtyRegionMode DirtyRegionMode() { CheckClosed(); return m_session.DirtyRegionMode(); }
    void DirtyRegionMode(winrt::Windows::Graphics::Capture::GraphicsCaptureDirtyRegionMode value) { CheckClosed(); m_session.DirtyRegionMode(value); }
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem CaptureItem() { return m_item; }

    void SetPixelFormat(winrt::Windows::Graphics::DirectX::DirectXPixelFormat pixelFormat)
    {
        CheckClosed();
        auto newFormat = std::optional(pixelFormat);
        m_pixelFormatUpdate.exchange(newFormat);
    }

    bool VisualizeDirtyRegions() { CheckClosed(); return m_visualizeDirtyRegions.load(); }
    void VisualizeDirtyRegions(bool value);

    winrt::Windows::Foundation::TimeSpan MinUpdateInterval() { CheckClosed(); return m_session.MinUpdateInterval(); }
    void MinUpdateInterval(winrt::Windows::Foundation::TimeSpan value) { CheckClosed(); m_session.MinUpdateInterval(value); }

    void Close();

private:
    void OnFrameArrived(
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
        winrt::Windows::Foundation::IInspectable const& args);

    inline void CheckClosed()
    {
        if (m_closed.load() == true)
        {
            throw winrt::hresult_error(RO_E_CLOSED);
        }
    }

    void ResizeSwapChain();
    bool TryResizeSwapChain(winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame const& frame);
    bool TryUpdatePixelFormat();

private:
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem m_item{ nullptr };
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession m_session{ nullptr };
    winrt::Windows::Graphics::SizeInt32 m_lastSize;

    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice m_device{ nullptr };
    winrt::com_ptr<IDXGISwapChain1> m_swapChain{ nullptr };
    winrt::com_ptr<ID3D11Device> m_d3dDevice{ nullptr };
    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext{ nullptr };
    winrt::Windows::Graphics::DirectX::DirectXPixelFormat m_pixelFormat;

    std::atomic<std::optional<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>> m_pixelFormatUpdate = std::nullopt;

    std::atomic<bool> m_closed = false;

    std::shared_ptr<DirtyRegionVisualizer> m_dirtyRegionVisualizer;
    std::atomic<bool> m_visualizeDirtyRegions = false;
};