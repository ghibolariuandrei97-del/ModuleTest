// HudSprites - screen-space sprites for AMXX (GoldSrc), no precache-position limits.
// Method: one sprite entity per element, projected in front of the owner's eyes
// inside AddToFullPack_Post, and hidden from everybody else.

#include "amxxmodule.h"
#include "entity_state.h"

#ifdef _MSC_VER
#pragma comment(linker, "/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8")
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>

#define MAX_ELEM  1024
#define MAX_SPR   128
#define MAGIC     0x48535052
#define DIST      8.0f     // units in front of the eye (near clip is ~4)
#define ASPECT    0.75f    // vertical/horizontal screen ratio (4:3 default)
#define REF_WIDTH 640.0f   // scale 1.0 == native pixel size at 640 px wide
#define DEG2RAD   0.01745329252f

struct SprInfo { char path[96]; int model; int frames; };
struct Elem
{
	bool used; unsigned gen; edict_t *ent;
	int owner, spr, mode, r, g, b, a;
	float x, y, scale, fps, start, expire, frame;
};

static SprInfo g_spr[MAX_SPR];
static int     g_sprCount = 0;
static Elem    g_el[MAX_ELEM];
static bool    g_precacheOpen = true;   // true doar cat timp harta se incarca

static Elem *Resolve(cell h)
{
	int i = (int)(h & 0xFFF) - 1;
	if (i < 0 || i >= MAX_ELEM) return NULL;
	Elem &e = g_el[i];
	if (!e.used || (e.gen & 0x7FFFF) != (unsigned)(h >> 12)) return NULL;
	return &e;
}

static void FreeElem(Elem &e)
{
	e.used = false;
	if (e.ent) e.ent->v.effects |= EF_NODRAW;
}

static void ApplyRender(Elem &e)
{
	e.ent->v.rendermode  = e.mode;
	e.ent->v.renderamt   = (float)e.a;
	e.ent->v.rendercolor = Vector((float)e.r, (float)e.g, (float)e.b);
}

static const char *PlName(AMX *amx)
{
	int i = MF_FindScriptByAmx(amx);
	const char *n = (i >= 0) ? MF_GetScriptName(i) : NULL;
	return n ? n : "unknown";
}

static cell PrecacheFail(AMX *amx, const char *name, const char *why)
{
	MF_Log("[HudSprites] hs_precache(\"%s\") failed in %s: %s", name, PlName(amx), why);
	return -1;
}

// ---------------------------------------------------------------- natives

// hs_precache(const name[]) -> sprite id sau -1. Apel doar din plugin_precache().
static cell AMX_NATIVE_CALL hs_precache(AMX *amx, cell *p)
{
	int len;
	char *in = MF_GetAmxString(amx, p[1], 0, &len);

	char name[80];
	if (len <= 0)
		return PrecacheFail(amx, "", "empty sprite name");
	if (len >= (int)sizeof(name))
		return PrecacheFail(amx, "...", "sprite name too long");
	strncpy(name, in, sizeof(name) - 1);
	name[sizeof(name) - 1] = 0;

	// accepta "x", "x.spr" si "sprites/x.spr"
	if (!strncmp(name, "sprites/", 8)) memmove(name, name + 8, strlen(name + 8) + 1);
	size_t l = strlen(name);
	if (l > 4 && !strcmp(name + l - 4, ".spr")) name[l - 4] = 0;

	if (!name[0] || strstr(name, "..") || strchr(name, ':'))
		return PrecacheFail(amx, name, "invalid sprite name");

	char path[96];
	snprintf(path, sizeof(path), "sprites/%s.spr", name);

	// deja precache-uit -> intoarce acelasi id
	for (int i = 0; i < g_sprCount; i++)
		if (!strcmp(g_spr[i].path, path)) return i;

	if (!g_precacheOpen)
		return PrecacheFail(amx, name, "called too late, use it only inside plugin_precache()");

	if (g_sprCount >= MAX_SPR)
		return PrecacheFail(amx, name, "sprite limit reached");

	int flen = 0;
	byte *f = LOAD_FILE_FOR_ME(path, &flen);
	if (!f)
	{
		char why[160];
		snprintf(why, sizeof(why), "file %s not found (put it in cstrike/sprites, check the name)", path);
		return PrecacheFail(amx, name, why);
	}

	bool magic = flen >= 36 && !memcmp(f, "IDSP", 4);
	int version = magic ? *(int *)(f + 4) : 0;
	int frames  = magic ? *(int *)(f + 32) : 0;
	FREE_FILE(f);

	if (!magic)
		return PrecacheFail(amx, name, "not a valid .spr file (bad header or file too small)");
	if (version != 2)
	{
		char why[96];
		snprintf(why, sizeof(why), "unsupported sprite version %d (need 2)", version);
		return PrecacheFail(amx, name, why);
	}
	if (frames < 1)
		return PrecacheFail(amx, name, "sprite has no frames");

	SprInfo &s = g_spr[g_sprCount];
	strncpy(s.path, path, sizeof(s.path) - 1);
	s.path[sizeof(s.path) - 1] = 0;
	s.model = PRECACHE_MODEL(s.path);
	if (!s.model)
		return PrecacheFail(amx, name, "the engine refused to precache the model");
	s.frames = frames;
	return g_sprCount++;
}

