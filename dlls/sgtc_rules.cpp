// sgtc_rules.cpp — StargateTC teams/rules/triggers/env (NEW file).
//
// WHAT THIS IS: clean-room port of the SGTC 2.0 server rules slice onto the
// hlsdk-xash3d base. Bodies reconstructed from ref/stc_i386.so disassembly
// (nm -C / objdump -d -C); exact verified facts are cited inline as
// fileoff/VMA notes. Anything layout-level the client drains opaquely (env
// payload field order) is marked BEST-EFFORT.
//
// STOCK HOOK POINTS (report only — NO stock file is edited by this change):
//   1. hlsdk-xash3d/dlls/gamerules.cpp:310 `InstallGameRules()` — add:
//         const char *mode = CVAR_GET_STRING( "mp_gamemode" );
//         CGameRules *sg = SGTC_InstallGameRules( mode );
//         if( sg ) return sg;
//     before the stock deathmatch/teamplay chain. Needs #include "sgtc_rules.h".
//   2. hlsdk-xash3d/dlls/player.cpp:191 `LinkUserMessages()` — add
//     `SGTC_LinkUserMessages();` at the end (registers the gmsg* below).
//   3. hlsdk-xash3d/dlls/player.cpp:2701 `EntSelectSpawnPoint()` — SGTC team
//     spawns are filtered in CStargateTeamDM/CP/Unas::GetPlayerSpawnSpot, so
//     no edit is strictly required; optional: fall through to
//     "info_player_torri"/"info_player_goauld" when no DM spot exists.
//   4. hlsdk-xash3d/dlls/triggers.cpp — no hook needed (SGTC trigger classes
//     derive CBaseToggle and live entirely here; stock CBaseTrigger/
//     CTriggerMultiple are TU-local to triggers.cpp with no header decl).
//   5. Build lists (see yield report for exact SRCS lines):
//        hlsdk-xash3d/dlls/CMakeLists.txt  ->  sgtc_rules.cpp
//        hlsdk-xash3d/dlls/Makefile        ->  sgtc_rules.o
//
// OUT OF SCOPE (owned by sibling slices): weapon_invis/CInvis (weapons),
// monster_implanted/skull, func_tankgoauld, gate/DHD, votesys/token.

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "game.h"
#include "saverestore.h"
#include "sgtc_rules.h"

// Brush-trigger setup shared by the SGTC trigger classes. Replicates
// CBaseTrigger::InitTrigger (triggers.cpp) — InitTrigger itself is TU-local
// to triggers.cpp and cannot be called from here.
static void SGTC_InitTrigger( CBaseToggle *pTrigger )
{
	entvars_t *pev = pTrigger->pev;
	if( pev->angles != g_vecZero )
		SetMovedir( pev );
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	if( CVAR_GET_FLOAT( "showtriggers" ) == 0 )
		SetBits( pev->effects, EF_NODRAW );
}

// ---------------------------------------------------------------------------
// User messages. Header-owned (env/radar) + file-global HUD/round messages the
// rules classes send. Names are the exact BSS gmsg* symbols of
// ref/stc_i386.so (verified with `nm -C --defined-only ref/stc_i386.so`);
// they are deliberately NOT static so hl.so exports them, exactly as the
// reference does. Wire names + sizes are decoded from the ref
// LinkUserMessages() REG_USER_MSG table @0x132d44 (push imm before each
// `call *%edx`): -1 = variable-length, 0..n = fixed byte size.
// ---------------------------------------------------------------------------
int gmsgRadarBlip = 0;
int gmsgEnvParticule = 0;
int gmsgEnvRain = 0;
int gmsgEnvFountain = 0;
int gmsgEnvNewLens = 0;
int gmsgEnvAddRing = 0;
int gmsgChangeParticuleState = 0;
int gmsgProgress = 0;
int gmsgGtickets = 0;
int gmsgTtickets = 0;
int gmsgTicketMode = 0;
int gmsgHudInfoMode = 0;
int gmsgConcuss = 0;
int gmsgHasKey = 0;
int gmsgStamina = 0;
int gmsgSubModel = 0;
int gmsgArmor = 0;
int gmsgSpectator = 0;
int gmsgAddPortrait = 0;
int gmsgAboutMe = 0;
int gmsgTogglePortrait = 0;
int gmsgReset = 0;
int gmsgNewRnd = 0;
int gmsgSetHudRoundTime = 0;
int gmsgVGUIMenu = 0;
int gmsgHideVGUI = 0;
int gmsgZatTouch = 0;
int gmsgRadio = 0;

void SGTC_LinkUserMessages( void )
{
	if( gmsgRadarBlip )
		return; // already registered
	gmsgRadarBlip = REG_USER_MSG( "Blip", -1 );
	gmsgProgress = REG_USER_MSG( "Progress", -1 );
	gmsgGtickets = REG_USER_MSG( "Gtickets", 1 );
	gmsgTtickets = REG_USER_MSG( "Ttickets", 1 );
	gmsgTicketMode = REG_USER_MSG( "TicketMode", 1 );
	gmsgHudInfoMode = REG_USER_MSG( "HudInfoMode", 1 );
	gmsgConcuss = REG_USER_MSG( "Concuss", 2 );
	gmsgHasKey = REG_USER_MSG( "HasKey", 2 );
	gmsgStamina = REG_USER_MSG( "Stamina", 1 );
	gmsgSubModel = REG_USER_MSG( "SubModel", 1 );
	gmsgArmor = REG_USER_MSG( "Armor", 4 );
	gmsgSpectator = REG_USER_MSG( "Spectator", 2 );
	gmsgAddPortrait = REG_USER_MSG( "AddPortrait", -1 );
	gmsgAboutMe = REG_USER_MSG( "AboutMe", 5 );
	gmsgTogglePortrait = REG_USER_MSG( "TogglePor", 0 );
	gmsgReset = REG_USER_MSG( "Reset", -1 );
	gmsgNewRnd = REG_USER_MSG( "NewRnd", 0 );
	gmsgSetHudRoundTime = REG_USER_MSG( "SetRndTim", 3 );
	gmsgVGUIMenu = REG_USER_MSG( "VGUIMenu", 1 );
	gmsgHideVGUI = REG_USER_MSG( "HideVGUI", 0 );
	gmsgZatTouch = REG_USER_MSG( "ZatTouch", 0 );
	gmsgRadio = REG_USER_MSG( "Radio", 2 );
	gmsgEnvParticule = REG_USER_MSG( "EnvFull", 47 );
	gmsgEnvRain = REG_USER_MSG( "EnvRain", 15 );
	gmsgEnvFountain = REG_USER_MSG( "EnvFount", 45 );
	gmsgEnvNewLens = REG_USER_MSG( "NewLens", 10 );
	gmsgEnvAddRing = REG_USER_MSG( "AddRing", -1 );
	gmsgChangeParticuleState = REG_USER_MSG( "ChgPartSt", 2 );
}

// ---------------------------------------------------------------------------
// Mode installer. mp_gamemode prefix chain from InstallGameRules disasm
// (rodata 0x1d1ab0: "sg_" "sgi_" "ctp_" "un_" "inv_" "rep_").
// ---------------------------------------------------------------------------
CGameRules *SGTC_InstallGameRules( const char *pszMode )
{
	if( !pszMode || !*pszMode )
		return NULL;
	if( !strncmp( pszMode, "sgi_", 4 ) || !strncmp( pszMode, "ctp_", 4 ) )
		return new CStargateTeamCP;
	if( !strncmp( pszMode, "sg_", 3 ) )
		return new CStargateTeamDM;
	if( !strncmp( pszMode, "un_", 3 ) )
		return new CStargateUnas;
	if( !strncmp( pszMode, "inv_", 4 ) )
		return new CStargateInvis;
	if( !strncmp( pszMode, "rep_", 4 ) )
		return NULL; // CStargateReplicateur: sibling slice owns it
	return NULL;
}

extern cvar_t timeleft, fragsleft;
extern int gmsgGameMode;
extern int gmsgSayText;
extern int gmsgTeamInfo;
extern int gmsgTeamNames;
extern int gmsgScoreInfo;
extern int gmsgDeathMsg;

// Team spawn classname tables (rodata 0x1d7997 block ordering).
static const char *const s_SGTCDMSpawns[SGTC_MAX_SGTC_TEAMS] =
{
	"info_player_torri",
	"info_player_goauld",
};

