// sgtc_vgui.h — StargateTC VGUI-less viewport + screen-status API.
//
// Port of the TFC VGUI analogues the SGTC client.dll needs
// (TeamFortressViewport team/class menus, ScorePanel, MOTD window,
// SpectatorPanel) onto the hlsdk-xash3d VGUI-less base.
//
// Design: false_vgui Panel stubs render nothing in Xash. Visible output
// stays on the stock HUD fallbacks (CHudMenu text menus, CHudMOTD,
// CHudScoreboard, CHudSpectator). This viewport only handles the
// VGUIMenu/TeamNames/ValClass protocol bytes and prints stub guidance;
// it never draws widgets (false_vgui has no Frame/Label/Button/Table).
//
// NEW file — sgtc_* only, no stock edits.
// Direct stub headers used: <VGUI_Panel.h>, <VGUI_App.h>
// (transitive: VGUI.h, VGUI_Dar.h, VGUI_Scheme.h, VGUI_Color.h,
//  VGUI_Cursor.h, VGUI_KeyCode.h, VGUI_MouseCode.h).
#pragma once

#include <VGUI_Panel.h>
#include <VGUI_App.h>

// Menu ids mirror the server VGUIMenu byte (stock TFC values, kept so the
// SGTC server's VGUIMenu/ShowMenu protocol needs no changes).
#define SGTC_MENU_DEFAULT     1
#define SGTC_MENU_TEAM        2
#define SGTC_MENU_CLASS       3
#define SGTC_MENU_MAPBRIEFING 4
#define SGTC_MENU_INTRO       5
#define SGTC_MENU_CLASSHELP   6
#define SGTC_MENU_SPECHELP    9

// SGTC teams: jointeam 1-4 + 5 = auto/spectate (client.dll strings:
// "jointeam 1".."jointeam 5", "jointeam %d").
#define SGTC_TEAM_TORRI   1
#define SGTC_TEAM_MARINE  2
#define SGTC_TEAM_GOAULD  3
#define SGTC_TEAM_UNAS    4
#define SGTC_TEAM_AUTO    5
#define SGTC_MAX_TEAMS    4
// ref m_sTeamNames stride is 0x10 bytes (v1 this+i*0x40 uses 0x40 per team for
// names+slots; 2.0 uses +0x10/entry) — 16 bytes per name, matching ref caps.
#define SGTC_MAX_TEAMNAME 16

class CSgtcViewport : public vgui::Panel
{
public:
	CSgtcViewport( int x, int y, int wide, int tall );

	// Shows stub guidance for a VGUIMenu id; live screens stay on the
	// stock HUD fallbacks (see SgtcVgui_ScreenStatus).
	void ShowVGUIMenu( int menu );
	void HideVGUIMenu();

	// Message handlers — same wire format as stock TeamFortressViewport.
	int MsgFunc_VGUIMenu( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TeamNames( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ValClass( const char *pszName, int iSize, void *pbuf );

	virtual void paintBackground();

	int m_iCurrentMenu;
	int m_iNumberOfTeams;
	char m_sTeamNames[SGTC_TEAM_AUTO + 1][SGTC_MAX_TEAMNAME];
	int m_iValidClasses[5];
};

extern CSgtcViewport *g_pSgtcViewport;

// Create + parent to the engine root panel (no-op when the engine has no
// VGUI root, which is the normal Xash case). Call from HUD_VidInit.
void SgtcVgui_AttachToRoot( void );
void SgtcVgui_Detach( void );

// Team helpers (client.dll protocol: "jointeam %d", "menuselect %d").
const char *SgtcTeamName( int team );
void Sgtc_JoinTeam( int team );

// Per-screen live-vs-stub status: returns "live", "stub", or "unknown".
// Screens: "team", "class", "score", "motd", "spectator", "commandmenu",
// "serverbrowser", "gate".
const char *SgtcVgui_ScreenStatus( const char *screen );
void SgtcVgui_DumpStatus( void );
