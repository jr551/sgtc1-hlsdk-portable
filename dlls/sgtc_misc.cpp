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
	if( pPlayer->pev->iuser4 )
		return FALSE;
	pPlayer->pev->iuser4 = 1;
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
