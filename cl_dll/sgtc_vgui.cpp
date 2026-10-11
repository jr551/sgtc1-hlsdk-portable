// sgtc_vgui.cpp — StargateTC VGUI-less viewport + screen-status API.
//
// Port of the TFC VGUI analogues the SGTC client.dll needs
// (TeamFortressViewport team/class menus, ScorePanel, MOTD window,
// SpectatorPanel) onto the hlsdk-xash3d VGUI-less base.
//
// STATUS (stub vs live):
//   Root/viewport panel ....... STUB  — false_vgui Panel draws nothing;
//                                the real TFC viewport was a widget host.
//   Team menu (Torri/Marine/  STUB→live — VGUI buttons are stubs; we print
//     Goa'uld/Unas)                 jointeam cmds and show the team's slot.
//   Class menu ................ STUB→live — 12 info/*.txt classes; server
//                                uses ShowMenu/ValClass, so CHudMenu's live
//                                text menu already picks it up.
//   ScorePanel ................ STUB (container) + LIVE CHudScoreboard —
//                                the xash scoreboard hooks ScoreInfo/
//                                TeamInfo/TeamScore directly.
//   MOTD / map briefing ....... LIVE  — CHudMOTD draws motd.txt text.
//   SpectatorPanel ............ LIVE  — CHudSpectator + Spec_Mode titles.
//   CommandMenu ............... STUB  — commandmenu.txt parsed only for
//                                console echo; no CCommandMenu widgets.
//   Server browser ............ STUB  — no browser on VGUI-less client.
//   Gate/DHD overlay .......... STUB  — no client-side VGUI screens for the
//                                gate; DHD UI is world/model + ShowMenu.
//
// false_vgui has no Frame/Label/Button/TablePanel/TextImage, so widget
// code is intentionally NOT ported — only the Panel-derived viewport,
// the VGUIMenu/TeamNames/ValClass protocol handlers and the stub-vs-live
// table are implemented here.
//
// NEW file — sgtc_* only, no stock edits.
// Headers used: hud.h, cl_util.h, parsemsg.h, sgtc_vgui.h
//               (-> VGUI_Panel.h, VGUI_App.h + false_vgui transitive set)

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "sgtc_vgui.h"

#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// globals
// ---------------------------------------------------------------------------

CSgtcViewport *g_pSgtcViewport = NULL;

// Fallback names for jointeam 1-5 (client.dll has "jointeam %d" and the
// engine sends TeamNames for the display text; these match mod order).
static const char *s_sgtcTeamNames[SGTC_TEAM_AUTO + 1] = {
	"",			// 0 unused
	"Torri",		// SGTC_TEAM_TORRI
	"Marine",		// SGTC_TEAM_MARINE
	"Goa'uld",		// SGTC_TEAM_GOAULD
	"Unas",			// SGTC_TEAM_UNAS
	"Auto/Spectate"		// SGTC_TEAM_AUTO
};

// ---------------------------------------------------------------------------
// CSgtcViewport — Panel-derived root; only paintBackground is real on Xash
// ---------------------------------------------------------------------------

CSgtcViewport::CSgtcViewport( int x, int y, int wide, int tall )
	: Panel( x, y, wide, tall )
{
	g_pSgtcViewport = this;
	m_iCurrentMenu = SGTC_MENU_DEFAULT;
	m_iNumberOfTeams = SGTC_MAX_TEAMS;
	memset( m_sTeamNames, 0, sizeof( m_sTeamNames ) );
	memset( m_iValidClasses, 0, sizeof( m_iValidClasses ) );
	for ( int i = 1; i <= SGTC_TEAM_AUTO; i++ )
		strncpy( m_sTeamNames[i], s_sgtcTeamNames[i], SGTC_MAX_TEAMNAME - 1 );
}
// HUD_GetRect() is defined in cdll_int.cpp (stock file — we don't edit it);
// declare it so this translation unit links against the existing helper.
extern int *HUD_GetRect( void );


void CSgtcViewport::paintBackground()
{
	// Same hook as the stock xash TeamFortressViewport: let the engine paint
	// the world behind HUD panels. Safe on false_vgui (virtual, engine call).
	gEngfuncs.VGui_ViewportPaintBackground( HUD_GetRect() );
}

// ---------------------------------------------------------------------------
// menu dispatch — stub widgets would need >200 extra vgui classes; on Xash
// we surface the same screens through the live CHudMenu text menu and
// console guidance instead of dead panels.
// ---------------------------------------------------------------------------

void CSgtcViewport::ShowVGUIMenu( int menu )
{
	m_iCurrentMenu = menu;

	switch ( menu )
	{
	case SGTC_MENU_TEAM:
		gEngfuncs.Con_Printf(
			"[sgtc] Team select — jointeam 1=Torri 2=Marine 3=Goa'uld 4=Unas 5=Auto. "
			"VGUI buttons stubbed; use \"jointeam <n>\" or the ShowMenu text menu.\n" );
		break;
	case SGTC_MENU_CLASS:
		gEngfuncs.Con_Printf(
			"[sgtc] Class select — VGUI class panel stubbed; server ShowMenu "
			"(CHudMenu text menu) is the live path. See info/<class>.txt for blurbs.\n" );
		break;
	case SGTC_MENU_MAPBRIEFING:
		gEngfuncs.Con_Printf( "[sgtc] Map briefing — showing MOTD text (VGUI briefing stubbed).\n" );
		// fall through to MOTD display path
		m_iCurrentMenu = SGTC_MENU_INTRO;
		break;
	case SGTC_MENU_INTRO:
		// MOTD window — live CHudMOTD draws it; just note the stub window.
		if ( gHUD.m_MOTD.m_bShow )
			gEngfuncs.Con_Printf( "[sgtc] MOTD on HUD (VGUI MOTD window stubbed).\n" );
		break;
	case SGTC_MENU_CLASSHELP:
	case SGTC_MENU_SPECHELP:
		gEngfuncs.Con_Printf( "[sgtc] Help panel stubbed (VGUI-less).\n" );
		break;
	default:
		break;
	}
}