// Portrait/model names from InitHUD disasm (oneill..unas, 11 names).
static const char *const s_SGTCPortraits[] =
{
	"oneill", "carter", "jackson", "tealc", "commander",
	"systemlord", "anubis", "goauld3", "snakegoauld", "horus", "unas",
};
#define SGTC_NUM_PORTRAITS ( sizeof( s_SGTCPortraits ) / sizeof( s_SGTCPortraits[0] ) )

// ===========================================================================
// CStargateTeamDM
// ===========================================================================
CStargateTeamDM::CStargateTeamDM( void )
{
	// .so ctor: strncpy(teamlist, "Torri;Goa'uld"), overridden by the
	// mp_teamlist cvar when set (mirrors CHalfLifeTeamplay::CHalfLifeTeamplay).
	strncpy( m_szSGTCTeams[0], "Torri", SGTC_TEAMNAME_LEN );
	strncpy( m_szSGTCTeams[1], "Goa'uld", SGTC_TEAMNAME_LEN );
	m_iSGTCTeamCount = SGTC_MAX_SGTC_TEAMS;
	m_iSGTCTeamScores[0] = m_iSGTCTeamScores[1] = 0;
	m_bRoundOver = FALSE;

	const char *list = CVAR_GET_STRING( "mp_teamlist" );
	if( list && *list )
	{
		char buf[64];
		strncpy( buf, list, sizeof( buf ) - 1 );
		buf[sizeof( buf ) - 1] = '\0';
		char *semi = strchr( buf, ';' );
		if( semi )
		{
			*semi = '\0';
			strncpy( m_szSGTCTeams[0], buf, SGTC_TEAMNAME_LEN );
			strncpy( m_szSGTCTeams[1], semi + 1, SGTC_TEAMNAME_LEN );
		}
		else
		{
			strncpy( m_szSGTCTeams[0], buf, SGTC_TEAMNAME_LEN );
		}
	}
	RecountTeams( FALSE );
}

int CStargateTeamDM::GetTeamIndex( const char *pTeamName )
{
	// Exact shape of the .so GetTeamIndex: strcasecmp loop, -1 on miss.
	if( pTeamName && *pTeamName )
	{
		for( int tm = 0; tm < m_iSGTCTeamCount; tm++ )
		{
			if( !stricmp( m_szSGTCTeams[tm], pTeamName ) )
				return tm;
		}
	}
	return -1;
}

const char *CStargateTeamDM::GetIndexedTeamName( int teamIndex )
{
	if( teamIndex < 0 || teamIndex >= m_iSGTCTeamCount )
		return "";
	return m_szSGTCTeams[teamIndex];
}

BOOL CStargateTeamDM::IsValidTeam( const char *pTeamName )
{
	return ( GetTeamIndex( pTeamName ) != -1 ) ? TRUE : FALSE;
}

const char *CStargateTeamDM::GetTeamID( CBaseEntity *pEntity )
{
	if( !pEntity || !pEntity->pev )
		return "";
	return pEntity->TeamID();
}

// PlayerRelationship is inherited from CHalfLifeTeamplay (teamplay_gamerules.cpp
// :429): identical same-team stricmp logic; the SGTC header has no decl, so the
// stock virtual covers all call sites below.

BOOL CStargateTeamDM::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		if( friendlyfire.value == 0 && pAttacker != pPlayer )
			return FALSE;
	}
	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

BOOL CStargateTeamDM::ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target )
{
	CBaseEntity *pTgt = CBaseEntity::Instance( target );
	if( pTgt && pTgt->IsPlayer() && PlayerRelationship( pPlayer, pTgt ) == GR_TEAMMATE )
		return FALSE;
	return CHalfLifeMultiplay::ShouldAutoAim( pPlayer, target );
}

int CStargateTeamDM::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if( !pKilled )
		return 0;
	if( !pAttacker )
		return 1;
	if( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;
	return 1;
}

const char *CStargateTeamDM::TeamWithFewestPlayers( void )
{
	int teamCount[SGTC_MAX_SGTC_TEAMS] = { 0, 0 };
	int minPlayers = 9999;
	const char *pName = m_szSGTCTeams[0];
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );
		if( plr )
		{
			int team = GetTeamIndex( plr->TeamID() );
			if( team >= 0 )
				teamCount[team]++;
		}
	}
	for( int i = 0; i < m_iSGTCTeamCount; i++ )
	{
		if( teamCount[i] < minPlayers )
		{
			minPlayers = teamCount[i];
			pName = m_szSGTCTeams[i];
		}
	}
	return pName;
}

void CStargateTeamDM::RecountTeams( BOOL bResendInfo )
{
	// Fixed 2-team table: nothing to recount beyond optional rebroadcast.
	if( !bResendInfo )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamNames );
		WRITE_BYTE( m_iSGTCTeamCount );
		for( int i = 0; i < m_iSGTCTeamCount; i++ )
			WRITE_STRING( m_szSGTCTeams[i] );
	MESSAGE_END();
}

const char *CStargateTeamDM::SetDefaultPlayerTeam( CBasePlayer *pPlayer )
{
	char *mdls = g_engfuncs.pfnInfoKeyValue(
		g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" );
	strncpy( pPlayer->m_szTeamName, mdls, TEAM_NAME_LENGTH );

	if( pPlayer->m_szTeamName[0] == '\0' || !IsValidTeam( pPlayer->m_szTeamName ) )
		strncpy( pPlayer->m_szTeamName, TeamWithFewestPlayers(), TEAM_NAME_LENGTH );

	return pPlayer->m_szTeamName;
}

void CStargateTeamDM::ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib )
{
	// Mirrors CHalfLifeTeamplay::ChangePlayerTeam (kill, keyvalues, HUD fan-out).
	int damageFlags = DMG_GENERIC | ( bGib ? DMG_ALWAYSGIB : DMG_NEVERGIB );
	if( bKill )
	{
		entvars_t *pevWorld = VARS( INDEXENT( 0 ) );
		pPlayer->TakeDamage( pevWorld, pevWorld, 900, damageFlags );
	}
	if( pPlayer->m_szTeamName != pTeamName )
		strncpy( pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH );

	int clientIndex = pPlayer->entindex();
	g_engfuncs.pfnSetClientKeyValue( clientIndex,
		g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", pPlayer->m_szTeamName );
	g_engfuncs.pfnSetClientKeyValue( clientIndex,
		g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", pPlayer->m_szTeamName );

	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( clientIndex );
		WRITE_STRING( pPlayer->m_szTeamName );
	MESSAGE_END();

	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( clientIndex );
		WRITE_SHORT( (int)pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
		WRITE_SHORT( 0 );
		WRITE_SHORT( GetTeamIndex( pPlayer->m_szTeamName ) + 1 );
	MESSAGE_END();
}

void CStargateTeamDM::SGTC_SendTeamNames( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict() );
		WRITE_BYTE( m_iSGTCTeamCount );
		for( int i = 0; i < m_iSGTCTeamCount; i++ )
			WRITE_STRING( m_szSGTCTeams[i] );
	MESSAGE_END();
}

void CStargateTeamDM::InitHUD( CBasePlayer *pPlayer )
{
	SetDefaultPlayerTeam( pPlayer );
	CHalfLifeMultiplay::InitHUD( pPlayer );
	SGTC_SendTeamNames( pPlayer );
	RecountTeams( TRUE );
	StartPortrait( pPlayer );

	char text[128];
	sprintf( text, "* you are on team '%s'\n", pPlayer->m_szTeamName );
	UTIL_SayText( text, pPlayer );

	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );
		if( plr && IsValidTeam( plr->TeamID() ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->edict() );
				WRITE_BYTE( plr->entindex() );
				WRITE_STRING( plr->TeamID() );
			MESSAGE_END();
		}
	}
}

void CStargateTeamDM::UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 1 ); // teamplay
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_ONE, gmsgTicketMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 0 ); // DM: no tickets
	MESSAGE_END();
}

void CStargateTeamDM::SGTC_GiveTeamLoadout( CBasePlayer *pPlayer )
{
	// Team melee + zat from DM PlayerSpawn strings (couteau/coujaf/lance/zat).
	int team = GetTeamIndex( pPlayer->m_szTeamName );
	pPlayer->GiveNamedItem( "weapon_couteau" );
	if( team == SGTC_TEAM_GOAULD - 1 )
	{
		pPlayer->GiveNamedItem( "weapon_lance" );
		SET_MODEL( ENT( pPlayer->pev ), "models/player/goauld3/goauld3.mdl" );
	}
	else
	{
		pPlayer->GiveNamedItem( "weapon_coujaf" );
		SET_MODEL( ENT( pPlayer->pev ), "models/player/commander/commander.mdl" );
	}
	pPlayer->GiveNamedItem( "weapon_zat" );
}

