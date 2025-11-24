#include "Input.h"
#include "Ch_PCH.h"

#include "GLFW/glfw3.h"
#include "Window\Window.h"
#include "Input.h"
#include "InputCodes.h"
#include "BackBone\DeafultExtensions.h"

namespace Chilli
{
#pragma region Input Implementation

	Input::Input()
	{
		for (int i = 0; i < Input_key_Count; i++)
			_KeyStates[i] = InputResult::INPUT_RELEASE;
		for (int i = 0; i < Input_mouse_Count; i++)
			_MouseButtonStates[i] = InputResult::INPUT_RELEASE;
	}

	void Input::Terminate()
	{
		_WindowHandle = nullptr;
		CH_CORE_INFO("Input Service Terminate!");
	}

	void Input::SetActiveWindow(BackBone::AssetHandle<Window> WindowHandle)
	{
		CH_CORE_ASSERT(WindowHandle.ValPtr != nullptr, "Provide a Valid AssetHandle<Window>");

		_WindowHandle = WindowHandle.ValPtr->GetRawHandle();
	}

	void Input::UpdateEvents(EventHandler* EventManager)
	{
		// Key Events
		auto KeyPressedRead = EventManager->GetEventStorage< KeyPressedEvent>();
		if (KeyPressedRead->ActiveSize > 0)
		{
			for (auto KeyEvent : *KeyPressedRead)
			{
				auto KeyCode = KeyEvent.GetKeyCode();
				_KeyStates[KeyCode] = InputResult::INPUT_PRESS;
				_SetModStates(KeyEvent.GetMods());
			}
		}

		auto KeyRepeatRead = EventManager->GetEventStorage< KeyRepeatEvent>();
		if (KeyRepeatRead->ActiveSize > 0)
		{
			for (auto KeyEvent : *KeyRepeatRead)
			{
				auto KeyCode = KeyEvent.GetKeyCode();
				_KeyStates[KeyCode] = InputResult::INPUT_REPEAT;
				_SetModStates(KeyEvent.GetMods());
			}
		}

		auto KeyReleaseRead = EventManager->GetEventStorage< KeyReleasedEvent>();
		if (KeyReleaseRead->ActiveSize > 0)
		{
			for (auto KeyEvent : *KeyReleaseRead)
			{
				auto KeyCode = KeyEvent.GetKeyCode();
				_KeyStates[KeyCode] = InputResult::INPUT_RELEASE;
				_SetModStates(KeyEvent.GetMods());
			}
		}

		// Mouse Event
		auto MousePressedRead = EventManager->GetEventStorage< MouseButtonPressedEvent>();
		if (MousePressedRead->ActiveSize > 0)
		{
			for (auto MouseEvent : *MousePressedRead)
			{
				auto ButtonCode = MouseEvent.GetButtonCode();
				_MouseButtonStates[ButtonCode] = InputResult::INPUT_PRESS;
			}
		}
		auto MouseRepeatRead = EventManager->GetEventStorage< MouseButtonRepeatEvent>();
		if (MouseRepeatRead->ActiveSize > 0)
		{
			for (auto MouseEvent : *MouseRepeatRead)
			{
				auto ButtonCode = MouseEvent.GetButtonCode();
				_MouseButtonStates[ButtonCode] = InputResult::INPUT_REPEAT;
			}
		}
		auto MouseReleasedRead = EventManager->GetEventStorage< MouseButtonReleasedEvent>();
		if (MouseReleasedRead->ActiveSize > 0)
		{
			for (auto MouseEvent : *MouseReleasedRead)
			{
				auto ButtonCode = MouseEvent.GetButtonCode();
				_MouseButtonStates[ButtonCode] = InputResult::INPUT_RELEASE;
			}
		}
		
		// Cursor Pos
		auto CursorPosRead = EventManager->GetEventStorage<CursorPosEvent>();
		if (CursorPosRead->ActiveSize > 0)
			for (auto CursorEvent : *CursorPosRead)
				_CursorPos = {(int) CursorEvent.GetX(),(int)CursorEvent.GetY() };
	}

	InputResult Input::GetKeyState(Input_key KeyCode)
	{
		return _KeyStates[KeyCode];
	}

