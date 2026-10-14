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

void CReetou::SecondaryAttack( void )	// ref CReetou::SecondaryAttack @0x162680
{
	// ref gate: m_pPlayer->m_flNextAttack <= gpGlobals->time (ItemPostFrame
	// CanAttack(m_flNextSecondaryAttack) already gates; this matches the ref).
	if( m_pPlayer->m_flNextAttack > gpGlobals->time )
		return;

	m_bSgtcCloakOn = !m_bSgtcCloakOn;	// ref this+0x204 toggle

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire2,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/invisdec.wav",
		1.0f, 0.8f );

	m_flSgtcCloakTimer = gpGlobals->time + 1.0f;	// ref this+0x1fc
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.0f;	// ref this+0x1d4
	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 3.0f;	// ref this+0x1d8
	// ref writes this+0x134 (m_pfnThink): cloak-off arms a guard stub at
	// 0x175, cloak-on arms SearchInvisible. Ported via a local thunk.
	SetThink( m_bSgtcCloakOn ? &CReetou::ThinkSearchInvisible : &CReetou::ThinkGuardStub );
	pev->nextthink = gpGlobals->time + 0.3f;
}

void CReetou::ThinkSearchInvisible( void )
{
	// ref SearchInvisible @0x175 (armed from SecondaryAttack):
	// schedule the snag on the owner, then re-arm the guard stub.
	m_pPlayer->m_flSgtcSnagTimer = gpGlobals->time + 0.3f;
	SetThink( &CReetou::ThinkGuardStub );
}

void CReetou::ThinkGuardStub( void )
{
	// ref stub @0x175: single-shot guard; clears the think.
	SetThink( NULL );
}

SGTC_NO_RELOAD( CReetou )
SGTC_WEAPON_IDLE( CReetou )
