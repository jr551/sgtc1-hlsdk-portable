// sgtc_hud.h — StargateTC custom HUD element (client.dll @ FUN_10072e5f hooks).
#pragma once
#if !defined( SGTC_HUD_H )
#define SGTC_HUD_H

#include "hud.h"

#define SGTC_BLIP_SLOTS 28
#define SGTC_RADAR_SLOTS 256
#define SGTC_PORTRAIT_SLOTS 33

// ref client.dll: SGTC HUD state on CHudSgtc (offsets from REA decompile).
class CHudSgtc : public CHudBase
{
public:
	int Init( void );
	int VidInit( void );
	int Draw( float flTime );
	void Reset( void );

	int MsgFunc_ZatTouch( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Gtickets( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Ttickets( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TicketMode( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HudInfoMode( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Stamina( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SubModel( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HasKey( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Radio( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Armor( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Blip( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Progress( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_HideHud( const char *pszName, int iSize, void *pbuf );
	// portrait / round messages — drain extended fields when present
	int MsgFunc_AddPortrait( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_Reset( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_AboutMe( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_TogglePor( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_SetRndTim( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_NewRnd( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ClearEnv( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_EnvFull( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_EnvRain( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_EnvFount( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_NewLens( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_AddRing( const char *pszName, int iSize, void *pbuf );
	int MsgFunc_ChgPartSt( const char *pszName, int iSize, void *pbuf );

	int m_iGtickets;
	int m_iTtickets;
	int m_iTicketMode;
	int m_iHudInfoMode;
	int m_iStamina;
	int m_iSubModel;
	int m_iHasKey;
	int m_iRadioTeam;
	int m_iRadioMsg;

	float m_flArmorBar;		// ref +0x2c
	int m_iArmorAux;		// ref DAT_100f71d8
	int m_iArmorOverlay;		// ref +0x1684
	int m_iArmorState;		// ref +0x1690
	int m_iArmorParam;		// ref +0x168c

	int m_iBlipProgress;		// ref +0x1674 (Progress msg cmd 0/100)

	int m_iRadarFocusX;		// ref +0x169c (Blip type 0x32)
	int m_iRadarFocusY;		// ref +0x16a0
	int m_iRadarType[SGTC_RADAR_SLOTS];	// ref +0x564
	int m_iRadarX[SGTC_RADAR_SLOTS];	// ref +0x960
	int m_iRadarY[SGTC_RADAR_SLOTS];	// ref +0xd5c
	int m_iRadarAlpha[SGTC_RADAR_SLOTS];	// ref +0x1158

	int m_fZatTouchActive;		// ref +0x16f4
	float m_flZatTouchTime;		// ref +0x16f8

	int m_iHideHud;

	// ref @0x100768d4 — AddPortrait record (short,long,short×6,string,byte)
	int m_iPortraitSprite;		// +0x24
	int m_iPortraitEnt;		// +0x3c
	float m_flPortraitTime;		// +0x28
	int m_iPortraitX;		// +0x40  (server ent index)
	int m_iPortraitHp[3];		// +0x44/48/4c
	char m_szPortraitName[32];
	int m_iPortraitId;		// last byte
	int m_iPortraitActiveTeam;	// ref DAT_100f70c8 (AddPortrait team filter)

	int m_iAboutMeSelf;		// ref +0x20
	int m_iAboutMePlayer;		// ref +0x40
	int m_iKeyMode;			// ref +0x34
	float m_flKeyTime;		// ref +0x30
	int m_iVoteMode;		// ref DAT_100babe4
	float m_flRoundTime;		// ref +0x16ec
	float m_flRoundStart;		// ref +0x16e8
	int m_iTicketModeFlag;		// ref +0x58 (TicketMode msg)
	int m_iHudInfoByte;		// ref +0x54 (HudInfoMode msg, FUN_10076bce)
	int m_iProgress;		// ref +0x38 (Progress msg)
	int m_iProgressIcon;		// ref DAT_100e1b94
	int m_iProgressTeam[SGTC_BLIP_SLOTS];	// ref DAT_100e1aa4[28]
	int m_iProgressEnd;		// ref DAT_100e1b98

	// ref FUN_10072502 / FUN_10072694 portrait ring
	int m_iPortraitSlotId[SGTC_PORTRAIT_SLOTS];		// +0x64
	float m_flPortraitSlotHpPct[SGTC_PORTRAIT_SLOTS];	// +0xe4 (param_2)
	int m_iPortraitSlotTeam[SGTC_PORTRAIT_SLOTS];		// +0x164
	int m_iPortraitSlotAux[SGTC_PORTRAIT_SLOTS];		// +0x1e4
	int m_iPortraitSlotEnt[SGTC_PORTRAIT_SLOTS];		// +0x264
	int m_iPortraitSlotHp[SGTC_PORTRAIT_SLOTS][3];		// +0x2e4/+0x364/+0x3e4
	char m_szPortraitSlotName[SGTC_PORTRAIT_SLOTS][32];	// +0x464 (strdup in ref)
	int m_iPortraitSlotColor[SGTC_PORTRAIT_SLOTS];		// +0x4e4
	int m_iPortraitSlotFlash[SGTC_PORTRAIT_SLOTS];		// +0x1568
	int m_iPortraitSlotGray[SGTC_PORTRAIT_SLOTS];		// +0x15ec
};

extern CHudSgtc g_SgtcHud;

#endif
