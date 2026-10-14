// sgtc_vgui_panels.cpp — StargateTC VGUI menus 10, 11, 12 and 60.
//
// Port of the four SGTC-only CMenuPanel screens the mod's client.dll adds to
// the TFC viewport (briefing, team+class select, weapon loadout, pack select).
// Evidence (ref = stargatetc/cl_dlls/client.dll, sha256 9ec27e22…4fb3, Ghidra
// 12.1.4):
//
//   menu 10  CSgtcBriefingPanel   ctor FUN_1003f640 @0x1003f640,
//                                 refresh FUN_100404a0 @0x100404a0 (vtable
//                                 +0x210, called from Open @0x10040740),
//                                 reset FUN_10040433 @0x10040433.
//                                 Shown by ShowVGUIMenu case 10 via
//                                 FUN_100482af (pre-created panel).
//   menu 12  CSgtcWeaponSelPanel  ctor FUN_100261c0 @0x100261c0,
//                                 Reset FUN_10029888, Open FUN_1002982b,
//                                 Update FUN_10028323, shown by case 0xc via
//                                 FUN_10047ebb (viewport +0xa9c); Apply sends
//                                 "gond4h9wp <28 values>" (0x100b3ee4).
//   menu 60  CSgtcPackSelPanel    ctor FUN_10029ea0 @0x10029ea0, shown by
//                                 case 0x3c via FUN_10047fb8 (viewport
//                                 +0xaa0); buttons #PACK_1..#PACK_4,
//                                 #Annuler, #Apply.
//   menu 11  CSgtcTeamClassPanel  ctor FUN_1002bec0 @0x1002bec0,
//                                 refresh FUN_1002edd2 @0x1002edd2 (vtable
//                                 +0x210, called from Open @0x1002ef88),
//                                 reset FUN_1002ed4a @0x1002ed4a.
//                                 Shown by ShowVGUIMenu case 0xb via
//                                 FUN_100483ac (pre-created panel).
//
// Verified strings (search_strings/read_bytes on the ref):
//   0x100b4cb4 "title", 0x100b4cbc "#objectif_manquant", 0x100b4cd0/4 "#Ok"
//   0x100b4164 "Identifiez-vous", 0x100b41a0 "Choississez d'abords une classe"
//   0x100b41c0 "Stargate TC , B.I.G. & Endolaylith"
//   0x100b4174 "fichier sg1 absent", 0x100b4188 "fichier goauld absent"
//   0x100b43a0 "fichier absent %s"
//   0x100b41e4 "#Torri", 0x100b41ec "#Goauld", 0x100b41f4 "#Annuler",
//   0x100b4200 "#AutoAssign", 0x100b420c "#Spectateur"
//   0x100b4218 "spectate", 0x100b4224 "autopick", 0x100b42d8 "classe %d",
//   0x100b42e4 "jointeam %d"
//   0x100b4230 "oneill".."esclave" (info file base names),
//   0x100b4280 "#Oneill"..#Esclave (localised button labels),
//   0x100b42f8.. "info_oneill.txt" .. "info_esclave.txt",
//   0x100b43c4 "info_equipe.txt", 0x100b43d4 "info_sg1.txt",
//   0x100b43e4 "info_goauld.txt"
//
// Verified commands (ref ctor assembly):
//   class cell (row 0..1, col 0..4): "classe %d" with col+1 (1..5) and
//   "jointeam %d" with row+1 (1 = Torri row, 2 = Goauld row), then close.
//   AutoAssign "autopick", Spectateur "spectate", Annuler close.
//   Those jointeam ids are what our server accepts (dlls/sgtc_rules.h
//   SGTC_TEAM_TORRI 1 / SGTC_TEAM_GOAULD 2, CStargateTeamDM::ClientCommand).
//
// Both panels need the real widget stack (Label/Button/ScrollPanel/TextPanel)
// and the real TeamFortressViewport, so this file compiles only when the tree
// ships them (SGTC_REAL_VGUI, see sgtc_vgui.h); the VGUI-less tree keeps the
// console stubs in sgtc_vgui.cpp and this translation unit is empty.
//
// NEW file — sgtc_* only, no stock edits.
// Headers used: hud.h, cl_util.h, camera.h, kbutton.h, const.h, sgtc_vgui.h,
//               vgui_int.h, vgui_TeamFortressViewport.h

#include "hud.h"
#include "cl_util.h"
#include "camera.h"
#include "kbutton.h"
#include "const.h"
#include "sgtc_vgui.h"

#ifdef SGTC_REAL_VGUI

#include "vgui_int.h"
#include "vgui_TeamFortressViewport.h"
#include "demo_api.h"
#include <VGUI_TextImage.h>		// gEngfuncs.pDemoAPI->IsPlayingback()

#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// shared helpers
// ---------------------------------------------------------------------------

// gEngfuncs.COM_LoadFile wrapper (ref: (*DAT_100e7864)(name, 5, NULL)).
// Caller frees with COM_FreeFile.
static char *SgtcPanels_LoadFile( const char *pszFile )
{
	return (char *)gEngfuncs.COM_LoadFile( (char *)pszFile, 5, NULL );
}

// Show a SGTC panel through the real viewport, the same way
// TeamFortressViewport::ShowVGUIMenu does for the stock menus: set the id,
// activate, parent, then either open it or queue it behind the current menu
// (ids already queued are ignored — ref dedup).
static void SgtcPanels_ShowMenu( CMenuPanel *pNewMenu, int iMenu )
{
	if ( !pNewMenu || !gViewPort )
		return;

	// Don't open menus in demo playback (ref ShowVGUIMenu early-out).
	if ( gEngfuncs.pDemoAPI->IsPlayingback() )
		return;

	gViewPort->HideCommandMenu();
	pNewMenu->SetMenuID( iMenu );
	pNewMenu->SetActive( true );
	pNewMenu->setParent( gViewPort );

	if ( gViewPort->m_pCurrentMenu == pNewMenu )
	{
		// Re-sent while already up: re-open (ref pre-created panels are reused).
		pNewMenu->Open();
		gViewPort->UpdateCursorState();
		return;
	}

	for ( CMenuPanel *pMenu = gViewPort->m_pCurrentMenu; pMenu; pMenu = pMenu->GetNextMenu() )
	{
		if ( pMenu->GetMenuID() == iMenu )
			return;
	}

	if ( gViewPort->m_pCurrentMenu )
	{
		gViewPort->m_pCurrentMenu->SetNextMenu( pNewMenu );
	}
	else
	{
		gViewPort->SetCurrentMenu( pNewMenu );	// sets m_pCurrentMenu + Open()
		gViewPort->UpdateCursorState();
	}
}

//============================================================================
// Menu 10 — briefing/objective window (ref FUN_1003f640 / FUN_100404a0)
//============================================================================
class CSgtcBriefingPanel : public CMenuPanel
{
public:
	CSgtcBriefingPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );

	virtual void Open( void );	// refresh labels + briefing, then base Open
	virtual void Reset( void );	// re-arm the briefing load latch

private:
	void Refresh( void );

	CTransparentPanel *m_pWindow;
	Label *m_pTitle;
	Label *m_pServerName;
	Label *m_pMapName;
	CTFScrollPanel *m_pScrollPanel;
	TextPanel *m_pBriefing;
	CommandButton *m_pOKButton;
	bool m_bBriefingLoaded;
};

