// sgtc_player.cpp — StargateTC CBasePlayer extensions (ref/stc_i386.so).
// RadarScan @0x135592: MSG_ONE gmsgRadarBlip, BYTE type + 2× WRITE_COORD (×0.1).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "sgtc_rules.h"
#include "sgtc_shield.h"

static const struct
{
	const char *pszClass;
	int iBlipType;
} s_radarClasses[] =
{
	{ "player",            2 },
	{ "item_objective",    6 },
	{ "game_nuke",         6 },
	{ "info_marker",       5 },
	{ "monster_mine",      1 },
	{ "weapon_c4",         5 },
	{ "monster_tac",       3 },
	{ "larve",             1 },
	{ "struct_goa_actual", 6 },
	{ "struct_tau_actual", 6 },
	{ "grenade",           1 },
	{ "monster_satchel",   4 },
};

static void SGTC_SendRadarBlip( CBasePlayer *pPlayer, int iType, int iDx, int iDy )
{
	if( !gmsgRadarBlip )
		return;
	MESSAGE_BEGIN( MSG_ONE, gmsgRadarBlip, NULL, pPlayer->edict() );
	WRITE_BYTE( iType );
	WRITE_COORD( (float)iDx * 0.1f );
	WRITE_COORD( (float)iDy * 0.1f );
	MESSAGE_END();
}

static BOOL SGTC_RadarTeamVisible( CBasePlayer *pViewer, CBaseEntity *pEnt )
{
	BOOL entAlive = FALSE;
	if( pEnt->IsPlayer() )
		entAlive = ((CBasePlayer *)pEnt)->IsAlive();
	else
	{
		CBaseMonster *pMon = pEnt->MyMonsterPointer();
		entAlive = ( pMon && pMon->IsAlive() );
	}
	int entTeam = pEnt->pev->team;
	int viewTeam = pViewer->pev->team;

	if( entAlive )
	{
		if( entTeam != viewTeam )
			return FALSE;
	}
	if( entTeam > 0 && viewTeam > 0 && entTeam != viewTeam )
		return FALSE;
	return TRUE;
}

static BOOL SGTC_RadarPassesFilters( CBasePlayer *pPlayer, CBaseEntity *pEnt, int iDx, int iDy )
{
	const int kHalf = 90;		// ref Endo_roundtoi(9*5*2) @0x135604
	const float kMaxDist = 929.0f;	// cmp 0x3a1 @0x13596c

	if( pEnt == pPlayer )
		return FALSE;
	if( abs( iDx ) <= kHalf && abs( iDy ) <= kHalf )
		return FALSE;

	Vector vecDelta( (float)iDx, (float)iDy, pEnt->pev->origin.z - pPlayer->pev->origin.z );
	if( vecDelta.Length() > kMaxDist )
		return FALSE;

	if( !SGTC_RadarTeamVisible( pPlayer, pEnt ) )
		return FALSE;

	// ref pev+0x148 / +0x14c: skip when entity has a future hide-until time (> now).
	if( pEnt->pev->effects & EF_NODRAW )
		return FALSE;
	if( pEnt->pev->dmgtime > gpGlobals->time )
		return FALSE;

	return TRUE;
}

// ref CBasePlayer::RadarScan @0x135592
void CBasePlayer::SGTC_RadarScan( void )
{
	if( gpGlobals->time < m_flRadarNext )
		return;

	if( !IsAlive() || pev->deadflag != DEAD_NO || IsObserver() )
	{
		m_flRadarNext = gpGlobals->time + 0.25f;
		return;
	}

	if( pev->team <= 0 )
	{
		m_flRadarNext = gpGlobals->time + 0.25f;
		return;
	}

	const int nClasses = (int)( sizeof( s_radarClasses ) / sizeof( s_radarClasses[0] ) );
	for( int i = 0; i < nClasses; i++ )
	{
		const char *pszClass = s_radarClasses[i].pszClass;
		const int iType = s_radarClasses[i].iBlipType;

		CBaseEntity *pEnt = NULL;
		while( ( pEnt = UTIL_FindEntityByClassname( pEnt, pszClass ) ) != NULL )
		{
			const int iDx = (int)( pEnt->pev->origin.x - pev->origin.x );
			const int iDy = (int)( pEnt->pev->origin.y - pev->origin.y );

			if( !SGTC_RadarPassesFilters( this, pEnt, iDx, iDy ) )
				continue;

			SGTC_SendRadarBlip( this, iType, iDx, iDy );
		}
	}

	m_flRadarNext = gpGlobals->time + 0.25f;
}

