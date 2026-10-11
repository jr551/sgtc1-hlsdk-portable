// sgtc_wpn_glstaff.cpp — StargateTC CBigLanceJaffa (split from sgtc_weapons.cpp; NEW file).

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
// CBigLanceJaffa - "glstaff" / deathglider cannon. Analogue: rpg.cpp.
// Event: events/biglance.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_glstaff, CBigLanceJaffa );

void CBigLanceJaffa::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_deathglider.mdl" );
	m_iId = SGTC_WPN_BIGLANCE;
	m_iDefaultAmmo = 22;
	FallInit();
}

void CBigLanceJaffa::Precache( void )
{
	PRECACHE_MODEL( "models/v_deathglider.mdl" );
	PRECACHE_MODEL( "models/w_deathglider.mdl" );
	PRECACHE_MODEL( "models/p_deathglider.mdl" );
	PRECACHE_MODEL( "sprites/laserbeam.spr" );

	PRECACHE_SOUND( "weapons/tirlancegoa.wav" );
	PRECACHE_SOUND( "weapons/lancegoavide.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/biglance.sc" );
}

SGTC_GETITEM( CBigLanceJaffa, SGTC_AMMO_ENERGIEGL, 22, WEAPON_NOCLIP, 3, 5, SGTC_WPN_BIGLANCE, 5, 20 )
SGTC_ADD_TO_PLAYER( CBigLanceJaffa )

BOOL CBigLanceJaffa::Deploy( void )
{
	return DefaultDeploy( "models/v_deathglider.mdl", "models/p_deathglider.mdl", SGTC_DRAW, "egon" );
}

void CBigLanceJaffa::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
	{
		PlayEmptySound();
		return;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16;
	Vector vecDir = gpGlobals->v_forward;

#ifndef CLIENT_DLL
	// ref CBigLanceJaffa::PrimaryAttack: CreateBigLanceProj( vecSrc,
	//   m_pPlayer->pev->velocity, tr.vecEndPos, m_pPlayer, this ).
	{
		TraceResult tr;
		UTIL_TraceLine( vecSrc, vecSrc + vecDir * 8192.0f, dont_ignore_monsters,
			m_pPlayer->edict(), &tr );
		CBigLanceProj::CreateBigLanceProj( vecSrc, m_pPlayer->pev->velocity,
			tr.vecEndPos, m_pPlayer, this );
	}
#endif

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		0, 0, 0, 0 );

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.25 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

SGTC_NO_SECONDARY( CBigLanceJaffa )
SGTC_NO_RELOAD( CBigLanceJaffa )
SGTC_WEAPON_IDLE( CBigLanceJaffa )