CSgtcBriefingPanel::CSgtcBriefingPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall )
	: CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
	// ref schemes: "Title Font" / "Briefing Text" (the third handle it looks up,
	// "Team Info Text", is unused by the briefing panel).
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hBriefingText = pSchemes->getSchemeHandle( "Briefing Text" );
	int r, g, b, a;

	// Background TextPanel + LineBorder (ref +0xf8: 0x15,0x15, W-0x2a, H-0x3c)
	TextPanel *pBackground = new TextPanel( "", XRES( 21 ), YRES( 21 ),
		ScreenWidth - XRES( 42 ), ScreenHeight - YRES( 36 ) );
	pBackground->setParent( this );
	pBackground->setBorder( new LineBorder( Color( 255 * 0.7, 170 * 0.7, 0, 0 ) ) );

	// Inner window (ref +0xdc) with its own LineBorder
	m_pWindow = new CTransparentPanel( 255, XRES( 42 ), YRES( 42 ),
		ScreenWidth - XRES( 84 ), ScreenHeight - YRES( 84 ) );
	m_pWindow->setParent( this );
	m_pWindow->setBorder( new LineBorder( Color( 255 * 0.7, 170 * 0.7, 0, 0 ) ) );

	// Title (ref +0xec, Title Font, ScreenWidth/2-200 @0x23; the ref passes the
	// literal string "title" — kept verbatim)
	m_pTitle = new Label( "", ScreenWidth / 2 - XRES( 200 ), YRES( 35 ) );
	m_pTitle->setParent( m_pWindow );
	m_pTitle->setFont( pSchemes->getFont( hTitleScheme ) );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setBgColor( r, g, b, a );
	m_pTitle->setText( "%s", "title" );
	// The ref ctor hides this label right after creating it (vtable+0x24 =
	// Panel::setVisible, Panel has exactly 127 virtuals so +0x24 is Panel slot 9
	// = setVisible): the orig briefing panel shows no title text — verified on
	// the running orig client (only the blue server/map labels are visible).
	m_pTitle->setVisible( false );

	// Server name (ref +0xe8) and map name (ref +0xe4) labels: Title Font,
	// fg 0x55,0x73,0xb9, bg black, content alignment 3. In the SDK enum the mod
	// was built against that value is Label::a_west (a_northwest=0, a_north,
	// a_northeast, a_west) — not centred, contrary to the port spec's note.
	m_pServerName = new Label( "", XRES( 42 ), YRES( 62 ) );
	m_pServerName->setParent( m_pWindow );
	m_pServerName->setFont( pSchemes->getFont( hTitleScheme ) );
	m_pServerName->setFgColor( 0x55, 0x73, 0xb9, 0 );
	m_pServerName->setBgColor( 0, 0, 0, 0xff );
	m_pServerName->setContentAlignment( vgui::Label::a_west );

	m_pMapName = new Label( "", XRES( 42 ), YRES( 82 ) );
	m_pMapName->setParent( m_pWindow );
	m_pMapName->setFont( pSchemes->getFont( hTitleScheme ) );
	m_pMapName->setFgColor( 0x55, 0x73, 0xb9, 0 );
	m_pMapName->setBgColor( 0, 0, 0, 0xff );
	m_pMapName->setContentAlignment( vgui::Label::a_west );

	// Scroll panel (ref +0xd8) + TextPanel (ref +0xf0, "Briefing Text" scheme,
	// fg 0x55,0x73,0xb9) holding the map briefing. Default text is the localised
	// "#objectif_manquant" key, exactly as the ref sets it
	// (FUN_1007ad9c(s__objectif_manquant_100b4cbc)).
	//
	// Ref geometry (FUN_1003f640): the TextPanel is built with the *scroll
	// panel's* own size and a fixed 0x2d (45 px) y offset inside the scroll
	// client, and is never resized afterwards. The client is therefore larger
	// than the client clip on both axes, so ScrollPanel's auto-visible mode
	// shows BOTH scrollbars at runtime — the orig panel shows the vertical bar
	// on the right and the horizontal bar at the bottom of the text area, even
	// though the ctor calls setScrollBarVisible(0,0) (ScrollPanel slot +0x1fc =
	// setScrollBarVisible; Panel has 127 virtuals, so +0x1fc is ScrollPanel's
	// first own slot, ahead of setScrollBarAutoVisible at +0x200).
	const int iScrollWide = ScreenWidth - XRES( 84 );
	const int iScrollTall = ScreenHeight - YRES( 190 );

	m_pScrollPanel = new CTFScrollPanel( XRES( 42 ), YRES( 102 ), iScrollWide, iScrollTall );
	m_pScrollPanel->setParent( m_pWindow );
	m_pScrollPanel->setScrollBarVisible( false, false );

	m_pBriefing = new TextPanel( "", 0, 45, iScrollWide, iScrollTall );
	m_pBriefing->setParent( m_pScrollPanel->getClient() );
	m_pBriefing->setFont( pSchemes->getFont( hBriefingText ) );
	pSchemes->getFgColor( hBriefingText, r, g, b, a );
	m_pBriefing->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hBriefingText, r, g, b, a );
	m_pBriefing->setBgColor( r, g, b, a );
	m_pBriefing->setText( gHUD.m_TextMessage.BufferedLocaliseTextString( "#objectif_manquant" ) );

	// OK button (ref +0xd4, localised "#Ok", close handler id 0xb)
	m_pOKButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Ok" ),
		XRES( 42 ), ScreenHeight - YRES( 72 ), CMENU_SIZE_X, BUTTON_SIZE_Y );
	m_pOKButton->setParent( this );
	m_pOKButton->addActionSignal( new CMenuHandler_TextWindow( HIDE_TEXTWINDOW ) );

	m_bBriefingLoaded = false;

	m_pScrollPanel->validate();
}

void CSgtcBriefingPanel::Open( void )
{
	Refresh();
	CMenuPanel::Open();
}

void CSgtcBriefingPanel::Reset( void )
{
	CMenuPanel::Reset();
	m_bBriefingLoaded = false;
}

void CSgtcBriefingPanel::Refresh( void )
{
	// Server name arrives in the ServerName usermsg (viewport m_szServerName,
	// MsgFunc_ServerName @0x1004a0a9); the ref fills the label from the same
	// value (FUN_100404a0 "%s").
	m_pServerName->setText( "%s", gViewPort->m_szServerName );

	if ( m_bBriefingLoaded )
	{
		m_pScrollPanel->validate();
		return;
	}

	// Map name + briefing file from the level name (ref FUN_100404a0):
	// "maps/<map>.bsp" → map label "<map>", briefing file "maps/<map>.txt".
	const char *pszLevel = gEngfuncs.pfnGetLevelName();
	if ( !pszLevel || !pszLevel[0] )
		return;

	const char *pszBase = pszLevel;
	const char *pszSlash = strrchr( pszLevel, '/' );
	if ( !pszSlash )
		pszSlash = strrchr( pszLevel, '\\' );
	if ( pszSlash )
		pszBase = pszSlash + 1;

	char szMapName[64];
	strlcpy( szMapName, pszBase, sizeof( szMapName ) );
	char *pch = strrchr( szMapName, '.' );
	if ( pch )
		*pch = 0;
	m_pMapName->setText( "%s", szMapName );

	char szBriefingFile[256];
	strlcpy( szBriefingFile, pszLevel, sizeof( szBriefingFile ) );
	pch = strrchr( szBriefingFile, '.' );
	if ( pch )
		*pch = 0;
	strlcat( szBriefingFile, ".txt", sizeof( szBriefingFile ) );

	char *pfile = SgtcPanels_LoadFile( szBriefingFile );
	if ( pfile )
	{
		// Missing briefing keeps the localised "#objectif_manquant" default,
		// like the ref (it only calls setText when COM_LoadFile succeeded).
		m_pBriefing->setText( pfile );
		gEngfuncs.COM_FreeFile( pfile );
	}
	m_bBriefingLoaded = true;

	// Ref never resizes the text panel; ScrollPanel's auto-visible mode picks up
	// the overflow on validate().
	m_pScrollPanel->validate();

}

//============================================================================
// Menu 11 — "Identifiez-vous" team + class select (ref FUN_1002bec0 / FUN_1002edd2)
//============================================================================

#define SGTC_TEAMCLASS_TEAMS		2	// Torri column, Goauld column
#define SGTC_TEAMCLASS_PER_TEAM		5	// class cells per column
#define SGTC_TEAMCLASS_COUNT		( SGTC_TEAMCLASS_TEAMS * SGTC_TEAMCLASS_PER_TEAM )
#define SGTC_TEAMCLASS_INFO_LEN		1024

