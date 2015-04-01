#ifndef UDINPUT_H
#define UDINPUT_H
//
// Copyright (c) Euclideon Pty Ltd
//
// Creator: Manu Evans, Feb 2015
//

#include "udPlatform.h"
#include "udResult.h"

// input API
enum udInputDevice
{
  udID_Keyboard,
  udID_Mouse,
  udID_Gamepad,
//  udID_TouchScreen,
//  udID_Accelerometer,
//  udID_Compass,

  udID_Max
};

enum udInputDeviceState
{
  udIDS_Ready,
  udIDS_Unavailable
};

udInputDeviceState udInput_GetDeviceState(udInputDevice device, int deviceIndex);

bool udInput_WasPressed(udInputDevice device, int control, int deviceIndex = -1);
bool udInput_WasReleased(udInputDevice device, int control, int deviceIndex = -1);
float udInput_State(udInputDevice device, int control, int deviceIndex = -1);

unsigned int udInput_LockMouseOnButtons(unsigned int buttonBits);

// controls for devices
enum udMouseControls
{
  udMC_LeftButton,
  udMC_MiddleButton,
  udMC_RightButton,
  udMC_Button4,
  udMC_Button5,
  udMC_XDelta,
  udMC_YDelta,
  udMC_XAbsolute,
  udMC_YAbsolute,
  udMC_XBrowser,
  udMC_YBrowser,
  udMC_XScreen,
  udMC_YScreen,

  udMC_Max
};

enum udGamepadControl
{
  udGC_AxisLX,
  udGC_AxisLY,
  udGC_AxisRX,
  udGC_AxisRY,
  udGC_ButtonA,
  udGC_ButtonB,
  udGC_ButtonX,
  udGC_ButtonY,
  udGC_ButtonLB,
  udGC_ButtonRB,
  udGC_ButtonLT,
  udGC_ButtonRT,
  udGC_ButtonLThumb,
  udGC_ButtonRThumb,
  udGC_ButtonStart,
  udGC_ButtonBack,
  udGC_ButtonDUp,
  udGC_ButtonDDown,
  udGC_ButtonDLeft,
  udGC_ButtonDRight,
  udGC_Home,

  udGC_Max
};

enum udKeyCodes
{
  udKC_Unknown = 0,

  udKC_LShift,
  udKC_LCtrl,
  udKC_LAlt,
  udKC_RShift,
  udKC_RCtrl,
  udKC_RAlt,
  udKC_LWin,
  udKC_RWin,
  udKC_Menu,

  udKC_Left,
  udKC_Up,
  udKC_Right,
  udKC_Down,

  udKC_Backspace,
  udKC_Tab,

  udKC_Enter,

  udKC_Escape,

  udKC_Insert,
  udKC_Delete,

  udKC_PageUp,
  udKC_PageDown,
  udKC_End,
  udKC_Home,

  udKC_PrintScreen,
  udKC_Pause,

  udKC_CapsLock,
  udKC_ScrollLock,
  udKC_NumLock,

  udKC_Space,

  udKC_Semicolon,
  udKC_Equals,
  udKC_Comma,
  udKC_Hyphen,
  udKC_Period,
  udKC_ForwardSlash,
  udKC_Grave,

  udKC_LeftBracket,
  udKC_BackSlash,
  udKC_RightBracket,
  udKC_Apostrophe,

  udKC_A,
  udKC_B,
  udKC_C,
  udKC_D,
  udKC_E,
  udKC_F,
  udKC_G,
  udKC_H,
  udKC_I,
  udKC_J,
  udKC_K,
  udKC_L,
  udKC_M,
  udKC_N,
  udKC_O,
  udKC_P,
  udKC_Q,
  udKC_R,
  udKC_S,
  udKC_T,
  udKC_U,
  udKC_V,
  udKC_W,
  udKC_X,
  udKC_Y,
  udKC_Z,

  udKC_0,
  udKC_1,
  udKC_2,
  udKC_3,
  udKC_4,
  udKC_5,
  udKC_6,
  udKC_7,
  udKC_8,
  udKC_9,

  udKC_F1,
  udKC_F2,
  udKC_F3,
  udKC_F4,
  udKC_F5,
  udKC_F6,
  udKC_F7,
  udKC_F8,
  udKC_F9,
  udKC_F10,
  udKC_F11,
  udKC_F12,

  udKC_Numpad0,
  udKC_Numpad1,
  udKC_Numpad2,
  udKC_Numpad3,
  udKC_Numpad4,
  udKC_Numpad5,
  udKC_Numpad6,
  udKC_Numpad7,
  udKC_Numpad8,
  udKC_Numpad9,
  udKC_NumpadMultiply,
  udKC_NumpadPlus,
  udKC_NumpadMinus,
  udKC_NumpadDecimal,
  udKC_NumpadDivide,
  udKC_NumpadEnter,
  udKC_NumpadComma,
  udKC_NumpadEquals,

  udKC_Max
};


inline udKeyCodes udInput_AsciiToKeyCode(unsigned char c)
{
  extern unsigned char udAsciiToUDKey[128];
  return (udKeyCodes)udAsciiToUDKey[c];
}


// internal
void udInput_Init();
void udInput_Update();

#endif // UDINPUT_H
