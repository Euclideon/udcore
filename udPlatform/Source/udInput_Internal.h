#include "udInput.h"

#define MAX_KEYBOARDS 1
#define MAX_MOUSES 1
#define MAX_GAMEPADS 4

struct InputState
{
  uint8_t keys[MAX_KEYBOARDS][udKC_Max];
  float mouse[MAX_MOUSES][udMC_Max];
  float gamepad[MAX_GAMEPADS][udGC_Max];
};

extern InputState gInputState[2];
extern int gCurrentInputState;

extern unsigned char udAsciiToUDKey[128];

void udInput_Init();
void udInput_Update();

void udInput_InitInternal();
void udInput_UpdateInternal();