// ref class tables: "oneill".."esclave" (0x100b4230) are the info file base
// names, "#Oneill".."#Esclave" (0x100b4280) the localised cell labels.
struct SgtcClassEntry
{
	const char *pszLabel;	// titles.txt key used as the button text
	const char *pszInfo;	// info file loaded into the cell's info panel
};

static const SgtcClassEntry s_SgtcClasses[SGTC_TEAMCLASS_COUNT] =
{
	{ "#Oneill",  "info_oneill.txt" },	// row 0 (Torri): 0x100b42f8
	{ "#Carter",  "info_carter.txt" },
	{ "#Jackson", "info_jackson.txt" },
	{ "#Tealc",   "info_tealc.txt" },
	{ "#Soldat",  "info_soldat.txt" },
	{ "#Dieu",    "info_dieu.txt" },	// row 1 (Goauld): 0x100b434c
	{ "#Anubis",  "info_anubis.txt" },
	{ "#Jaffa",   "info_jaffa.txt" },
	{ "#Prima",   "info_prima.txt" },
	{ "#Esclave", "info_esclave.txt" }
};

class CSgtcTeamClassPanel : public CMenuPanel
{
public:
	CSgtcTeamClassPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );

	virtual void Reset( void );
	virtual void SetActiveInfo( int iInput );	// hovered class cell (0..9)

	void SetActiveTeam( int iTeam );		// hovered/selected column (0..1)

private:
	CTransparentPanel *m_pTeamWindow[SGTC_TEAMCLASS_TEAMS];
	Label *m_pTeamHeader[SGTC_TEAMCLASS_TEAMS];
	TextPanel *m_pTeamInfo[SGTC_TEAMCLASS_TEAMS];	// info_sg1 / info_goauld
	CommandButton *m_pClassButton[SGTC_TEAMCLASS_COUNT];
	TextPanel *m_pClassInfo[SGTC_TEAMCLASS_COUNT];	// one visible at a time
	CommandButton *m_pTeamButton[SGTC_TEAMCLASS_TEAMS];
	CommandButton *m_pCancelButton;
	CommandButton *m_pAutoButton;
	CommandButton *m_pSpecButton;
	TextPanel *m_pFooter;				// info_equipe.txt
	char m_szClassInfo[SGTC_TEAMCLASS_COUNT][SGTC_TEAMCLASS_INFO_LEN];
	int m_iCurrentClass;
	int m_iCurrentTeam;
};

// Hover handlers (ref pcharNode<1>: class cells carry the cell index, the team
// buttons carry 10/11).
class CSgtcHandler_ClassOver : public InputSignal
{
public:
	CSgtcHandler_ClassOver( CSgtcTeamClassPanel *pPanel, int iClass )
	{
		m_pPanel = pPanel;
		m_iClass = iClass;
	}

	void cursorEntered( Panel *panel );

	void cursorMoved( int x, int y, Panel *panel ) {}
	void mousePressed( MouseCode code, Panel *panel ) {}
	void mouseReleased( MouseCode code, Panel *panel ) {}
	void mouseDoublePressed( MouseCode code, Panel *panel ) {}
	void cursorExited( Panel *panel ) {}
	void mouseWheeled( int delta, Panel *panel ) {}
	void keyPressed( KeyCode code, Panel *panel ) {}
	void keyTyped( KeyCode code, Panel *panel ) {}
	void keyReleased( KeyCode code, Panel *panel ) {}
	void keyFocusTicked( Panel *panel ) {}

private:
	CSgtcTeamClassPanel *m_pPanel;
	int m_iClass;
};

class CSgtcHandler_TeamOver : public InputSignal
{
public:
	CSgtcHandler_TeamOver( CSgtcTeamClassPanel *pPanel, int iTeam )
	{
		m_pPanel = pPanel;
		m_iTeam = iTeam;
	}

	void cursorEntered( Panel *panel );

	void cursorMoved( int x, int y, Panel *panel ) {}
	void mousePressed( MouseCode code, Panel *panel ) {}
	void mouseReleased( MouseCode code, Panel *panel ) {}
	void mouseDoublePressed( MouseCode code, Panel *panel ) {}
	void cursorExited( Panel *panel ) {}
	void mouseWheeled( int delta, Panel *panel ) {}
	void keyPressed( KeyCode code, Panel *panel ) {}
	void keyTyped( KeyCode code, Panel *panel ) {}
	void keyReleased( KeyCode code, Panel *panel ) {}
	void keyFocusTicked( Panel *panel ) {}

private:
	CSgtcTeamClassPanel *m_pPanel;
	int m_iTeam;
};

