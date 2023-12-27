#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <dispatcherqueue.h>

namespace robmikh::common::desktop
{
    namespace impl
    {
        inline void ShutdownAndThenPostQuitMessage(winrt::Windows::System::DispatcherQueueController const& controller, int exitCode)
        {
            auto action = controller.ShutdownQueueAsync();
            action.Completed([exitCode](auto&&, auto&&)
                {
                    PostQuitMessage(exitCode);
                });
        }
    }

    inline auto CreateDispatcherQueueControllerForCurrentThread()
    {
        namespace abi = ABI::Windows::System;

        DispatcherQueueOptions options
        {
            sizeof(DispatcherQueueOptions),
            DQTYPE_THREAD_CURRENT,
            DQTAT_COM_NONE
        };

        winrt::Windows::System::DispatcherQueueController controller{ nullptr };
        winrt::check_hresult(CreateDispatcherQueueController(options, reinterpret_cast<abi::IDispatcherQueueController**>(winrt::put_abi(controller))));
        return controller;
    }

    inline int ShutdownDispatcherQueueControllerAndWait(winrt::Windows::System::DispatcherQueueController const& controller, int exitCode)
    {
        impl::ShutdownAndThenPostQuitMessage(controller, exitCode);
        MSG msg = {};
        while (GetMessageW(&msg, nullptr, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        return static_cast<int>(msg.wParam);
    }
}