edict_t *CStargateTeamDM::SGTC_FindTeamSpawn( const char *pszClassname )
{
	CBaseEntity *pSpot = NULL;
	for( int i = RANDOM_LONG( 1, 5 ); i > 0; i-- )
		pSpot = UTIL_FindEntityByClassname( pSpot, (char *)pszClassname );
	if( !pSpot )
		pSpot = UTIL_FindEntityByClassname( pSpot, (char *)pszClassname );
	CBaseEntity *pFirst = pSpot;
	while( pSpot )
	{
		if( pSpot->pev->origin != Vector( 0, 0, 0 ) )
			return pSpot->edict();
		pSpot = UTIL_FindEntityByClassname( pSpot, (char *)pszClassname );
		if( pSpot == pFirst )
			break;
	}
	return NULL;
}

edict_t *CStargateTeamDM::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	// Team-filtered spawn (.so Think/PlayerSpawn use info_player_torri /
	// info_player_goauld; stock EntSelectSpawnPoint only knows DM/start).
	int team = GetTeamIndex( pPlayer->m_szTeamName );
	const char *SpotClass = ( team == 1 ) ? s_SGTCDMSpawns[1] : s_SGTCDMSpawns[0];
	edict_t *spot = SGTC_FindTeamSpawn( SpotClass );
	if( !spot || FNullEnt( spot ) )
		spot = SGTC_FindTeamSpawn( "info_player_deathmatch" );
	if( !spot || FNullEnt( spot ) )
		return CGameRules::GetPlayerSpawnSpot( pPlayer );

	pPlayer->pev->origin = VARS( spot )->origin + Vector( 0, 0, 1 );
	pPlayer->pev->angles = VARS( spot )->angles;
	pPlayer->pev->v_angle = VARS( spot )->angles;
	pPlayer->pev->fixangle = TRUE;
	return spot;
}

void CStargateTeamDM::PlayerSpawn( CBasePlayer *pPlayer )
{
	pPlayer->StopObserver();
	CHalfLifeMultiplay::PlayerSpawn( pPlayer );
	SGTC_GiveTeamLoadout( pPlayer );
}

void CStargateTeamDM::PlayerThink( CBasePlayer *pPlayer )
{
	// .so PlayerThink maintains portrait/round HUD; base sim is stock.
	CHalfLifeMultiplay::PlayerThink( pPlayer );
}

BOOL CStargateTeamDM::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
	(void)pPlayer;
	return m_bRoundOver ? FALSE : TRUE;
}

BOOL CStargateTeamDM::FPlayerCanChangeInfo( CBasePlayer *pPlayer )
{
	(void)pPlayer;
	return TRUE;
}

void CStargateTeamDM::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
	// monster_skull drop on death (PlayerKilled strings).
	CBaseEntity::Create( "monster_skull", pVictim->pev->origin, pVictim->pev->angles, NULL );
	RecountTeams( TRUE );
}

void CStargateTeamDM::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	if( pVictim && pKiller && ( pKiller->flags & FL_CLIENT ) )
	{
		CBasePlayer *pk = (CBasePlayer *)CBaseEntity::Instance( pKiller );
		if( pk && pk != pVictim && PlayerRelationship( pVictim, pk ) == GR_TEAMMATE )
		{
			MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
				WRITE_BYTE( ENTINDEX( ENT( pKiller ) ) );
				WRITE_BYTE( ENTINDEX( pVictim->edict() ) );
				WRITE_STRING( "teammate" );
			MESSAGE_END();
			return;
		}
	}
	CHalfLifeMultiplay::DeathNotice( pVictim, pKiller, pInflictor );
}

void CStargateTeamDM::ClientDisconnected( edict_t *pClient )
{
	CHalfLifeMultiplay::ClientDisconnected( pClient );
	RecountTeams( TRUE );
}

BOOL CStargateTeamDM::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	// Verified commands: menuselect/jointeam/showmenu/autopick/classe/radio.
	if( FStrEq( pcmd, "jointeam" ) )
	{
		if( CMD_ARGC() < 2 )
			return TRUE;
		int team = atoi( CMD_ARGV( 1 ) );
		if( team == 5 ) // auto
			ChangePlayerTeam( pPlayer, TeamWithFewestPlayers(), TRUE, TRUE );
		else if( team == SGTC_TEAM_TORRI )
			ChangePlayerTeam( pPlayer, m_szSGTCTeams[0], TRUE, TRUE );
		else if( team == SGTC_TEAM_GOAULD )
			ChangePlayerTeam( pPlayer, m_szSGTCTeams[1], TRUE, TRUE );
		else
			ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "Teams Unbalanced\n" );
		return TRUE;
	}
	if( FStrEq( pcmd, "autopick" ) )
	{
		ChangePlayerTeam( pPlayer, TeamWithFewestPlayers(), TRUE, TRUE );
		return TRUE;
	}
	if( FStrEq( pcmd, "showmenu" ) || FStrEq( pcmd, "menuselect" ) || FStrEq( pcmd, "classe" ) )
		return TRUE; // VGUI-less client stub handles display
	return CHalfLifeTeamplay::ClientCommand( pPlayer, pcmd );
}

void CStargateTeamDM::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	char *mdls = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );
	if( !stricmp( mdls, pPlayer->m_szTeamName ) )
		return;
	if( !IsValidTeam( mdls ) )
	{
		int clientIndex = pPlayer->entindex();
		g_engfuncs.pfnSetClientKeyValue( clientIndex,
			g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", pPlayer->m_szTeamName );
		char text[128];
		sprintf( text, "* Can't change team to '%s'\n", mdls );
		UTIL_SayText( text, pPlayer );
		return;
	}
	char text[128];
	sprintf( text, "* %s changed to team '%s'\n", STRING( pPlayer->pev->netname ), mdls );
	UTIL_SayTextAll( text, pPlayer );
	ChangePlayerTeam( pPlayer, mdls, TRUE, TRUE );
	RecountTeams( TRUE );
}

int CStargateTeamDM::DeadPlayerWeapons( CBasePlayer *pPlayer )
{
	(void)pPlayer;
	return GR_PLR_DROP_GUN_ACTIVE; // .so DeadPlayerWeapons: keep active gun
}

int CStargateTeamDM::DeadPlayerAmmo( CBasePlayer *pPlayer )
{
	(void)pPlayer;
	return GR_PLR_DROP_AMMO_ACTIVE; // .so DeadPlayerAmmo path
}

void CStargateTeamDM::Goal( int iTeam )
{
	// .so Goal(int): first-goal music + score++ (choice of team table).
	if( iTeam < 0 || iTeam >= m_iSGTCTeamCount )
		return;
	m_iSGTCTeamScores[iTeam]++;
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( 0 );
		WRITE_SHORT( m_iSGTCTeamScores[0] );
		WRITE_SHORT( m_iSGTCTeamScores[1] );
		WRITE_SHORT( 0 );
		WRITE_SHORT( iTeam + 1 );
	MESSAGE_END();
}

void CStargateTeamDM::EndRound( void )
{
	// .so EndRound: broadcasts both team scores.
	m_bRoundOver = TRUE;
	for( int i = 0; i < m_iSGTCTeamCount; i++ )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamNames );
			WRITE_BYTE( i );
			WRITE_STRING( m_szSGTCTeams[i] );
		MESSAGE_END();
	}
}

void CStargateTeamDM::NewRound( void )
{
	// .so NewRound: resets scores (round entities — corpse_control /
	// effect_bigshield / monster_shieldemit / lockdown / trapbox / zatboom —
	// respawn via map reset, not enumerated here).
	m_bRoundOver = FALSE;
	m_iSGTCTeamScores[0] = m_iSGTCTeamScores[1] = 0;
	MESSAGE_BEGIN( MSG_ALL, gmsgNewRnd );
		WRITE_BYTE( 1 );
	MESSAGE_END();
	RecountTeams( TRUE );
}

void CStargateTeamDM::GlobalText( int iTeam, int iText )
{
	// .so GlobalText: per-team SayText fan-out.
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pl = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if( !pl || pl->pev->deadflag != DEAD_NO )
			continue;
		if( iTeam && GetTeamIndex( pl->m_szTeamName ) != iTeam - 1 )
			continue;
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, pl->edict() );
			WRITE_BYTE( ENTINDEX( INDEXENT( 0 ) ) );
			WRITE_STRING( UTIL_VarArgs( "#SGTC_GLOBAL_%d", iText ) );
		MESSAGE_END();
	}
}

