// sgtc_misc.cpp — remaining SGTC point entities (ref item_NVG @0xfcb84,
// special_teleport_in @0x195e1e / CSpecialTeleport @0x1955a6, zat_proj @0x1b6cae).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "items.h"
#include "sgtc_rules.h"
#include "sgtc_weapons.h"
#include "sgtc_misc.h"

extern int gmsgFlashlight;

// ---------------------------------------------------------------------------
// item_NVG — ref CItemNVG::MyTouch @0xfda98 sets player+0xe1c; we use iuser4.
// ---------------------------------------------------------------------------
class CItemNVG : public CItem
{
public:
	void Spawn( void );
	void Precache( void );
	BOOL MyTouch( CBasePlayer *pPlayer );
};

void CItemNVG::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_9mmclip.mdl" );
	CItem::Spawn();
}

void CItemNVG::Precache( void )
{
	PRECACHE_MODEL( "models/w_9mmclip.mdl" );
	PRECACHE_SOUND( "items/gunpickup2.wav" );
}

BOOL CItemNVG::MyTouch( CBasePlayer *pPlayer )
{
	if( pPlayer->m_iHasNVG )	// ref +0xe1c
		return FALSE;
	pPlayer->m_iHasNVG = 1;
	if( gmsgFlashlight )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgFlashlight, NULL, pPlayer->edict() );
			WRITE_BYTE( 1 );
			WRITE_COORD( 1.0f );
		MESSAGE_END();
	}
	return TRUE;
}

LINK_ENTITY_TO_CLASS( item_NVG, CItemNVG );

// ---------------------------------------------------------------------------
// special_teleport_in — ref CSpecialTeleport (Spawn @0x1955a6, TeleportTouch).
// ---------------------------------------------------------------------------
class CSpecialTeleport : public CBaseToggle
{
public:
	void Spawn( void );
	void KeyValue( KeyValueData *pkvd );
	void EXPORT TeleportTouch( CBaseEntity *pOther );
	void EXPORT TeleportThink( void );
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	void Reset( void );
	float m_flDelay;
};

void CSpecialTeleport::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "delay" ) )
	{
		m_flDelay = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CSpecialTeleport::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	SetTouch( &CSpecialTeleport::TeleportTouch );
	SetThink( &CSpecialTeleport::TeleportThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	if( m_flDelay <= 0 )
		m_flDelay = 0.1f;
}

void CSpecialTeleport::TeleportTouch( CBaseEntity *pOther )
{
	if( !pOther || !pOther->IsPlayer() || !pOther->IsAlive() )
		return;
	edict_t *pent = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );
	if( FNullEnt( pent ) )
		return;
	Vector dest = VARS( pent )->origin;
	dest.z -= pOther->pev->mins.z;
	UTIL_SetOrigin( pOther->pev, dest );
	pOther->pev->angles = pent->v.angles;
	pOther->pev->fixangle = TRUE;
	pOther->pev->velocity = g_vecZero;
}

void CSpecialTeleport::TeleportThink( void )
{
	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CSpecialTeleport::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	Reset();
	SUB_UseTargets( pActivator, useType, value );
}

void CSpecialTeleport::Reset( void )
{
	pev->solid = SOLID_TRIGGER;
}

LINK_ENTITY_TO_CLASS( special_teleport_in, CSpecialTeleport );

// ---------------------------------------------------------------------------
// zat_proj — ref CZatProj @0x1b454c (MOVETYPE_FLY, touch damage).
// ---------------------------------------------------------------------------
class CZat;
class CZatProj : public CBaseEntity
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT ProjTouch( CBaseEntity *pOther );
	void EXPORT Think( void );
	static CZatProj *CreateZatProj( const Vector &vecOrigin, const Vector &vecAngles,
		CBaseEntity *pOwner, CZat *pWeapon );

	float m_flDamage;
};

void CZatProj::Precache( void )
{
	PRECACHE_MODEL( "sprites/zatwave.spr" );
	PRECACHE_SOUND( "weapons/zatimpact.wav" );
}

void CZatProj::Spawn( void )
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	SET_MODEL( ENT( pev ), "sprites/zatwave.spr" );
	SetTouch( &CZatProj::ProjTouch );
	SetThink( &CZatProj::Think );
	pev->nextthink = gpGlobals->time + 5.0f;
}

