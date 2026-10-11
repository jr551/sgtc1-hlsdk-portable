// sgtc_wpn_goaregen.cpp — StargateTC CMedicxgoa (split from sgtc_weapons.cpp; NEW file).

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
// CMedicxgoa - Goa'uld regeneration device. Analogue: healthkit.cpp.
// No client event.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_goaregen, CMedicxgoa );

void CMedicxgoa::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_goaregen.mdl" );
	m_iId = SGTC_WPN_GOAREGEN;
	m_iClip = -1;
	m_iDefaultAmmo = 150;
	FallInit();
}

void CMedicxgoa::Precache( void )
{
	PRECACHE_MODEL( "models/v_goaregen.mdl" );
	PRECACHE_MODEL( "models/w_goaregen.mdl" );
	PRECACHE_MODEL( "models/p_goaregen.mdl" );
	PRECACHE_SOUND( "weapons/goaregen.wav" );
}

SGTC_GETITEM( CMedicxgoa, SGTC_AMMO_GOAREGEN, 150, WEAPON_NOCLIP, 3, 4, SGTC_WPN_GOAREGEN, 0, 0 )
SGTC_ADD_TO_PLAYER( CMedicxgoa )

BOOL CMedicxgoa::IsUseable( void )
{
	return ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 );
}

BOOL CMedicxgoa::Deploy( void )
{
	return SGTC_DEPLOY( 1, "models/v_goaregen.mdl", "models/p_goaregen.mdl", SGTC_DRAW, "egon" );
}

void CMedicxgoa::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	if( m_pPlayer->pev->health >= m_pPlayer->pev->max_health )
		return;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 15;
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 0 )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;

	m_pPlayer->TakeHealth( 25, DMG_GENERIC );
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/goaregen.wav", 1.0, ATTN_NORM );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}

SGTC_NO_SECONDARY( CMedicxgoa )
SGTC_NO_RELOAD( CMedicxgoa )
SGTC_WEAPON_IDLE( CMedicxgoa )