CSgtcTeamClassPanel::CSgtcTeamClassPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall )
	: CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
	// ref schemes: "Title Font" (title + team headers) / "Briefing Text" (all
	// info panels, fg 0x55,0x73,0xb9, bg from the scheme).
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hBriefingText = pSchemes->getSchemeHandle( "Briefing Text" );
	int r, g, b, a;

	// Title (ref +0x1b0, literal French string, not localised)
	Label *pTitle = new Label( "", XRES( 40 ), YRES( 20 ) );
	pTitle->setParent( this );
	pTitle->setFont( pSchemes->getFont( hTitleScheme ) );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	pTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	pTitle->setBgColor( r, g, b, a );
	pTitle->setText( "%s", "Identifiez-vous" );

	const int iWindowWide = ( ScreenWidth - XRES( 104 ) ) / 2;
	const int iWindowTall = ScreenHeight - YRES( 156 );
	const int iClassWide = XRES( 48 );
	char *pfile = NULL;

	// Per-team columns: Torri (ref window +0x114) and Goauld (+0x118), each with
	// its team header label, its team info panel and its 5 class cells.
	for ( int iTeam = 0; iTeam < SGTC_TEAMCLASS_TEAMS; iTeam++ )
	{
		const int iWindowX = XRES( 40 ) + iTeam * ( iWindowWide + XRES( 24 ) );

		m_pTeamWindow[iTeam] = new CTransparentPanel( 255, iWindowX, YRES( 48 ), iWindowWide, iWindowTall );
		m_pTeamWindow[iTeam]->setParent( this );
		m_pTeamWindow[iTeam]->setBorder( new LineBorder( Color( 255 * 0.7, 170 * 0.7, 0, 0 ) ) );

		// Team header (ref +0x180 "+#Torri" / +0x184 "+#Goauld")
		m_pTeamHeader[iTeam] = new Label( "", XRES( 6 ), YRES( 4 ) );
		m_pTeamHeader[iTeam]->setParent( m_pTeamWindow[iTeam] );
		m_pTeamHeader[iTeam]->setFont( pSchemes->getFont( hTitleScheme ) );
		pSchemes->getFgColor( hTitleScheme, r, g, b, a );
		m_pTeamHeader[iTeam]->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hTitleScheme, r, g, b, a );
		m_pTeamHeader[iTeam]->setBgColor( r, g, b, a );
		m_pTeamHeader[iTeam]->setText( "%s", gHUD.m_TextMessage.BufferedLocaliseTextString(
			iTeam == 0 ? "#Torri" : "#Goauld" ) );

		// Team info panel (ref +0x148 info_sg1.txt / +0x14c info_goauld.txt).
		// Fallbacks are the ref's own literals.
		m_pTeamInfo[iTeam] = new TextPanel( "", XRES( 6 ), YRES( 22 ), iWindowWide - XRES( 12 ), YRES( 64 ) );
		m_pTeamInfo[iTeam]->setParent( m_pTeamWindow[iTeam] );
		m_pTeamInfo[iTeam]->setFont( pSchemes->getFont( hBriefingText ) );
		pSchemes->getFgColor( hBriefingText, r, g, b, a );
		m_pTeamInfo[iTeam]->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hBriefingText, r, g, b, a );
		m_pTeamInfo[iTeam]->setBgColor( r, g, b, a );
		m_pTeamInfo[iTeam]->setText( iTeam == 0 ? "fichier sg1 absent" : "fichier goauld absent" );

		pfile = SgtcPanels_LoadFile( iTeam == 0 ? "info_sg1.txt" : "info_goauld.txt" );
		if ( pfile )
		{
			m_pTeamInfo[iTeam]->setText( pfile );
			gEngfuncs.COM_FreeFile( pfile );
		}
	}

	// Class cells (ref: 5 columns x 2 rows; cell (row,col) = index row*5+col).
	for ( int iClass = 0; iClass < SGTC_TEAMCLASS_COUNT; iClass++ )
	{
		const int iTeam = iClass / SGTC_TEAMCLASS_PER_TEAM;	// row: 0 Torri, 1 Goauld
		const int iCol = iClass % SGTC_TEAMCLASS_PER_TEAM;
		char szCommand[MAX_COMMAND_SIZE];

		// Cell button, parented to its team window (ref: buttons live in the
		// column's window). Text is the localised "#<Class>" key.
		m_pClassButton[iClass] = new CommandButton(
			gHUD.m_TextMessage.BufferedLocaliseTextString( s_SgtcClasses[iClass].pszLabel ),
			XRES( 6 ) + iCol * ( iClassWide + XRES( 4 ) ), YRES( 92 ),
			iClassWide, BUTTON_SIZE_Y );
		m_pClassButton[iClass]->setParent( m_pTeamWindow[iTeam] );

		// ref signals: "classe %d" (col+1 = class slot 1..5), "jointeam %d"
		// (row+1 = team id 1 Torri / 2 Goauld) and a close signal.
		snprintf( szCommand, sizeof( szCommand ), "classe %d", iCol + 1 );
		m_pClassButton[iClass]->addActionSignal( new CMenuHandler_StringCommand( szCommand ) );

		snprintf( szCommand, sizeof( szCommand ), "jointeam %d", iTeam + 1 );
		m_pClassButton[iClass]->addActionSignal( new CMenuHandler_StringCommand( szCommand ) );

		m_pClassButton[iClass]->addActionSignal( new CMenuHandler_TextWindow( HIDE_TEXTWINDOW ) );

		// Mouse-over shows the class blurb (ref pcharNode<1>(this, iClass)).
		m_pClassButton[iClass]->addInputSignal( new CSgtcHandler_ClassOver( this, iClass ) );

		// Per-cell info panel (ref +0x150 array), default ref text.
		m_pClassInfo[iClass] = new TextPanel( "", XRES( 6 ), YRES( 128 ),
			iWindowWide - XRES( 12 ), iWindowTall - YRES( 136 ) );
		m_pClassInfo[iClass]->setParent( m_pTeamWindow[iTeam] );
		m_pClassInfo[iClass]->setFont( pSchemes->getFont( hBriefingText ) );
		pSchemes->getFgColor( hBriefingText, r, g, b, a );
		m_pClassInfo[iClass]->setFgColor( r, g, b, a );
		pSchemes->getBgColor( hBriefingText, r, g, b, a );
		m_pClassInfo[iClass]->setBgColor( r, g, b, a );
		m_pClassInfo[iClass]->setVisible( false );

		// Load the class blurb once; missing file keeps the ref's
		// "fichier absent %s" (with the file name substituted).
		pfile = SgtcPanels_LoadFile( s_SgtcClasses[iClass].pszInfo );
		if ( pfile )
		{
			strlcpy( m_szClassInfo[iClass], pfile, SGTC_TEAMCLASS_INFO_LEN );
			gEngfuncs.COM_FreeFile( pfile );
		}
		else
		{
			snprintf( m_szClassInfo[iClass], SGTC_TEAMCLASS_INFO_LEN,
				"fichier absent %s", s_SgtcClasses[iClass].pszInfo );
		}
	}

	// Team buttons (ref +0xd4 Torri / +0xd8 Goauld, localised "#Torri"/"#Goauld")
	// with the jointeam ids our server accepts (1 = Torri, 2 = Goauld).
	for ( int iTeam = 0; iTeam < SGTC_TEAMCLASS_TEAMS; iTeam++ )
	{
		char szCommand[32];
		snprintf( szCommand, sizeof( szCommand ), "jointeam %d",
			iTeam == 0 ? SGTC_SRV_TEAM_TORRI : SGTC_SRV_TEAM_GOAULD );

		m_pTeamButton[iTeam] = new CommandButton(
			gHUD.m_TextMessage.BufferedLocaliseTextString( iTeam == 0 ? "#Torri" : "#Goauld" ),
			XRES( 40 ) + iTeam * XRES( 108 ), ScreenHeight - YRES( 56 ),
			XRES( 100 ), BUTTON_SIZE_Y );
		m_pTeamButton[iTeam]->setParent( this );
		m_pTeamButton[iTeam]->addActionSignal( new CMenuHandler_StringCommandWatch( szCommand, true ) );
		m_pTeamButton[iTeam]->addInputSignal( new CSgtcHandler_TeamOver( this, iTeam ) );
	}

	// Bottom buttons (ref +0x104 Annuler / +0x10c AutoAssign / +0x108 Spectateur)
	m_pCancelButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Annuler" ),
		XRES( 296 ), ScreenHeight - YRES( 56 ), XRES( 100 ), BUTTON_SIZE_Y );
	m_pCancelButton->setParent( this );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow( HIDE_TEXTWINDOW ) );

	m_pAutoButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#AutoAssign" ),
		XRES( 404 ), ScreenHeight - YRES( 56 ), XRES( 100 ), BUTTON_SIZE_Y );
	m_pAutoButton->setParent( this );
	m_pAutoButton->addActionSignal( new CMenuHandler_StringCommand( "autopick", 1 ) );

	m_pSpecButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Spectateur" ),
		XRES( 512 ), ScreenHeight - YRES( 56 ), XRES( 100 ), BUTTON_SIZE_Y );
	m_pSpecButton->setParent( this );
	m_pSpecButton->addActionSignal( new CMenuHandler_StringCommand( "spectate", 1 ) );

	// Footer panel (ref +0x17c info_equipe.txt, fallback literal)
	m_pFooter = new TextPanel( "", XRES( 40 ), ScreenHeight - YRES( 104 ),
		ScreenWidth - XRES( 80 ), YRES( 40 ) );
	m_pFooter->setParent( this );
	m_pFooter->setFont( pSchemes->getFont( hBriefingText ) );
	pSchemes->getFgColor( hBriefingText, r, g, b, a );
	m_pFooter->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hBriefingText, r, g, b, a );
	m_pFooter->setBgColor( r, g, b, a );
	m_pFooter->setText( "Stargate TC , B.I.G. & Endolaylith" );

	pfile = SgtcPanels_LoadFile( "info_equipe.txt" );
	if ( pfile )
	{
		m_pFooter->setText( pfile );
		gEngfuncs.COM_FreeFile( pfile );
	}

	Reset();
}

void CSgtcTeamClassPanel::Reset( void )
{
	// ref FUN_1002ed4a: no team/class selected, all cell info panels hidden.
	CMenuPanel::Reset();
	m_iCurrentClass = 0;
	m_iCurrentTeam = -1;

	for ( int i = 0; i < SGTC_TEAMCLASS_COUNT; i++ )
		m_pClassInfo[i]->setVisible( false );

	// ref FUN_1002edd2 also clears the armed state of every button.
	for ( int i = 0; i < SGTC_TEAMCLASS_COUNT; i++ )
		m_pClassButton[i]->setArmed( false );
	for ( int i = 0; i < SGTC_TEAMCLASS_TEAMS; i++ )
		m_pTeamButton[i]->setArmed( false );
	m_pCancelButton->setArmed( false );
	m_pAutoButton->setArmed( false );
	m_pSpecButton->setArmed( false );

	// Deviation from the ref: FUN_1002edd2 hides the team window that is not
	// selected, which also hides its 5 class cells — the ref panel therefore
	// opens with no class grid at all until a team button is hovered. The port
	// keeps both columns on screen (all 10 cells stay clickable) and only
	// switches the per-class blurb.
	for ( int i = 0; i < SGTC_TEAMCLASS_TEAMS; i++ )
		m_pTeamWindow[i]->setVisible( true );
}

