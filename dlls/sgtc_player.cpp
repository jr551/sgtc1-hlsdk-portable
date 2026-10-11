// sgtc_player.cpp — StargateTC CBasePlayer extensions (ref/stc_i386.so).
// RadarScan @0x135592: MSG_ONE gmsgRadarBlip, BYTE type + 2× WRITE_COORD (×0.1).

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "sgtc_rules.h"

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

void SGTC_PlayerThinkRadar( CBasePlayer *pPlayer )
{
	if( !pPlayer )
		return;
	pPlayer->SGTC_RadarScan();
}
