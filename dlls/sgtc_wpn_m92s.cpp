// sgtc_wpn_m92s.cpp — StargateTC CM92s (split from sgtc_weapons.cpp; NEW file).

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
// CM92s - "M92S" heavy pistol. Analogue: python.cpp (single-shot hitscan).
// Event: events/eagle.sc.
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_m92s, CM92s );

void CM92s::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_m92s.mdl" );
	m_iId = SGTC_WPN_M92S;
	m_iDefaultAmmo = 60;
	FallInit();
}

void CM92s::Precache( void )
{
	PRECACHE_MODEL( "models/v_m92s.mdl" );
	PRECACHE_MODEL( "models/w_m92s.mdl" );
	PRECACHE_MODEL( "models/p_m92s.mdl" );
	PRECACHE_MODEL( "models/pshell.mdl" );

	PRECACHE_SOUND( "items/9mmclip1.wav" );
	PRECACHE_SOUND( "items/9mmclip2.wav" );
	PRECACHE_SOUND( "weapons/pl_gun1.wav" );
	PRECACHE_SOUND( "weapons/pl_gun2.wav" );
	PRECACHE_SOUND( "weapons/pl_gun3.wav" );
	PRECACHE_SOUND( "weapons/pistol-01.wav" );
	PRECACHE_SOUND( "weapons/pistol-02.wav" );
	PRECACHE_SOUND( "weapons/pistol-03.wav" );

	m_usFire = PRECACHE_EVENT( 1, "events/eagle.sc" );
}

SGTC_GETITEM( CM92s, SGTC_AMMO_9MMC, 60, 12, 1, 3, SGTC_WPN_M92S, 0, 10 )
SGTC_ADD_TO_PLAYER( CM92s )

BOOL CM92s::Deploy( void )
{
	return DefaultDeploy( "models/v_m92s.mdl", "models/p_m92s.mdl", SGTC_DRAW, "onehanded" );
}

void CM92s::M92sFire( float flSpread, float flCycleTime )
{
	if( m_iClip <= 0 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;
	m_iClip--;
	m_pPlayer->pev->effects = (int)( m_pPlayer->pev->effects ) | EF_MUZZLEFLASH;
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );
	Vector vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming,
		Vector( flSpread, flSpread, flSpread ), 8192, SGTC_BULLET_M92S, 0, 15,
		m_pPlayer->pev, m_pPlayer->random_seed );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), m_usFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y,
		0, 0, ( m_iClip == 0 ) ? 1 : 0, 0 );

	if( !m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0 )
		m_pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 );

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay( flCycleTime );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() +
		UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
}

void CM92s::PrimaryAttack( void )
{
	if( m_pPlayer->pev->waterlevel == 3 )
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	M92sFire( 0.01, 0.5 );
}

SGTC_NO_SECONDARY( CM92s )
SGTC_WEAPON_RELOAD( CM92s, 12 )
SGTC_WEAPON_IDLE( CM92s )

// Reference (ref/stc_i386.so) maps weapon_mp5 and weapon_9mmAR to stock
// CMP5 (mp5.cpp) and weapon_handgrenade to stock CHandGrenade; stock
// registrations stand, so CSGTCMP5 is removed (was duplicate factory).
