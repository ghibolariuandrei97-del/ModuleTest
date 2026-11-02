#include "amxxmodule.h"

static cell AMX_NATIVE_CALL my_hello(AMX *amx, cell *params)
{
    int len;
    char *msg = MF_GetAmxString(amx, params[1], 0, &len);
    MF_Log("my_hello: %s", msg);
    return len;
}

AMX_NATIVE_INFO my_natives[] = {
    { "my_hello", my_hello },
    { NULL, NULL }
};

void OnAmxxAttach()
{
    MF_AddNatives(my_natives);
}

void OnAmxxDetach()
{
}