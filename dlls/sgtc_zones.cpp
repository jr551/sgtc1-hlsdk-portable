// sgtc_zones.cpp — SGTC brush zones (ref/stc_i386.so factories @0x19d1d4..0x19d24c).
// REA: CTriggerShelter::Spawn @0x199920, CTriggerBomb @0x19a36a/0x19a10c,
// CTriggerCapture @0x199a056, CTriggerGTravel @0x19a51a / GTravelTouch @0x19a40c.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "sgtc_rules.h"

#include "weapons.h"

// ---------------------------------------------------------------------------
// zone_shelter — nuke-proof volume (FGD). Ref only InitTrigger + clears fields.
// ---------------------------------------------------------------------------
class CTriggerShelter : public CBaseToggle
{
public:
	void Spawn( void );
};

void CTriggerShelter::Spawn( void )
{
	SGTC_InitTrigger( this );
}

LINK_ENTITY_TO_CLASS( zone_shelter, CTriggerShelter );

// ---------------------------------------------------------------------------
// zone_gatetravel — marks players for DHD/gate travel (ref GTravelTouch @0x19a40c).
// ---------------------------------------------------------------------------
class CTriggerGTravel : public CBaseToggle
{
public:
	void Spawn( void );
	void EXPORT GateTravelTouch( CBaseEntity *pOther );
};

void CTriggerGTravel::Spawn( void )
{
	SGTC_InitTrigger( this );
	SetTouch( &CTriggerGTravel::GateTravelTouch );
}

void CTriggerGTravel::GateTravelTouch( CBaseEntity *pOther )
{
	if( !pOther || !pOther->IsPlayer() || !pOther->IsAlive() )
		return;

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;
	pPlayer->pev->fuser3 = gpGlobals->time + 1.0f;
	pPlayer->pev->iuser1 = 1;
	pPlayer->pev->iuser2 = (int)pev->team;
}

LINK_ENTITY_TO_CLASS( zone_gatetravel, CTriggerGTravel );

// ---------------------------------------------------------------------------
// zone_capture — CP capture brush (ref CTriggerCapture::* @0x199a056).
// ---------------------------------------------------------------------------
class CTriggerCapture : public CBaseToggle
{
public:
	void Spawn( void );
	void Precache( void );
	void EXPORT CaptureTouch( CBaseEntity *pOther );
	void EXPORT CaptureThink( void );
	EHANDLE m_hCapturer;
	int m_iCapturing;
	BOOL m_fActive;
};

void CTriggerCapture::Precache( void )
{
	PRECACHE_SOUND( "misc/talk.wav" );
}

void CTriggerCapture::Spawn( void )
{
	Precache();
	SGTC_InitTrigger( this );
	SetTouch( &CTriggerCapture::CaptureTouch );
	SetThink( &CTriggerCapture::CaptureThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	m_hCapturer = NULL;
	m_iCapturing = 0;
	m_fActive = FALSE;
}

void CTriggerCapture::CaptureTouch( CBaseEntity *pOther )
{
	if( !pOther || !pOther->IsPlayer() || !pOther->IsAlive() )
		return;
	if( !g_pGameRules || !g_pGameRules->IsTeamplay() )
		return;

	CBasePlayer *pl = (CBasePlayer *)pOther;
	if( !m_hCapturer )
		m_hCapturer = pl;
	m_fActive = TRUE;
}

void CTriggerCapture::CaptureThink( void )
{
	if( m_hCapturer )
	{
		CBaseEntity *pCap = m_hCapturer;
		if( pCap && pCap->IsPlayer() )
		{
			if( m_fActive && g_pGameRules && g_pGameRules->IsTeamplay() )
			{
				CStargateTeamDM *sg = (CStargateTeamDM *)g_pGameRules;
				int idx = g_pGameRules->GetTeamIndex( ((CBasePlayer *)pCap)->m_szTeamName );
				if( idx >= 0 )
					sg->Goal( idx );
			}
		}
		m_hCapturer = NULL;
	}
	m_fActive = FALSE;
	pev->nextthink = gpGlobals->time + 0.1f;
}

LINK_ENTITY_TO_CLASS( zone_capture, CTriggerCapture );

// ---------------------------------------------------------------------------
// zone_bomb_target — bomb plant zone (ref CTriggerBomb::* @0x19a36a).
// ---------------------------------------------------------------------------
class CTriggerBomb : public CBaseToggle
{
public:
	void Spawn( void );
	void EXPORT BombZoneTouch( CBaseEntity *pOther );
	void EXPORT BombThink( void );
	float m_flBombTime;
	BOOL m_fArmed;
	BOOL m_fPlanted;
};

void CTriggerBomb::Spawn( void )
{
	SGTC_InitTrigger( this );
	SetTouch( &CTriggerBomb::BombZoneTouch );
	SetThink( &CTriggerBomb::BombThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	m_flBombTime = 0;
	m_fArmed = FALSE;
	m_fPlanted = FALSE;
}

void CTriggerBomb::BombZoneTouch( CBaseEntity *pOther )
{
	if( !pOther || !pOther->IsPlayer() || !pOther->IsAlive() )
		return;
	if( m_fPlanted )
		return;
	m_fArmed = TRUE;
	m_fPlanted = TRUE;
	m_flBombTime = gpGlobals->time + 30.0f;
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/c4_beep.wav", 1.0f, ATTN_NORM );
	SUB_UseTargets( pOther, USE_ON, 0 );
}

void CTriggerBomb::BombThink( void )
{
	if( m_fPlanted && m_flBombTime > 0 && gpGlobals->time >= m_flBombTime )
	{
		RadiusDamage( pev->origin, pev, pev, 400.0f, 512.0f, CLASS_NONE, DMG_BLAST );
		SUB_UseTargets( this, USE_ON, 0 );
		m_fPlanted = FALSE;
		m_fArmed = FALSE;
	}
	pev->nextthink = gpGlobals->time + 0.1f;
}

LINK_ENTITY_TO_CLASS( zone_bomb_target, CTriggerBomb );
