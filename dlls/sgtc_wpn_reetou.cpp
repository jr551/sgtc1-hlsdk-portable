// sgtc_wpn_reetou.cpp — StargateTC CReetou (split from sgtc_weapons.cpp; NEW file).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "sgtc_weapons.h"
#include "sgtc_lanceproj.h"

// ===========================================================================
// CReetou - Reetou disruptor. Analogue: hornetgun.cpp (energy projectile).
// Events: events/reetou.sc (primary) / events/reetou2.sc (secondary).
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_reetou, CReetou );

void CReetou::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_reetou.mdl" );
	m_iId = SGTC_WPN_REETOU;
	m_iDefaultAmmo = 10;
	FallInit();
}

void CReetou::Precache( void )
{
	PRECACHE_MODEL( "models/v_reetou.mdl" );
	PRECACHE_MODEL( "models/w_reetou.mdl" );
	PRECACHE_MODEL( "models/p_reetou.mdl" );
	PRECACHE_MODEL( "models/w_projreetou.mdl" );

	PRECACHE_SOUND( "weapons/invis.wav" );
	PRECACHE_SOUND( "weapons/invisdec.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/reetou.sc" );
	m_usFire2 = PRECACHE_EVENT( 1, "events/reetou2.sc" );
}

SGTC_GETITEM( CReetou, SGTC_AMMO_TER, 10, WEAPON_NOCLIP, 4, 4, SGTC_WPN_REETOU, 5, -10 )
SGTC_ADD_TO_PLAYER( CReetou )

BOOL CReetou::Deploy( void )
{
	return SGTC_DEPLOY( 1, "models/v_reetou.mdl", "models/p_reetou.mdl", SGTC_DRAW, "gauss" );
}

void CReetou::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16;
	Vector vecDir = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	// ref CReetou::Attack: CreateReetouProj( vecSrc, v_angle, m_pPlayer, this ).
	CReetouProj::CreateReetouProj( vecSrc, m_pPlayer->pev->v_angle,
		m_pPlayer, (CReetou *)this );
#endif

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		0, 0, 0, 0 );

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CReetou::SecondaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire2,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	// Cloak the shooter briefly (Reetou stealth), matching the .sc semantics.
	m_pPlayer->pev->effects |= EF_NODRAW;

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.5 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.5;
}

SGTC_NO_RELOAD( CReetou )
SGTC_WEAPON_IDLE( CReetou )
