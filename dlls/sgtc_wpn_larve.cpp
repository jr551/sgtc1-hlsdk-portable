// sgtc_wpn_larve.cpp — StargateTC CLarve (split from sgtc_weapons.cpp; NEW file).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "sgtc_weapons.h"

// File-local event handle (the class declares no m_usFire member).
static unsigned short s_usLarveFire;

// ===========================================================================
// CLarve - "Larve" bioweapon launcher. Analogue: squeakgrenade.cpp.
// Event: events/larve.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_larve, CLarve );

void CLarve::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_larve.mdl" );
	m_iId = SGTC_WPN_LARVE;
	m_iClip = -1;
	m_iDefaultAmmo = 2;
	FallInit();
}

void CLarve::Precache( void )
{
	PRECACHE_MODEL( "models/w_larve.mdl" );
	PRECACHE_MODEL( "models/v_larve.mdl" );
	PRECACHE_MODEL( "models/p_larve.mdl" );
	PRECACHE_MODEL( "models/w_squeak.mdl" );
	PRECACHE_SOUND( "weapons/larve1.wav" );
	PRECACHE_SOUND( "weapons/larve2.wav" );
	PRECACHE_SOUND( "common/null.wav" );

	UTIL_PrecacheOther( "monster_larve" );

	s_usLarveFire = PRECACHE_EVENT( 1, "events/larve.sc" );
}

SGTC_GETITEM( CLarve, SGTC_AMMO_LARVE, 2, WEAPON_NOCLIP, 5, 1, SGTC_WPN_LARVE, 5, 10 )
SGTC_ADD_TO_PLAYER( CLarve )

BOOL CLarve::Deploy( void )
{
	return DefaultDeploy( "models/v_larve.mdl", "models/p_larve.mdl", SGTC_DRAW, "squeak" );
}

void CLarve::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		return;
	}

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;
	Vector vecThrow = gpGlobals->v_forward * 400 + m_pPlayer->pev->velocity;

#ifndef CLIENT_DLL
	CBaseEntity *pLarve = Create( "monster_larve", vecSrc, m_pPlayer->pev->v_angle, m_pPlayer->edict() );
	if( pLarve )
		pLarve->pev->velocity = vecThrow;
#endif

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/larve1.wav", 1.0, ATTN_NORM );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), s_usLarveFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}

SGTC_NO_SECONDARY( CLarve )
SGTC_NO_RELOAD( CLarve )
SGTC_WEAPON_IDLE( CLarve )