// Mouse is over a class cell: bring up its blurb (ref SetActiveInfo/pcharNode)
void CSgtcTeamClassPanel::SetActiveInfo( int iInput )
{
	if ( iInput < 0 || iInput >= SGTC_TEAMCLASS_COUNT )
		return;

	m_iCurrentClass = iInput;
	m_iCurrentTeam = iInput / SGTC_TEAMCLASS_PER_TEAM;

	for ( int i = 0; i < SGTC_TEAMCLASS_COUNT; i++ )
		m_pClassInfo[i]->setVisible( i == iInput );

	m_pClassInfo[iInput]->setText( m_szClassInfo[iInput] );
}

// Mouse is over a team button: clear the other column's blurb so the two info
// areas never disagree (ref FUN_1002edd2 hides the other column entirely).
void CSgtcTeamClassPanel::SetActiveTeam( int iTeam )
{
	m_iCurrentTeam = iTeam;

	if ( iTeam < 0 || iTeam >= SGTC_TEAMCLASS_TEAMS )
		return;

	for ( int i = 0; i < SGTC_TEAMCLASS_COUNT; i++ )
	{
		if ( i / SGTC_TEAMCLASS_PER_TEAM != iTeam )
			m_pClassInfo[i]->setVisible( false );
	}
}

void CSgtcHandler_ClassOver::cursorEntered( Panel *panel )
{
	if ( m_pPanel )
		m_pPanel->SetActiveInfo( m_iClass );
}

void CSgtcHandler_TeamOver::cursorEntered( Panel *panel )
{
	if ( m_pPanel )
		m_pPanel->SetActiveTeam( m_iTeam );
}


//============================================================================
// Menu 12 — weapon loadout ("Identifiez-vous" + weapon picker).
// Ref: ctor FUN_100261c0 @0x100261c0, Reset FUN_10029888 @0x10029888 (vtable
// +0x1fc), Open FUN_1002982b @0x1002982b (vtable +0x200), Update FUN_10028323
// @0x10028323 (vtable +0x210), show helper FUN_10047ebb @0x10047ebb (viewport
// +0xa9c). Apply sends the ref's command word "gond4h9wp" (0x100b3ee4)
// followed by the 28 per-slot values — our server's handler
// (dlls/sgtc_player.cpp:1151, ref @0xcb34c) reads exactly
// CMD_ARGV(1..28) -> m_iSgtcGondArgs[] and SGTC_GiveFromMenu grants the
// matching spawn weapons.
//============================================================================

#define SGTC_WPSEL_SLOTS		28
#define SGTC_WPSEL_PRIMARY		14	// slots 0..13 (ref buttons at +0x200)
#define SGTC_WPSEL_SECONDARY	14	// slots 14..27 (ref buttons at +0x238)

// ref slot labels @0x100b3940 ("glstaff".."medicgoa") and the localised button
// keys @0x100b3a0c.. ("#WEAPON_GLSTAFF".."#WEAPON_MEDICGOA"); the per-slot info
// files are the ref's "info_<label>.txt" (Update switch @0x100b3cd0..).
struct SgtcWeaponEntry
{
	const char *pszLabel;	// ref slot label (also the info file base name)
	const char *pszKey;	// titles.txt key used as the button text
};

static const SgtcWeaponEntry s_SgtcWeapons[SGTC_WPSEL_SLOTS] =
{
	{ "glstaff",  "#WEAPON_GLSTAFF" },
	{ "usas",     "#WEAPON_USAS12" },
	{ "batsup",   "#WEAPON_BATSUP" },
	{ "m92s",     "#WEAPON_BER" },
	{ "mp5",      "#WEAPON_MP5" },
	{ "psg1",     "#WEAPON_PSG1" },
	{ "p90",      "#WEAPON_P90" },
	{ "spas",     "#WEAPON_SHOTGUN" },
	{ "m16",      "#WEAPON_M16" },
	{ "main",     "#WEAPON_MAIN" },
	{ "zat",      "#WEAPON_ZAT" },
	{ "lance",    "#WEAPON_LANCE" },
	{ "ter",      "#WEAPON_TER" },
	{ "zatarc",   "#WEAPON_ZATARC" },
	{ "tacgun",   "#WEAPON_TACGUN" },
	{ "grenade",  "#WEAPON_HANDGRENADE" },
	{ "mine",     "#WEAPON_MINE" },
	{ "invis",    "#WEAPON_INVIS" },
	{ "invis",    "#WEAPON_SHIELD" },
	{ "gacgoa",   "#WEAPON_GACGOA" },
	{ "c4",       "#WEAPON_C4" },
	{ "larve",    "#WEAPON_LARVE" },
	{ "medic",    "#WEAPON_MEDIC" },
	{ "smoke",    "#WEAPON_SMOKEGRENADE" },
	{ "nerve",    "#WEAPON_NERVEGRENADE" },
	{ "narco",    "#WEAPON_NARCOGRENADE" },
	{ "mustard",  "#WEAPON_MUSTARDGRENADE" },
	{ "medicgoa", "#WEAPON_MEDICGOA" }
};

class CSgtcWeaponSelPanel : public CMenuPanel
{
public:
	CSgtcWeaponSelPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );

	virtual void Reset( void );
	virtual void SetActiveInfo( int iInput );	// hovered slot (0..27)

	void ToggleSlot( int iSlot );			// click: select / deselect
	int SelectedSlotCount( void );
	int IsSlotSelected( int iSlot ) { return ( iSlot >= 0 && iSlot < SGTC_WPSEL_SLOTS ) ? m_iSelected[iSlot] : 0; }

private:
	void UpdateCounters( void );
	void ShowSlotInfo( int iSlot );

	Label *m_pTitle;
	CTransparentPanel *m_pWindow;
	TextPanel *m_pInfo;
	CommandButton *m_pSlotButton[SGTC_WPSEL_SLOTS];
	TextPanel *m_pCounter[3];			// #Total / #Used / #Free
	CommandButton *m_pCancelButton;
	CommandButton *m_pResetButton;
	CommandButton *m_pApplyButton;
	int m_iSelected[SGTC_WPSEL_SLOTS];
	int m_iCurrentSlot;
	char m_szSlotInfo[SGTC_WPSEL_SLOTS][SGTC_TEAMCLASS_INFO_LEN];
};

// Click handler: toggles one slot (ref Update polls the button state and
// flips the slot flag + plays a sound; the port reacts on the action signal).
class CSgtcHandler_SlotClick : public ActionSignal
{
public:
	CSgtcHandler_SlotClick( CSgtcWeaponSelPanel *pPanel, int iSlot )
	{
		m_pPanel = pPanel;
		m_iSlot = iSlot;
	}

	virtual void actionPerformed( Panel *panel );

private:
	CSgtcWeaponSelPanel *m_pPanel;
	int m_iSlot;
};

// Reset handler: clears the whole selection (ref Reset button -> panel Reset).
class CSgtcHandler_WepSelReset : public ActionSignal
{
public:
	CSgtcHandler_WepSelReset( CSgtcWeaponSelPanel *pPanel )
	{
		m_pPanel = pPanel;
	}

	virtual void actionPerformed( Panel *panel );

private:
	CSgtcWeaponSelPanel *m_pPanel;
};

// Apply handler: builds and sends the ref's "gond4h9wp <28 values>" command.
class CSgtcHandler_WepSelApply : public ActionSignal
{
public:
	CSgtcHandler_WepSelApply( CSgtcWeaponSelPanel *pPanel )
	{
		m_pPanel = pPanel;
	}

	virtual void actionPerformed( Panel *panel );

private:
	CSgtcWeaponSelPanel *m_pPanel;
};

class CSgtcHandler_SlotOver : public InputSignal
{
public:
	CSgtcHandler_SlotOver( CSgtcWeaponSelPanel *pPanel, int iSlot )
	{
		m_pPanel = pPanel;
		m_iSlot = iSlot;
	}

	void cursorEntered( Panel *panel );