void CStargateTeamDM::AddPortrait( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgAddPortrait, NULL, pPlayer->edict() );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_STRING( pPlayer->m_szTeamName );
	MESSAGE_END();
}

void CStargateTeamDM::StartPortrait( CBasePlayer *pPlayer )
{
	// Default portrait model per team (InitHUD model table).
	MESSAGE_BEGIN( MSG_ONE, gmsgAddPortrait, NULL, pPlayer->edict() );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_STRING( GetTeamIndex( pPlayer->m_szTeamName ) == 1 ? "goauld3" : "oneill" );
	MESSAGE_END();
}

void CStargateTeamDM::RedrawPortrait( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgTogglePortrait );
		WRITE_BYTE( pPlayer->entindex() );
	MESSAGE_END();
}

void CStargateTeamDM::UpdateClassCount( void )
{
	// Tallies portrait/model usage across clients (InitHUD table upkeep).
	int counts[SGTC_NUM_PORTRAITS];
	for( size_t m = 0; m < SGTC_NUM_PORTRAITS; m++ )
		counts[m] = 0;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );
		if( !plr )
			continue;
		char *mdls = g_engfuncs.pfnInfoKeyValue(
			g_engfuncs.pfnGetInfoKeyBuffer( plr->edict() ), "model" );
		for( size_t m = 0; m < SGTC_NUM_PORTRAITS; m++ )
		{
			if( !stricmp( mdls, s_SGTCPortraits[m] ) )
			{
				counts[m]++;
				break;
			}
		}
	}
}

void CStargateTeamDM::Think( void )
{
	CHalfLifeTeamplay::Think();
	if( m_bRoundOver )
		return;

	// Key-holder sanity (Think strings: "GameRule: More than one player
	// has a key!! Total %d").
	int keys = 0;
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pl = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if( pl && pl->m_rgItems[ITEM_SECURITY] > 0 )
			keys += pl->m_rgItems[ITEM_SECURITY];
	}
	if( keys > 1 )
		ALERT( at_console, "GameRule: More than one player has a key!! Total %d\n", keys );

	float flTimeLimit = CVAR_GET_FLOAT( "mp_timelimit" ) * 60;
	if( flTimeLimit != 0 && gpGlobals->time >= flTimeLimit )
		EndRound();
}

// ===========================================================================
// CStargateTeamCP
// ===========================================================================
CStargateTeamCP::CStargateTeamCP( void ) : m_bSpawnsInversed( FALSE )
{
}

void CStargateTeamCP::InverseSpawns( void )
{
	// .so InverseSpawns: swaps the torri/goauld spawn tables mid-round.
	m_bSpawnsInversed = !m_bSpawnsInversed;
}

edict_t *CStargateTeamCP::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
	int team = GetTeamIndex( pPlayer->m_szTeamName );
	if( m_bSpawnsInversed )
		team = 1 - team;
	const char *SpotClass = ( team == 1 ) ? s_SGTCDMSpawns[1] : s_SGTCDMSpawns[0];
	edict_t *spot = SGTC_FindTeamSpawn( SpotClass );
	if( !spot || FNullEnt( spot ) )
		return CStargateTeamDM::GetPlayerSpawnSpot( pPlayer );
	pPlayer->pev->origin = VARS( spot )->origin + Vector( 0, 0, 1 );
	pPlayer->pev->angles = VARS( spot )->angles;
	pPlayer->pev->v_angle = VARS( spot )->angles;
	pPlayer->pev->fixangle = TRUE;
	return spot;
}

void CStargateTeamCP::PlayerSpawn( CBasePlayer *pPlayer )
{
	CStargateTeamDM::PlayerSpawn( pPlayer );
	// CTP capture HUD mode: TicketMode on.
	MESSAGE_BEGIN( MSG_ONE, gmsgTicketMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 1 );
	MESSAGE_END();
}

// ===========================================================================
// CStargateUnas / CStargateInvis
// ===========================================================================
CStargateUnas::CStargateUnas( void )
{
	// .so Unas ctor teamlist "Unas;Marine" (fileoff 0x1dc5f9).
	strncpy( m_szSGTCTeams[0], "Unas", SGTC_TEAMNAME_LEN );
	strncpy( m_szSGTCTeams[1], "Marine", SGTC_TEAMNAME_LEN );
	m_iSGTCTeamCount = SGTC_MAX_SGTC_TEAMS;
	RecountTeams( FALSE );
}

int CStargateUnas::GetTeamIndex( const char *pTeamName )
{
	if( pTeamName && *pTeamName )
	{
		for( int tm = 0; tm < m_iSGTCTeamCount; tm++ )
		{
			if( !stricmp( m_szSGTCTeams[tm], pTeamName ) )
				return tm;
		}
	}
	return -1;
}

const char *CStargateUnas::GetIndexedTeamName( int teamIndex )
{
	if( teamIndex < 0 || teamIndex >= m_iSGTCTeamCount )
		return "";
	return m_szSGTCTeams[teamIndex];
}

BOOL CStargateUnas::IsValidTeam( const char *pTeamName )
{
	return ( GetTeamIndex( pTeamName ) != -1 ) ? TRUE : FALSE;
}

const char *CStargateUnas::GetTeamID( CBaseEntity *pEntity )
{
	if( !pEntity || !pEntity->pev )
		return "";
	return pEntity->TeamID();
}

// PlayerRelationship is inherited from CHalfLifeTeamplay: identical same-team
// stricmp logic; no decl in the SGTC header, so the stock virtual is used.

BOOL CStargateUnas::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	if( pAttacker && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE )
	{
		if( friendlyfire.value == 0 && pAttacker != pPlayer )
			return FALSE;
	}
	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

BOOL CStargateUnas::ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target )
{
	CBaseEntity *pTgt = CBaseEntity::Instance( target );
	if( pTgt && pTgt->IsPlayer() && PlayerRelationship( pPlayer, pTgt ) == GR_TEAMMATE )
		return FALSE;
	return CHalfLifeMultiplay::ShouldAutoAim( pPlayer, target );
}

int CStargateUnas::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
	if( !pKilled )
		return 0;
	if( !pAttacker )
		return 1;
	if( pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;
	return 1;
}

const char *CStargateUnas::TeamWithFewestPlayers( void )
{
	int teamCount[SGTC_MAX_SGTC_TEAMS] = { 0, 0 };
	int minPlayers = 9999;
	const char *pName = m_szSGTCTeams[0];
	for( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity *plr = UTIL_PlayerByIndex( i );
		if( plr )
		{
			int team = GetTeamIndex( plr->TeamID() );
			if( team >= 0 )
				teamCount[team]++;
		}
	}
	for( int i = 0; i < m_iSGTCTeamCount; i++ )
	{
		if( teamCount[i] < minPlayers )
		{
			minPlayers = teamCount[i];
			pName = m_szSGTCTeams[i];
		}
	}
	return pName;
}

void CStargateUnas::RecountTeams( BOOL bResendInfo )
{
	if( !bResendInfo )
		return;
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamNames );
		WRITE_BYTE( m_iSGTCTeamCount );
		for( int i = 0; i < m_iSGTCTeamCount; i++ )
			WRITE_STRING( m_szSGTCTeams[i] );
	MESSAGE_END();
}

const char *CStargateUnas::SetDefaultPlayerTeam( CBasePlayer *pPlayer )
{
	char *mdls = g_engfuncs.pfnInfoKeyValue(
		g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model" );
	strncpy( pPlayer->m_szTeamName, mdls, TEAM_NAME_LENGTH );
	if( pPlayer->m_szTeamName[0] == '\0' || !IsValidTeam( pPlayer->m_szTeamName ) )
		strncpy( pPlayer->m_szTeamName, TeamWithFewestPlayers(), TEAM_NAME_LENGTH );
	return pPlayer->m_szTeamName;
}

void CStargateUnas::ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib )
{
	int damageFlags = DMG_GENERIC | ( bGib ? DMG_ALWAYSGIB : DMG_NEVERGIB );
	if( bKill )
	{
		entvars_t *pevWorld = VARS( INDEXENT( 0 ) );
		pPlayer->TakeDamage( pevWorld, pevWorld, 900, damageFlags );
	}
	if( pPlayer->m_szTeamName != pTeamName )
		strncpy( pPlayer->m_szTeamName, pTeamName, TEAM_NAME_LENGTH );
	int clientIndex = pPlayer->entindex();
	g_engfuncs.pfnSetClientKeyValue( clientIndex,
		g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", pPlayer->m_szTeamName );
	g_engfuncs.pfnSetClientKeyValue( clientIndex,
		g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "team", pPlayer->m_szTeamName );
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( clientIndex );
		WRITE_STRING( pPlayer->m_szTeamName );
	MESSAGE_END();
}

void CStargateUnas::SGTC_SendTeamNames( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgTeamNames, NULL, pPlayer->edict() );
		WRITE_BYTE( m_iSGTCTeamCount );
		for( int i = 0; i < m_iSGTCTeamCount; i++ )
			WRITE_STRING( m_szSGTCTeams[i] );
	MESSAGE_END();
}

