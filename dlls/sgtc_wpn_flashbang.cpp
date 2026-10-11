// sgtc_wpn_flashbang.cpp — StargateTC CFlashBang (split from sgtc_weapons.cpp; NEW file).

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
// CFlashBang - flash grenade. Analogue: handgrenade.cpp (timed throw).
// No client event (client table NULL).
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_flashbang, CFlashBang );

void CFlashBang::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_grenade.mdl" );
	m_iId = SGTC_WPN_FLASHBANG;
	m_iClip = -1;
	m_iDefaultAmmo = 4;
	FallInit();
}

void CFlashBang::Precache( void )
{
	PRECACHE_MODEL( "models/w_grenade.mdl" );
	PRECACHE_MODEL( "models/v_grenade.mdl" );
	PRECACHE_MODEL( "models/p_grenade.mdl" );
	PRECACHE_SOUND( "common/null.wav" );
}

SGTC_GETITEM( CFlashBang, SGTC_AMMO_FLASH, 10, WEAPON_NOCLIP, 5, 5, SGTC_WPN_FLASHBANG, 24, 10 )
SGTC_ADD_TO_PLAYER( CFlashBang )

BOOL CFlashBang::Deploy( void )
{
	return DefaultDeploy( "models/v_grenade.mdl", "models/p_grenade.mdl", SGTC_DRAW, "crowbar" );
}

void CFlashBang::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->pev->weapons &= ~( 1 << SGTC_WPN_FLASHBANG );
		DestroyItem();
	}

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM );
}

void CFlashBang::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc = m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16;
	Vector vecThrow = gpGlobals->v_forward * 400 + m_pPlayer->pev->velocity;

#ifndef CLIENT_DLL
	CGrenade::ShootTimed( m_pPlayer->pev, vecSrc, vecThrow, 2.0 );
#endif

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

	m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5;

	if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = GetNextAttackDelay( 0.5 );
}

SGTC_NO_SECONDARY( CFlashBang )
SGTC_NO_RELOAD( CFlashBang )
SGTC_WEAPON_IDLE( CFlashBang )

// Reference maps weapon_handgrenade to stock CHandGrenade (handgrenade.cpp);
// stock registration stands, so CSGTCHandGrenade is removed.
