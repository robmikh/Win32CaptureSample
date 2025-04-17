#include "pch.h"
#include "SimpleCapture.h"

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

SimpleCapture::SimpleCapture(
    winrt::IDirect3DDevice const& device,
    std::shared_ptr<DirtyRegionVisualizer> const& dirtyRegionVisualizer,
    winrt::GraphicsCaptureItem const& item, 
    winrt::DirectXPixelFormat pixelFormat)
{
    m_item = item;
    m_device = device;
    m_pixelFormat = pixelFormat;
    m_dirtyRegionVisualizer = dirtyRegionVisualizer;

    m_d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    m_d3dDevice->GetImmediateContext(m_d3dContext.put());

    auto format = static_cast<DXGI_FORMAT>(m_pixelFormat);
    m_swapChain = util::CreateDXGISwapChain(m_d3dDevice, static_cast<uint32_t>(m_item.Size().Width), static_cast<uint32_t>(m_item.Size().Height),
        format, 2).as<IDXGISwapChain3>();
    winrt::check_hresult(m_swapChain->SetColorSpace1(GetColorSpaceFromPixelFormat(format)));

    // We use 'CreateFreeThreaded' instead of 'Create' so that the FrameArrived
    // event fires on a thread other than our UI thread. If you use the 'Create' 
    // method, it's best not to do it on the UI thread. Using the 'Create' method
    // also means you must have a DispatcherQueue on that thread and you must be
    // pumping messages.
    m_framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, m_pixelFormat, 2, m_item.Size());
    m_session = m_framePool.CreateCaptureSession(m_item);
    m_lastSize = m_item.Size();
    m_framePool.FrameArrived({ this, &SimpleCapture::OnFrameArrived });
}

void SimpleCapture::StartCapture()
{
    CheckClosed();
    m_session.StartCapture();
}

winrt::ICompositionSurface SimpleCapture::CreateSurface(winrt::Compositor const& compositor)
{
    CheckClosed();
    return util::CreateCompositionSurfaceForSwapChain(compositor, m_swapChain.get());
}

void SimpleCapture::VisualizeDirtyRegions(bool value)
{
    CheckClosed();
    if (m_dirtyRegionVisualizer != nullptr) {
        auto expected = !value;
        m_visualizeDirtyRegions.compare_exchange_strong(expected, value);
    }
}

void SimpleCapture::Close()
{
    auto expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {
        m_session.Close();
        m_framePool.Close();

        m_swapChain = nullptr;
        m_framePool = nullptr;
        m_session = nullptr;
        m_item = nullptr;
    }
}

void SimpleCapture::ResizeSwapChain()
{
    auto format = static_cast<DXGI_FORMAT>(m_pixelFormat);
    winrt::check_hresult(m_swapChain->ResizeBuffers(2, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height),
        format, 0));
    winrt::check_hresult(m_swapChain->SetColorSpace1(GetColorSpaceFromPixelFormat(format)));
}

bool SimpleCapture::TryResizeSwapChain(winrt::Direct3D11CaptureFrame const& frame)
{
    auto const contentSize = frame.ContentSize();
    if ((contentSize.Width != m_lastSize.Width) ||
        (contentSize.Height != m_lastSize.Height))
    {
        // The thing we have been capturing has changed size, resize the swap chain to match.
        m_lastSize = contentSize;
        ResizeSwapChain();
        return true;
    }
    return false;
}

bool SimpleCapture::TryUpdatePixelFormat()
{
    auto newFormat = m_pixelFormatUpdate.exchange(std::nullopt);
    if (newFormat.has_value())
    {
        auto pixelFormat = newFormat.value();
        if (pixelFormat != m_pixelFormat)
        {
            m_pixelFormat = pixelFormat;
            ResizeSwapChain();
            return true;
        }
    }
    return false;
}