	InputResult Input::GetMouseButtonState(Input_mouse ButtonCode)
	{
		auto glfwinputid = InputMouseToGLFW(ButtonCode);

		if (glfwinputid == -1)
			CH_CORE_ERROR("Cant Conver to GLFW Mouse Button Code!");

		int Result = glfwGetMouseButton((GLFWwindow*)_WindowHandle, glfwinputid);

		InputResult LastState = _MouseButtonStates[ButtonCode];

		if (LastState == InputResult::INPUT_RELEASE && Result == GLFW_PRESS)
			_MouseButtonStates[ButtonCode] = InputResult::INPUT_PRESS;
		else if ((LastState == InputResult::INPUT_PRESS && Result == GLFW_RELEASE))
			_MouseButtonStates[ButtonCode] = InputResult::INPUT_RELEASE;

		return _MouseButtonStates[ButtonCode];
	}

	bool Input::GetModState(Input_mod mod)
	{
		return _ModStates & mod;
	}

	const char* Input::KeyToString(Input_key Key)
	{
		return InputKeyToString(Key);
	}

	const char* Input::MouseToString(Input_mouse Mouse)
	{
		return InputMouseToString(Mouse);
	}

	void Input::Clear()
	{
		uint32_t KeyIndex = 0;
		for (auto& state : _KeyStates)
		{
			if (IsKeyDown((Input_key)KeyIndex))
				state = InputResult::INPUT_RELEASE; // Or keep separate tracking
		}
		uint32_t MouseIndex = 0;
		for (auto& state : _MouseButtonStates)
		{
			if (IsMouseButtonDown((Input_mouse)MouseIndex))
				state = InputResult::INPUT_RELEASE; // Or keep separate tracking
		}
	}

	void Input::_SetModStates(int ModState)
	{
		_ModStates = 0;
		if (ModState & Input_mod_Shift)
			_ModStates += Input_mod_Shift;
		if (ModState & Input_mod_Control)
			_ModStates += Chilli::Input_mod_Control;
		if (ModState & Input_mod_Alt)
			_ModStates += Chilli::Input_mod_Alt;
		if (ModState & Input_mod_Super)
			_ModStates += Chilli::Input_mod_Super;
		if (ModState & Input_mod_CapsLock)
			_ModStates += Chilli::Input_mod_CapsLock;
		if (ModState & Input_mod_NumLock)
			_ModStates += Chilli::Input_mod_NumLock;
	}
#pragma endregion 