	void cursorMoved( int x, int y, Panel *panel ) {}
	void mousePressed( MouseCode code, Panel *panel ) {}
	void mouseReleased( MouseCode code, Panel *panel ) {}
	void mouseDoublePressed( MouseCode code, Panel *panel ) {}
	void cursorExited( Panel *panel ) {}
	void mouseWheeled( int delta, Panel *panel ) {}
	void keyPressed( KeyCode code, Panel *panel ) {}
	void keyTyped( KeyCode code, Panel *panel ) {}
	void keyReleased( KeyCode code, Panel *panel ) {}
	void keyFocusTicked( Panel *panel ) {}

private:
	CSgtcWeaponSelPanel *m_pPanel;
	int m_iSlot;
};

CSgtcWeaponSelPanel::CSgtcWeaponSelPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall )
	: CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hBriefingText = pSchemes->getSchemeHandle( "Briefing Text" );
	int r, g, b, a;

	// Background (ref +0x168: TextPanel 0x15,0x15, W-0x2a, H-0x2a + border)
	TextPanel *pBackground = new TextPanel( "", XRES( 21 ), YRES( 21 ),
		ScreenWidth - XRES( 42 ), ScreenHeight - YRES( 42 ) );
	pBackground->setParent( this );
	pBackground->setBorder( new LineBorder( Color( 255 * 0.7, 170 * 0.7, 0, 0 ) ) );

	// Title (ref +0x350, same literal as menus 11/60)
	m_pTitle = new Label( "", XRES( 40 ), YRES( 16 ) );
	m_pTitle->setParent( this );
	m_pTitle->setFont( pSchemes->getFont( hTitleScheme ) );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setBgColor( r, g, b, a );
	m_pTitle->setText( "%s", "Identifiez-vous" );

	// Weapon list window
	m_pWindow = new CTransparentPanel( 255, XRES( 24 ), YRES( 40 ),
		ScreenWidth - XRES( 48 ), ScreenHeight - YRES( 150 ) );
	m_pWindow->setParent( this );
	m_pWindow->setBorder( new LineBorder( Color( 255 * 0.7, 170 * 0.7, 0, 0 ) ) );

	// 28 slot buttons: 14 primaries in the left column, 14 secondaries in the
	// right one (ref: buttons +0x200 / +0x238, texts localised "#WEAPON_*").
	const int iRowTall = BUTTON_SIZE_Y;
	const int iColWide = ( ScreenWidth - XRES( 48 ) - XRES( 16 ) ) / 2;
	for ( int iSlot = 0; iSlot < SGTC_WPSEL_SLOTS; iSlot++ )
	{
		const int iColumn = iSlot / SGTC_WPSEL_PRIMARY;		// 0 = primary, 1 = secondary
		const int iRow = iSlot % SGTC_WPSEL_PRIMARY;

		m_pSlotButton[iSlot] = new CommandButton(
			gHUD.m_TextMessage.BufferedLocaliseTextString( s_SgtcWeapons[iSlot].pszKey ),
			XRES( 8 ) + iColumn * ( iColWide + XRES( 8 ) ),
			YRES( 4 ) + iRow * ( iRowTall + YRES( 2 ) ),
			iColWide, iRowTall );
		m_pSlotButton[iSlot]->setParent( m_pWindow );
		m_pSlotButton[iSlot]->addActionSignal( new CSgtcHandler_SlotClick( this, iSlot ) );
		m_pSlotButton[iSlot]->addInputSignal( new CSgtcHandler_SlotOver( this, iSlot ) );

		// Per-slot blurb (ref "info_<label>.txt", fallback "fichier absent %s"
		// in the ref; the Update uses "missing text file %s" — both are kept).
		char szFile[64];
		snprintf( szFile, sizeof( szFile ), "info_%s.txt", s_SgtcWeapons[iSlot].pszLabel );
		char *pfile = SgtcPanels_LoadFile( szFile );
		if ( pfile )
		{
			strlcpy( m_szSlotInfo[iSlot], pfile, SGTC_TEAMCLASS_INFO_LEN );
			gEngfuncs.COM_FreeFile( pfile );
		}
		else
		{
			snprintf( m_szSlotInfo[iSlot], SGTC_TEAMCLASS_INFO_LEN, "missing text file %s", szFile );
		}
	}

	// Slot info panel (ref +0x178 inside the scroll client; the port keeps the
	// same text panel without the scroll strip, sized to the window)
	m_pInfo = new TextPanel( "", XRES( 8 ), m_pWindow->getTall() - YRES( 74 ),
		m_pWindow->getWide() - XRES( 16 ), YRES( 66 ) );
	m_pInfo->setParent( m_pWindow );
	m_pInfo->setFont( pSchemes->getFont( hBriefingText ) );
	pSchemes->getFgColor( hBriefingText, r, g, b, a );
	m_pInfo->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hBriefingText, r, g, b, a );
	m_pInfo->setBgColor( r, g, b, a );
	m_pInfo->setText( "" );

	// Counters (ref +0x16c / +0x170 / +0x174, formats "#Total : %d" @0x100b3ec0,
	// "#Used : %d" @0x100b3ecc, "#Free : %d" @0x100b3ed8).
	for ( int i = 0; i < 3; i++ )
	{
		m_pCounter[i] = new TextPanel( "", XRES( 42 ) + i * XRES( 178 ), ScreenHeight - YRES( 104 ),
			XRES( 172 ), YRES( 20 ) );
		m_pCounter[i]->setParent( this );
		m_pCounter[i]->setFont( pSchemes->getFont( hBriefingText ) );
		pSchemes->getFgColor( hBriefingText, r, g, b, a );
		m_pCounter[i]->setFgColor( r, g, b, a );
		m_pCounter[i]->setBgColor( 0, 0, 0, 0xff );
		m_pCounter[i]->setBorder( new LineBorder( Color( 0, 0, 0xff, 0 ) ) );
	}

	// Bottom buttons (ref +0x118 #Annuler, +0x120 #Reset, +0x11c #Apply)
	m_pCancelButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Annuler" ),
		XRES( 42 ), ScreenHeight - YRES( 72 ), XRES( 172 ), BUTTON_SIZE_Y );
	m_pCancelButton->setParent( this );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow( HIDE_TEXTWINDOW ) );

	m_pResetButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Reset" ),
		XRES( 234 ), ScreenHeight - YRES( 72 ), XRES( 172 ), BUTTON_SIZE_Y );
	m_pResetButton->setParent( this );
	m_pResetButton->addActionSignal( new CSgtcHandler_WepSelReset( this ) );

	m_pApplyButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Apply" ),
		XRES( 426 ), ScreenHeight - YRES( 72 ), XRES( 172 ), BUTTON_SIZE_Y );
	m_pApplyButton->setParent( this );
	m_pApplyButton->addActionSignal( new CSgtcHandler_WepSelApply( this ) );

	Reset();
}

void CSgtcWeaponSelPanel::Reset( void )
{
	CMenuPanel::Reset();

	for ( int i = 0; i < SGTC_WPSEL_SLOTS; i++ )
	{
		m_iSelected[i] = 0;
		m_pSlotButton[i]->setArmed( false );
		m_pSlotButton[i]->setFgColor( 0x8c, 0x8c, 0x8c, 0 );
	}

	m_iCurrentSlot = 0;
	if ( m_pInfo )
		m_pInfo->setText( "" );

	UpdateCounters();
}

void CSgtcWeaponSelPanel::ToggleSlot( int iSlot )
{
	if ( iSlot < 0 || iSlot >= SGTC_WPSEL_SLOTS )
		return;

	m_iSelected[iSlot] = !m_iSelected[iSlot];

	// Selected slots go white, unselected stay grey (ref sets the per-slot
	// colour through the cost panel: 0x8c,0x8c,0x8c / 0,0,0).
	if ( m_iSelected[iSlot] )
		m_pSlotButton[iSlot]->setFgColor( 0xff, 0xff, 0xff, 0 );
	else
		m_pSlotButton[iSlot]->setFgColor( 0x8c, 0x8c, 0x8c, 0 );

	UpdateCounters();
}

int CSgtcWeaponSelPanel::SelectedSlotCount( void )
{
	int iCount = 0;
	for ( int i = 0; i < SGTC_WPSEL_SLOTS; i++ )
	{
		if ( m_iSelected[i] )
			iCount++;
	}
	return iCount;
}