DXGI_COLOR_SPACE_TYPE SimpleCapture::GetColorSpaceFromPixelFormat(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
    default:
        throw winrt::hresult_error(E_INVALIDARG, L"Unknown color space for pixel format.");
    }
}

void SimpleCapture::OnFrameArrived(winrt::Direct3D11CaptureFramePool const& sender, winrt::IInspectable const&)
{
    auto swapChainResizedToFrame = false;

    {
        auto frame = sender.TryGetNextFrame();
        swapChainResizedToFrame = TryResizeSwapChain(frame);

        winrt::com_ptr<ID3D11Texture2D> backBuffer;
        winrt::check_hresult(m_swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void()));

        auto surfaceTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

        // If we have a dirty region visualizer, then we're running on a build
        // of Windows that supports dirty regions.
        bool renderRects = m_dirtyRegionVisualizer && frame.DirtyRegionMode() == winrt::GraphicsCaptureDirtyRegionMode::ReportAndRender;

        if (!renderRects)
        {
            // On builds of Windows that don't support dirty regions or when the dirty
            // region mode is set to ReportOnly, the entire frame has been rendered.

            // copy surfaceTexture to backBuffer
            m_d3dContext->CopyResource(backBuffer.get(), surfaceTexture.get());
        }
        else
        {
            // When the dirty region mode is set to ReportAndRender, only the pixels within
            // the dirty region are valid. To visualize this, we'll clear our render target
            // to opaque black and copy out the dirty regions.

            // First, let's clear our render target
            winrt::com_ptr<ID3D11RenderTargetView> rtv;
            winrt::check_hresult(m_d3dDevice->CreateRenderTargetView(backBuffer.get(), nullptr, rtv.put()));
            float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
            m_d3dContext->ClearRenderTargetView(rtv.get(), clearColor);

            D3D11_TEXTURE2D_DESC desc = {};
            surfaceTexture->GetDesc(&desc);
            int textureWidth = static_cast<int>(desc.Width);
            int textureHeight = static_cast<int>(desc.Height);

            // Next, let's copy out each dirty region
            auto dirtyRegion = frame.DirtyRegions();
            for (auto&& dirtyRegion : dirtyRegion)
            {
                // Some of these checks are a bit paranoid. The real thing we need to look out for
                // is when the render target and the source texture differ in size (e.g. during a
                // window resize, where we resize the swap chain before we resize the frame pool).

                if (dirtyRegion.X >= textureWidth || dirtyRegion.Y >= textureHeight)
                {
                    continue;
                }

                int right = dirtyRegion.X + dirtyRegion.Width;
                int bottom = dirtyRegion.Y + dirtyRegion.Height;

                if (right <= 0 || bottom <= 0)
                {
                    continue;
                }

                int left = std::max(dirtyRegion.X, 0);
                int top = std::max(dirtyRegion.Y, 0);
                right = std::min(right, textureWidth);
                bottom = std::min(bottom, textureHeight);

                D3D11_BOX region = {};
                region.left = static_cast<uint32_t>(left);
                region.right = static_cast<uint32_t>(right);
                region.top = static_cast<uint32_t>(top);
                region.bottom = static_cast<uint32_t>(bottom);
                region.back = 1;
                m_d3dContext->CopySubresourceRegion(backBuffer.get(), 0, static_cast<uint32_t>(left), static_cast<uint32_t>(top), 0, surfaceTexture.get(), 0, &region);
            }
        }

        if (m_dirtyRegionVisualizer && m_visualizeDirtyRegions.load())
        {
            m_dirtyRegionVisualizer->Render(backBuffer, frame);
        }
    }

    DXGI_PRESENT_PARAMETERS presentParameters{};
    m_swapChain->Present1(1, 0, &presentParameters);

    swapChainResizedToFrame = swapChainResizedToFrame || TryUpdatePixelFormat();

    if (swapChainResizedToFrame)
    {
        m_framePool.Recreate(m_device, m_pixelFormat, 2, m_lastSize);
    }
}