	int InputKeysToGLFW(Input_key key)
	{
		switch (key)
		{
		case Input_key_0:
			return GLFW_KEY_0;
		case Input_key_1:
			return GLFW_KEY_1;
		case Input_key_2:
			return GLFW_KEY_2;
		case Input_key_3:
			return GLFW_KEY_3;
		case Input_key_4:
			return GLFW_KEY_4;
		case Input_key_5:
			return GLFW_KEY_5;
		case Input_key_6:
			return GLFW_KEY_6;
		case Input_key_7:
			return GLFW_KEY_7;
		case Input_key_8:
			return GLFW_KEY_8;
		case Input_key_9:
			return GLFW_KEY_9;
		case Input_key_A:
			return GLFW_KEY_A;
		case Input_key_B:
			return GLFW_KEY_B;
		case Input_key_C:
			return GLFW_KEY_C;
		case Input_key_D:
			return GLFW_KEY_D;
		case Input_key_E:
			return GLFW_KEY_E;
		case Input_key_F:
			return GLFW_KEY_F;
		case Input_key_G:
			return GLFW_KEY_G;
		case Input_key_H:
			return GLFW_KEY_H;
		case Input_key_I:
			return GLFW_KEY_I;
		case Input_key_J:
			return GLFW_KEY_J;
		case Input_key_K:
			return GLFW_KEY_K;
		case Input_key_L:
			return GLFW_KEY_L;
		case Input_key_M:
			return GLFW_KEY_M;
		case Input_key_N:
			return GLFW_KEY_N;
		case Input_key_O:
			return GLFW_KEY_O;
		case Input_key_P:
			return GLFW_KEY_P;
		case Input_key_Q:
			return GLFW_KEY_Q;
		case Input_key_R:
			return GLFW_KEY_R;
		case Input_key_S:
			return GLFW_KEY_S;
		case Input_key_T:
			return GLFW_KEY_T;
		case Input_key_U:
			return GLFW_KEY_U;
		case Input_key_V:
			return GLFW_KEY_V;
		case Input_key_W:
			return GLFW_KEY_W;
		case Input_key_X:
			return GLFW_KEY_X;
		case Input_key_Y:
			return GLFW_KEY_Y;
		case Input_key_Z:
			return GLFW_KEY_Z;
		case Input_key_Space:
			return GLFW_KEY_SPACE;
		case Input_key_Escape:
			return GLFW_KEY_ESCAPE;
		case Input_key_Enter:
			return GLFW_KEY_ENTER;
		case Input_key_Tab:
			return GLFW_KEY_TAB;
		case Input_key_Backspace:
			return GLFW_KEY_BACKSPACE;
		case Input_key_Left:
			return GLFW_KEY_LEFT;
		case Input_key_Right:
			return GLFW_KEY_RIGHT;
		case Input_key_Up:
			return GLFW_KEY_UP;
		case Input_key_Down:
			return GLFW_KEY_DOWN;
		case Input_key_LeftShift:
			return GLFW_KEY_LEFT_SHIFT;
		case Input_key_RightShift:
			return GLFW_KEY_RIGHT_SHIFT;
		case Input_key_LeftControl:
			return GLFW_KEY_LEFT_CONTROL;
		case Input_key_RightControl:
			return GLFW_KEY_RIGHT_CONTROL;
		case Input_key_LeftAlt:
			return GLFW_KEY_LEFT_ALT;
		case Input_key_RightAlt:
			return GLFW_KEY_RIGHT_ALT;
		case Input_key_F1:
			return GLFW_KEY_F1;
		case Input_key_F2:
			return GLFW_KEY_F2;
		case Input_key_F3:
			return GLFW_KEY_F3;
		case Input_key_F4:
			return GLFW_KEY_F4;
		case Input_key_F5:
			return GLFW_KEY_F5;
		case Input_key_F6:
			return GLFW_KEY_F6;
		case Input_key_F7:
			return GLFW_KEY_F7;
		case Input_key_F8:
			return GLFW_KEY_F8;
		case Input_key_F9:
			return GLFW_KEY_F9;
		case Input_key_F10:
			return GLFW_KEY_F10;
		case Input_key_F11:
			return GLFW_KEY_F11;
		case Input_key_F12:
			return GLFW_KEY_F12;
		case Input_key_Unknown:
			return GLFW_KEY_UNKNOWN;
		case Input_key_Apostrophe:
			return GLFW_KEY_APOSTROPHE;
		case Input_key_Comma:
			return GLFW_KEY_COMMA;
		case Input_key_Minus:
			return GLFW_KEY_MINUS;
		case Input_key_Period:
			return GLFW_KEY_PERIOD;
		case Input_key_Slash:
			return GLFW_KEY_SLASH;
		case Input_key_Semicolon:
			return GLFW_KEY_SEMICOLON;
		case Input_key_Equal:
			return GLFW_KEY_EQUAL;
		case Input_key_LeftBracket:
			return GLFW_KEY_LEFT_BRACKET;
		case Input_key_Backslash:
			return GLFW_KEY_BACKSLASH;
		case Input_key_RightBracket:
			return GLFW_KEY_RIGHT_BRACKET;
		case Input_key_GraveAccent:
			return GLFW_KEY_GRAVE_ACCENT;
		case Input_key_World1:
			return GLFW_KEY_WORLD_1;
		case Input_key_World2:
			return GLFW_KEY_WORLD_2;
		case Input_key_Insert:
			return GLFW_KEY_INSERT;
		case Input_key_Delete:
			return GLFW_KEY_DELETE;
		case Input_key_PageUp:
			return GLFW_KEY_PAGE_UP;
		case Input_key_PageDown:
			return GLFW_KEY_PAGE_DOWN;
		case Input_key_Home:
			return GLFW_KEY_HOME;
		case Input_key_End:
			return GLFW_KEY_END;
		case Input_key_CapsLock:
			return GLFW_KEY_CAPS_LOCK;
		case Input_key_ScrollLock:
			return GLFW_KEY_SCROLL_LOCK;
		case Input_key_NumLock:
			return GLFW_KEY_NUM_LOCK;
		case Input_key_PrintScreen:
			return GLFW_KEY_PRINT_SCREEN;
		case Input_key_Pause:
			return GLFW_KEY_PAUSE;
		case Input_key_F13:
			return GLFW_KEY_F13;
		case Input_key_F14:
			return GLFW_KEY_F14;
		case Input_key_F15:
			return GLFW_KEY_F15;
		case Input_key_F16:
			return GLFW_KEY_F16;
		case Input_key_F17:
			return GLFW_KEY_F17;
		case Input_key_F18:
			return GLFW_KEY_F18;
		case Input_key_F19:
			return GLFW_KEY_F19;
		case Input_key_F20:
			return GLFW_KEY_F20;
		case Input_key_F21:
			return GLFW_KEY_F21;
		case Input_key_F22:
			return GLFW_KEY_F22;
		case Input_key_F23:
			return GLFW_KEY_F23;
		case Input_key_F24:
			return GLFW_KEY_F24;
		case Input_key_F25:
			return GLFW_KEY_F25;
		case Input_key_Kp0:
			return GLFW_KEY_KP_0;
		case Input_key_Kp1:
			return GLFW_KEY_KP_1;
		case Input_key_Kp2:
			return GLFW_KEY_KP_2;
		case Input_key_Kp3:
			return GLFW_KEY_KP_3;
		case Input_key_Kp4:
			return GLFW_KEY_KP_4;
		case Input_key_Kp5:
			return GLFW_KEY_KP_5;
		case Input_key_Kp6:
			return GLFW_KEY_KP_6;
		case Input_key_Kp7:
			return GLFW_KEY_KP_7;
		case Input_key_Kp8:
			return GLFW_KEY_KP_8;
		case Input_key_Kp9:
			return GLFW_KEY_KP_9;
		case Input_key_KpDecimal:
			return GLFW_KEY_KP_DECIMAL;
		case Input_key_KpDivide:
			return GLFW_KEY_KP_DIVIDE;
		case Input_key_KpMultiply:
			return GLFW_KEY_KP_MULTIPLY;
		case Input_key_KpSubtract:
			return GLFW_KEY_KP_SUBTRACT;
		case Input_key_KpAdd:
			return GLFW_KEY_KP_ADD;
		case Input_key_KpEnter:
			return GLFW_KEY_KP_ENTER;
		case Input_key_KpEqual:
			return GLFW_KEY_KP_EQUAL;
		case Input_key_LeftSuper:
			return GLFW_KEY_LEFT_SUPER;
		case Input_key_RightSuper:
			return GLFW_KEY_RIGHT_SUPER;
		case Input_key_Menu:
			return GLFW_KEY_MENU;

		default:
			return -1;
		}
	}