// ref @0x133aca — toggle +0x925, MSG_ONE gmsgSubModel: 0,0,mode,iMaxAmmo1().
void CBasePlayer::SGTC_WeapDispSet( int mode )
{
	if( mode == 1 )
		m_bWeapSubModel = 1;
	else if( mode == 2 )
		m_bWeapSubModel = 0;

	if( !gmsgSubModel )
		return;

	int iMaxAmmo = 0;
	if( m_pActiveItem )
		iMaxAmmo = ((CBasePlayerWeapon *)m_pActiveItem)->iMaxAmmo1();

	MESSAGE_BEGIN( MSG_ONE, gmsgSubModel, NULL, pev );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 0 );
	WRITE_BYTE( mode );
	WRITE_BYTE( iMaxAmmo );
	MESSAGE_END();
}

// ref @0x1336e8 — MSG_ONE gmsgStamina when m_iStamina (+0x43c) != m_iStaminaSent (+0xa24).
void CBasePlayer::SGTC_UpdateStamina( void )
{
	if( m_iStamina < 0 )
		m_iStamina = 0;

	if( m_iStamina == m_iStaminaSent )
		return;

	if( !gmsgStamina )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgStamina, NULL, pev );
	WRITE_BYTE( m_iStamina );
	MESSAGE_END();
	m_iStaminaSent = m_iStamina;
}

// ref @0x1390b2 — shield charge, spawn effect_shield, follow owner.
void CBasePlayer::SGTC_ShieldThinker( void )
{
	if( m_flShieldLatch == 0.0f )
		m_flShieldLatch = gpGlobals->time - 1.0f;	// ref @0x1390e6

	if( gpGlobals->time >= m_flShieldThinkNext )
	{
		if( m_iShieldCharge > 0 && ( m_bShieldWall || m_bShieldSprint ) )
		{
			if( gpGlobals->frametime < 1.0f )
				m_iShieldCharge--;
		}
		else
		{
			m_bShieldSprint = 0;
			m_bShieldWall = 0;
		}

		if( m_iShieldCharge <= 9 && !m_bShieldWall && !m_bShieldSprint )
			m_iShieldCharge++;

		m_flShieldThinkNext = gpGlobals->time + 1.5f;	// ref rodata @0x1d74c4
	}

	if( !m_bShieldWall )
		return;

	if( !m_bShieldSpawned )
	{
		CBaseEntity *pShield = CBaseEntity::Create( "effect_shield", pev->origin, pev->v_angle, edict() );
		if( !pShield )
			return;

		pShield->pev->owner = edict();
		m_bShieldSpawned = 1;

		if( m_bShieldLarge )
		{
			pShield->pev->scale = 2.0f;
			pShield->pev->renderamt = 100.0f;
			pShield->pev->framerate = 1.0f;
		}
		return;
	}

	BOOL bFound = FALSE;
	CBaseEntity *pEnt = NULL;
	while( ( pEnt = UTIL_FindEntityByClassname( pEnt, "effect_shield" ) ) != NULL )
	{
		if( pEnt->pev->owner != edict() )
			continue;

		pEnt->pev->origin = pev->origin;
		pEnt->pev->origin.z -= 40.0f;	// ref @0x1d74dc
		if( m_flShieldLatch <= gpGlobals->time )
			pEnt->pev->renderamt = 130.0f;
		else
			pEnt->pev->renderamt = 100.0f;

		((CShield *)pEnt)->Think();
		bFound = TRUE;
	}

	m_bShieldSpawned = bFound;
}

// ref @0x13a02d..0x13a0da — drain 10 stamina (+0x43c) on movement tick (+0xa6c).
static void SGTC_TryStaminaDrainFromMovement( CBasePlayer *p )
{
	if( gpGlobals->time < p->m_flMoveLatch )
		return;

	if( p->pev->flags & FL_FROZEN )
		return;

	if( p->m_flMaxSpeedLatch == 0.0f )
		return;

	if( p->pev->fuser2 != p->m_flMaxSpeedLatch )
		return;

	const float move = p->pev->velocity.x + p->pev->velocity.y + p->pev->velocity.z;
	if( move <= 0.0f )
		return;

	p->m_iStamina -= 10;
	p->SGTC_UpdateStamina();
	p->m_flMoveLatch = gpGlobals->time + 0.5f;	// ref rodata @0x1d755c
}

