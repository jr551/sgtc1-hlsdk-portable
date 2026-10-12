// sgtc_wpn_zatarc.cpp — StargateTC CZatarc (split from sgtc_weapons.cpp; NEW file).

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
// CZatarc - "Zat'arc" heavy energy gun. Analogue: gauss.cpp.
// Event: events/zatarc.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_zatarc, CZatarc );

void CZatarc::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_zar.mdl" );
	m_iId = SGTC_WPN_ZATARC;
	m_iDefaultAmmo = 40;
	FallInit();
}

void CZatarc::Precache( void )
{
	PRECACHE_MODEL( "models/v_zar.mdl" );
	PRECACHE_MODEL( "models/w_zar.mdl" );
	PRECACHE_MODEL( "models/p_zar.mdl" );
	PRECACHE_MODEL( "sprites/laserbeam.spr" );
	PRECACHE_MODEL( "sprites/Muz1.spr" );

	PRECACHE_SOUND( "weapons/zar.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/zatarc.sc" );
}

SGTC_GETITEM( CZatarc, SGTC_AMMO_ENERGIE5, 40, WEAPON_NOCLIP, 3, 0, SGTC_WPN_ZATARC, 0, 15 )
SGTC_ADD_TO_PLAYER( CZatarc )

BOOL CZatarc::Deploy( void )
{
	return SGTC_DEPLOY( 1, "models/v_zar.mdl", "models/p_zar.mdl", SGTC_DRAW, "egon" );
}

void CZatarc::PrimaryAttack( void )
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
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming,
		VECTOR_CONE_1DEGREES, 8192, SGTC_BULLET_ZATARC, 0, 25,
		m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		0, 0, 0, 0 );

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CZatarc::SecondaryAttack( void )	// ref CZatarc::SecondaryAttack @0x1c75d4 (zoom toggle)
{
	// ref gate: m_pPlayer->m_flNextAttack <= time.
	if( m_pPlayer->m_flNextAttack > gpGlobals->time )
		return;

	// ref cycles pev->fov 35 -> 0, 50 -> 35, else (0) -> 50, stamping the
	// zoom-mode slot (m_iFOV stage: 0 / 0x23=35 / 0x32=50) and the toggle flag.
	if( m_pPlayer->pev->fov == 35.0f )
	{
		m_pPlayer->m_iFOV = 0;
		m_pPlayer->pev->fov = 0.0f;
		m_iSgtcZoomStage = 0;
	}
	else if( m_pPlayer->pev->fov == 50.0f )
	{
		m_pPlayer->m_iFOV = 0x23;
		m_pPlayer->pev->fov = 35.0f;
		m_iSgtcZoomStage = 1;
	}
	else
	{
		m_pPlayer->m_iFOV = 0x32;
		m_pPlayer->pev->fov = 50.0f;
		m_iSgtcZoomStage = 1;
	}
	pev->nextthink = UTIL_WeaponTimeBase() + 0.1f;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
}

SGTC_NO_RELOAD( CZatarc )
SGTC_WEAPON_IDLE( CZatarc )
