// sgtc_wpn_lance.cpp — StargateTC CLanceJaffa (split from sgtc_weapons.cpp; NEW file).

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
// CLanceJaffa - "Lance Jaffa" staff weapon. Analogue: rpg.cpp (projectile).
// Events: events/lance.sc (bolt) / events/lance2.sc (melee bash).
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_lance, CLanceJaffa );

void CLanceJaffa::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_lancegoa.mdl" );
	m_iId = SGTC_WPN_LANCE;
	m_iDefaultAmmo = 22;
	FallInit();
}

void CLanceJaffa::Precache( void )
{
	PRECACHE_MODEL( "models/v_lancegoa.mdl" );
	PRECACHE_MODEL( "models/w_lancegoa.mdl" );
	PRECACHE_MODEL( "models/p_lancegoa.mdl" );
	PRECACHE_MODEL( "sprites/laserbeam.spr" );

	PRECACHE_SOUND( "weapons/tirlancegoa.wav" );
	PRECACHE_SOUND( "weapons/lancegoavide.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit1.wav" );
	PRECACHE_SOUND( "weapons/cbar_hit2.wav" );
	PRECACHE_SOUND( "weapons/hitbody1.wav" );
	PRECACHE_SOUND( "weapons/hitbody2.wav" );
	PRECACHE_SOUND( "weapons/hitbody3.wav" );
	PRECACHE_SOUND( "weapons/swing1.wav" );
	PRECACHE_SOUND( "weapons/swing2.wav" );
	PRECACHE_SOUND( "weapons/swing3.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/lance.sc" );
	m_usFire2 = PRECACHE_EVENT( 1, "events/lance2.sc" );
}

SGTC_GETITEM( CLanceJaffa, SGTC_AMMO_ENERGIE3, 22, WEAPON_NOCLIP, 3, 2, SGTC_WPN_LANCE, 5, 15 )
SGTC_ADD_TO_PLAYER( CLanceJaffa )

BOOL CLanceJaffa::Deploy( void )
{
	return SGTC_DEPLOY( 1, "models/v_lancegoa.mdl", "models/p_lancegoa.mdl", SGTC_DRAW, "egon" );
}

void CLanceJaffa::PrimaryAttack( void )
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
	// ref CLanceJaffa::PrimaryAttack @0x1005d6: CreateLanceProj( vecSrc,
	//   m_pPlayer->pev->velocity, tr.vecEndPos, m_pPlayer, this ).
	{
		TraceResult tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 8192.0f, dont_ignore_monsters,
			m_pPlayer->edict(), &tr );
		CLanceProj::CreateLanceProj( vecSrc, m_pPlayer->pev->velocity,
			tr.vecEndPos, m_pPlayer, this );
	}
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

void CLanceJaffa::SecondaryAttack( void )
{
	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire2,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );

#ifndef CLIENT_DLL
	if( tr.flFraction < 1.0 )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );
		if( pEntity )
		{
			ClearMultiDamage();
			pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgCrowbar * 2,
				gpGlobals->v_forward, &tr, DMG_CLUB );
			ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
		}
	}
#endif

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_flNextPrimaryAttack = GetNextAttackDelay( 0.75 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
}

SGTC_NO_RELOAD( CLanceJaffa )
SGTC_WEAPON_IDLE( CLanceJaffa )
