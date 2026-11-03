#ifndef __MODULECONFIG_H__
#define __MODULECONFIG_H__

#define MODULE_NAME "Hudsprites"
#define MODULE_VERSION "1.0"
#define MODULE_AUTHOR "Craxor"
#define MODULE_URL "https://github.com/ghibolariuandrei97"
#define MODULE_LOGTAG "HUDSPRITES"
#define MODULE_LIBRARY "hudsprites"
#define MODULE_LIBCLASS ""
#define MODULE_DATE __DATE__

#define USE_METAMOD

// prototypes
void OnAmxxAttach();
void OnAmxxDetach();
void ServerDeactivate();
void StartFrame();
void ClientDisconnect(edict_t *pEntity);
int AddToFullPack_Post(struct entity_state_s *state, int e, edict_t *ent,
                       edict_t *host, int hostflags, int player,
                       unsigned char *pSet);

// hooks
#define FN_AMXX_ATTACH        OnAmxxAttach
#define FN_AMXX_DETACH        OnAmxxDetach
#define FN_ServerDeactivate   ServerDeactivate
#define FN_StartFrame         StartFrame
#define FN_ClientDisconnect   ClientDisconnect
#define FN_AddToFullPack_Post AddToFullPack_Post

#endif
