#pragma once

#include "Input\Input.h"

namespace Chilli
{
	enum EventType
	{
		KeyPressed,
		KeyReleased,
		KeyRepeat,
		MouseButtonPressed,
		MouseButtonReleased,
		MouseButtonRepeat,
		CursorPos,
		MouseScroll,
		WindowClose,
		WindowResize,
		WindowMinimized,
		WindowOutOfFocus,
		FrameBufferResize
	};

	struct Event
	{
	public:
		Event(EventType type) : Type(type) {}

		EventType GetType() { return Type; }

	private:
		EventType Type;
	};

#define EVENT_MACRO_FUNC(x) \
    static EventType GetStaticType() { return x; }

	struct WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() : Event(EventType::WindowClose) {}

		EVENT_MACRO_FUNC(EventType::WindowClose);

	};

	struct WindowMinimizedEvent : public Event
	{
	public:
		WindowMinimizedEvent() : Event(EventType::WindowMinimized) {}

		EVENT_MACRO_FUNC(EventType::WindowMinimized);
	};

	struct KeyPressedEvent : public Event
	{
	public:
		KeyPressedEvent(Input_key code, int Mods) : ModsState(Mods), KeyCode(code), Event(EventType::KeyPressed) {}

		EVENT_MACRO_FUNC(EventType::KeyPressed);
		Input_key GetKeyCode() const { return KeyCode; }
		int GetMods() const { return ModsState; }

	private:
		Input_key KeyCode;
		int ModsState;
	};

	struct KeyReleasedEvent : public Event
	{
	public:
		KeyReleasedEvent(Input_key code, int Mods) :ModsState(Mods), KeyCode(code), Event(EventType::KeyReleased) {}

		EVENT_MACRO_FUNC(EventType::KeyReleased);
		Input_key GetKeyCode() const { return KeyCode; }
		int GetMods() const { return ModsState; }

	private:
		Input_key KeyCode;
		int ModsState;
	};

	struct KeyRepeatEvent : public Event
	{
	public:
		KeyRepeatEvent(Input_key code, int Mods) :ModsState(Mods), KeyCode(code), Event(EventType::KeyReleased) {}

		EVENT_MACRO_FUNC(EventType::KeyReleased);
		Input_key GetKeyCode() const { return KeyCode; }
		int GetMods() const { return ModsState; }

	private:
		Input_key KeyCode;
		int ModsState;
	};

	struct MouseButtonReleasedEvent : public Event
	{
	public:
		MouseButtonReleasedEvent(Input_mouse code) : ButtonCode(code), Event(EventType::MouseButtonReleased) {}

		EVENT_MACRO_FUNC(EventType::MouseButtonReleased);
		Input_mouse GetButtonCode() const { return ButtonCode; }

	private:
		Input_mouse ButtonCode;
	};

	struct MouseButtonRepeatEvent : public Event
	{
	public:
		MouseButtonRepeatEvent(Input_mouse code) : ButtonCode(code), Event(EventType::MouseButtonRepeat) {}

		EVENT_MACRO_FUNC(EventType::MouseButtonRepeat);
		Input_mouse GetButtonCode() const { return ButtonCode; }

	private:
		Input_mouse ButtonCode;
	};

	struct MouseButtonPressedEvent : public Event
	{
	public:
		MouseButtonPressedEvent(Input_mouse code) : ButtonCode(code), Event(EventType::MouseButtonPressed) {}

		EVENT_MACRO_FUNC(EventType::MouseButtonPressed);
		Input_mouse GetButtonCode() const { return ButtonCode; }

	private:
		Input_mouse ButtonCode;
	};

	struct WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(int newx, int newy) : x(newx), y(newy), Event(EventType::WindowResize) {}
		WindowResizeEvent() :Event(EventType::WindowResize) {}

		EVENT_MACRO_FUNC(EventType::WindowResize);

		int GetX() const { return x; }
		int GetY() const { return y; }

	private:
		int x, y;
	};

	struct FrameBufferResizeEvent : public Event
	{
	public:
		FrameBufferResizeEvent(int newx, int newy) : x(newx), y(newy), Event(EventType::FrameBufferResize) {}

		EVENT_MACRO_FUNC(EventType::FrameBufferResize);

		int GetX() const { return x; }
		int GetY() const { return y; }

	private:
		int x, y;
	};

	struct CursorPosEvent : public Event
	{
	public:
		CursorPosEvent(double newx, double newy) : x(newx), y(newy), Event(EventType::CursorPos) {}

		EVENT_MACRO_FUNC(EventType::CursorPos);

		double GetX() const { return x; }
		double GetY() const { return y; }

	private:
		double x, y;
	};

	struct MouseScrollEvent : public Event
	{
	public:
		MouseScrollEvent(double x, double y) : xoffset(x), yoffset(y), Event(EventType::MouseScroll) {}

		EVENT_MACRO_FUNC(EventType::MouseScroll);

		double GetXOffset() const { return xoffset; }
		double GetYOffset() const { return yoffset; }

	private:
		double xoffset, yoffset;
	};

	class EventDispatcher
	{
		template<typename T>
		using EventFn = std::function<void(T&)>;

	public:
		EventDispatcher(Event& event) : m_Event(event) {}

		template<typename T>
		bool Dispatch(const EventFn<T>& func)
		{
			if (m_Event.GetType() == T::GetStaticType())
			{
				func(static_cast<T&>(m_Event));
				return true;
			}
			return false;
		}

	private:
		Event& m_Event;
	};
} // namespace VEngine