void CStargateUnas::InitHUD( CBasePlayer *pPlayer )
{
	SetDefaultPlayerTeam( pPlayer );
	CHalfLifeMultiplay::InitHUD( pPlayer );
	SGTC_SendTeamNames( pPlayer );
	RecountTeams( TRUE );
	UpdateGameMode( pPlayer );
}

void CStargateUnas::UpdateGameMode( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->edict() );
		WRITE_BYTE( 1 );
	MESSAGE_END();
}

edict_t *CStargateUnas::SGTC_FindTeamSpawn( const char *pszClassname )
{
	CBaseEntity *pSpot = NULL;
	for( int i = RANDOM_LONG( 1, 5 ); i > 0; i-- )
		pSpot = UTIL_FindEntityByClassname( pSpot, (char *)pszClassname );
	if( !pSpot )
		pSpot = UTIL_FindEntityByClassname( pSpot, (char *)pszClassname );
	return pSpot ? pSpot->edict() : NULL;
}

void CStargateUnas::PlayerSpawn( CBasePlayer *pPlayer )
{
	// Verified Unas PlayerSpawn strings: Unas gets lance+invis + FR line;
	// Marine gets 9mmhandgun/9mmAR/main/zat/flashbang + ammo + FR line.
	pPlayer->StopObserver();
	CHalfLifeMultiplay::PlayerSpawn( pPlayer );
	int team = GetTeamIndex( pPlayer->m_szTeamName );
	if( team == 0 )
	{
		pPlayer->GiveNamedItem( "weapon_lance" );
		pPlayer->GiveNamedItem( "weapon_invis" );
		SET_MODEL( ENT( pPlayer->pev ), "models/player/unas/unas.mdl" );
		UTIL_SayText( "Vous etes le Unas, bonne boucherie :-)\n", pPlayer );
	}
	else
	{
		pPlayer->GiveNamedItem( "weapon_9mmhandgun" );
		pPlayer->GiveNamedItem( "weapon_9mmAR" );
		pPlayer->GiveNamedItem( "weapon_main" );
		pPlayer->GiveNamedItem( "weapon_zat" );
		pPlayer->GiveNamedItem( "weapon_flashbang" );
		pPlayer->GiveAmmo( 60, "9mm", 200 );
		pPlayer->GiveAmmo( 2, "ARgrenades", 10 );
		SET_MODEL( ENT( pPlayer->pev ), "models/player.mdl" );
		UTIL_SayText( "Vous etes un Marine, bonne chance...\n", pPlayer );
	}
}

void CStargateUnas::Think( void )
{
	CHalfLifeTeamplay::Think();
}

void CStargateUnas::PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
	RecountTeams( TRUE );
}

void CStargateUnas::DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor )
{
	CHalfLifeMultiplay::DeathNotice( pVictim, pKiller, pInflictor );
}

BOOL CStargateUnas::ClientCommand( CBasePlayer *pPlayer, const char *pcmd )
{
	if( FStrEq( pcmd, "jointeam" ) )
	{
		if( CMD_ARGC() < 2 )
			return TRUE;
		int team = atoi( CMD_ARGV( 1 ) );
		if( team == 5 )
			ChangePlayerTeam( pPlayer, TeamWithFewestPlayers(), TRUE, TRUE );
		else if( team == 1 )
			ChangePlayerTeam( pPlayer, m_szSGTCTeams[0], TRUE, TRUE );
		else if( team == 2 )
			ChangePlayerTeam( pPlayer, m_szSGTCTeams[1], TRUE, TRUE );
		return TRUE;
	}
	if( FStrEq( pcmd, "autopick" ) )
	{
		ChangePlayerTeam( pPlayer, TeamWithFewestPlayers(), TRUE, TRUE );
		return TRUE;
	}
	return CHalfLifeTeamplay::ClientCommand( pPlayer, pcmd );
}

void CStargateUnas::ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer )
{
	char *mdls = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );
	if( !stricmp( mdls, pPlayer->m_szTeamName ) )
		return;
	if( !IsValidTeam( mdls ) )
	{
		int clientIndex = pPlayer->entindex();
		g_engfuncs.pfnSetClientKeyValue( clientIndex,
			g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ), "model", pPlayer->m_szTeamName );
		return;
	}
	ChangePlayerTeam( pPlayer, mdls, TRUE, TRUE );
	RecountTeams( TRUE );
}

CStargateInvis::CStargateInvis( void )
{
	// .so Invis ctor teamlist "Torri;Invisible" (fileoff 0x1dad91).
	strncpy( m_szSGTCTeams[0], "Torri", SGTC_TEAMNAME_LEN );
	strncpy( m_szSGTCTeams[1], "Invisible", SGTC_TEAMNAME_LEN );
}

void CStargateInvis::PlayerSpawn( CBasePlayer *pPlayer )
{
	// Invis mode: Torri-side DM loadout plus the invis device.
	pPlayer->StopObserver();
	CHalfLifeMultiplay::PlayerSpawn( pPlayer );
	pPlayer->GiveNamedItem( "weapon_couteau" );
	pPlayer->GiveNamedItem( "weapon_coujaf" );
	pPlayer->GiveNamedItem( "weapon_invis" );
	pPlayer->GiveNamedItem( "weapon_zat" );
}

// ===========================================================================
// Spawn points + info_* point entities
// ===========================================================================
void CSGTCPlayerStart::Spawn( void )
{
	CPointEntity::Spawn();
}
LINK_ENTITY_TO_CLASS( info_player_torri, CSGTCPlayerStart );
LINK_ENTITY_TO_CLASS( info_player_marine, CSGTCPlayerStart );
LINK_ENTITY_TO_CLASS( info_player_goauld, CSGTCPlayerStart );
LINK_ENTITY_TO_CLASS( info_player_unas, CSGTCPlayerStart );

LINK_ENTITY_TO_CLASS( info_marker, CPointEntity );
LINK_ENTITY_TO_CLASS( info_radar_block, CPointEntity );
LINK_ENTITY_TO_CLASS( info_observer, CPointEntity );

void CFlash::Spawn( void )
{
	Precache();
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	if( pev->model )
		SET_MODEL( ENT( pev ), STRING( pev->model ) );
	SetTouch( &CFlash::FlashTouch );
	SetThink( &CFlash::Think );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CFlash::Precache( void )
{
	PRECACHE_SOUND( "weapons/flashbang.wav" );
}

void CFlash::FlashTouch( CBaseEntity *pOther )
{
	if( !pOther || !pOther->IsPlayer() )
		return;
	MyTouch( (CBasePlayer *)pOther );
}

BOOL CFlash::MyTouch( CBasePlayer *pPlayer )
{
	// Flash pickup: full flash battery + sound (mirrors h_battery).
	if( pPlayer->m_iFlashBattery >= 100 )
		return FALSE;
	pPlayer->m_iFlashBattery = 100;
	EMIT_SOUND( ENT( pev ), CHAN_ITEM, "weapons/flashbang.wav", 1, ATTN_NORM );
	return TRUE;
}

void CFlash::Think( void )
{
	pev->nextthink = gpGlobals->time + 0.1f;
}
LINK_ENTITY_TO_CLASS( info_flash, CFlash );

TYPEDESCRIPTION CCapInfo::m_SaveData[] =
{
	DEFINE_FIELD( CCapInfo, m_iWinType, FIELD_INTEGER ),
	DEFINE_FIELD( CCapInfo, m_iTeam, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CCapInfo, CPointEntity );

void CCapInfo::Spawn( void )
{
	// FGD "Capzone Settings": type (win) + m_team (condition).
	pev->solid = SOLID_NOT;
}

void CCapInfo::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iWinType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "m_team" ) )
	{
		m_iTeam = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( info_cap_rules, CCapInfo );

// ===========================================================================
// Triggers (CBaseToggle-based; see header note)
// ===========================================================================
TYPEDESCRIPTION CTriggerTeleportAdvanced::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerTeleportAdvanced, m_flTimeTel, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerTeleportAdvanced, m_flArmTime, FIELD_TIME ),
	DEFINE_FIELD( CTriggerTeleportAdvanced, m_hTeleportTarget, FIELD_EHANDLE ),
};
IMPLEMENT_SAVERESTORE( CTriggerTeleportAdvanced, CBaseToggle );