// hs_create(id, sprite, Float:x, Float:y, Float:scale, Float:life) -> handle or 0
static cell AMX_NATIVE_CALL hs_create(AMX *amx, cell *p)
{
	int id = p[1], spr = p[2];
	if (id < 1 || id > gpGlobals->maxClients) { MF_LogError(amx, AMX_ERR_NATIVE, "Invalid player %d", id); return 0; }

	if (spr == -1) return 0;   // precache a esuat, motivul e deja in log
	if (spr < 0 || spr >= g_sprCount)
	{
		MF_Log("[HudSprites] hs_create: invalid sprite id %d in %s", spr, PlName(amx));
		return 0;
	}

	int i = 0;
	while (i < MAX_ELEM && g_el[i].used) i++;
	if (i == MAX_ELEM) return 0;

	Elem &e = g_el[i];
	if (!e.ent)
	{
		edict_t *t = CREATE_NAMED_ENTITY(ALLOC_STRING("info_target"));
		if (FNullEnt(t)) return 0;
		t->v.movetype = MOVETYPE_NONE;
		t->v.solid    = SOLID_NOT;
		t->v.iuser4   = MAGIC;
		t->v.iuser3   = i;
		e.ent = t;
	}

	e.used = true; e.gen++;
	e.owner = id; e.spr = spr;
	e.x = amx_ctof(p[3]); e.y = amx_ctof(p[4]); e.scale = amx_ctof(p[5]);
	e.fps = 0; e.frame = 0; e.start = gpGlobals->time;
	float life = amx_ctof(p[6]);
	e.expire = life > 0 ? gpGlobals->time + life : 0;
	e.mode = kRenderTransAdd; e.r = e.g = e.b = 255; e.a = 255;

	SET_MODEL(e.ent, g_spr[spr].path);
	e.ent->v.effects &= ~EF_NODRAW;
	ApplyRender(e);
	return (cell)(((e.gen & 0x7FFFF) << 12) | (unsigned)(i + 1));
}

static cell AMX_NATIVE_CALL hs_move(AMX *, cell *p)
{
	Elem *e = Resolve(p[1]); if (!e) return 0;
	e->x = amx_ctof(p[2]); e->y = amx_ctof(p[3]); return 1;
}

static cell AMX_NATIVE_CALL hs_scale(AMX *, cell *p)
{
	Elem *e = Resolve(p[1]); if (!e) return 0;
	e->scale = amx_ctof(p[2]); return 1;
}

static cell AMX_NATIVE_CALL hs_frame(AMX *, cell *p)
{
	Elem *e = Resolve(p[1]); if (!e) return 0;
	e->fps = 0; e->frame = (float)p[2]; return 1;
}

static cell AMX_NATIVE_CALL hs_animate(AMX *, cell *p)
{
	Elem *e = Resolve(p[1]); if (!e) return 0;
	e->fps = amx_ctof(p[2]); e->start = gpGlobals->time; return 1;
}

// hs_color(handle, r, g, b, alpha, mode)
static cell AMX_NATIVE_CALL hs_color(AMX *, cell *p)
{
	Elem *e = Resolve(p[1]); if (!e) return 0;
	e->r = p[2]; e->g = p[3]; e->b = p[4]; e->a = p[5]; e->mode = p[6];
	ApplyRender(*e); return 1;
}

static cell AMX_NATIVE_CALL hs_remove(AMX *, cell *p)
{
	Elem *e = Resolve(p[1]); if (!e) return 0;
	FreeElem(*e); return 1;
}

