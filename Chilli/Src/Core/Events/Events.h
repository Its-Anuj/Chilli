#pragma once

#include "Input\Input.h"

namespace Chilli
{
	struct Event
	{
	public:
		Event() {}

		virtual const char* GetName() const = 0;

	private:
	};
	struct WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;
		const char* GetName() const override { return "WindowCloseEvent"; }
	};

	struct WindowMinimizedEvent : public Event
	{
	public:
		WindowMinimizedEvent() = default;
		const char* GetName() const override { return "WindowMinimizedEvent"; }
	};

	struct KeyPressedEvent : public Event
	{
	public:
		KeyPressedEvent(Input_key code, int Mods)
			: ModsState(Mods), KeyCode(code) {
		}

		const char* GetName() const override { return "KeyPressedEvent"; }

		Input_key GetKeyCode() const { return KeyCode; }
		int GetMods() const { return ModsState; }

	private:
		Input_key KeyCode;
		int ModsState;
	};

	struct KeyReleasedEvent : public Event
	{
	public:
		KeyReleasedEvent(Input_key code, int Mods)
			: ModsState(Mods), KeyCode(code) {
		}

		const char* GetName() const override { return "KeyReleasedEvent"; }

		Input_key GetKeyCode() const { return KeyCode; }
		int GetMods() const { return ModsState; }

	private:
		Input_key KeyCode;
		int ModsState;
	};

	struct KeyRepeatEvent : public Event
	{
	public:
		KeyRepeatEvent(Input_key code, int Mods)
			: ModsState(Mods), KeyCode(code) {
		}

		const char* GetName() const override { return "KeyRepeatEvent"; }

		Input_key GetKeyCode() const { return KeyCode; }
		int GetMods() const { return ModsState; }

	private:
		Input_key KeyCode;
		int ModsState;
	};

	struct MouseButtonReleasedEvent : public Event
	{
	public:
		MouseButtonReleasedEvent(Input_mouse code)
			: ButtonCode(code) {
		}

		const char* GetName() const override { return "MouseButtonReleasedEvent"; }

		Input_mouse GetButtonCode() const { return ButtonCode; }

	private:
		Input_mouse ButtonCode;
	};

	struct MouseButtonRepeatEvent : public Event
	{
	public:
		MouseButtonRepeatEvent(Input_mouse code)
			: ButtonCode(code) {
		}

		const char* GetName() const override { return "MouseButtonRepeatEvent"; }

		Input_mouse GetButtonCode() const { return ButtonCode; }

	private:
		Input_mouse ButtonCode;
	};

	struct MouseButtonPressedEvent : public Event
	{
	public:
		MouseButtonPressedEvent(Input_mouse code)
			: ButtonCode(code) {
		}

		const char* GetName() const override { return "MouseButtonPressedEvent"; }

		Input_mouse GetButtonCode() const { return ButtonCode; }

	private:
		Input_mouse ButtonCode;
	};

	struct WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(int newx, int newy) : x(newx), y(newy) {}
		WindowResizeEvent() = default;

		const char* GetName() const override { return "WindowResizeEvent"; }

		int GetX() const { return x; }
		int GetY() const { return y; }

	private:
		int x{}, y{};
	};

	struct FrameBufferResizeEvent : public Event
	{
	public:
		FrameBufferResizeEvent(int newx, int newy) : x(newx), y(newy) {}

		const char* GetName() const override { return "FrameBufferResizeEvent"; }

		int GetX() const { return x; }
		int GetY() const { return y; }

	private:
		int x{}, y{};
	};

	struct CursorPosEvent : public Event
	{
	public:
		CursorPosEvent(double newx, double newy) : x(newx), y(newy) {}

		const char* GetName() const override { return "CursorPosEvent"; }

		double GetX() const { return x; }
		double GetY() const { return y; }

	private:
		double x{}, y{};
	};

	struct MouseScrollEvent : public Event
	{
	public:
		MouseScrollEvent(double x, double y) : xoffset(x), yoffset(y) {}

		const char* GetName() const override { return "MouseScrollEvent"; }

		double GetXOffset() const { return xoffset; }
		double GetYOffset() const { return yoffset; }

	private:
		double xoffset{}, yoffset{};
	};

	struct SetCursorEvent : public Event
	{
	public:// Use std::vector to ensure the event 'owns' the data while it's in the queue

		SetCursorEvent(const Cursor* cursor, Window* UsingWindow, DeafultCursorTypes Type = DeafultCursorTypes::Deafult)
			:_Cursor(cursor), _UsingWindow(UsingWindow),  _Type(Type)
		{
		}

		const char* GetName() const override { return "SetCursorEvent"; }
		const DeafultCursorTypes GetType() const { return _Type; }
		const Cursor* GetCursor()const {
			return _Cursor;
		};

		const Window* GetUsingWindow() const {
			return _UsingWindow;
		}

	private:
		const Cursor* _Cursor;
		const Window* _UsingWindow;
		const DeafultCursorTypes _Type;
	};

} // namespace VEngine