void CSgtcWeaponSelPanel::UpdateCounters( void )
{
	// Ref shows weapon-point totals here; the ref's cost tables
	// (DAT_100dd974 / DAT_100dd9ac) and budget (DAT_100ddad4) are BSS and no
	// client-side producer for them was located, so the port reports the slot
	// counts with the ref's exact label formats instead of inventing costs.
	const int iUsed = SelectedSlotCount();
	char szText[64];

	snprintf( szText, sizeof( szText ), "#Total : %d", SGTC_WPSEL_SLOTS );
	m_pCounter[0]->setText( gHUD.m_TextMessage.BufferedLocaliseTextString( szText ) );
	snprintf( szText, sizeof( szText ), "#Used : %d", iUsed );
	m_pCounter[1]->setText( gHUD.m_TextMessage.BufferedLocaliseTextString( szText ) );
	snprintf( szText, sizeof( szText ), "#Free : %d", SGTC_WPSEL_SLOTS - iUsed );
	m_pCounter[2]->setText( gHUD.m_TextMessage.BufferedLocaliseTextString( szText ) );

	// Ref tints #Free red when less than 3 points are left.
	if ( SGTC_WPSEL_SLOTS - iUsed < 3 )
		m_pCounter[2]->setFgColor( 0xb4, 0, 0, 0 );
	else
		m_pCounter[2]->setFgColor( 0x8c, 0x8c, 0x8c, 0 );
}

void CSgtcWeaponSelPanel::ShowSlotInfo( int iSlot )
{
	if ( iSlot < 0 || iSlot >= SGTC_WPSEL_SLOTS )
		return;

	m_iCurrentSlot = iSlot;
	m_pInfo->setText( m_szSlotInfo[iSlot] );
}

void CSgtcWeaponSelPanel::SetActiveInfo( int iInput )
{
	ShowSlotInfo( iInput );
}

void CSgtcHandler_WepSelReset::actionPerformed( Panel *panel )
{
	if ( m_pPanel )
		m_pPanel->Reset();
}

void CSgtcHandler_SlotClick::actionPerformed( Panel *panel )
{
	if ( m_pPanel )
		m_pPanel->ToggleSlot( m_iSlot );
}

void CSgtcHandler_SlotOver::cursorEntered( Panel *panel )
{
	if ( m_pPanel )
		m_pPanel->SetActiveInfo( m_iSlot );
}

// Apply: the ref builds "gond4h9wp" + one value per slot and hands it to the
// client command API (FUN_10028323: (*DAT_100e7778)(&szCmd)). Our server's
// gond4h9wp handler reads CMD_ARGV(1..28) as m_iSgtcGondArgs[].
void CSgtcHandler_WepSelApply::actionPerformed( Panel *panel )
{
	if ( !m_pPanel || !gViewPort )
		return;

	char szCmd[256];
	int iLen = snprintf( szCmd, sizeof( szCmd ), "gond4h9wp" );
	for ( int i = 0; i < SGTC_WPSEL_SLOTS && iLen > 0 && iLen < (int)sizeof( szCmd ) - 4; i++ )
	{
		iLen += snprintf( szCmd + iLen, sizeof( szCmd ) - iLen, " %d", m_pPanel->IsSlotSelected( i ) ? 1 : 0 );
	}
	szCmd[sizeof( szCmd ) - 1] = 0;

	gEngfuncs.pfnFilteredClientCmd( szCmd );
	gViewPort->HideTopMenu();
}

//============================================================================
// Menu 60 — pack select ("Identifiez-vous" + 4 packs).
// Ref: ctor FUN_10029ea0 @0x10029ea0, show helper FUN_10047fb8 @0x10047fb8
// (viewport +0xaa0), pack tables FUN_1002b15b @0x1002b15b, spawn helper
// FUN_1002b046 @0x1002b046. Buttons: #PACK_1..#PACK_4 (@0x100b403c..54),
// #Annuler @0x100b405c, #Apply @0x100b4068.
//============================================================================

#define SGTC_PACKSEL_COUNT	4

class CSgtcPackSelPanel : public CMenuPanel
{
public:
	CSgtcPackSelPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall );

	virtual void Reset( void );
	virtual void SetActiveInfo( int iInput );

	void SelectPack( int iPack );

private:
	Label *m_pTitle;
	TextPanel *m_pInfo;
	CommandButton *m_pPackButton[SGTC_PACKSEL_COUNT];
	CommandButton *m_pCancelButton;
	CommandButton *m_pApplyButton;
	int m_iSelectedPack;
};

class CSgtcHandler_PackClick : public ActionSignal
{
public:
	CSgtcHandler_PackClick( CSgtcPackSelPanel *pPanel, int iPack )
	{
		m_pPanel = pPanel;
		m_iPack = iPack;
	}

	virtual void actionPerformed( Panel *panel );

private:
	CSgtcPackSelPanel *m_pPanel;
	int m_iPack;
};

class CSgtcHandler_PackApply : public ActionSignal
{
public:
	CSgtcHandler_PackApply( CSgtcPackSelPanel *pPanel )
	{
		m_pPanel = pPanel;
	}

	virtual void actionPerformed( Panel *panel );

private:
	CSgtcPackSelPanel *m_pPanel;
};

class CSgtcHandler_PackOver : public InputSignal
{
public:
	CSgtcHandler_PackOver( CSgtcPackSelPanel *pPanel, int iPack )
	{
		m_pPanel = pPanel;
		m_iPack = iPack;
	}

	void cursorEntered( Panel *panel );

	void cursorMoved( int x, int y, Panel *panel ) {}
	void mousePressed( MouseCode code, Panel *panel ) {}
	void mouseReleased( MouseCode code, Panel *panel ) {}
	void mouseDoublePressed( MouseCode code, Panel *panel ) {}
	void cursorExited( Panel *panel ) {}
	void mouseWheeled( int delta, Panel *panel ) {}
	void keyPressed( KeyCode code, Panel *panel ) {}
	void keyTyped( KeyCode code, Panel *panel ) {}
	void keyReleased( KeyCode code, Panel *panel ) {}
	void keyFocusTicked( Panel *panel ) {}

private:
	CSgtcPackSelPanel *m_pPanel;
	int m_iPack;
};