CZatProj *CZatProj::CreateZatProj( const Vector &vecOrigin, const Vector &vecAngles,
	CBaseEntity *pOwner, CZat *pWeapon )
{
	CZatProj *p = GetClassPtr( (CZatProj *)NULL );
	p->pev->origin = vecOrigin;
	p->pev->angles = vecAngles;
	p->pev->owner = pOwner ? pOwner->edict() : NULL;
	p->m_flDamage = 25.0f;
	p->Spawn();
	UTIL_MakeVectors( vecAngles );
	p->pev->velocity = gpGlobals->v_forward * 1500.0f;
	(void)pWeapon;
	return p;
}

void CZatProj::ProjTouch( CBaseEntity *pOther )
{
	if( !pOther || pOther->edict() == pev->owner )
		return;
	if( pOther->pev->takedamage )
	{
		entvars_t *pAttacker = pev;
		if( pev->owner )
		{
			CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
			if( pOwner )
				pAttacker = pOwner->pev;
		}
		if( pOther->IsPlayer() )
		{
			CBasePlayer *pHit = (CBasePlayer *)pOther;
			if( pHit->IsAlive() )
				pHit->Zat( pAttacker );
		}
		else
			pOther->TakeDamage( pev, pAttacker, m_flDamage, DMG_SHOCK );
	}
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "weapons/zatimpact.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM );
	UTIL_Remove( this );
}

void CZatProj::Think( void )
{
	UTIL_Remove( this );
}

LINK_ENTITY_TO_CLASS( zat_proj, CZatProj );

// ---------------------------------------------------------------------------
// entity_spawn_round — ref CEntSpawn (Spawn @0x195bea, Event @0x195ba6).
// Round-scoped spawn marker: CStargateTeamDM::NewRound() (ref @0x184f42) walks
// every "entity_spawn_round" entity and calls Event() on it, which re-creates
// the classname stored in the marker's "message" key at the marker's own
// origin/angles. Spawn() caches STRING(pev->message) once so Event() can fire
// repeatedly across rounds.
// ---------------------------------------------------------------------------
void CEntSpawn::Spawn( void )
{
	// ref @0x195bea — member +0x19c = gpGlobals->pStringBase + pev->message
	// (STRING(pev->message) pre-resolved), and pev+0x104 nextthink =
	// gpGlobals->time + 0.1. No networkable state, no model.
	m_pSpawnClass = STRING( pev->message );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CEntSpawn::Event( void )
{
	// ref @0x195ba6 — CBaseEntity::Create( cached +0x19c, pev->origin (+0x8),
	// pev->angles (+0x50), NULL ). The marker itself stays in the world.
	CBaseEntity::Create( m_pSpawnClass, pev->origin, pev->angles, NULL );
}

LINK_ENTITY_TO_CLASS( entity_spawn_round, CEntSpawn );

// ---------------------------------------------------------------------------
// temp_eject — ref CEject (Spawn @0x195c9e, Think @0x195c48, factory
// @0x195ebe). Invisible short-lived carrier entity created by the firing path
// (the FireBulletsPlayer creation site fills +0x19c..+0x1bc on the returned
// pointer); Think() removes it once its lifetime expired and the firing path
// has armed it.
// ---------------------------------------------------------------------------
class CEject : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT Think( void );

	// fields written by the creation site (ref FireBulletsPlayer):
	int m_iEjectFlag;		// ref +0x19c — set to 1
	float m_flSpreadX;		// ref +0x1a4 — spread.x
	float m_flSpreadZ;		// ref +0x1a8 — spread.z
	float m_flSpreadY;		// ref +0x1ac — spread.y
	Vector m_vecEjectOrigin;	// ref +0x1b0 — Vector from the creation site
	char m_bEjectArmed;		// ref +0x1bc — 1 once armed; gates Think removal
};

