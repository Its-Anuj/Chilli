#pragma once

// Based on GLfw3

namespace Chilli
{
    enum Input_key
    {
        Input_key_Unknown,
        Input_key_Space,
        Input_key_Apostrophe, /* ' */
        Input_key_Comma,      /* , */
        Input_key_Minus,      /* - */
        Input_key_Period,     /* . */
        Input_key_Slash,      /* / */

        Input_key_0,
        Input_key_1,
        Input_key_2,
        Input_key_3,
        Input_key_4,
        Input_key_5,
        Input_key_6,
        Input_key_7,
        Input_key_8,
        Input_key_9,

        Input_key_Semicolon, /* ; */
        Input_key_Equal,

        Input_key_A,
        Input_key_B,
        Input_key_C,
        Input_key_D,
        Input_key_E,
        Input_key_F,
        Input_key_G,
        Input_key_H,
        Input_key_I,
        Input_key_J,
        Input_key_K,
        Input_key_L,
        Input_key_M,
        Input_key_N,
        Input_key_O,
        Input_key_P,
        Input_key_Q,
        Input_key_R,
        Input_key_S,
        Input_key_T,
        Input_key_U,
        Input_key_V,
        Input_key_W,
        Input_key_X,
        Input_key_Y,
        Input_key_Z,

        Input_key_LeftBracket,  /* [ */
        Input_key_Backslash,     /* \ */
        Input_key_RightBracket, /* ] */
        Input_key_GraveAccent,  /* ` */
        Input_key_World1,       /* non-US #1 */
        Input_key_World2,       /* non-US #2 */

        /* Function keys */
        Input_key_Escape,
        Input_key_Enter,
        Input_key_Tab,
        Input_key_Backspace,
        Input_key_Insert,
        Input_key_Delete,
        Input_key_Right,
        Input_key_Left,
        Input_key_Down,
        Input_key_Up,
        Input_key_PageUp,
        Input_key_PageDown,
        Input_key_Home,
        Input_key_End,
        Input_key_CapsLock,
        Input_key_ScrollLock,
        Input_key_NumLock,
        Input_key_PrintScreen,
        Input_key_Pause,
        Input_key_F1,
        Input_key_F2,
        Input_key_F3,
        Input_key_F4,
        Input_key_F5,
        Input_key_F6,
        Input_key_F7,
        Input_key_F8,
        Input_key_F9,
        Input_key_F10,
        Input_key_F11,
        Input_key_F12,
        Input_key_F13,
        Input_key_F14,
        Input_key_F15,
        Input_key_F16,
        Input_key_F17,
        Input_key_F18,
        Input_key_F19,
        Input_key_F20,
        Input_key_F21,
        Input_key_F22,
        Input_key_F23,
        Input_key_F24,
        Input_key_F25,

        /* Keypad */
        Input_key_Kp0,
        Input_key_Kp1,
        Input_key_Kp2,
        Input_key_Kp3,
        Input_key_Kp4,
        Input_key_Kp5,
        Input_key_Kp6,
        Input_key_Kp7,
        Input_key_Kp8,
        Input_key_Kp9,
        Input_key_KpDecimal,
        Input_key_KpDivide,
        Input_key_KpMultiply,
        Input_key_KpSubtract,
        Input_key_KpAdd,
        Input_key_KpEnter,
        Input_key_KpEqual,

        Input_key_LeftShift,
        Input_key_LeftControl,
        Input_key_LeftAlt,
        Input_key_LeftSuper,
        Input_key_RightShift,
        Input_key_RightControl,
        Input_key_RightAlt,
        Input_key_RightSuper,
        Input_key_Menu,
        Input_key_Count
    };

    enum Input_mouse
    {
        Input_mouse_Unknown,
        Input_mouse_Left,
        Input_mouse_Right,
        Input_mouse_Middle,
        Input_mouse_Button4,
        Input_mouse_Button5,
        Input_mouse_Button6,
        Input_mouse_Button7,
        Input_mouse_Button8,
        Input_mouse_Count
    };

    enum Input_mod
    {
        Input_mod_Shift,
        Input_mod_Control,
        Input_mod_Alt,
        Input_mod_Super,
        Input_mod_CapsLock,
        Input_mod_NumLock,
        Input_mod_Count
    };

    int InputKeysToGLFW(Input_key key);
    int InputMouseToGLFW(Input_mouse mouse);
    int InputModsToGLFW(Input_mod mod);

    Input_key GLFWToInputKeys(int key);
    Input_mouse GLFWToInputMouse(int mouse);
    Input_mod GLFWToInputMods(int mod);
    
    const char* InputMouseToString(Input_mouse mouse);
    const char* InputModToString(Input_mod mod);
    const char* InputKeyToString(Input_key key);
} // namespace VEngine