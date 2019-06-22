#include "pch.h"
#include "App.h"
#include "SimpleCapture.h"
#include "Win32WindowEnumeration.h"

using namespace winrt;
using namespace Windows::UI;
using namespace Windows::UI::Composition;

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    LPSTR     cmdLine,
    int       cmdShow);

auto g_app = std::make_shared<App>();
auto g_windows = EnumerateWindows();

LRESULT CALLBACK WndProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam);

int CALLBACK WinMain(
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

    // Create the window
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

    HWND hwnd = CreateWindow(
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
        NULL);
    WINRT_VERIFY(hwnd);

    ShowWindow(hwnd, cmdShow);
    UpdateWindow(hwnd);

    // Create combo box
    HWND comboBoxHwnd = CreateWindow(
        WC_COMBOBOX,
        L"",
        CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,
        10,
        10,
        200,
        200,
        hwnd,
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
        hwnd,
        NULL,
        instance,
        NULL);
    WINRT_VERIFY(buttonHwnd);

    // Create a DispatcherQueue for our thread
    auto controller = CreateDispatcherQueueControllerForCurrentThread();

    // Initialize Composition
    auto compositor = Compositor();
    auto target = CreateDesktopWindowTarget(compositor, hwnd, true);
    auto root = compositor.CreateContainerVisual();
    root.RelativeSizeAdjustment({ 1.0f, 1.0f });
    target.Root(root);

    auto picker = CreateCapturePickerForHwnd(hwnd);

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

LRESULT CALLBACK WndProc(
    HWND   hwnd,
    UINT   msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_COMMAND:
        {
            auto command = HIWORD(wParam);
            switch (command)
            {
            case CBN_SELCHANGE:
                {
                    auto index = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
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
        return DefWindowProc(hwnd, msg, wParam, lParam);
        break;
    }

    return 0;
}