void CEject::Spawn( void )
{
	// ref @0x195c9e, in write order:
	pev->movetype = MOVETYPE_FOLLOW;	// ref pev+0x108 = 10 (GoldSrc MOVETYPE_FOLLOW)
	pev->solid = SOLID_BBOX;		// ref pev+0x10c = 2
	SET_MODEL( ENT( pev ), "sprites/zatimpact.spr" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->effects |= EF_NODRAW;		// ref pev+0x118 |= 0x80
	pev->angles = g_vecZero;		// ref pev+0x50
	pev->rendermode = kRenderTransAdd;	// ref pev+0x148 = 5
	pev->renderamt = 0.0f;			// ref pev+0x14c = 0
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "temp_eject" );	// ref pev+0x0
	SetThink( &CEject::Think );		// ref this+0x134/0x138 = Think (vslot +0xc4)
	pev->nextthink = gpGlobals->time + 0.5f;	// ref pev+0x104
}

void CEject::Think( void )
{
	// ref @0x195c48 — remove once the scheduled time has passed AND the
	// creation site has armed the eject (+0x1bc).
	if( pev->nextthink < gpGlobals->time && m_bEjectArmed != 0 )
		UTIL_Remove( this );
}

LINK_ENTITY_TO_CLASS( temp_eject, CEject );

// ---------------------------------------------------------------------------
// effect_bigshield — ref CBigShield (Spawn @0x162672, Think @0x162548,
// Classify @0x16253e, factory @0x162836). Team-sized forcefield wall: own
// model, its own renderamt regeneration loop driven by pev->health
// (100 → renderamt 130, 80 → renderamt 225 + crash sound + reset to 100), and
// a 0.25s stepping gate. CStargateTeamDM::NewRound() removes every
// "effect_bigshield" entity each round (ref NewRound @0x184f42).
// ---------------------------------------------------------------------------
class CBigShield : public CBaseEntity
{
public:
	void Spawn( void );
	void EXPORT Think( void );
	int Classify( void ) { return CLASS_NONE; }	// ref @0x16253e returns 0

	float m_flExpire;		// ref +0x19c — lifetime gate (read only outside this class)
	float m_flNextArmorStep;	// ref +0x1a0 — 0.25s gate for the renderamt stepping
};

void CBigShield::Spawn( void )
{
	// ref @0x162672, in write order:
	SET_MODEL( ENT( pev ), "models/bigshield.mdl" );
	UTIL_SetSize( pev, Vector( 0, 0, 0 ), Vector( 0, 0, 0 ) );
	pev->solid = SOLID_NOT;			// ref pev+0x10c = 0
	pev->movetype = MOVETYPE_NONE;		// ref pev+0x108 = 0
	pev->health = 100.0f;			// ref pev+0x160 = 100.0
	pev->view_ofs = g_vecZero;		// ref pev+0x174 = (0,0,0)
	pev->renderamt = 130.0f;		// ref pev+0x14c = 130.0
	pev->rendermode = kRenderTransAdd;	// ref pev+0x148 = 5
	m_flExpire = gpGlobals->time + 1.0f;	// ref +0x19c, first store
	SetThink( &CBigShield::Think );		// ref this+0x134/0x138 = Think (vslot +0xc4)
	m_flExpire = gpGlobals->time + 1.0f;	// ref +0x19c, second store (as in ref)
	Think();				// ref: immediate virtual call via vslot +0xc4
}

void CBigShield::Think( void )
{
	// ref @0x162548 — renderamt stepping is gated on +0x1a0 (which Spawn never
	// initialises; the first pass that wins the gate arms it for 0.25s).
	if( m_flNextArmorStep < gpGlobals->time )
	{
		if( pev->health == 100.0f )		// ref pev+0x160 == 100.0
			pev->renderamt = 130.0f;	// ref pev+0x14c

		if( pev->health == 80.0f )		// ref pev+0x160 == 80.0 (damaged wall)
		{
			pev->renderamt = 225.0f;	// ref pev+0x14c
			pev->health = 100.0f;		// ref pev+0x160
			EMIT_SOUND( ENT( pev ), CHAN_VOICE, "forcefield/forcefield_crash.wav", 1.0f, ATTN_NORM );
		}
		m_flNextArmorStep = gpGlobals->time + 0.25f;
	}
	pev->nextthink = gpGlobals->time + 0.2f;	// ref pev+0x104
}

LINK_ENTITY_TO_CLASS( effect_bigshield, CBigShield );
