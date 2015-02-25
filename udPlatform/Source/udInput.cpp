#include "udInput_Internal.h"

const int gMaxDevices[udID_Max] =
{
  MAX_KEYBOARDS, // udID_Keyboard
  MAX_MOUSES, // udID_Mouse
  MAX_GAMEPADS, // udID_Gamepad
//  1, // udID_TouchScreen
//  1, // udID_Accelerometer
//  1  // udID_Compas
};

//static InputState liveState;
InputState gInputState[2];
int gCurrentInputState = 0;

static unsigned int mouseLock = 0;
static unsigned int mouseLocked = 0;

static int ignoreMouseEvents = 0;


// ********************************************************
// Author: Manu Evans, March 2015
udInputDeviceState udInput_GetDeviceState(udInputDevice, int)
{
  return udIDS_Ready;
}

// ********************************************************
// Author: Manu Evans, March 2015
unsigned int udInput_LockMouseOnButtons(unsigned int buttonBits)
{
  unsigned int old = mouseLock;
  mouseLock = buttonBits;
  return old;
}

// ********************************************************
// Author: Manu Evans, March 2015
bool udInput_WasPressed(udInputDevice device, int control, int deviceIndex)
{
  if (deviceIndex == -1)
  {
    for (int i=0; i<gMaxDevices[device]; ++i)
      if(udInput_WasPressed(device, control, i))
        return true;
    return false;
  }

  InputState prev = gInputState[1-gCurrentInputState];
  InputState state = gInputState[gCurrentInputState];
  switch (device)
  {
    case udID_Keyboard:
      return state.keys[deviceIndex][control] && !prev.keys[deviceIndex][control];
    case udID_Mouse:
      return state.mouse[deviceIndex][control] && !prev.mouse[deviceIndex][control];;
    case udID_Gamepad:
      return state.gamepad[deviceIndex][control] && !prev.gamepad[deviceIndex][control];
  }
  return false;
}

// ********************************************************
// Author: Manu Evans, March 2015
bool udInput_WasReleased(udInputDevice device, int control, int deviceIndex)
{
  if (deviceIndex == -1)
  {
    for (int i=0; i<gMaxDevices[device]; ++i)
      if(udInput_WasReleased(device, control, i))
        return true;
    return false;
  }

  InputState prev = gInputState[1-gCurrentInputState];
  InputState state = gInputState[gCurrentInputState];
  switch (device)
  {
    case udID_Keyboard:
      return !state.keys[deviceIndex][control] && prev.keys[deviceIndex][control];
    case udID_Mouse:
      return !state.mouse[deviceIndex][control] && prev.mouse[deviceIndex][control];
    case udID_Gamepad:
      return !state.gamepad[deviceIndex][control] && prev.gamepad[deviceIndex][control];
  }
  return false;
}

// ********************************************************
// Author: Manu Evans, March 2015
float udInput_State(udInputDevice device, int control, int deviceIndex)
{
  if (deviceIndex == -1)
  {
    float state = 0.f;
    for (int i=0; i<gMaxDevices[device]; ++i)
      state += udInput_State(device, control, i);
    return state;
  }

  InputState state = gInputState[gCurrentInputState];
  switch (device)
  {
    case udID_Keyboard:
      return state.keys[deviceIndex][control] ? 1.f : 0.f;
    case udID_Mouse:
      return state.mouse[deviceIndex][control];
    case udID_Gamepad:
      return state.gamepad[deviceIndex][control];
//    case udID_TouchScreen:
//    case udID_Accelerometer:
//    case udID_Compas:
      // etc...
  }
  return 0.0;
}

// ********************************************************
// Author: Manu Evans, March 2015
void udInput_Init()
{
  udInput_InitInternal();
}

// ********************************************************
// Author: Manu Evans, March 2015
void udInput_Update()
{
  // switch input frame
  gCurrentInputState = 1 - gCurrentInputState;

  udInput_UpdateInternal();
}

// --------------------------------------------------------
// Author: Manu Evans, March 2015
void udInput_UpdateInternal()
{
  UDASSERT(false, "TODO");
}