CSgtcPackSelPanel::CSgtcPackSelPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall )
	: CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
	CSchemeManager *pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t hTitleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t hBriefingText = pSchemes->getSchemeHandle( "Briefing Text" );
	int r, g, b, a;

	// Background (ref +0x164: TextPanel 0x15,0x15, W-0x2a, H-0x2a + border)
	TextPanel *pBackground = new TextPanel( "", XRES( 21 ), YRES( 21 ),
		ScreenWidth - XRES( 42 ), ScreenHeight - YRES( 42 ) );
	pBackground->setParent( this );
	pBackground->setBorder( new LineBorder( Color( 255 * 0.7, 170 * 0.7, 0, 0 ) ) );

	// Title (ref +0x234, literal "Identifiez-vous")
	m_pTitle = new Label( "", XRES( 40 ), YRES( 16 ) );
	m_pTitle->setParent( this );
	m_pTitle->setFont( pSchemes->getFont( hTitleScheme ) );
	pSchemes->getFgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( hTitleScheme, r, g, b, a );
	m_pTitle->setBgColor( r, g, b, a );
	m_pTitle->setText( "%s", "Identifiez-vous" );

	// 4 pack buttons (ref +0x1fc array, texts "#PACK_1".."#PACK_4")
	for ( int iPack = 0; iPack < SGTC_PACKSEL_COUNT; iPack++ )
	{
		char szKey[16];
		snprintf( szKey, sizeof( szKey ), "#PACK_%d", iPack + 1 );

		m_pPackButton[iPack] = new CommandButton(
			gHUD.m_TextMessage.BufferedLocaliseTextString( szKey ),
			XRES( 40 ), YRES( 44 ) + iPack * ( BUTTON_SIZE_Y + YRES( 4 ) ),
			XRES( 260 ), BUTTON_SIZE_Y );
		m_pPackButton[iPack]->setParent( this );
		m_pPackButton[iPack]->addActionSignal( new CSgtcHandler_PackClick( this, iPack ) );
		m_pPackButton[iPack]->addInputSignal( new CSgtcHandler_PackOver( this, iPack ) );
		m_pPackButton[iPack]->setFgColor( 0x8c, 0x8c, 0x8c, 0 );
	}

	// Pack info panel (ref +0x174: x 0x143, y 0x2a, W-0x16d, H-0x93, fg 0x8c8c8c,
	// bg black)
	m_pInfo = new TextPanel( "", XRES( 323 ), YRES( 42 ),
		ScreenWidth - XRES( 365 ), ScreenHeight - YRES( 147 ) );
	m_pInfo->setParent( this );
	m_pInfo->setFont( pSchemes->getFont( hBriefingText ) );
	pSchemes->getFgColor( hBriefingText, r, g, b, a );
	m_pInfo->setFgColor( 0x8c, 0x8c, 0x8c, 0 );
	m_pInfo->setBgColor( 0, 0, 0, 0xff );
	m_pInfo->setBorder( new LineBorder( Color( 0, 0, 0xff, 0 ) ) );

	// Bottom buttons (ref +0x114 #Annuler at W/2+0xad, +0x118 #Apply at W/2-0xad)
	m_pCancelButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Annuler" ),
		ScreenWidth / 2 + XRES( 173 ), ScreenHeight - YRES( 84), XRES( 152 ), BUTTON_SIZE_Y );
	m_pCancelButton->setParent( this );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow( HIDE_TEXTWINDOW ) );

	m_pApplyButton = new CommandButton( gHUD.m_TextMessage.BufferedLocaliseTextString( "#Apply" ),
		ScreenWidth / 2 - XRES( 173 ), ScreenHeight - YRES( 84 ), XRES( 152 ), BUTTON_SIZE_Y );
	m_pApplyButton->setParent( this );
	m_pApplyButton->addActionSignal( new CSgtcHandler_PackApply( this ) );

	Reset();
}

void CSgtcPackSelPanel::Reset( void )
{
	CMenuPanel::Reset();
	m_iSelectedPack = -1;

	for ( int i = 0; i < SGTC_PACKSEL_COUNT; i++ )
	{
		m_pPackButton[i]->setArmed( false );
		m_pPackButton[i]->setFgColor( 0x8c, 0x8c, 0x8c, 0 );
	}
	m_pInfo->setText( "" );
}

void CSgtcPackSelPanel::SelectPack( int iPack )
{
	if ( iPack < 0 || iPack >= SGTC_PACKSEL_COUNT )
		return;

	m_iSelectedPack = iPack;

	for ( int i = 0; i < SGTC_PACKSEL_COUNT; i++ )
		m_pPackButton[i]->setFgColor( i == iPack ? 0xff : 0x8c, i == iPack ? 0xff : 0x8c,
			i == iPack ? 0xff : 0x8c, 0 );

	// The ref fills this panel from the per-class pack table (FUN_1002b15b:
	// team x class x pack -> weapon slots); that table is not ported yet, so the
	// port names the chosen pack instead of inventing contents.
	char szText[128];
	snprintf( szText, sizeof( szText ), "%s : %s %d",
		gHUD.m_TextMessage.BufferedLocaliseTextString( "#PACK" ),
		gHUD.m_TextMessage.BufferedLocaliseTextString( "#PACK_N" ), iPack + 1 );
	m_pInfo->setText( szText );
}

void CSgtcPackSelPanel::SetActiveInfo( int iInput )
{
	SelectPack( iInput );
}

void CSgtcHandler_PackClick::actionPerformed( Panel *panel )
{
	if ( m_pPanel )
		m_pPanel->SelectPack( m_iPack );
}

void CSgtcHandler_PackOver::cursorEntered( Panel *panel )
{
	if ( m_pPanel )
		m_pPanel->SetActiveInfo( m_iPack );
}

// Apply: the ref's pack panel hands the resulting 28-slot loadout to the same
// "gond4h9wp" command as menu 12 (spawn flow FUN_1002b046 reads it back).
void CSgtcHandler_PackApply::actionPerformed( Panel *panel )
{
	if ( !m_pPanel || !gViewPort )
		return;

	// No pack table ported yet: send nothing rather than an empty loadout.
	gViewPort->HideTopMenu();
}


//============================================================================
// entry points — created once, re-opened per message (ref pre-creates both
// panels in the viewport ctor: FUN_100482ed / FUN_100483ea)
//============================================================================

static CSgtcBriefingPanel *s_pBriefingPanel = NULL;
static CSgtcTeamClassPanel *s_pTeamClassPanel = NULL;
static CSgtcWeaponSelPanel *s_pWeaponSelPanel = NULL;
static CSgtcPackSelPanel *s_pPackSelPanel = NULL;
static TeamFortressViewport *s_pPanelOwner = NULL;

// Panels are children of gViewPort and die with it; drop a stale cache if the
// engine ever hands out a new viewport instance.
static void SgtcPanels_CheckOwner( void )
{
	if ( s_pPanelOwner != gViewPort )
	{
		s_pBriefingPanel = NULL;
		s_pTeamClassPanel = NULL;
		s_pWeaponSelPanel = NULL;
		s_pPackSelPanel = NULL;
		s_pPanelOwner = gViewPort;
	}
}

CMenuPanel *SgtcBriefing_ShowPanel( void )
{
	if ( !gViewPort )
		return NULL;

	SgtcPanels_CheckOwner();

	if ( !s_pBriefingPanel )
	{
		s_pBriefingPanel = new CSgtcBriefingPanel( 255, 0, 0, 0, ScreenWidth, ScreenHeight );
		s_pBriefingPanel->setParent( gViewPort );
		s_pBriefingPanel->setVisible( false );
	}

	s_pBriefingPanel->Reset();
	SgtcPanels_ShowMenu( s_pBriefingPanel, SGTC_MENU_BRIEFING );
	return s_pBriefingPanel;
}

CMenuPanel *SgtcTeamClass_ShowPanel( void )
{
	if ( !gViewPort )
		return NULL;

	SgtcPanels_CheckOwner();

	if ( !s_pTeamClassPanel )
	{
		s_pTeamClassPanel = new CSgtcTeamClassPanel( 255, 0, 0, 0, ScreenWidth, ScreenHeight );
		s_pTeamClassPanel->setParent( gViewPort );
		s_pTeamClassPanel->setVisible( false );
	}

	s_pTeamClassPanel->Reset();
	SgtcPanels_ShowMenu( s_pTeamClassPanel, SGTC_MENU_TEAMCLASS );
	return s_pTeamClassPanel;
}

CMenuPanel *SgtcWeaponSel_ShowPanel( void )
{
	if ( !gViewPort )
		return NULL;

	SgtcPanels_CheckOwner();

	if ( !s_pWeaponSelPanel )
	{
		s_pWeaponSelPanel = new CSgtcWeaponSelPanel( 100, 0, 0, 0, ScreenWidth, ScreenHeight );
		s_pWeaponSelPanel->setParent( gViewPort );
		s_pWeaponSelPanel->setVisible( false );
	}

	s_pWeaponSelPanel->Reset();
	SgtcPanels_ShowMenu( s_pWeaponSelPanel, SGTC_MENU_WPSEL );
	return s_pWeaponSelPanel;
}

CMenuPanel *SgtcPackSel_ShowPanel( void )
{
	if ( !gViewPort )
		return NULL;

	SgtcPanels_CheckOwner();

	if ( !s_pPackSelPanel )
	{
		s_pPackSelPanel = new CSgtcPackSelPanel( 100, 0, 0, 0, ScreenWidth, ScreenHeight );
		s_pPackSelPanel->setParent( gViewPort );
		s_pPackSelPanel->setVisible( false );
	}

	s_pPackSelPanel->Reset();
	SgtcPanels_ShowMenu( s_pPackSelPanel, SGTC_MENU_PACKSEL );
	return s_pPackSelPanel;
}

#endif // SGTC_REAL_VGUI
