// sgtc_wpn_tacgun.cpp — StargateTC CTacgun (split from sgtc_weapons.cpp; NEW file).

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
static unsigned short s_usTacFire;

// ===========================================================================
// CTacgun - "Tacgun" deployable anti-invisibility sentry. Analogue:
// tripmine.cpp (deploy via Create). Event: events/tac.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_tacgun, CTacgun );

void CTacgun::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_tacb.mdl" );
	m_iId = SGTC_WPN_TAC;
	m_iClip = -1;
	m_iDefaultAmmo = 3;
	FallInit();
}

void CTacgun::Precache( void )
{
	PRECACHE_MODEL( "models/v_tac.mdl" );
	PRECACHE_MODEL( "models/p_tac.mdl" );
	PRECACHE_MODEL( "models/w_tacb.mdl" );
	PRECACHE_SOUND( "weapons/tac.wav" );
	PRECACHE_SOUND( "weapons/tacs.wav" );
	PRECACHE_SOUND( "weapons/tacp.wav" );
	PRECACHE_SOUND( "weapons/tacact.wav" );
	PRECACHE_SOUND( "weapons/place.wav" );
	PRECACHE_SOUND( "common/null.wav" );

	UTIL_PrecacheOther( "monster_tac" );

	s_usTacFire = PRECACHE_EVENT( 1, "events/tac.sc" );
}

SGTC_GETITEM( CTacgun, SGTC_AMMO_TAC, 3, WEAPON_NOCLIP, 4, 2, SGTC_WPN_TAC, 24, -10 )
SGTC_ADD_TO_PLAYER( CTacgun )

BOOL CTacgun::Deploy( void )
{
	return SGTC_DEPLOY( 2, "models/v_tac.mdl", "models/p_tac.mdl", SGTC_DRAW, "trip" );
}

void CTacgun::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->pev->weapons &= ~( 1 << SGTC_WPN_TAC );
		DestroyItem();
	}

	SendWeaponAnim( SGTC_HOLSTER );
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM );
}

void CTacgun::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = gpGlobals->v_forward;

	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 128, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), s_usTacFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0 );

#ifndef CLIENT_DLL
	if( tr.flFraction < 1.0 )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if( pEntity )
		{
			Vector angles = UTIL_VecToAngles( tr.vecPlaneNormal );
			CBaseEntity::Create( "monster_tac", tr.vecEndPos + tr.vecPlaneNormal * 8, angles, m_pPlayer->edict() );
			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/place.wav", 1.0, ATTN_NORM );

			if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
			{
				RetireWeapon();
				return;
			}
		}
	}
#endif

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

SGTC_NO_SECONDARY( CTacgun )
SGTC_NO_RELOAD( CTacgun )
SGTC_WEAPON_IDLE( CTacgun )