	int InputMouseToGLFW(Input_mouse mouse)
	{
		switch (mouse)
		{
		case Input_mouse_Left:
			return GLFW_MOUSE_BUTTON_LEFT;
		case Input_mouse_Right:
			return GLFW_MOUSE_BUTTON_RIGHT;
		case Input_mouse_Middle:
			return GLFW_MOUSE_BUTTON_MIDDLE;
		case Input_mouse_Button4:
			return GLFW_MOUSE_BUTTON_4;
		case Input_mouse_Button5:
			return GLFW_MOUSE_BUTTON_5;
		case Input_mouse_Button6:
			return GLFW_MOUSE_BUTTON_6;
		case Input_mouse_Button7:
			return GLFW_MOUSE_BUTTON_7;
		case Input_mouse_Button8:
			return GLFW_MOUSE_BUTTON_8;
		default:
			return -1;
		}
	}

	int InputModsToGLFW(Input_mod mod)
	{
		switch (mod)
		{
		case Input_mod_Shift:
			return GLFW_MOD_SHIFT;
		case Input_mod_Control:
			return GLFW_MOD_CONTROL;
		case Input_mod_Alt:
			return GLFW_MOD_ALT;
		case Input_mod_Super:
			return GLFW_MOD_SUPER;
		case Input_mod_CapsLock:
			return GLFW_MOD_CAPS_LOCK;
		case Input_mod_NumLock:
			return GLFW_MOD_NUM_LOCK;
		default:
			return 0;
		}
	}

