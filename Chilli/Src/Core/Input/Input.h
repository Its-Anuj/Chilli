#pragma once

#include "InputCodes.h"
#include "Maths.h"
#include "BackBone\BackBone.h"

namespace Chilli
{
    class Window;

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

        void SetActiveWindow(BackBone::AssetHandle<Window> WindowHandle);
        void SetActiveWindow(void* RawWindowHandle) { _WindowHandle = RawWindowHandle; }
        
        InputResult GetKeyState(Input_key KeyCode);
        InputResult GetMouseButtonState(Input_mouse ButtonCode);

        bool GetModState(Input_mod mod);
        void SetCursorPos(const IVec2& Pos);
        const IVec2 GetCursorPos() const { return _CursorPos; }

        void SetModState(Input_mod mod, bool state);

        const char* KeyToString(Input_key Key);
        const char* MouseToString(Input_mouse Mouse);

    private:
        void* _WindowHandle = nullptr;
        InputResult _KeyStates[Input_key_Count];
        InputResult _MouseButtonStates[Input_mouse_Count];
        bool _ModStates[Input_mod_Count];
        IVec2 _CursorPos;

    };
} // namespace VEngine