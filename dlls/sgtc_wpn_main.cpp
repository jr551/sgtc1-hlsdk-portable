// sgtc_wpn_main.cpp — StargateTC CMainDieu (split from sgtc_weapons.cpp; NEW file).

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
// CMainDieu - "Main Dieu" hand device. Analogue: egon.cpp (energy device).
// Events: main.sc / mainc.sc / main2.sc / mains.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_main, CMainDieu );

void CMainDieu::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_main.mdl" );
	m_iId = SGTC_WPN_MAIN;
	m_iClip = -1;
	m_iDefaultAmmo = 100;
	FallInit();
}

void CMainDieu::Precache( void )
{
	PRECACHE_MODEL( "models/v_main.mdl" );
	PRECACHE_MODEL( "models/w_main.mdl" );
	PRECACHE_MODEL( "models/p_main.mdl" );
	PRECACHE_MODEL( "sprites/handtir.spr" );
	PRECACHE_MODEL( "sprites/shockwave.spr" );

	PRECACHE_SOUND( "weapons/tirmain.wav" );
	PRECACHE_SOUND( "weapons/tirmaincont.wav" );
	PRECACHE_SOUND( "weapons/mainvide.wav" );
	PRECACHE_SOUND( "weapons/empty.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/main.sc" );
	m_usFire2 = PRECACHE_EVENT( 1, "events/main2.sc" );
}

SGTC_GETITEM( CMainDieu, SGTC_AMMO_ENERGIE, 100, WEAPON_NOCLIP, 3, 1, SGTC_WPN_MAIN, 0, 25 )
SGTC_ADD_TO_PLAYER( CMainDieu )

BOOL CMainDieu::Deploy( void )
{
	return DefaultDeploy( "models/v_main.mdl", "models/p_main.mdl", SGTC_DRAW, "egon" );
}

void CMainDieu::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
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
		VECTOR_CONE_2DEGREES, 8192, SGTC_BULLET_ZAT, 0, 15,
		m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		0, 0, 0, 0 );

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.3 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CMainDieu::SecondaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 5;
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 0 )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;

	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 3, vecSrc, vecAiming,
		VECTOR_CONE_4DEGREES, 8192, SGTC_BULLET_ZAT, 0, 15,
		m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire2,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		0, 0, 0, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}

SGTC_NO_RELOAD( CMainDieu )
SGTC_WEAPON_IDLE( CMainDieu )
