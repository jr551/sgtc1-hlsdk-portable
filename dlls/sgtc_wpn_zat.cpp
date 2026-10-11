// sgtc_wpn_zat.cpp — StargateTC CZat (split from sgtc_weapons.cpp; NEW file).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "sgtc_weapons.h"

// ===========================================================================
// CZat - "Zat" energy gun. Analogue: gauss.cpp (energy hitscan).
// Events: events/zata.sc (primary) / events/zatb.sc (repeat secondary).
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_zat, CZat );

void CZat::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_zat.mdl" );
	m_iId = SGTC_WPN_ZAT;
	m_iDefaultAmmo = 100;
	FallInit();
}

void CZat::Precache( void )
{
	PRECACHE_MODEL( "models/v_zat.mdl" );
	PRECACHE_MODEL( "models/w_zat.mdl" );
	PRECACHE_MODEL( "models/p_zat.mdl" );
	PRECACHE_MODEL( "sprites/zatimpact.spr" );
	PRECACHE_MODEL( "sprites/zatwave.spr" );

	PRECACHE_SOUND( "weapons/zatvide.wav" );
	PRECACHE_SOUND( "weapons/zatimpact.wav" );

	m_usFireA = PRECACHE_EVENT( 1, "events/zata.sc" );
	m_usFireB = PRECACHE_EVENT( 1, "events/zatb.sc" );
}

SGTC_GETITEM( CZat, SGTC_AMMO_ENERGIE2, 100, WEAPON_NOCLIP, 1, 2, SGTC_WPN_ZAT, 5, 15 )
SGTC_ADD_TO_PLAYER( CZat )

BOOL CZat::Deploy( void )
{
	return SGTC_DEPLOY( 1, "models/v_zat.mdl", "models/p_zat.mdl", SGTC_DRAW, "onehanded" );
}

void CZat::PrimaryAttack( void )
{
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming,
		VECTOR_CONE_2DEGREES, 8192, SGTC_BULLET_ZAT, 0, 12,
		m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFireA,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		0, 0, 0, 0 );

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.25 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CZat::SecondaryAttack( void )
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

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming,
		VECTOR_CONE_3DEGREES, 8192, SGTC_BULLET_ZAT, 0, 12,
		m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFireB,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		0, 0, 0, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}

SGTC_NO_RELOAD( CZat )
SGTC_WEAPON_IDLE( CZat )