void CTriggerTeleportAdvanced::Spawn( void )
{
	SGTC_InitTrigger( this );
	SetTouch( &CTriggerTeleportAdvanced::AdvTeleportTouch );
	SetThink( &CTriggerTeleportAdvanced::TeleportThink );
	Reset();
}

void CTriggerTeleportAdvanced::KeyValue( KeyValueData *pkvd )
{
	// Verified key: "time_tel" (rodata 0x1ddc14), atof.
	if( FStrEq( pkvd->szKeyName, "time_tel" ) )
	{
		m_flTimeTel = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CTriggerTeleportAdvanced::Reset( void )
{
	m_hTeleportTarget = NULL;
	// .so Reset: nextthink = now + delay, ltime = now - delay.
	pev->nextthink = gpGlobals->time + m_flTimeTel;
	pev->ltime = gpGlobals->time - m_flTimeTel;
	m_flArmTime = 0;
}

void CTriggerTeleportAdvanced::AdvTeleportTouch( CBaseEntity *pOther )
{
	// .so AdvTeleportTouch: arms only if now - ltime >= delay; stores the
	// toucher and its trigger time for TeleportThink.
	if( gpGlobals->time - pev->ltime < m_flTimeTel )
		return;
	if( !pOther )
		return;
	m_hTeleportTarget = pOther;
	m_flArmTime = gpGlobals->time;
	pev->ltime = gpGlobals->time;
}

void CTriggerTeleportAdvanced::TeleportThink( void )
{
	// Fires the armed teleport: move the stored entity to our target
	// (info_teleport_destination / info_marker, FGD "special in" chain).
	pev->nextthink = gpGlobals->time + 0.1f;
	CBaseEntity *pOther = m_hTeleportTarget;
	if( !pOther )
		return;
	if( gpGlobals->time - m_flArmTime < m_flTimeTel )
		return;
	m_hTeleportTarget = NULL;
	edict_t *pentTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) );
	if( FNullEnt( pentTarget ) )
		return;
	Vector tmp = VARS( pentTarget )->origin;
	if( pOther->IsPlayer() )
		tmp.z -= pOther->pev->mins.z;
	tmp.z++;
	pOther->pev->flags &= ~FL_ONGROUND;
	UTIL_SetOrigin( pOther->pev, tmp );
	pOther->pev->angles = pentTarget->v.angles;
	if( pOther->IsPlayer() )
		pOther->pev->v_angle = pentTarget->v.angles;
	pOther->pev->fixangle = TRUE;
	pOther->pev->velocity = pOther->pev->basevelocity = g_vecZero;
}
LINK_ENTITY_TO_CLASS( trigger_adv_teleport, CTriggerTeleportAdvanced );

TYPEDESCRIPTION CTriggerGoal::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerGoal, m_iTeam, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CTriggerGoal, CBaseToggle );

void CTriggerGoal::Spawn( void )
{
	// .so Spawn: solid trigger, touch = GoalTouch.
	SGTC_InitTrigger( this );
	SetTouch( &CTriggerGoal::GoalTouch );
}