	Input_key GLFWToInputKeys(int key)
	{
		switch (key)
		{
		case GLFW_KEY_0: return Input_key_0;
		case GLFW_KEY_1: return Input_key_1;
		case GLFW_KEY_2: return Input_key_2;
		case GLFW_KEY_3: return Input_key_3;
		case GLFW_KEY_4: return Input_key_4;
		case GLFW_KEY_5: return Input_key_5;
		case GLFW_KEY_6: return Input_key_6;
		case GLFW_KEY_7: return Input_key_7;
		case GLFW_KEY_8: return Input_key_8;
		case GLFW_KEY_9: return Input_key_9;
		case GLFW_KEY_A: return Input_key_A;
		case GLFW_KEY_B: return Input_key_B;
		case GLFW_KEY_C: return Input_key_C;
		case GLFW_KEY_D: return Input_key_D;
		case GLFW_KEY_E: return Input_key_E;
		case GLFW_KEY_F: return Input_key_F;
		case GLFW_KEY_G: return Input_key_G;
		case GLFW_KEY_H: return Input_key_H;
		case GLFW_KEY_I: return Input_key_I;
		case GLFW_KEY_J: return Input_key_J;
		case GLFW_KEY_K: return Input_key_K;
		case GLFW_KEY_L: return Input_key_L;
		case GLFW_KEY_M: return Input_key_M;
		case GLFW_KEY_N: return Input_key_N;
		case GLFW_KEY_O: return Input_key_O;
		case GLFW_KEY_P: return Input_key_P;
		case GLFW_KEY_Q: return Input_key_Q;
		case GLFW_KEY_R: return Input_key_R;
		case GLFW_KEY_S: return Input_key_S;
		case GLFW_KEY_T: return Input_key_T;
		case GLFW_KEY_U: return Input_key_U;
		case GLFW_KEY_V: return Input_key_V;
		case GLFW_KEY_W: return Input_key_W;
		case GLFW_KEY_X: return Input_key_X;
		case GLFW_KEY_Y: return Input_key_Y;
		case GLFW_KEY_Z: return Input_key_Z;
		case GLFW_KEY_SPACE: return Input_key_Space;
		case GLFW_KEY_ESCAPE: return Input_key_Escape;
		case GLFW_KEY_ENTER: return Input_key_Enter;
		case GLFW_KEY_TAB: return Input_key_Tab;
		case GLFW_KEY_BACKSPACE: return Input_key_Backspace;
		case GLFW_KEY_LEFT: return Input_key_Left;
		case GLFW_KEY_RIGHT: return Input_key_Right;
		case GLFW_KEY_UP: return Input_key_Up;
		case GLFW_KEY_DOWN: return Input_key_Down;
		case GLFW_KEY_LEFT_SHIFT: return Input_key_LeftShift;
		case GLFW_KEY_RIGHT_SHIFT: return Input_key_RightShift;
		case GLFW_KEY_LEFT_CONTROL: return Input_key_LeftControl;
		case GLFW_KEY_RIGHT_CONTROL: return Input_key_RightControl;
		case GLFW_KEY_LEFT_ALT: return Input_key_LeftAlt;
		case GLFW_KEY_RIGHT_ALT: return Input_key_RightAlt;
		case GLFW_KEY_F1: return Input_key_F1;
		case GLFW_KEY_F2: return Input_key_F2;
		case GLFW_KEY_F3: return Input_key_F3;
		case GLFW_KEY_F4: return Input_key_F4;
		case GLFW_KEY_F5: return Input_key_F5;
		case GLFW_KEY_F6: return Input_key_F6;
		case GLFW_KEY_F7: return Input_key_F7;
		case GLFW_KEY_F8: return Input_key_F8;
		case GLFW_KEY_F9: return Input_key_F9;
		case GLFW_KEY_F10: return Input_key_F10;
		case GLFW_KEY_F11: return Input_key_F11;
		case GLFW_KEY_F12: return Input_key_F12;
		case GLFW_KEY_APOSTROPHE: return Input_key_Apostrophe;
		case GLFW_KEY_COMMA: return Input_key_Comma;
		case GLFW_KEY_MINUS: return Input_key_Minus;
		case GLFW_KEY_PERIOD: return Input_key_Period;
		case GLFW_KEY_SLASH: return Input_key_Slash;
		case GLFW_KEY_SEMICOLON: return Input_key_Semicolon;
		case GLFW_KEY_EQUAL: return Input_key_Equal;
		case GLFW_KEY_LEFT_BRACKET: return Input_key_LeftBracket;
		case GLFW_KEY_BACKSLASH: return Input_key_Backslash;
		case GLFW_KEY_RIGHT_BRACKET: return Input_key_RightBracket;
		case GLFW_KEY_GRAVE_ACCENT: return Input_key_GraveAccent;
		case GLFW_KEY_WORLD_1: return Input_key_World1;
		case GLFW_KEY_WORLD_2: return Input_key_World2;
		case GLFW_KEY_INSERT: return Input_key_Insert;
		case GLFW_KEY_DELETE: return Input_key_Delete;
		case GLFW_KEY_PAGE_UP: return Input_key_PageUp;
		case GLFW_KEY_PAGE_DOWN: return Input_key_PageDown;
		case GLFW_KEY_HOME: return Input_key_Home;
		case GLFW_KEY_END: return Input_key_End;
		case GLFW_KEY_CAPS_LOCK: return Input_key_CapsLock;
		case GLFW_KEY_SCROLL_LOCK: return Input_key_ScrollLock;
		case GLFW_KEY_NUM_LOCK: return Input_key_NumLock;
		case GLFW_KEY_PRINT_SCREEN: return Input_key_PrintScreen;
		case GLFW_KEY_PAUSE: return Input_key_Pause;
		case GLFW_KEY_F13: return Input_key_F13;
		case GLFW_KEY_F14: return Input_key_F14;
		case GLFW_KEY_F15: return Input_key_F15;
		case GLFW_KEY_F16: return Input_key_F16;
		case GLFW_KEY_F17: return Input_key_F17;
		case GLFW_KEY_F18: return Input_key_F18;
		case GLFW_KEY_F19: return Input_key_F19;
		case GLFW_KEY_F20: return Input_key_F20;
		case GLFW_KEY_F21: return Input_key_F21;
		case GLFW_KEY_F22: return Input_key_F22;
		case GLFW_KEY_F23: return Input_key_F23;
		case GLFW_KEY_F24: return Input_key_F24;
		case GLFW_KEY_F25: return Input_key_F25;
		case GLFW_KEY_KP_0: return Input_key_Kp0;
		case GLFW_KEY_KP_1: return Input_key_Kp1;
		case GLFW_KEY_KP_2: return Input_key_Kp2;
		case GLFW_KEY_KP_3: return Input_key_Kp3;
		case GLFW_KEY_KP_4: return Input_key_Kp4;
		case GLFW_KEY_KP_5: return Input_key_Kp5;
		case GLFW_KEY_KP_6: return Input_key_Kp6;
		case GLFW_KEY_KP_7: return Input_key_Kp7;
		case GLFW_KEY_KP_8: return Input_key_Kp8;
		case GLFW_KEY_KP_9: return Input_key_Kp9;
		case GLFW_KEY_KP_DECIMAL: return Input_key_KpDecimal;
		case GLFW_KEY_KP_DIVIDE: return Input_key_KpDivide;
		case GLFW_KEY_KP_MULTIPLY: return Input_key_KpMultiply;
		case GLFW_KEY_KP_SUBTRACT: return Input_key_KpSubtract;
		case GLFW_KEY_KP_ADD: return Input_key_KpAdd;
		case GLFW_KEY_KP_ENTER: return Input_key_KpEnter;
		case GLFW_KEY_KP_EQUAL: return Input_key_KpEqual;
		case GLFW_KEY_LEFT_SUPER: return Input_key_LeftSuper;
		case GLFW_KEY_RIGHT_SUPER: return Input_key_RightSuper;
		case GLFW_KEY_MENU: return Input_key_Menu;
		case GLFW_KEY_UNKNOWN: return Input_key_Unknown;
		default: return Input_key_Unknown;
		}
	}

