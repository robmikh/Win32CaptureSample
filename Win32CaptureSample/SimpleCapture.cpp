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
    winrt::com_ptr<IDXGIFactory1> const& dxgiFactory,
    winrt::com_ptr<ID3D12CommandQueue> const& d3d12Queue,
    winrt::com_ptr<ID3D11On12Device> const& d3d11on12Device,
    winrt::GraphicsCaptureItem const& item, 
    winrt::DirectXPixelFormat pixelFormat)
{
    m_item = item;
    m_device = device;
    m_pixelFormat = pixelFormat;
    m_dirtyRegionVisualizer = dirtyRegionVisualizer;
    m_d3d11on12Device = d3d11on12Device;

    m_d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    m_d3dDevice->GetImmediateContext(m_d3dContext.put());

    auto itemSize = item.Size();
    auto width = static_cast<uint32_t>(itemSize.Width);
    auto height = static_cast<uint32_t>(itemSize.Height);
    auto format = static_cast<DXGI_FORMAT>(m_pixelFormat);

    DXGI_SWAP_CHAIN_DESC1 desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.Format = format;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.BufferCount = 2;
    desc.Scaling = DXGI_SCALING_STRETCH;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
    desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

    auto factory2 = dxgiFactory.as<IDXGIFactory2>();
    winrt::com_ptr<IDXGISwapChain1> swapChain1;
    winrt::check_hresult(factory2->CreateSwapChainForComposition(d3d12Queue.get(), &desc, nullptr, swapChain1.put()));
    m_swapChain = swapChain1.as<IDXGISwapChain3>();
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

        winrt::com_ptr<ID3D12Resource> resource;
        auto backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
        winrt::check_hresult(m_swapChain->GetBuffer(backBufferIndex, winrt::guid_of<ID3D12Resource>(), resource.put_void()));

        winrt::com_ptr<ID3D11Texture2D> backBuffer;
        D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
        winrt::check_hresult(m_d3d11on12Device->CreateWrappedResource(
            resource.get(),
            &d3d11Flags,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT,
            winrt::guid_of<ID3D11Texture2D>(),
            backBuffer.put_void()));

        std::array<ID3D11Resource*, 1> resources = { backBuffer.get() };
        m_d3d11on12Device->AcquireWrappedResources(
            resources.data(),
            static_cast<uint32_t>(resources.size()));
        
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

        m_d3d11on12Device->ReleaseWrappedResources(
            resources.data(),
            static_cast<uint32_t>(resources.size()));
        m_d3dContext->Flush();
    } // We currently fail here when the frame is returned to the pool. This 
      // is because there is an API currently missing in 11-on-12.

    DXGI_PRESENT_PARAMETERS presentParameters{};
    m_swapChain->Present1(1, 0, &presentParameters);

    swapChainResizedToFrame = swapChainResizedToFrame || TryUpdatePixelFormat();

    if (swapChainResizedToFrame)
    {
        m_framePool.Recreate(m_device, m_pixelFormat, 2, m_lastSize);
    }
}
