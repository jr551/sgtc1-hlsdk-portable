// sgtc_wpn_medic.cpp — StargateTC CMedicx (split from sgtc_weapons.cpp; NEW file).

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
// CMedicx - human medical device. Analogue: healthkit.cpp (heal, no fire).
// No client event.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_medic, CMedicx );

void CMedicx::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_medic.mdl" );
	m_iId = SGTC_WPN_MEDIC;
	m_iClip = -1;
	m_iDefaultAmmo = 200;
	FallInit();
}

void CMedicx::Precache( void )
{
	PRECACHE_MODEL( "models/v_medic.mdl" );
	PRECACHE_MODEL( "models/w_medic.mdl" );
	PRECACHE_MODEL( "models/p_medic.mdl" );
	PRECACHE_SOUND( "weapons/medic.wav" );
}

SGTC_GETITEM( CMedicx, SGTC_AMMO_MEDGOO, 200, WEAPON_NOCLIP, 6, 0, SGTC_WPN_MEDIC, 0, 1 )
SGTC_ADD_TO_PLAYER( CMedicx )

BOOL CMedicx::IsUseable( void )
{
	return ( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0 );
}

BOOL CMedicx::Deploy( void )
{
	return DefaultDeploy( "models/v_medic.mdl", "models/p_medic.mdl", SGTC_DRAW, "onehanded" );
}

void CMedicx::PrimaryAttack( void )
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		return;

	if( m_pPlayer->pev->health >= m_pPlayer->pev->max_health )
		return;

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= 10;
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < 0 )
		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] = 0;

	m_pPlayer->TakeHealth( 10, DMG_GENERIC );
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/medic.wav", 1.0, ATTN_NORM );
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}

SGTC_NO_SECONDARY( CMedicx )
SGTC_NO_RELOAD( CMedicx )
SGTC_WEAPON_IDLE( CMedicx )