	Input_mouse GLFWToInputMouse(int mouse)
	{
		switch (mouse)
		{
		case GLFW_MOUSE_BUTTON_LEFT:   return Input_mouse_Left;
		case GLFW_MOUSE_BUTTON_RIGHT:  return Input_mouse_Right;
		case GLFW_MOUSE_BUTTON_MIDDLE: return Input_mouse_Middle;
		case GLFW_MOUSE_BUTTON_4:      return Input_mouse_Button4;
		case GLFW_MOUSE_BUTTON_5:      return Input_mouse_Button5;
		case GLFW_MOUSE_BUTTON_6:      return Input_mouse_Button6;
		case GLFW_MOUSE_BUTTON_7:      return Input_mouse_Button7;
		case GLFW_MOUSE_BUTTON_8:      return Input_mouse_Button8;
		default:                       return Input_mouse_Left; // Or a custom 'Unknown' if defined
		}
	}

	Input_mod GLFWToInputMods(int mod)
	{
		switch (mod)
		{
		case GLFW_MOD_SHIFT:    return Input_mod_Shift;
		case GLFW_MOD_CONTROL:  return Input_mod_Control;
		case GLFW_MOD_ALT:      return Input_mod_Alt;
		case GLFW_MOD_SUPER:    return Input_mod_Super;
		case GLFW_MOD_CAPS_LOCK:return Input_mod_CapsLock;
		case GLFW_MOD_NUM_LOCK: return Input_mod_NumLock;
		default:                return Input_mod_Shift; // Or a custom 'Unknown' if defined
		}
	}

