#include <coco.h>

static char lastKey = 0;

uint8_t kbhit(void)
{
    return (char)(lastKey || (lastKey = inkey()) || (lastKey = inkey()));
}

char cgetc(void)
{
    char key = lastKey;

    lastKey = 0;

    while (!key)
    {
        key = (char)inkey();
    }

    return key;
}


uint8_t readJoystick()
{
    // byte*  joy = *readJoystickPositions();
    // return joy[0];
    return 0;
}
