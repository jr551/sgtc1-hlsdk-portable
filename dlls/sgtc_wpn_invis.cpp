// sgtc_wpn_invis.cpp — StargateTC CInvis (split from sgtc_weapons.cpp; NEW file).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "soundent.h"
#include "sgtc_weapons.h"

// File-local event handle (the class declares no m_usFire member).
static unsigned short s_usInvisFire;

// ===========================================================================
// CInvis - personal cloaking device. Analogue: gauss.cpp (charge device).
// Events: events/invis.sc (activate) / events/shield.sc (shield effect).
// ===========================================================================

LINK_ENTITY_TO_CLASS( weapon_invis, CInvis );

void CInvis::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_invis.mdl" );
	m_iId = SGTC_WPN_INVIS;
	m_iClip = -1;
	m_iDefaultAmmo = 10;
	m_bInvisActive = 0;
	m_flInvisLatchA = 0.0f;
	m_flInvisLatchB = 0.0f;
	FallInit();
}

void CInvis::Precache( void )
{
	PRECACHE_MODEL( "models/w_invis.mdl" );
	PRECACHE_MODEL( "models/v_invis.mdl" );
	PRECACHE_MODEL( "models/p_invis.mdl" );
	PRECACHE_SOUND( "weapons/invis.wav" );
	PRECACHE_SOUND( "weapons/empty.wav" );
	PRECACHE_SOUND( "forcefield/forcefield_fail.wav" );
	PRECACHE_SOUND( "forcefield/forcefield_loading.wav" );
	PRECACHE_SOUND( "forcefield/forcefield_unloading.wav" );

	s_usInvisFire = PRECACHE_EVENT( 1, "events/invis.sc" );
}

int CInvis::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = SGTC_AMMO_INVIS;
	p->iMaxAmmo1 = 10;
	p->pszAmmo2 = SGTC_AMMO_INVIS;
	p->iMaxAmmo2 = 10;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 3;
	p->iFlags = 5;
	p->iId = m_iId = SGTC_WPN_INVIS;
	p->iWeight = -10;
	return 1;
}

SGTC_ADD_TO_PLAYER( CInvis )

BOOL CInvis::Deploy( void )
{
	return SGTC_DEPLOY( 2, "models/v_invis.mdl", "models/p_invis.mdl", SGTC_DRAW, "python" );
}

// ref @0xfbb62 — toggle +0x208, ToggleVis @0x136454, ammo > 3.
void CInvis::PrimaryAttack( void )
{
	if( !m_pPlayer )
		return;
	if( m_pPlayer->m_bShieldWall )
		return;
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 3 )
	{
		PlayEmptySound();
		return;
	}

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/invis.wav", 1.0, ATTN_NORM );

	m_bInvisActive ^= 1;
	m_pPlayer->SGTC_ToggleVis();

	if( m_bInvisActive )
	{
		m_flInvisLatchA = 1.0f;	// ref rodata @0x1d2614 region (+0x134)
		m_flInvisLatchB = 0.0f;
		m_pPlayer->pev->animtime = gpGlobals->time;
		m_pPlayer->m_bSnagInvisFlag = 1;
		m_pPlayer->m_bSgtcCloakTick = 1;
		m_pPlayer->m_flCloakTickNext = 0.0f;
	}
	else
	{
		m_flInvisLatchA = 0.0f;
		m_flInvisLatchB = 0.0f;
		m_pPlayer->pev->effects &= ~EF_NODRAW;
		m_pPlayer->pev->rendermode = kRenderNormal;
		m_pPlayer->m_bSgtcCloakTick = 0;
	}

	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

	PLAYBACK_EVENT_FULL( SGTC_EVENT_FLAGS, m_pPlayer->edict(), s_usInvisFire,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0, 0, 0 );

	m_flNextPrimaryAttack = GetNextAttackDelay( 1.0 );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
}

void CInvis::SecondaryAttack( void )
{
	if( !m_pPlayer )
		return;
	if( m_bInvisActive )
		return;
	if( gpGlobals->time < m_pPlayer->m_flGateTravel && !m_pPlayer->IsOnLadder() )
		return;
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 3 )
		return;

	PrimaryAttack();
}

void CInvis::InvisThink( void )
{
	// Cloak decays over time while the device is held.
	if( m_pPlayer && m_pPlayer->pev->effects & EF_NODRAW )
	{
		m_pPlayer->pev->effects &= ~EF_NODRAW;
	}
}

SGTC_NO_RELOAD( CInvis )
SGTC_WEAPON_IDLE( CInvis )
