#include "pch.h"
#include "SimpleCapture.h"

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

namespace util
{
    using namespace uwp;
}

SimpleCapture::SimpleCapture(winrt::IDirect3DDevice const& device,
    winrt::GraphicsCaptureItem const& item,
    winrt::DirectXPixelFormat pixelFormat, HWND hwnd)
{
    m_item = item;
    m_device = device;
    m_pixelFormat = pixelFormat;
    m_hWnd = hwnd; // SPOUT
 
    auto d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
    d3dDevice->GetImmediateContext(m_d3dContext.put());

    m_swapChain = util::CreateDXGISwapChain(d3dDevice,
        static_cast<uint32_t>(m_item.Size().Width),
        static_cast<uint32_t>(m_item.Size().Height),
        static_cast<DXGI_FORMAT>(m_pixelFormat), 2);

    // Creating our frame pool with 'Create' instead of 'CreateFreeThreaded'
    // means that the frame pool's FrameArrived event is called on the thread
    // the frame pool was created on. This also means that the creating thread
    // must have a DispatcherQueue. If you use this method, it's best not to do
    // it on the UI thread. 
    m_framePool = winrt::Direct3D11CaptureFramePool::Create(m_device, m_pixelFormat, 2, m_item.Size());

    m_session = m_framePool.CreateCaptureSession(m_item);
    m_lastSize = m_item.Size();
    m_framePool.FrameArrived({ this, &SimpleCapture::OnFrameArrived });

    // SPOUT
    // Initialize DirectX.
    // A device pointer must be passed in if a DirectX 11.0 device is available.
    // Otherwise a different device is created in the SpoutDX class.
    spoutsender.OpenDirectX11(d3dDevice.get());

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
    return util::CreateCompositionSurfaceForSwapChain(compositor, m_swapChain.get());
}

void SimpleCapture::Close()
{
    auto expected = false;
    if (m_closed.compare_exchange_strong(expected, true))
    {
        // SPOUT
        spoutsender.ReleaseSender();
        m_hWnd = nullptr;

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
    winrt::check_hresult(m_swapChain->ResizeBuffers(2,
        static_cast<uint32_t>(m_lastSize.Width),
        static_cast<uint32_t>(m_lastSize.Height),
        static_cast<DXGI_FORMAT>(m_pixelFormat), 0));

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
    auto lock = m_lock.lock_exclusive();
    if (m_pixelFormatUpdate.has_value())
    {
        auto pixelFormat = m_pixelFormatUpdate.value();
        m_pixelFormatUpdate = std::nullopt;
        if (pixelFormat != m_pixelFormat)
        {
            m_pixelFormat = pixelFormat;
            ResizeSwapChain();
            return true;
        }
    }
    return false;
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

        // copy surfaceTexture to backBuffer
        m_d3dContext->CopyResource(backBuffer.get(), surfaceTexture.get());

        //
        // SPOUT
        //

            // Frame surface texture size
        D3D11_TEXTURE2D_DESC desc{};
        surfaceTexture.get()->GetDesc(&desc);
        int surfaceWidth  = desc.Width;
        int surfaceHeight = desc.Height;

        // Capture window size
        RECT rect{};
        GetWindowRect(m_hWnd, &rect);
        int windowWidth  = (int)(rect.right - rect.left);
        int windowHeight = (int)(rect.bottom - rect.top);

        // Send the client area of a window (m_hWnd)
        if (m_hWnd) {

            if (m_bClient) { // Client area checkbox

                // Capture client size
                GetClientRect(m_hWnd, &rect);
                int clientWidth  = (int)(rect.right - rect.left);
                int clientHeight = (int)(rect.bottom - rect.top);

                // Assume equal borders left and right
                int xoffset = 0;
                if (surfaceWidth > clientWidth)
                    xoffset = (surfaceWidth - clientWidth)/2;

                // Assume the bottom border is the same as left and right
                int yoffset = 0;
                if (surfaceHeight > clientHeight)
                    yoffset = (surfaceHeight - clientHeight)-xoffset;

                // Client width and height
                unsigned int width  = (unsigned int)clientWidth;
                unsigned int height = (unsigned int)clientHeight;

                // If the window is selected or picked while minimized and then restored,
                // offsets can be negative for a few frames while the window is restoring
                // and the client width is not immediately reduced to zero when minimized.
                // Also the surface size is reduced by 8. Unknown reason.
                if (surfaceHeight == windowHeight) yoffset -= 8;
                //
                if (surfaceWidth >= clientWidth && surfaceHeight >= clientHeight
                    && xoffset >= 0 && yoffset >= 0 && width > 1 && height > 1) {
                    spoutsender.SendTexture(surfaceTexture.get(), (unsigned int)xoffset, (unsigned int)yoffset, width, height);
                }
                else {
                    spoutsender.SendTexture(surfaceTexture.get());
                }
            } // endif client
            else {
                // The whole window
                if (surfaceHeight == windowHeight) {
                    // Restored from minimized
                    spoutsender.SendTexture(surfaceTexture.get(), 0, 0, surfaceWidth, surfaceHeight-8);
                }
                else {
                    spoutsender.SendTexture(surfaceTexture.get());
                }
            }
        } // endif window capture
        else {
            // The whole frame for a monitor
            spoutsender.SendTexture(surfaceTexture.get());
        } // end send surface texture

    } // end copy surfaceTexture to backBuffer

    DXGI_PRESENT_PARAMETERS presentParameters{};
    m_swapChain->Present1(1, 0, &presentParameters);

    swapChainResizedToFrame = swapChainResizedToFrame || TryUpdatePixelFormat();

    if (swapChainResizedToFrame)
    {
        m_framePool.Recreate(m_device, m_pixelFormat, 2, m_lastSize);
    }
}
