#include "pch.h"
#include "DirtyRegionVisualizer.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
    using namespace Windows::System;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

namespace util
{
    using namespace robmikh::common::uwp;
}

DirtyRegionVisualizer::DirtyRegionVisualizer(winrt::com_ptr<ID3D11Device> const& d3dDevice)
{
    m_d2dFactory = util::CreateD2DFactory();
    m_d2dDevice = util::CreateD2DDevice(m_d2dFactory, d3dDevice);

    winrt::check_hresult(m_d2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, m_d2dContext.put()));
    
    D2D1_COLOR_F color = { 1.0f, 0.0f, 0.0f, 0.6f };
    winrt::check_hresult(m_d2dContext->CreateSolidColorBrush(color, m_brush.put()));
}

void DirtyRegionVisualizer::Render(
    winrt::com_ptr<ID3D11Texture2D> const& renderTargetTexture, 
    winrt::Direct3D11CaptureFrame const& captureFrame)
{
    auto dirtyRegion = captureFrame.DirtyRegions();
    if (dirtyRegion.Size() > 0)
    {
        auto dxgiTexture = renderTargetTexture.as<IDXGISurface>();
        winrt::com_ptr<ID2D1Bitmap1> d2dBitmap;
        winrt::check_hresult(m_d2dContext->CreateBitmapFromDxgiSurface(dxgiTexture.get(), nullptr, d2dBitmap.put()));

        m_d2dContext->SetTarget(d2dBitmap.get());
        auto unsetTarget = wil::scope_exit([d2dContext = m_d2dContext]()
            {
                d2dContext->SetTarget(nullptr);
            });

        m_d2dContext->BeginDraw();
        auto endDraw = wil::scope_exit([d2dContext = m_d2dContext]()
            {
                winrt::check_hresult(d2dContext->EndDraw());
            });

        for (auto&& dirtyRegion : dirtyRegion)
        {
            D2D1_RECT_F rect =
            {
                static_cast<float>(dirtyRegion.X),
                static_cast<float>(dirtyRegion.Y),
                static_cast<float>(dirtyRegion.X + dirtyRegion.Width),
                static_cast<float>(dirtyRegion.Y + dirtyRegion.Height),
            };

            m_d2dContext->FillRectangle(
                rect,
                m_brush.get());
        }
    }
}
