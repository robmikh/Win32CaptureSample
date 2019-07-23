#include "pch.h"
#include "SimpleCapture.h"
#include <DirectXTex.h>

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::System;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
    using namespace Windows::Foundation::Numerics;
    using namespace Windows::UI;
    using namespace Windows::UI::Composition;
}

SimpleCapture::SimpleCapture(winrt::IDirect3DDevice const& device, winrt::GraphicsCaptureItem const& item)
{
    m_item = item;
    m_device = device;

    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    d3dDevice->GetImmediateContext(m_d3dContext.put());

    m_swapChain = CreateDXGISwapChain(d3dDevice, static_cast<uint32_t>(m_item.Size().Width), static_cast<uint32_t>(m_item.Size().Height),
        DXGI_FORMAT_B8G8R8A8_UNORM, 2);

    m_framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(m_device, winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_item.Size());
    m_session = m_framePool.CreateCaptureSession(m_item);
    m_lastSize = m_item.Size();
    m_framePool.FrameArrived({ this, &SimpleCapture::OnFrameArrived });

    WINRT_ASSERT(m_session != nullptr);
}

void SimpleCapture::StartCapture()
{
    CheckClosed();
    m_session.StartCapture();
}

winrt::ICompositionSurface SimpleCapture::CreateSurface(winrt::Compositor const& compositor)
{
    CheckClosed();
    return CreateCompositionSurfaceForSwapChain(compositor, m_swapChain.get());
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

bool SimpleCapture::TryResizeSwapChain(const winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame& frame)
{
    auto const contentSize = frame.ContentSize();
    if ((contentSize.Width != m_lastSize.Width) ||
        (contentSize.Height != m_lastSize.Height))
    {
        // The thing we have been capturing has changed size, resize the swap chain to match.
        m_lastSize = contentSize;
        m_swapChain->ResizeBuffers(2, static_cast<uint32_t>(m_lastSize.Width), static_cast<uint32_t>(m_lastSize.Height),
            DXGI_FORMAT_B8G8R8A8_UNORM, 0);
        return true;
    }
    return false;
}

void SimpleCapture::OnFrameArrived(winrt::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const&)
{
    auto swapChainResizedToFrame = false;

    {
        auto frame = sender.TryGetNextFrame();
        swapChainResizedToFrame = TryResizeSwapChain(frame);

        winrt::com_ptr<ID3D11Texture2D> backBuffer;
        winrt::check_hresult(m_swapChain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), backBuffer.put_void()));
        auto surfaceTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
        // copy surfaceTexture to backBuffer
        m_d3dContext->CopyResource(backBuffer.get(), surfaceTexture.get());

        if (m_captureNextImage)
        {
            m_captureNextImage = false;

            DirectX::ScratchImage im;
            winrt::check_hresult(DirectX::CaptureTexture(GetDXGIInterfaceFromObject<ID3D11Device>(m_device).get(),
                m_d3dContext.get(), surfaceTexture.get(), im));
            const auto& realImage = *im.GetImage(0, 0, 0);
            winrt::check_hresult(DirectX::SaveToWICFile(realImage, DirectX::WIC_FLAGS_NONE,
                GUID_ContainerFormatPng, L"output_tex3.png"));
        }
    }

    DXGI_PRESENT_PARAMETERS presentParameters{};
    m_swapChain->Present1(1, 0, &presentParameters);

    if (swapChainResizedToFrame)
    {
        m_framePool.Recreate(m_device, winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, m_lastSize);
    }
}
