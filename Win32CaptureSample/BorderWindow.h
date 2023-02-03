#pragma once

struct BorderWindow : robmikh::common::desktop::DesktopWindow<BorderWindow>
{
	static const std::wstring ClassName;
	BorderWindow(winrt::Windows::UI::Composition::Compositor const& compositor);
	LRESULT MessageHandler(UINT const message, WPARAM const wparam, LPARAM const lparam);

	winrt::Windows::UI::Color BorderColor() { return m_colorBrush.Color(); }
	void BorderColor(winrt::Windows::UI::Color const& value);
	float BorderThickness() { return m_thickness; }
	void BorderThickness(float value);
	void PositionOver(HWND otherWindowHandle);
	void PositionOver(HMONITOR monitorHandle);
	void MoveTo(int32_t x, int32_t y);
	void Resize(int32_t width, int32_t height);
	void MoveAndResize(int32_t x, int32_t y, int32_t width, int32_t height);
	void Show();
	void Hide();
	void Close();

private:
	static void RegisterWindowClass();

private:
	winrt::Windows::UI::Composition::CompositionColorBrush m_colorBrush{ nullptr };
	winrt::Windows::UI::Composition::CompositionNineGridBrush m_nineGridBrush{ nullptr };
	winrt::Windows::UI::Composition::CompositionTarget m_target{ nullptr };
	float m_thickness = 1.0f;
	HWND m_lastWindowOver = nullptr;
	HMONITOR m_lastMonitorOver = nullptr;
};