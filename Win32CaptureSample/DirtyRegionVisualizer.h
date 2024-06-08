#pragma once

class DirtyRegionVisualizer
{
public:
    DirtyRegionVisualizer(
        winrt::com_ptr<ID3D11Device> const& d3dDevice);
    ~DirtyRegionVisualizer() {}

    void Render(
        winrt::com_ptr<ID3D11Texture2D> const& renderTargetTexture,
        winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame const& captureFrame);

private:
    winrt::com_ptr<ID2D1Device> m_d2dDevice{ nullptr };
    winrt::com_ptr<ID2D1Factory1> m_d2dFactory{ nullptr };
    winrt::com_ptr<ID2D1DeviceContext> m_d2dContext{ nullptr };
    winrt::com_ptr<ID2D1SolidColorBrush> m_brush{ nullptr };
};