	const char* InputKeyToString(Input_key key)
	{
		switch (key)
		{
		case Input_key_0: return "0";
		case Input_key_1: return "1";
		case Input_key_2: return "2";
		case Input_key_3: return "3";
		case Input_key_4: return "4";
		case Input_key_5: return "5";
		case Input_key_6: return "6";
		case Input_key_7: return "7";
		case Input_key_8: return "8";
		case Input_key_9: return "9";
		case Input_key_A: return "A";
		case Input_key_B: return "B";
		case Input_key_C: return "C";
		case Input_key_D: return "D";
		case Input_key_E: return "E";
		case Input_key_F: return "F";
		case Input_key_G: return "G";
		case Input_key_H: return "H";
		case Input_key_I: return "I";
		case Input_key_J: return "J";
		case Input_key_K: return "K";
		case Input_key_L: return "L";
		case Input_key_M: return "M";
		case Input_key_N: return "N";
		case Input_key_O: return "O";
		case Input_key_P: return "P";
		case Input_key_Q: return "Q";
		case Input_key_R: return "R";
		case Input_key_S: return "S";
		case Input_key_T: return "T";
		case Input_key_U: return "U";
		case Input_key_V: return "V";
		case Input_key_W: return "W";
		case Input_key_X: return "X";
		case Input_key_Y: return "Y";
		case Input_key_Z: return "Z";
		case Input_key_Space: return "Space";
		case Input_key_Escape: return "Escape";
		case Input_key_Enter: return "Enter";
		case Input_key_Tab: return "Tab";
		case Input_key_Backspace: return "Backspace";
		case Input_key_Left: return "Left";
		case Input_key_Right: return "Right";
		case Input_key_Up: return "Up";
		case Input_key_Down: return "Down";
		case Input_key_LeftShift: return "LeftShift";
		case Input_key_RightShift: return "RightShift";
		case Input_key_LeftControl: return "LeftControl";
		case Input_key_RightControl: return "RightControl";
		case Input_key_LeftAlt: return "LeftAlt";
		case Input_key_RightAlt: return "RightAlt";
		case Input_key_F1: return "F1";
		case Input_key_F2: return "F2";
		case Input_key_F3: return "F3";
		case Input_key_F4: return "F4";
		case Input_key_F5: return "F5";
		case Input_key_F6: return "F6";
		case Input_key_F7: return "F7";
		case Input_key_F8: return "F8";
		case Input_key_F9: return "F9";
		case Input_key_F10: return "F10";
		case Input_key_F11: return "F11";
		case Input_key_F12: return "F12";
		case Input_key_Unknown: return "Unknown";
		case Input_key_Apostrophe: return "Apostrophe";
		case Input_key_Comma: return "Comma";
		case Input_key_Minus: return "Minus";
		case Input_key_Period: return "Period";
		case Input_key_Slash: return "Slash";
		case Input_key_Semicolon: return "Semicolon";
		case Input_key_Equal: return "Equal";
		case Input_key_LeftBracket: return "LeftBracket";
		case Input_key_Backslash: return "Backslash";
		case Input_key_RightBracket: return "RightBracket";
		case Input_key_GraveAccent: return "GraveAccent";
		case Input_key_World1: return "World1";
		case Input_key_World2: return "World2";
		case Input_key_Insert: return "Insert";
		case Input_key_Delete: return "Delete";
		case Input_key_PageUp: return "PageUp";
		case Input_key_PageDown: return "PageDown";
		case Input_key_Home: return "Home";
		case Input_key_End: return "End";
		case Input_key_CapsLock: return "CapsLock";
		case Input_key_ScrollLock: return "ScrollLock";
		case Input_key_NumLock: return "NumLock";
		case Input_key_PrintScreen: return "PrintScreen";
		case Input_key_Pause: return "Pause";
		case Input_key_F13: return "F13";
		case Input_key_F14: return "F14";
		case Input_key_F15: return "F15";
		case Input_key_F16: return "F16";
		case Input_key_F17: return "F17";
		case Input_key_F18: return "F18";
		case Input_key_F19: return "F19";
		case Input_key_F20: return "F20";
		case Input_key_F21: return "F21";
		case Input_key_F22: return "F22";
		case Input_key_F23: return "F23";
		case Input_key_F24: return "F24";
		case Input_key_F25: return "F25";
		case Input_key_Kp0: return "Kp0";
		case Input_key_Kp1: return "Kp1";
		case Input_key_Kp2: return "Kp2";
		case Input_key_Kp3: return "Kp3";
		case Input_key_Kp4: return "Kp4";
		case Input_key_Kp5: return "Kp5";
		case Input_key_Kp6: return "Kp6";
		case Input_key_Kp7: return "Kp7";
		case Input_key_Kp8: return "Kp8";
		case Input_key_Kp9: return "Kp9";
		case Input_key_KpDecimal: return "KpDecimal";
		case Input_key_KpDivide: return "KpDivide";
		case Input_key_KpMultiply: return "KpMultiply";
		case Input_key_KpSubtract: return "KpSubtract";
		case Input_key_KpAdd: return "KpAdd";
		case Input_key_KpEnter: return "KpEnter";
		case Input_key_KpEqual: return "KpEqual";
		case Input_key_LeftSuper: return "LeftSuper";
		case Input_key_RightSuper: return "RightSuper";
		case Input_key_Menu: return "Menu";
		default: return "Invalid";
		}
	}

	const char* InputMouseToString(Input_mouse mouse)
	{
		switch (mouse)
		{
		case Input_mouse_Left: return "Left";
		case Input_mouse_Right: return "Right";
		case Input_mouse_Middle: return "Middle";
		case Input_mouse_Button4: return "Button4";
		case Input_mouse_Button5: return "Button5";
		case Input_mouse_Button6: return "Button6";
		case Input_mouse_Button7: return "Button7";
		case Input_mouse_Button8: return "Button8";
		default: return "Invalid";
		}
	}

	const char* InputModToString(Input_mod mod)
	{
		switch (mod)
		{
		case Input_mod_Shift: return "Shift";
		case Input_mod_Control: return "Control";
		case Input_mod_Alt: return "Alt";
		case Input_mod_Super: return "Super";
		case Input_mod_CapsLock: return "CapsLock";
		case Input_mod_NumLock: return "NumLock";
		default: return "Invalid";
		}
	}
} // namespace VEngine