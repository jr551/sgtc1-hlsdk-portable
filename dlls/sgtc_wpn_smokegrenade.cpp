// sgtc_wpn_smokegrenade.cpp — StargateTC CSmokeGrenade (split from sgtc_weapons.cpp; NEW file).

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
// CSmokeGrenade - smoke grenade. Analogue: handgrenade.cpp. No client event.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_smokegrenade, CSmokeGrenade );

void CSmokeGrenade::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_smokegrenade.mdl" );
	m_iId = SGTC_WPN_SMOKEGRENADE;
	m_iClip = -1;
	m_iDefaultAmmo = 1;
	FallInit();
}

void CSmokeGrenade::Precache( void )
{
	PRECACHE_MODEL( "models/w_smokegrenade.mdl" );
	PRECACHE_MODEL( "models/v_smokegrenade.mdl" );
	PRECACHE_MODEL( "models/p_smokegrenade.mdl" );
	PRECACHE_SOUND( "weapons/gasflow.wav" );
	PRECACHE_SOUND( "common/null.wav" );
}

SGTC_GETITEM( CSmokeGrenade, SGTC_AMMO_SMOKE, 1, WEAPON_NOCLIP, 6, 2, SGTC_WPN_SMOKEGRENADE, 5, 5 )
SGTC_ADD_TO_PLAYER( CSmokeGrenade )

BOOL CSmokeGrenade::Deploy( void )
{
	return DefaultDeploy( "models/v_smokegrenade.mdl", "models/p_smokegrenade.mdl", SGTC_DRAW, "crowbar" );
}

void CSmokeGrenade::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;
	Vector vecThrow = gpGlobals->v_forward * 400 + m_pPlayer->pev->velocity;

#ifndef CLIENT_DLL
	CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, 2.0 );
#endif

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/gasflow.wav", 1.0, ATTN_NORM );

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

	if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
}

SGTC_NO_SECONDARY( CSmokeGrenade )
SGTC_NO_RELOAD( CSmokeGrenade )
SGTC_WEAPON_IDLE( CSmokeGrenade )
