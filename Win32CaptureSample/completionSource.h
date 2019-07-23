#pragma once
// taken from https://gist.github.com/kennykerr/2a5b5995d0b2d9a0b646218d6a6cecb7
template <typename T>
struct completion_source
{
    completion_source()
    {
        m_signal.attach(CreateEvent(nullptr, true, false, nullptr));
    }

    void set(T const& value)
    {
        m_value = value;
        SetEvent(m_signal.get());
    }

    bool await_ready() const noexcept
    {
        return WaitForSingleObject(m_signal.get(), 0) == 0;
    }

    void await_suspend(std::experimental::coroutine_handle<> resume)
    {
        m_wait.attach(winrt::check_pointer(CreateThreadpoolWait(callback, resume.address(), nullptr)));
        SetThreadpoolWait(m_wait.get(), m_signal.get(), nullptr);
    }

    T await_resume() const noexcept
    {
        return m_value;
    }

private:

    static void __stdcall callback(PTP_CALLBACK_INSTANCE, void* context, PTP_WAIT, TP_WAIT_RESULT) noexcept
    {
        std::experimental::coroutine_handle<>::from_address(context)();
    }

    struct wait_traits
    {
        using type = PTP_WAIT;

        static void close(type value) noexcept
        {
            CloseThreadpoolWait(value);
        }

        static constexpr type invalid() noexcept
        {
            return nullptr;
        }
    };

    winrt::handle m_signal;
    winrt::handle_type<wait_traits> m_wait;
    T m_value{};
};