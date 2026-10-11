// sgtc_wpn_mustardgrenade.cpp — StargateTC CMustardGrenade (split from sgtc_weapons.cpp; NEW file).

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
// CMustardGrenade - mustard gas grenade. Analogue: handgrenade.cpp. No event.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_mustardgrenade, CMustardGrenade );

void CMustardGrenade::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_mustardgrenade.mdl" );
	m_iId = SGTC_WPN_MUSTARDGRENADE;
	m_iClip = -1;
	m_iDefaultAmmo = 1;
	FallInit();
}

void CMustardGrenade::Precache( void )
{
	PRECACHE_MODEL( "models/w_mustardgrenade.mdl" );
	PRECACHE_MODEL( "models/v_mustardgrenade.mdl" );
	PRECACHE_MODEL( "models/p_mustardgrenade.mdl" );
	PRECACHE_SOUND( "weapons/gasflow.wav" );
	PRECACHE_SOUND( "common/null.wav" );
}

SGTC_GETITEM( CMustardGrenade, SGTC_AMMO_MUSTARD, 1, WEAPON_NOCLIP, 6, 5, SGTC_WPN_MUSTARDGRENADE, 5, 5 )
SGTC_ADD_TO_PLAYER( CMustardGrenade )

BOOL CMustardGrenade::Deploy( void )
{
	return SGTC_DEPLOY( 2, "models/v_mustardgrenade.mdl", "models/p_mustardgrenade.mdl", SGTC_DRAW, "crowbar" );
}

void CMustardGrenade::PrimaryAttack( void )
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

SGTC_NO_SECONDARY( CMustardGrenade )
SGTC_NO_RELOAD( CMustardGrenade )
SGTC_WEAPON_IDLE( CMustardGrenade )

// Reference maps weapon_c4 to stock CSatchel (satchel.cpp); stock
// registration stands, so CSGTCC4 is removed.

// Reference maps weapon_mine to the stock tripmine weapon class and
// monster_mine to stock CTripmineGrenade (tripmine.cpp); stock registrations
// stand, so the CSGTCMine weapon class is removed (also fixes the duplicate
// CSGTCMine class name vs sgtc_monsters.h's monster).