void CTriggerGoal::KeyValue( KeyValueData *pkvd )
{
	// Verified key: "m_team" atoi (FGD 1=Tau'ri 2=Goa'uld).
	if( FStrEq( pkvd->szKeyName, "m_team" ) )
	{
		m_iTeam = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CTriggerGoal::GoalTouch( CBaseEntity *pOther )
{
	// .so GoalTouch: master check -> team-mismatch mapping (1<->2) ->
	// alive+player check -> same-team guard -> g_pGameRules->Goal(mapped).
	if( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
		return;
	int mapped = 0;
	if( m_iTeam == 1 )
		mapped = 2;
	else if( m_iTeam == 2 )
		mapped = 1;
	if( !mapped || !pOther || !pOther->IsPlayer() || !pOther->IsAlive() )
		return;
	CBasePlayer *pl = (CBasePlayer *)pOther;
	if( !g_pGameRules || !g_pGameRules->IsTeamplay() )
		return;
	int idx = g_pGameRules->GetTeamIndex( pl->m_szTeamName );
	if( idx < 0 || idx + 1 == m_iTeam )
		return; // already on the goal's team: nothing to award
	CStargateTeamDM *sg = (CStargateTeamDM *)g_pGameRules;
	sg->Goal( mapped - 1 );
}
LINK_ENTITY_TO_CLASS( trigger_goal, CTriggerGoal );

TYPEDESCRIPTION CTriggerRandom::m_SaveData[] =
{
	DEFINE_FIELD( CTriggerRandom, m_flMinPeriod, FIELD_FLOAT ),
	DEFINE_FIELD( CTriggerRandom, m_flMaxPeriod, FIELD_FLOAT ),
};
IMPLEMENT_SAVERESTORE( CTriggerRandom, CBaseToggle );

void CTriggerRandom::Spawn( void )
{
	SGTC_InitTrigger( this );
	SetTouch( &CTriggerRandom::RandomTouch );
	SetUse( &CTriggerRandom::RandomUse );
	SetThink( &CTriggerRandom::RandomThink );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( m_flMinPeriod, m_flMaxPeriod );
}

void CTriggerRandom::KeyValue( KeyValueData *pkvd )
{
	// Verified keys: "minperiod"/"maxperiod" (rodata 0x1d9b44/0x1d9b4e).
	if( FStrEq( pkvd->szKeyName, "minperiod" ) )
	{
		m_flMinPeriod = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxperiod" ) )
	{
		m_flMaxPeriod = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CTriggerRandom::RandomTouch( CBaseEntity *pOther )
{
	// Own MultiTouch equivalent (stock MultiTouch is TU-local): clients /
	// allowed monsters / pushables, master-gated, then fire.
	entvars_t *pevToucher = pOther->pev;
	if( ( ( pevToucher->flags & FL_CLIENT ) && !( pev->spawnflags & SF_TRIGGER_NOCLIENTS ) ) ||
		( ( pevToucher->flags & FL_MONSTER ) && ( pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS ) ) ||
		( ( pev->spawnflags & SF_TRIGGER_PUSHABLES ) && FClassnameIs( pevToucher, "func_pushable" ) ) )
	{
		if( !UTIL_IsMasterTriggered( m_sMaster, pOther ) )
			return;
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
	}
}

void CTriggerRandom::RandomUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Own ToggleUse equivalent (stock ToggleUse is TU-local).
	if( pev->solid == SOLID_NOT )
	{
		pev->solid = SOLID_TRIGGER;
		gpGlobals->force_retouch++;
	}
	else
	{
		pev->solid = SOLID_NOT;
	}
	UTIL_SetOrigin( pev, pev->origin );
	(void)pActivator; (void)pCaller; (void)useType; (void)value;
}

void CTriggerRandom::RandomThink( void )
{
	// .so RandomThink: optional noise, SUB_UseTargets, reschedule by
	// RANDOM(min,max) added to now.
	if( !FStringNull( pev->noise ) )
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, STRING( pev->noise ), 1, ATTN_NORM );
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	float lo = m_flMinPeriod, hi = m_flMaxPeriod;
	if( hi < lo )
		hi = lo;
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( lo, hi );
	SetThink( &CTriggerRandom::RandomThink );
}
LINK_ENTITY_TO_CLASS( trigger_random, CTriggerRandom );
// trigger_secret: string-only in all 3 binaries (rodata 0x1dd920), no
// factory/class — maps use it as a one-shot random trigger. Alias it.
LINK_ENTITY_TO_CLASS( trigger_secret, CTriggerRandom );

// ===========================================================================
// item_objective (CItemSecurity behaviour + models/key.mdl)
// ===========================================================================
void CItemObjective::Spawn( void )
{
	Precache();
	SET_MODEL( ENT( pev ), "models/key.mdl" );
	CItem::Spawn();
}

void CItemObjective::Precache( void )
{
	PRECACHE_MODEL( "models/key.mdl" );
	PRECACHE_SOUND( "items/gunpickup2.wav" );
}

BOOL CItemObjective::MyTouch( CBasePlayer *pPlayer )
{
	// Mirrors stock CItemSecurity::MyTouch (items.cpp:292).
	pPlayer->m_rgItems[ITEM_SECURITY] += 1;
	MESSAGE_BEGIN( MSG_ONE, gmsgHasKey, NULL, pPlayer->edict() );
		WRITE_BYTE( pPlayer->m_rgItems[ITEM_SECURITY] );
	MESSAGE_END();
	return TRUE;
}
LINK_ENTITY_TO_CLASS( item_objective, CItemObjective );

// ===========================================================================
// env_model (CModel)
// ===========================================================================
TYPEDESCRIPTION CModel::m_SaveData[] =
{
	DEFINE_FIELD( CModel, m_iAnim, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CModel, CBaseAnimating );

void CModel::Spawn( void )
{
	PRECACHE_MODEL( (char *)STRING( pev->model ) );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetSize( pev, Vector( -20, -20, -20 ), Vector( 20, 20, 20 ) );
	if( m_iAnim != 0 )
	{
		pev->sequence = m_iAnim;
		pev->frame = 0;
		ResetSequenceInfo();
	}
	SetThink( &CModel::Animate );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CModel::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "anim" ) )
	{
		m_iAnim = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseAnimating::KeyValue( pkvd );
}

void CModel::Animate( void )
{
	// FGD spawnflag 2 = follow target.
	if( FBitSet( pev->spawnflags, 2 ) && pev->target )
	{
		CBaseEntity *t = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		if( t )
			UTIL_SetOrigin( pev, t->pev->origin );
	}
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
}
LINK_ENTITY_TO_CLASS( env_model, CModel );

// ===========================================================================
// Particle net entities. Payload layouts are BEST-EFFORT (client handlers
// drain opaquely): entindex + origin + param block, MSG_ALL on Spawn,
// MSG_ONE in SendValueTo.
// ===========================================================================
static void SGTC_ParseVector( const char *str, Vector &out )
{
	out = Vector( 0, 0, 0 );
	if( str )
		sscanf( str, "%f %f %f", &out.x, &out.y, &out.z );
}

static void SGTC_ParseColor( const char *str, Vector &out )
{
	int r = 128, g = 128, b = 128, a = 128;
	if( str )
		sscanf( str, "%d %d %d %d", &r, &g, &b, &a );
	out = Vector( (float)r, (float)g, (float)b );
	(void)a;
}

TYPEDESCRIPTION CEnvParticule::m_SaveData[] =
{
	DEFINE_FIELD( CEnvParticule, m_iCount, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvParticule, m_vecAccel, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvParticule, m_vecV0, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvParticule, m_vecSpread, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvParticule, m_vecSize, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvParticule, m_vecColor, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvParticule, m_flLife, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvParticule, m_flBounce, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvParticule, m_iSprite, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvParticule, m_iType, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvParticule, m_bActive, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CEnvParticule, CPointEntity );

void CEnvParticule::Spawn( void )
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	m_bActive = TRUE;
	SendValue();
}

void CEnvParticule::KeyValue( KeyValueData *pkvd )
{
	// Verified rodata keys (0x1d9b5f..0x1d9bbf block).
	if( FStrEq( pkvd->szKeyName, "nombre" ) || FStrEq( pkvd->szKeyName, "id" ) )
	{
		m_iCount = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "acceleration" ) )
	{
		SGTC_ParseVector( pkvd->szValue, m_vecAccel );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "v0" ) )
	{
		SGTC_ParseVector( pkvd->szValue, m_vecV0 );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "ecart" ) )
	{
		SGTC_ParseVector( pkvd->szValue, m_vecSpread );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "taille" ) )
	{
		float x = 2, y = 2;
		sscanf( pkvd->szValue, "%f %f", &x, &y );
		m_vecSize = Vector( x, y, 0 );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "_couleur" ) )
	{
		SGTC_ParseColor( pkvd->szValue, m_vecColor );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "vie" ) )
	{
		m_flLife = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "bounce" ) )
	{
		m_flBounce = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "sprite" ) )
	{
		m_iSprite = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CEnvParticule::Reset( void )
{
	m_bActive = TRUE;
}

void CEnvParticule::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( ShouldToggle( useType, m_bActive ) )
		m_bActive = !m_bActive;
	MESSAGE_BEGIN( MSG_ALL, gmsgChangeParticuleState );
		WRITE_SHORT( entindex() );
		WRITE_BYTE( m_bActive ? 1 : 0 );
	MESSAGE_END();
	(void)pActivator; (void)pCaller; (void)value;
}

void CEnvParticule::SendValue( void )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgEnvParticule, pev->origin );
		WRITE_SHORT( entindex() );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iCount );
		WRITE_COORD( m_vecAccel.x );
		WRITE_COORD( m_vecAccel.y );
		WRITE_COORD( m_vecAccel.z );
		WRITE_COORD( m_vecV0.x );
		WRITE_COORD( m_vecV0.y );
		WRITE_COORD( m_vecV0.z );
		WRITE_BYTE( (int)m_vecColor.x );
		WRITE_BYTE( (int)m_vecColor.y );
		WRITE_BYTE( (int)m_vecColor.z );
		WRITE_BYTE( m_iSprite );
		WRITE_BYTE( m_iType );
		WRITE_BYTE( m_bActive ? 1 : 0 );
	MESSAGE_END();
}

void CEnvParticule::SendValueTo( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgEnvParticule, pev->origin, pPlayer->edict() );
		WRITE_SHORT( entindex() );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iCount );
		WRITE_BYTE( m_iSprite );
		WRITE_BYTE( m_iType );
		WRITE_BYTE( m_bActive ? 1 : 0 );
	MESSAGE_END();
}
LINK_ENTITY_TO_CLASS( env_particule, CEnvParticule );

TYPEDESCRIPTION CEnvRain::m_SaveData[] =
{
	DEFINE_FIELD( CEnvRain, m_flDensity, FIELD_FLOAT ),
};
IMPLEMENT_SAVERESTORE( CEnvRain, CEnvParticule );

void CEnvRain::KeyValue( KeyValueData *pkvd )
{
	// .so CEnvRain::KeyValue handles "densite", rest falls to particule.
	if( FStrEq( pkvd->szKeyName, "densite" ) )
	{
		m_flDensity = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CEnvParticule::KeyValue( pkvd );
}

void CEnvRain::SendValue( void )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgEnvRain, pev->origin );
		WRITE_SHORT( entindex() );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iCount );
		WRITE_COORD( m_flDensity );
		WRITE_BYTE( m_iSprite );
		WRITE_BYTE( m_iType );
		WRITE_BYTE( m_bActive ? 1 : 0 );
	MESSAGE_END();
}

void CEnvRain::SendValueTo( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgEnvRain, pev->origin, pPlayer->edict() );
		WRITE_SHORT( entindex() );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iCount );
		WRITE_BYTE( m_bActive ? 1 : 0 );
	MESSAGE_END();
}
LINK_ENTITY_TO_CLASS( env_rain, CEnvRain );

TYPEDESCRIPTION CEnvLensFlare::m_SaveData[] =
{
	DEFINE_FIELD( CEnvLensFlare, m_vecColor, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvLensFlare, m_bUseOrigin, FIELD_BOOLEAN ),
	DEFINE_FIELD( CEnvLensFlare, m_vecOrigin, FIELD_VECTOR ),
};
IMPLEMENT_SAVERESTORE( CEnvLensFlare, CPointEntity );

