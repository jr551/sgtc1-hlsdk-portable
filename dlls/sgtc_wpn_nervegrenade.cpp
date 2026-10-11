// sgtc_wpn_nervegrenade.cpp — StargateTC CNerveGrenade (split from sgtc_weapons.cpp; NEW file).

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
// CNerveGrenade - nerve gas grenade. Analogue: handgrenade.cpp. No event.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_nervegrenade, CNerveGrenade );

void CNerveGrenade::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_nervegrenade.mdl" );
	m_iId = SGTC_WPN_NERVEGRENADE;
	m_iClip = -1;
	m_iDefaultAmmo = 1;
	FallInit();
}

void CNerveGrenade::Precache( void )
{
	PRECACHE_MODEL( "models/w_nervegrenade.mdl" );
	PRECACHE_MODEL( "models/v_nervegrenade.mdl" );
	PRECACHE_MODEL( "models/p_nervegrenade.mdl" );
	PRECACHE_SOUND( "weapons/gasflow.wav" );
	PRECACHE_SOUND( "common/null.wav" );
}

SGTC_GETITEM( CNerveGrenade, SGTC_AMMO_NERVE, 1, WEAPON_NOCLIP, 6, 3, SGTC_WPN_NERVEGRENADE, 5, 5 )
SGTC_ADD_TO_PLAYER( CNerveGrenade )

BOOL CNerveGrenade::Deploy( void )
{
	return SGTC_DEPLOY( 2, "models/v_nervegrenade.mdl", "models/p_nervegrenade.mdl", SGTC_DRAW, "crowbar" );
}

void CNerveGrenade::PrimaryAttack( void )
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

SGTC_NO_SECONDARY( CNerveGrenade )
SGTC_NO_RELOAD( CNerveGrenade )
SGTC_WEAPON_IDLE( CNerveGrenade )
