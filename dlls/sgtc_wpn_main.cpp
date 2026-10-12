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
	return SGTC_DEPLOY( 1, "models/v_main.mdl", "models/p_main.mdl", SGTC_DRAW, "egon" );
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

void CMainDieu::SecondaryAttack( void )	// ref CMainDieu::SecondaryAttack @0x11849c (ally AoE)
{
	// ref gate: m_pPlayer->m_flNextAttack <= time.
	if( m_pPlayer->m_flNextAttack > gpGlobals->time )
		return;

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 4 )
		return;

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire2,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecDir = gpGlobals->v_forward;
	m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecDir, Vector( 0.01, 0.01, 0.01 ),
		8192, SGTC_BULLET_ZAT, 100, 10, m_pPlayer->pev,
		m_pPlayer->random_seed );
	m_flSgtcBlastTimer = gpGlobals->time + 0.5f;	// ref this+0x1fc

	// ref TE_DLIGHT pulse (27, coord, 10, 255, 50, 50, 7, 0).
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSrc );
		WRITE_BYTE( TE_DLIGHT );
		WRITE_COORD( vecSrc.x );
		WRITE_COORD( vecSrc.y );
		WRITE_COORD( vecSrc.z );
		WRITE_BYTE( 10 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 50 );
		WRITE_BYTE( 50 );
		WRITE_BYTE( 7 );
		WRITE_BYTE( 0 );
	MESSAGE_END();
	m_pPlayer->SnagInvis();
	m_bSgtcBlastFlag = FALSE;	// ref clears player +0x4e9-adjacent byte

	// ref 100u ally sweep: same-team non-observer players gain
	// (dist-200) stamina (min 0) + UpdateStamina + vfunc(0x25); other
	// ents gain velocity + TE_DLIGHT + radius damage when hurt.
	CBaseEntity *pScan = NULL;
	while( ( pScan = UTIL_FindEntityInSphere( pScan, vecSrc, 100.0f ) ) != NULL )
	{
		if( pScan->IsPlayer() )
		{
			CBasePlayer *pAlly = (CBasePlayer *)pScan;
			if( pAlly != m_pPlayer && !pAlly->IsObserver() &&
				pAlly->pev->team == m_pPlayer->pev->team )
			{
				int dist = (int)( pAlly->pev->origin - m_pPlayer->pev->origin ).Length();
				pAlly->m_iStamina += dist - 200;
				if( pAlly->m_iStamina < 0 )
					pAlly->m_iStamina = 0;
				pAlly->SGTC_UpdateStamina();
				pAlly->m_fSgtcPushLatch = 0x25;	// ref vfunc +0x18c arg
			}
			continue;
		}
		if( !pScan->IsBSPModel() && pScan->IsAlive() )
		{
			int dist = (int)( pScan->pev->origin - m_pPlayer->pev->origin ).Length();
			pScan->pev->velocity = pScan->pev->velocity +
				gpGlobals->v_forward * (float)( ( 200 - dist ) * 10 );
			pScan->pev->velocity.z += (float)( ( 200 - dist ) * 3 );
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pScan->pev->origin );
				WRITE_BYTE( TE_DLIGHT );
				WRITE_COORD( pScan->pev->origin.x );
				WRITE_COORD( pScan->pev->origin.y );
				WRITE_COORD( pScan->pev->origin.z );
				WRITE_BYTE( 10 );
				WRITE_BYTE( 255 );
				WRITE_BYTE( 50 );
				WRITE_BYTE( 50 );
				WRITE_BYTE( 7 );
				WRITE_BYTE( 0 );
			MESSAGE_END();
		}
		if( pScan->pev->health > 0.0f )
			pScan->TakeDamage( pScan->pev, m_pPlayer->pev, 35.0f, DMG_ENERGYBEAM );	// ref 0x420c0000
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 5;
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.7f;	// ref this+0x1d8
	m_flNextSecondaryAttack = m_flNextPrimaryAttack;		// ref copies primary -> secondary
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;		// ref this+0x1dc
}

SGTC_NO_RELOAD( CMainDieu )
SGTC_WEAPON_IDLE( CMainDieu )
