// sgtc_wpn_usas.cpp — StargateTC CUsas (split from sgtc_weapons.cpp; NEW file).

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
// CUsas - "USAS" automatic shotgun. Analogue: shotgun.cpp.
// Event: events/usas.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_usas, CUsas );

void CUsas::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_usas.mdl" );
	m_iId = SGTC_WPN_USAS;
	m_iDefaultAmmo = 120;
	FallInit();
}

void CUsas::Precache( void )
{
	PRECACHE_MODEL( "models/v_usas.mdl" );
	PRECACHE_MODEL( "models/w_usas.mdl" );
	PRECACHE_MODEL( "models/p_usas.mdl" );
	PRECACHE_MODEL( "models/shell.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "weapons/dbarrel1.wav" );
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "weapons/reload1.wav" );
	PRECACHE_SOUND( "weapons/reload3.wav" );
	PRECACHE_SOUND( "weapons/spaspomp.wav" );
	PRECACHE_SOUND( "weapons/spastir.wav" );
	PRECACHE_SOUND( "weapons/spump.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/usas.sc" );
}

SGTC_GETITEM( CUsas, SGTC_AMMO_BUCKSHOT, 125, 10, 2, 5, SGTC_WPN_USAS, 0, 20 )
SGTC_ADD_TO_PLAYER( CUsas )

BOOL CUsas::Deploy( void )
{
	return SGTC_DEPLOY( 2, "models/v_usas.mdl", "models/p_usas.mdl", SGTC_DRAW, "mp5" );
}

void CUsas::PrimaryAttack( void )
{
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if( m_iClip <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_iClip--;
	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );

	m_pPlayer->FireBulletsPlayer( 4, vecSrc, vecAiming, VECTOR_CONE_6DEGREES,
		2048, SGTC_BULLET_USAS, 0, 5, m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.25 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

SGTC_NO_SECONDARY( CUsas )
SGTC_WEAPON_RELOAD( CUsas, 10 )
SGTC_WEAPON_IDLE( CUsas )
