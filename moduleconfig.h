#ifndef __MODULECONFIG_H__
#define __MODULECONFIG_H__

// Informatii modul
#define MODULE_NAME "Hudsprites"
#define MODULE_VERSION "1.0"
#define MODULE_AUTHOR "Craxor"
#define MODULE_URL "https://github.com/ghibolariuandrei97"
#define MODULE_LOGTAG "HUDSPRITES"
#define MODULE_LIBRARY "hudsprites"
#define MODULE_LIBCLASS ""
#define MODULE_DATE __DATE__

// Folosim Metamod
#define USE_METAMOD

// Forward declarations (tipurile complete vin din HLSDK mai tarziu)
struct edict_s;
typedef struct edict_s edict_t;
struct entity_state_s;

// Prototipuri
void OnAmxxAttach();
void OnAmxxDetach();
void ServerActivate_Post(edict_t *pEdictList, int edictCount, int clientMax);
void ServerDeactivate();
void StartFrame();
void ClientDisconnect(edict_t *pEntity);
int AddToFullPack_Post(struct entity_state_s *state, int e, edict_t *ent,
                       edict_t *host, int hostflags, int player,
                       unsigned char *pSet);

// Hook-uri
#define FN_ServerActivate_Post ServerActivate_Post
#define FN_AMXX_ATTACH        OnAmxxAttach
#define FN_AMXX_DETACH        OnAmxxDetach
#define FN_ServerDeactivate   ServerDeactivate
#define FN_StartFrame         StartFrame
#define FN_ClientDisconnect   ClientDisconnect
#define FN_AddToFullPack_Post AddToFullPack_Post

#endif // __MODULECONFIG_H__