void CSgtcViewport::HideVGUIMenu()
{
	m_iCurrentMenu = SGTC_MENU_DEFAULT;
}

// ---------------------------------------------------------------------------
// message handlers — same wire format as stock vgui_TeamFortressViewport
// ---------------------------------------------------------------------------

int CSgtcViewport::MsgFunc_VGUIMenu( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int menu = READ_BYTE();

	if ( menu == SGTC_MENU_MAPBRIEFING )
	{
		// briefing carries the map name; consume it like the stock handler.
		READ_STRING();
	}

	if ( menu < SGTC_MENU_DEFAULT || menu > SGTC_MENU_SPECHELP )
		return 1;

	ShowVGUIMenu( menu );
	return 1;
}

int CSgtcViewport::MsgFunc_TeamNames( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	m_iNumberOfTeams = READ_BYTE();

	for ( int i = 0; i < m_iNumberOfTeams && i <= SGTC_TEAM_AUTO; i++ )
	{
		int teamNum = i + 1;
		gHUD.m_TextMessage.LocaliseTextString(
			READ_STRING(), m_sTeamNames[teamNum], SGTC_MAX_TEAMNAME );
	}
	return 1;
}

int CSgtcViewport::MsgFunc_ValClass( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );
	for ( int i = 0; i < 5; i++ )
		m_iValidClasses[i] = READ_SHORT();
	return 1;
}

// ---------------------------------------------------------------------------
// attach/detach — same shape as stock HUD_VidInit block
// ---------------------------------------------------------------------------

void SgtcVgui_AttachToRoot( void )
{
	vgui::Panel *root = (vgui::Panel *)gEngfuncs.VGui_GetPanel();
	if ( !root )
	{
		// Normal on Xash (no vgui.dll host). Viewport still exists for the
		// protocol handlers; it just has no parent and draws nothing.
		gEngfuncs.Con_Printf( "[sgtc] VGUI root panel absent (VGUI-less client); menus stubbed to HUD.\n" );
		if ( !g_pSgtcViewport )
			g_pSgtcViewport = new CSgtcViewport( 0, 0, 0, 0 );
		return;
	}

	if ( g_pSgtcViewport )
	{
		g_pSgtcViewport->setSize( root->getWide(), root->getTall() );
	}
	else
	{
		g_pSgtcViewport = new CSgtcViewport( 0, 0, root->getWide(), root->getTall() );
		g_pSgtcViewport->setParent( root );
	}
}

void SgtcVgui_Detach( void )
{
	delete g_pSgtcViewport;
	g_pSgtcViewport = NULL;
}

// ---------------------------------------------------------------------------
// team helpers
// ---------------------------------------------------------------------------

const char *SgtcTeamName( int team )
{
	if ( team >= SGTC_TEAM_TORRI && team <= SGTC_TEAM_AUTO )
		return g_pSgtcViewport && g_pSgtcViewport->m_sTeamNames[team][0]
			? g_pSgtcViewport->m_sTeamNames[team]
			: s_sgtcTeamNames[team];
	return "Unknown";
}

void Sgtc_JoinTeam( int team )
{
	if ( team < SGTC_TEAM_TORRI || team > SGTC_TEAM_AUTO )
		return;
	char cmd[32];
	snprintf( cmd, sizeof( cmd ), "jointeam %d", team );
	ClientCmd( cmd );
}

// ---------------------------------------------------------------------------
// stub-vs-live status table — the acceptance report
// ---------------------------------------------------------------------------

const char *SgtcVgui_ScreenStatus( const char *screen )
{
	if ( !stricmp( screen, "team" ) )		return "stub";	// VGUI buttons → jointeam console
	if ( !stricmp( screen, "class" ) )		return "live";	// ShowMenu/CHudMenu text menu
	if ( !stricmp( screen, "score" ) )		return "live";	// CHudScoreboard
	if ( !stricmp( screen, "motd" ) )		return "live";	// CHudMOTD
	if ( !stricmp( screen, "spectator" ) )	return "live";	// CHudSpectator
	if ( !stricmp( screen, "commandmenu" ) )	return "stub";	// commandmenu.txt echo only
	if ( !stricmp( screen, "serverbrowser" ) )return "stub";
	if ( !stricmp( screen, "gate" ) )		return "stub";	// no client VGUI gate screens
	if ( !stricmp( screen, "progress" ) )	return "live";	// CHudSgtc @0x1002abe2 HUD bridge
	return "unknown";
}

void SgtcVgui_DumpStatus( void )
{
	static const char *screens[] = {
		"team", "class", "score", "motd", "spectator", "progress",
		"commandmenu", "serverbrowser", "gate", NULL
	};
	gEngfuncs.Con_Printf( "[sgtc] VGUI screen status (live=draws in Xash, stub=false_vgui no-op):\n" );
	for ( int i = 0; screens[i]; i++ )
		gEngfuncs.Con_Printf( "  %-14s : %s\n", screens[i], SgtcVgui_ScreenStatus( screens[i] ) );
}