// ref @0x139f18 — when +0x444/+0x938 differ from +0x45c/+0x934, MSG_ONE Progress.
void CBasePlayer::SGTC_SendProgressIfDirty( void )
{
	if( m_iProgressSentPri == m_iNukeCharge && m_iNukeBonusSent == m_iNukeBonus )
		return;

	if( !gmsgProgress )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgProgress, NULL, pev );
	WRITE_BYTE( m_iNukeCharge );
	WRITE_BYTE( m_iNukeBonus );
	for( int i = 0; i < 28; i++ )
		WRITE_BYTE( m_iProgressTeam[i] );
	WRITE_BYTE( 0 );
	MESSAGE_END();

	m_iProgressSentPri = m_iNukeCharge;
	m_iNukeBonusSent = m_iNukeBonus;
}

// ref @0x1398ef — one-shot camp warn: HasKey(m_iInBombZone, 1).
void CBasePlayer::SGTC_TryBombKeyCampWarn( void )
{
	if( m_bBombKeyWarnSent )
		return;
	if( m_iBombKeyTimer > 0 )
		return;

	const float flNow = gpGlobals->time;
	if( flNow == CVAR_GET_FLOAT( "tourny" ) )
		return;
	if( flNow == CVAR_GET_FLOAT( "allowmodevote" ) )
		return;
	if( g_iSgtcMapType == 1 )
		return;
	if( !gmsgHasKey )
		return;

	MESSAGE_BEGIN( MSG_ONE, gmsgHasKey, NULL, pev );
	WRITE_BYTE( m_iInBombZone );
	WRITE_BYTE( 1 );
	MESSAGE_END();
	m_bBombKeyWarnSent = 1;
}

// ref @0x13a438 — clamp key count, HasKey on enter/leave bomb zone state.
void CBasePlayer::SGTC_SyncBombZoneHasKey( void )
{
	if( m_iInBombZone > 1 )
		m_iInBombZone = 1;

	BOOL bSend = FALSE;
	if( m_iInBombZone > 0 )
	{
		if( !m_bBombZoneKeyLatch )
			bSend = TRUE;
	}
	else if( m_bBombZoneKeyLatch == 1 )
		bSend = TRUE;

	if( bSend && gmsgHasKey )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgHasKey, NULL, pev );
		WRITE_BYTE( m_iInBombZone );
		WRITE_BYTE( 0 );
		MESSAGE_END();
	}

	m_bBombZoneKeyLatch = ( m_iInBombZone > 0 ) ? 1 : 0;
}

// ref ClientCommand @0xcb456 — menuselect arg <=4 arms bomb-key warn timer.
void CBasePlayer::SGTC_MenuselectBombKey( int sel )
{
	if( sel > 4 )
		return;
	m_iBombKeyTimer = sel + 1;
	m_bBombKeyWarnSent = 0;
}

// ref @0x139814 — arm 0.2s window (rodata 0x1d7530), expire +0xa3e, copy to +0x4e8.
void CBasePlayer::SGTC_TickLanceStunLatch( void )
{
	static const float kLanceStunSec = 0.2f;

	if( !m_bLanceStunLatch && m_bLanceStunFlag )
		m_flLanceStunEnd = gpGlobals->time + kLanceStunSec;

	if( m_bLanceStunFlag && gpGlobals->time >= m_flLanceStunEnd )
		m_bLanceStunFlag = 0;

	m_bLanceStunLatch = m_bLanceStunFlag;
}

// ref CBasePlayer::SgtcThink @0x13963a (full body ~7 KB; tail @0x13a404).
void CBasePlayer::SGTC_SgtcThink( void )
{
	if( !IsAlive() )
		return;

	// ref @0x1397b3 — nuke bonus display counter floor.
	if( m_iNukeBonus <= 5 )
		m_iNukeBonus = 6;

	// ref @0x1397d4 — on ladder, drop personal shield deploy flag (+0x3e0).
	if( IsOnLadder() && m_bShieldWall )
		m_bShieldWall = 0;

	SGTC_TickLanceStunLatch();
	SGTC_TryBombKeyCampWarn();
	SGTC_SendProgressIfDirty();
	SGTC_TryStaminaDrainFromMovement( this );
	SGTC_ShieldThinker();
	SGTC_RadarScan();
	SGTC_SyncBombZoneHasKey();
}