void CEnvLensFlare::KeyValue( KeyValueData *pkvd )
{
	// Verified keys: _couleur / useorigine / origine.
	if( FStrEq( pkvd->szKeyName, "_couleur" ) )
	{
		SGTC_ParseColor( pkvd->szValue, m_vecColor );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "useorigine" ) )
	{
		m_bUseOrigin = atoi( pkvd->szValue ) != 0;
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "origine" ) )
	{
		SGTC_ParseVector( pkvd->szValue, m_vecOrigin );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CEnvLensFlare::SendValue( void )
{
	Vector org = m_bUseOrigin ? pev->origin : m_vecOrigin;
	MESSAGE_BEGIN( MSG_ALL, gmsgEnvNewLens, org );
		WRITE_SHORT( entindex() );
		WRITE_COORD( org.x );
		WRITE_COORD( org.y );
		WRITE_COORD( org.z );
		WRITE_BYTE( (int)m_vecColor.x );
		WRITE_BYTE( (int)m_vecColor.y );
		WRITE_BYTE( (int)m_vecColor.z );
	MESSAGE_END();
}

void CEnvLensFlare::SendValueTo( CBasePlayer *pPlayer )
{
	Vector org = m_bUseOrigin ? pev->origin : m_vecOrigin;
	MESSAGE_BEGIN( MSG_ONE, gmsgEnvNewLens, org, pPlayer->edict() );
		WRITE_SHORT( entindex() );
		WRITE_COORD( org.x );
		WRITE_COORD( org.y );
		WRITE_COORD( org.z );
		WRITE_BYTE( (int)m_vecColor.x );
		WRITE_BYTE( (int)m_vecColor.y );
		WRITE_BYTE( (int)m_vecColor.z );
	MESSAGE_END();
}
LINK_ENTITY_TO_CLASS( env_lensflare, CEnvLensFlare );

TYPEDESCRIPTION CEnvFountain::m_SaveData[] =
{
	DEFINE_FIELD( CEnvFountain, m_iCount, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFountain, m_vecAccel, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvFountain, m_vecDir, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvFountain, m_vecSize, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvFountain, m_vecColor, FIELD_VECTOR ),
	DEFINE_FIELD( CEnvFountain, m_flMinAngle, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFountain, m_flMaxAngle, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFountain, m_flMinRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFountain, m_flMaxRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFountain, m_flMinVel, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFountain, m_flMaxVel, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFountain, m_flLife, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFountain, m_flBounce, FIELD_FLOAT ),
	DEFINE_FIELD( CEnvFountain, m_iSprite, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFountain, m_iType, FIELD_INTEGER ),
	DEFINE_FIELD( CEnvFountain, m_bActive, FIELD_BOOLEAN ),
};
IMPLEMENT_SAVERESTORE( CEnvFountain, CPointEntity );

void CEnvFountain::KeyValue( KeyValueData *pkvd )
{
	// Verified rodata keys (0x1d9b5f..0x1d9c22 block).
	if( FStrEq( pkvd->szKeyName, "nombre" ) )
	{
		m_iCount = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "acceleration" ) )
	{
		SGTC_ParseVector( pkvd->szValue, m_vecAccel );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "directeur" ) )
	{
		SGTC_ParseVector( pkvd->szValue, m_vecDir );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "taille" ) )
	{
		float x = 2, y = 2;
		sscanf( pkvd->szValue, "%f %f", &x, &y );
		m_vecSize = Vector( x, y, 0 );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "_couleur" ) )
	{
		SGTC_ParseColor( pkvd->szValue, m_vecColor );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "minAngle" ) )
	{
		m_flMinAngle = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxAngle" ) )
	{
		m_flMaxAngle = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "minRadius" ) )
	{
		m_flMinRadius = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxRadius" ) )
	{
		m_flMaxRadius = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "minVelocity" ) )
	{
		m_flMinVel = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "maxVelocity" ) )
	{
		m_flMaxVel = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "vie" ) )
	{
		m_flLife = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "bounce" ) )
	{
		m_flBounce = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "sprite" ) )
	{
		m_iSprite = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CEnvFountain::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if( ShouldToggle( useType, m_bActive ) )
		m_bActive = !m_bActive;
	MESSAGE_BEGIN( MSG_ALL, gmsgChangeParticuleState );
		WRITE_SHORT( entindex() );
		WRITE_BYTE( m_bActive ? 1 : 0 );
	MESSAGE_END();
	(void)pActivator; (void)pCaller; (void)value;
}

void CEnvFountain::SendValue( void )
{
	MESSAGE_BEGIN( MSG_ALL, gmsgEnvFountain, pev->origin );
		WRITE_SHORT( entindex() );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iCount );
		WRITE_COORD( m_vecDir.x );
		WRITE_COORD( m_vecDir.y );
		WRITE_COORD( m_vecDir.z );
		WRITE_BYTE( (int)m_vecColor.x );
		WRITE_BYTE( (int)m_vecColor.y );
		WRITE_BYTE( (int)m_vecColor.z );
		WRITE_BYTE( m_iSprite );
		WRITE_BYTE( m_iType );
		WRITE_BYTE( m_bActive ? 1 : 0 );
	MESSAGE_END();
}

void CEnvFountain::SendValueTo( CBasePlayer *pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgEnvFountain, pev->origin, pPlayer->edict() );
		WRITE_SHORT( entindex() );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( m_iCount );
		WRITE_BYTE( m_bActive ? 1 : 0 );
	MESSAGE_END();
}
LINK_ENTITY_TO_CLASS( env_fountain, CEnvFountain );

// ===========================================================================
// env_fog — CClientFog (client-driven distance fog volume).
// Verified vs ref/stc_i386.so:
//   env_fog factory thunk @0x19d364 -> GetClassPtr<CClientFog>(...);
//   CClientFog::KeyValue  @0x19c03a: keys "startdist" (rodata 0x1ddcd0) and
//     "enddist" (0x1ddcda), both parsed with atoi() into float members
//     (this+0x19c / this+0x1a0), pkvd->fHandled = TRUE.
//   CClientFog::Spawn     @0x19c106: pev->movetype = 0 (MOVETYPE_NONE),
//     pev->solid = 0 (SOLID_NOT), pev->rendermode = 2 (kRenderTransTexture),
//     pev->renderamt = 0.0f. No Precache, no model, no think.
// The server side is a passive data entity; the client renders the fog from
// the startdist/enddist baseline fields.
// ===========================================================================
TYPEDESCRIPTION CClientFog::m_SaveData[] =
{
	DEFINE_FIELD( CClientFog, m_flStartDist, FIELD_FLOAT ),
	DEFINE_FIELD( CClientFog, m_flEndDist, FIELD_FLOAT ),
};
IMPLEMENT_SAVERESTORE( CClientFog, CBaseEntity );

void CClientFog::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "startdist" ) )
	{
		m_flStartDist = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if( FStrEq( pkvd->szKeyName, "enddist" ) )
	{
		m_flEndDist = (float)atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CClientFog::Spawn( void )
{
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->rendermode = kRenderTransTexture;
	pev->renderamt = 0.0f;
}
LINK_ENTITY_TO_CLASS( env_fog, CClientFog );

// ===========================================================================
// mode_relay
// ===========================================================================
TYPEDESCRIPTION CModeRelay::m_SaveData[] =
{
	DEFINE_ARRAY( CModeRelay, m_iModes, FIELD_INTEGER, 6 ),
	DEFINE_FIELD( CModeRelay, m_iTriggerState, FIELD_INTEGER ),
};
IMPLEMENT_SAVERESTORE( CModeRelay, CPointEntity );

void CModeRelay::Spawn( void )
{
	pev->solid = SOLID_NOT;
}

void CModeRelay::KeyValue( KeyValueData *pkvd )
{
	// Verified keys: mode_a..mode_f + triggerstate (rodata 0x1dd3e0..).
	static const char *const keys[6] =
		{ "mode_a", "mode_b", "mode_c", "mode_d", "mode_e", "mode_f" };
	for( int i = 0; i < 6; i++ )
	{
		if( FStrEq( pkvd->szKeyName, keys[i] ) )
		{
			m_iModes[i] = atoi( pkvd->szValue );
			pkvd->fHandled = TRUE;
			return;
		}
	}
	if( FStrEq( pkvd->szKeyName, "triggerstate" ) )
	{
		m_iTriggerState = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CPointEntity::KeyValue( pkvd );
}

void CModeRelay::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Fires targets when the active mp_gamemode letter matches a set mode;
	// with no mode_* keys set it behaves as a plain relay.
	const char *mode = CVAR_GET_STRING( "mp_gamemode" );
	BOOL anyMode = FALSE;
	BOOL match = FALSE;
	static const char letters[6] = { 'n', 't', 'c', 'k', 'e', 'f' };
	for( int i = 0; i < 6; i++ )
	{
		if( !m_iModes[i] )
			continue;
		anyMode = TRUE;
		if( mode && strchr( mode, letters[i] ) )
			match = TRUE;
	}
	if( !anyMode || match )
		SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
	(void)pCaller; (void)useType; (void)value;
}
LINK_ENTITY_TO_CLASS( mode_relay, CModeRelay );
