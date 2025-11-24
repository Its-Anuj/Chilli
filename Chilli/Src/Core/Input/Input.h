#pragma once

#include "InputCodes.h"
#include "Maths.h"
#include "BackBone\BackBone.h"

namespace Chilli
{
    class Window;
    struct EventHandler;

    enum class InputResult
    {
        INPUT_PRESS,
        INPUT_REPEAT,
        INPUT_RELEASE
    };

    class Input
    {
    public:
        Input();
        ~Input() {}

        void Terminate();
        void Clear();

        void SetActiveWindow(BackBone::AssetHandle<Window> WindowHandle);
        void SetActiveWindow(void* RawWindowHandle) { _WindowHandle = RawWindowHandle; }
        void UpdateEvents(EventHandler* EventManager);

        InputResult GetKeyState(Input_key KeyCode);
        InputResult GetMouseButtonState(Input_mouse ButtonCode);

        bool GetModState(Input_mod mod);
        bool IsModActive(Input_mod mod) {
            return  _ModStates & mod;
        }

        const IVec2 GetCursorPos() const { return _CursorPos; }

        bool IsKeyPressed(Input_key key) const {
            return _KeyStates[key] == InputResult::INPUT_PRESS;
        }

        bool IsKeyDown(Input_key key) const {
            return _KeyStates[key] == InputResult::INPUT_PRESS ||
                _KeyStates[key] == InputResult::INPUT_REPEAT;
        }

        bool IsKeyReleased(Input_key key) const {
            return _KeyStates[key] == InputResult::INPUT_RELEASE;
        }

        bool IsMouseButtonPressed(Input_mouse button) const {
            return _MouseButtonStates[button] == InputResult::INPUT_PRESS;
        }

        bool IsMouseButtonDown(Input_mouse button) const {
            return _MouseButtonStates[button] == InputResult::INPUT_PRESS ||
                _MouseButtonStates[button] == InputResult::INPUT_REPEAT;
        }

        bool IsMouseButtonRelease(Input_mouse button) const {
            return _MouseButtonStates[button] == InputResult::INPUT_RELEASE;
        }

        const char* KeyToString(Input_key Key);
        const char* MouseToString(Input_mouse Mouse);

    private:
        void _SetModStates(int ModState);
    private:
        void* _WindowHandle = nullptr;
        InputResult _KeyStates[Input_key_Count];
        InputResult _MouseButtonStates[Input_mouse_Count];
        int _ModStates = 0; 
        IVec2 _CursorPos;

    };
} // namespace VEngine