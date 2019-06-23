#include "pch.h"
#include "DesktopWindow.h"
#include "App.h"
#include "SimpleCapture.h"
#include "Win32WindowEnumeration.h"

using namespace winrt;
using namespace Windows::Graphics::Capture;
using namespace Windows::UI;
using namespace Windows::UI::Composition;
using namespace Windows::UI::Composition::Desktop;

// Globals
auto g_app = std::make_shared<App>();
auto g_windows = EnumerateWindows();

struct SampleWindow : DesktopWindow<SampleWindow>
{
    SampleWindow(HINSTANCE instance, int cmdShow) noexcept
    {
        WNDCLASSEX wcex = {};
        wcex.cbSize = sizeof(WNDCLASSEX);
        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc = WndProc;
        wcex.cbClsExtra = 0;
        wcex.cbWndExtra = 0;
        wcex.hInstance = instance;
        wcex.hIcon = LoadIcon(instance, MAKEINTRESOURCE(IDI_APPLICATION));
        wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wcex.lpszMenuName = NULL;
        wcex.lpszClassName = L"Win32CaptureSample";
        wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
        WINRT_VERIFY(RegisterClassEx(&wcex));
        WINRT_ASSERT(!m_window);

        WINRT_VERIFY(CreateWindow(
            L"Win32CaptureSample",
            L"Win32CaptureSample",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            800,
            600,
            NULL,
            NULL,
            instance,
            this));

        WINRT_ASSERT(m_window);

        ShowWindow(m_window, cmdShow);
        UpdateWindow(m_window);

        CreateControls(instance);
    }

    DesktopWindowTarget CreateWindowTarget(Compositor const& compositor)
    {
        return CreateDesktopWindowTarget(compositor, m_window, true);
    }

    void Initialize(winrt::Windows::Foundation::IUnknown const& object)
    {
        auto initializer = object.as<IInitializeWithWindow>();
        winrt::check_hresult(initializer->Initialize(m_window));
    }

    LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam) noexcept
    {
        switch (message)
        {
        case WM_COMMAND:
            {
                auto command = HIWORD(wparam);
                switch (command)
                {
                case CBN_SELCHANGE:
                {
                    auto index = SendMessage((HWND)lparam, CB_GETCURSEL, 0, 0);
                    auto window = g_windows[index];
                    g_app->StartCapture(window.Hwnd());
                }
                break;
                case BN_CLICKED:
                {
                    auto ignored = g_app->StartCaptureWithPickerAsync();
                }
                break;
                }
            }
            break;
        default:
            return base_type::MessageHandler(message, wparam, lparam);
            break;
        }

        return 0;
    }

private:
    void CreateControls(HINSTANCE instance)
    {
        // Create combo box
        HWND comboBoxHwnd = CreateWindow(
            WC_COMBOBOX,
            L"",
            CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
            10,
            10,
            200,
            200,
            m_window,
            NULL,
            instance,
            NULL);
        WINRT_VERIFY(comboBoxHwnd);

        // Populate combo box
        for (auto& window : g_windows)
        {
            SendMessage(comboBoxHwnd, CB_ADDSTRING, 0, (LPARAM)window.Title().c_str());
        }

        // Create button
        HWND buttonHwnd = CreateWindow(
            WC_BUTTON,
            L"Use Picker",
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            10,
            40,
            200,
            30,
            m_window,
            NULL,
            instance,
            NULL);
        WINRT_VERIFY(buttonHwnd);

        m_comboBoxHwnd = comboBoxHwnd;
        m_buttonHwnd = buttonHwnd;
    }

private:
    HWND m_comboBoxHwnd = NULL;
    HWND m_buttonHwnd = NULL;
};

int __stdcall WinMain(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    LPSTR     cmdLine,
    int       cmdShow)
{
    // Initialize COM
    init_apartment(apartment_type::single_threaded);

    // Check to see that capture is supported
    auto isCaptureSupported = winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported();
    if (!isCaptureSupported)
    {
        MessageBox(
            NULL,
            L"Screen capture is not supported on this device for this release of Windows!",
            L"Win32CaptureSample",
            MB_OK | MB_ICONERROR);

        return 1;
    }

    // Create Window
    auto window = SampleWindow(instance, cmdShow);

    // Create a DispatcherQueue for our thread
    auto controller = CreateDispatcherQueueControllerForCurrentThread();

    // Initialize Composition
    auto compositor = Compositor();
    auto target = window.CreateWindowTarget(compositor);
    auto root = compositor.CreateContainerVisual();
    root.RelativeSizeAdjustment({ 1.0f, 1.0f });
    target.Root(root);

    // Create and initialize the picker
    auto picker = GraphicsCapturePicker();
    window.Initialize(picker);

    // Enqueue our capture work on the dispatcher
    auto queue = controller.DispatcherQueue();
    auto success = queue.TryEnqueue([=]() -> void
    {
        g_app->Initialize(root, picker);
    });
    WINRT_VERIFY(success);

    // Message pump
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}