static cell AMX_NATIVE_CALL hs_clear(AMX *, cell *p)
{
	int n = 0;
	for (int i = 0; i < MAX_ELEM; i++)
		if (g_el[i].used && g_el[i].owner == p[1]) { FreeElem(g_el[i]); n++; }
	return n;
}

static cell AMX_NATIVE_CALL hs_valid(AMX *, cell *p) { return Resolve(p[1]) != NULL; }

static AMX_NATIVE_INFO g_natives[] =
{
	{ "hs_precache", hs_precache }, { "hs_create", hs_create },
	{ "hs_move", hs_move },         { "hs_scale", hs_scale },
	{ "hs_frame", hs_frame },       { "hs_animate", hs_animate },
	{ "hs_color", hs_color },       { "hs_remove", hs_remove },
	{ "hs_clear", hs_clear },       { "hs_valid", hs_valid },
	{ NULL, NULL }
};

void OnAmxxAttach() { MF_AddNatives(g_natives); }
void OnAmxxDetach() {}

// ---------------------------------------------------------------- hooks

void ServerDeactivate()
{
	g_precacheOpen = true;           // harta urmatoare isi face precache din nou
	memset(g_el, 0, sizeof(g_el));   // map change wipes all edicts
	g_sprCount = 0;                  // plugins re-precache every map
	RETURN_META(MRES_IGNORED);
}

void ServerActivate_Post(edict_t *pEdictList, int edictCount, int clientMax)
{
	g_precacheOpen = false;          // de aici nu se mai poate face precache
	RETURN_META(MRES_IGNORED);
}

void ClientDisconnect(edict_t *pEnt)
{
	int id = ENTINDEX(pEnt);
	for (int i = 0; i < MAX_ELEM; i++)
		if (g_el[i].used && g_el[i].owner == id) FreeElem(g_el[i]);
	RETURN_META(MRES_IGNORED);
}

// keeps every element inside the owner's PVS, handles life timers
void StartFrame()
{
	float t = gpGlobals->time;
	for (int i = 0; i < MAX_ELEM; i++)
	{
		Elem &e = g_el[i];
		if (!e.used) continue;
		if ((e.expire > 0 && t >= e.expire) || !MF_IsPlayerIngame(e.owner)) { FreeElem(e); continue; }
		edict_t *o = INDEXENT(e.owner);
		Vector eye = o->v.origin + o->v.view_ofs;
		SET_ORIGIN(e.ent, eye);
	}
	RETURN_META(MRES_IGNORED);
}

// the actual projection, done per client right before the packet is built
int AddToFullPack_Post(struct entity_state_s *st, int e, edict_t *ent, edict_t *host,
                       int hostflags, int player, unsigned char *pSet)
{
	if (!META_RESULT_ORIG_RET(int) || player || ent->v.iuser4 != MAGIC)
		RETURN_META_VALUE(MRES_IGNORED, 0);

	Elem &el = g_el[ent->v.iuser3];
	edict_t *owner = el.used ? INDEXENT(el.owner) : NULL;

	bool see = owner && (host == owner || (host->v.iuser1 && host->v.iuser2 == el.owner));
	if (!see) RETURN_META_VALUE(MRES_SUPERCEDE, 0);

	float fov = owner->v.fov > 0 ? owner->v.fov : 90.0f;
	float half = DIST * tanf(fov * 0.5f * DEG2RAD);   // half screen width at DIST

	float pitch = owner->v.v_angle.x * DEG2RAD, yaw = owner->v.v_angle.y * DEG2RAD;
	float sp = sinf(pitch), cp = cosf(pitch), sy = sinf(yaw), cy = cosf(yaw);

	Vector fwd(cp * cy, cp * sy, -sp), right(sy, -cy, 0), up(sp * cy, sp * sy, cp);
	Vector eye = owner->v.origin + owner->v.view_ofs;

	float rx = (el.x - 0.5f) * 2.0f * half;
	float uy = (0.5f - el.y) * 2.0f * half * ASPECT;
	Vector pos = eye + fwd * DIST + right * rx + up * uy;

	st->origin[0] = pos.x; st->origin[1] = pos.y; st->origin[2] = pos.z;
	st->scale = el.scale * 2.0f * half / REF_WIDTH;

	int frames = g_spr[el.spr].frames;
	if (el.fps > 0 && frames > 1)
		st->frame = fmodf((gpGlobals->time - el.start) * el.fps, (float)frames);
	else
		st->frame = el.frame;

	RETURN_META_VALUE(MRES_SUPERCEDE, 1);
}
