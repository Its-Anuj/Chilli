#pragma once

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
    struct KeyPressedEvent : public Event
    {
    public:
        KeyPressedEvent(int code) : KeyCode(code), Event(EventType::KeyPressed) {}

        EVENT_MACRO_FUNC(EventType::KeyPressed);
        int GetKeyCode() const { return KeyCode; }

    private:
        int KeyCode;
    };

    struct KeyReleasedEvent : public Event
    {
    public:
        KeyReleasedEvent(int code) : KeyCode(code), Event(EventType::KeyReleased) {}

        EVENT_MACRO_FUNC(EventType::KeyReleased);
        int GetKeyCode() const { return KeyCode; }

    private:
        int KeyCode;
    };

    struct KeyRepeatEvent : public Event
    {
    public:
        KeyRepeatEvent(int code) : KeyCode(code), Event(EventType::KeyReleased) {}

        EVENT_MACRO_FUNC(EventType::KeyReleased);
        int GetKeyCode() const { return KeyCode; }

    private:
        int KeyCode;
    };

    struct MouseButtonReleasedEvent : public Event
    {
    public:
        MouseButtonReleasedEvent(int code) : ButtonCode(code), Event(EventType::MouseButtonReleased) {}

        EVENT_MACRO_FUNC(EventType::MouseButtonReleased);
        int GetButtonCode() const { return ButtonCode; }

    private:
        int ButtonCode;
    };

    struct MouseButtonRepeatEvent : public Event
    {
    public:
        MouseButtonRepeatEvent(int code) : ButtonCode(code), Event(EventType::MouseButtonRepeat) {}

        EVENT_MACRO_FUNC(EventType::MouseButtonRepeat);
        int GetButtonCode() const { return ButtonCode; }

    private:
        int ButtonCode;
    };

    struct MouseButtonPressedEvent : public Event
    {
    public:
        MouseButtonPressedEvent(int code) : ButtonCode(code), Event(EventType::MouseButtonPressed) {}

        EVENT_MACRO_FUNC(EventType::MouseButtonPressed);
        int GetButtonCode() const { return ButtonCode; }

    private:
        int ButtonCode;
    };

    struct WindowResizeEvent : public Event
    {
    public:
        WindowResizeEvent(int newx, int newy) : x(newx), y(newy), Event(EventType::WindowResize) {}

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
} // namespace VEngine