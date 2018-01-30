/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2018 HertzDevil
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.  To obtain a
** copy of the GNU Library General Public License, write to the Free
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
*/

#include "MainFrm.h"
#include "version.h"		// // //
#include <algorithm>
#include "FamiTracker.h"
#include "FamiTrackerDoc.h"
#include "FamiTrackerView.h"
#include "ExportDialog.h"
#include "CreateWaveDlg.h"
#include "InstrumentEditDlg.h"
#include "ModulePropertiesDlg.h"
#include "ChannelsDlg.h"
#include "VisualizerWnd.h"
#include "TextExporter.h"
#include "ConfigGeneral.h"
#include "ConfigVersion.h"		// // //
#include "ConfigAppearance.h"
#include "ConfigMIDI.h"
#include "ConfigSound.h"
#include "ConfigShortcuts.h"
#include "ConfigWindow.h"
#include "ConfigMixer.h"
#include "Settings.h"
#include "Accelerator.h"
#include "SoundGen.h"
#include "MIDI.h"
#include "TrackerChannel.h"
#include "CommentsDlg.h"
#include "InstrumentFileTree.h"
#include "PatternAction.h"
#include "FrameAction.h"
#include "PatternEditor.h"
#include "FrameEditor.h"
#include "APU/APU.h"
// // //
#include "FamiTrackerModule.h"
#include "SongData.h"
#include "SongView.h"
#include "SongLengthScanner.h"
#include "AudioDriver.h"
#include "GrooveDlg.h"
#include "GotoDlg.h"
#include "BookmarkDlg.h"
#include "SwapDlg.h"
#include "SpeedDlg.h"
#include "FindDlg.h"
#include "TransposeDlg.h"
#include "DPI.h"
#include "Color.h"
#include "InstrumentService.h"
#include "ActionHandler.h"
#include "ModuleAction.h"
#include "SimpleFile.h"
#include "Instrument.h"
#include "InstrumentManager.h"
#include "Kraid.h"
#include "NumConv.h"
#include "InstrumentListCtrl.h"

namespace {

const unsigned MAX_UNDO_LEVELS = 64;		// // // moved

const UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CHIP,
	ID_INDICATOR_INSTRUMENT,
	ID_INDICATOR_OCTAVE,
	ID_INDICATOR_RATE,
	ID_INDICATOR_TEMPO,
	ID_INDICATOR_TIME,
	ID_INDICATOR_POS,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

// Timers
enum {
	TMR_WELCOME,
	TMR_AUDIO_CHECK,
	TMR_AUTOSAVE
};

// Repeat config
const int REPEAT_DELAY = 20;
const int REPEAT_TIME = 200;

} // namespace

// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
	m_cBannerEditName(IDS_INFO_TITLE),		// // //
	m_cBannerEditArtist(IDS_INFO_AUTHOR),		// // //
	m_cBannerEditCopyright(IDS_INFO_COPYRIGHT),		// // //
	m_iFrameEditorPos(FRAME_EDIT_POS_TOP),
	m_iInstrument(0),
	m_iTrack(0),
	m_iOctave(3),
	m_iInstNumDigit(0),		// // //
	m_iInstNumCurrent(MAX_INSTRUMENTS),		// // //
	m_iKraidCounter(0)		// // // Easter Egg
{
}

CMainFrame::~CMainFrame()
{
}

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_WM_ERASEBKGND()
	ON_WM_SHOWWINDOW()
	ON_WM_DESTROY()
	ON_WM_COPYDATA()
	ON_COMMAND(ID_FILE_GENERALSETTINGS, OnFileGeneralsettings)
	ON_COMMAND(ID_FILE_IMPORTTEXT, OnFileImportText)
	ON_COMMAND(ID_FILE_EXPORTTEXT, OnFileExportText)
	ON_COMMAND(ID_FILE_CREATE_NSF, OnCreateNSF)
	ON_COMMAND(ID_FILE_CREATEWAV, OnCreateWAV)
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, OnEditRedo)
	ON_COMMAND(ID_EDIT_CUT, OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, OnEditPaste)
	ON_COMMAND(ID_EDIT_DELETE, OnEditDelete)
	ON_COMMAND(ID_EDIT_SELECTALL, OnEditSelectall)
	ON_COMMAND(ID_EDIT_ENABLEMIDI, OnEditEnableMIDI)
	ON_COMMAND(ID_EDIT_EXPANDPATTERNS, OnEditExpandpatterns)
	ON_COMMAND(ID_EDIT_SHRINKPATTERNS, OnEditShrinkpatterns)
	ON_COMMAND(ID_EDIT_CLEARPATTERNS, OnEditClearPatterns)
	ON_COMMAND(ID_CLEANUP_REMOVEUNUSEDINSTRUMENTS, OnEditRemoveUnusedInstruments)
	ON_COMMAND(ID_CLEANUP_REMOVEUNUSEDPATTERNS, OnEditRemoveUnusedPatterns)
	ON_COMMAND(ID_CLEANUP_MERGEDUPLICATEDPATTERNS, OnEditMergeDuplicatedPatterns)
	ON_COMMAND(ID_INSTRUMENT_NEW, OnAddInstrument)
	ON_COMMAND(ID_INSTRUMENT_REMOVE, OnRemoveInstrument)
	ON_COMMAND(ID_INSTRUMENT_CLONE, OnCloneInstrument)
	ON_COMMAND(ID_INSTRUMENT_DEEPCLONE, OnDeepCloneInstrument)
	ON_COMMAND(ID_INSTRUMENT_SAVE, OnSaveInstrument)
	ON_COMMAND(ID_INSTRUMENT_LOAD, OnLoadInstrument)
	ON_COMMAND(ID_INSTRUMENT_EDIT, OnEditInstrument)
	ON_COMMAND(ID_INSTRUMENT_ADD_2A03, OnAddInstrument2A03)
	ON_COMMAND(ID_INSTRUMENT_ADD_VRC6, OnAddInstrumentVRC6)
	ON_COMMAND(ID_INSTRUMENT_ADD_VRC7, OnAddInstrumentVRC7)
	ON_COMMAND(ID_INSTRUMENT_ADD_FDS, OnAddInstrumentFDS)
	ON_COMMAND(ID_INSTRUMENT_ADD_MMC5, OnAddInstrumentMMC5)
	ON_COMMAND(ID_INSTRUMENT_ADD_N163, OnAddInstrumentN163)
	ON_COMMAND(ID_INSTRUMENT_ADD_S5B, OnAddInstrumentS5B)
	ON_COMMAND(ID_MODULE_MODULEPROPERTIES, OnModuleModuleproperties)
	ON_COMMAND(ID_MODULE_CHANNELS, OnModuleChannels)
	ON_COMMAND(ID_MODULE_COMMENTS, OnModuleComments)
	ON_COMMAND(ID_MODULE_INSERTFRAME, OnModuleInsertFrame)
	ON_COMMAND(ID_MODULE_REMOVEFRAME, OnModuleRemoveFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAME, OnModuleDuplicateFrame)
	ON_COMMAND(ID_MODULE_DUPLICATEFRAMEPATTERNS, OnModuleDuplicateFramePatterns)
	ON_COMMAND(ID_MODULE_MOVEFRAMEDOWN, OnModuleMoveframedown)
	ON_COMMAND(ID_MODULE_MOVEFRAMEUP, OnModuleMoveframeup)
	ON_COMMAND(ID_TRACKER_PLAY, OnTrackerPlay)
	ON_COMMAND(ID_TRACKER_PLAY_START, OnTrackerPlayStart)
	ON_COMMAND(ID_TRACKER_PLAY_CURSOR, OnTrackerPlayCursor)
	ON_COMMAND(ID_TRACKER_STOP, OnTrackerStop)
	ON_COMMAND(ID_TRACKER_TOGGLE_PLAY, OnTrackerTogglePlay)
	ON_COMMAND(ID_TRACKER_PLAYPATTERN, OnTrackerPlaypattern)
	ON_COMMAND(ID_TRACKER_KILLSOUND, OnTrackerKillsound)
	ON_COMMAND(ID_TRACKER_SWITCHTOTRACKINSTRUMENT, OnTrackerSwitchToInstrument)
	// // //
	ON_COMMAND(ID_TRACKER_DISPLAYREGISTERSTATE, OnTrackerDisplayRegisterState)
	ON_COMMAND(ID_VIEW_CONTROLPANEL, OnViewControlpanel)
	ON_COMMAND(ID_HELP, CFrameWnd::OnHelp)
	ON_COMMAND(ID_HELP_FINDER, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP_PERFORMANCE, OnHelpPerformance)
	ON_COMMAND(ID_HELP_EFFECTTABLE, &CMainFrame::OnHelpEffecttable)
	ON_COMMAND(ID_HELP_FAQ, &CMainFrame::OnHelpFAQ)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWnd::OnContextHelp)
	ON_COMMAND(ID_FRAMEEDITOR_TOP, OnFrameeditorTop)
	ON_COMMAND(ID_FRAMEEDITOR_LEFT, OnFrameeditorLeft)
	ON_COMMAND(ID_NEXT_FRAME, OnNextFrame)
	ON_COMMAND(ID_PREV_FRAME, OnPrevFrame)
	ON_COMMAND(IDC_KEYREPEAT, OnKeyRepeat)
	ON_COMMAND(ID_NEXT_SONG, OnNextSong)
	ON_COMMAND(ID_PREV_SONG, OnPrevSong)
	ON_COMMAND(IDC_FOLLOW_TOGGLE, OnToggleFollow)
	ON_COMMAND(ID_FOCUS_PATTERN_EDITOR, OnSelectPatternEditor)
	ON_COMMAND(ID_FOCUS_FRAME_EDITOR, OnSelectFrameEditor)
	ON_COMMAND(ID_CMD_NEXT_INSTRUMENT, OnNextInstrument)
	ON_COMMAND(ID_CMD_PREV_INSTRUMENT, OnPrevInstrument)
	ON_COMMAND(ID_TOGGLE_SPEED, OnToggleSpeed)
	ON_COMMAND(ID_DECAY_FAST, OnDecayFast)
	ON_COMMAND(ID_DECAY_SLOW, OnDecaySlow)
	ON_BN_CLICKED(IDC_FRAME_INC, OnBnClickedIncFrame)
	ON_BN_CLICKED(IDC_FRAME_DEC, OnBnClickedDecFrame)
	ON_BN_CLICKED(IDC_FOLLOW, OnClickedFollow)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPEED_SPIN, OnDeltaposSpeedSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_TEMPO_SPIN, OnDeltaposTempoSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ROWS_SPIN, OnDeltaposRowsSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_FRAME_SPIN, OnDeltaposFrameSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_KEYSTEP_SPIN, OnDeltaposKeyStepSpin)
	ON_EN_CHANGE(IDC_INSTNAME, OnInstNameChange)
	ON_EN_CHANGE(IDC_KEYSTEP, OnEnKeyStepChange)
	ON_EN_CHANGE(IDC_SONG_NAME, OnEnSongNameChange)
	ON_EN_CHANGE(IDC_SONG_ARTIST, OnEnSongArtistChange)
	ON_EN_CHANGE(IDC_SONG_COPYRIGHT, OnEnSongCopyrightChange)
	ON_EN_SETFOCUS(IDC_KEYREPEAT, OnRemoveFocus)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, OnUpdateEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, OnUpdateEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CUT, OnUpdateEditCut)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, OnUpdateEditPaste)
	ON_UPDATE_COMMAND_UI(ID_EDIT_DELETE, OnUpdateEditDelete)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLEMIDI, OnUpdateEditEnablemidi)
	ON_UPDATE_COMMAND_UI(ID_MODULE_INSERTFRAME, OnUpdateInsertFrame)
	ON_UPDATE_COMMAND_UI(ID_MODULE_REMOVEFRAME, OnUpdateRemoveFrame)
	ON_UPDATE_COMMAND_UI(ID_MODULE_DUPLICATEFRAME, OnUpdateDuplicateFrame)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEDOWN, OnUpdateModuleMoveframedown)
	ON_UPDATE_COMMAND_UI(ID_MODULE_MOVEFRAMEUP, OnUpdateModuleMoveframeup)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_NEW, OnUpdateInstrumentNew)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_REMOVE, OnUpdateInstrumentRemove)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_CLONE, OnUpdateInstrumentClone)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_DEEPCLONE, OnUpdateInstrumentDeepClone)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_EDIT, OnUpdateInstrumentEdit)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_LOAD, OnUpdateInstrumentLoad)
	ON_UPDATE_COMMAND_UI(ID_INSTRUMENT_SAVE, OnUpdateInstrumentSave)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_INSTRUMENT, OnUpdateSBInstrument)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_OCTAVE, OnUpdateSBOctave)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_RATE, OnUpdateSBFrequency)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_TEMPO, OnUpdateSBTempo)
	ON_UPDATE_COMMAND_UI(ID_INDICATOR_CHIP, OnUpdateSBChip)
	ON_UPDATE_COMMAND_UI(IDC_KEYSTEP, OnUpdateKeyStepEdit)
	ON_UPDATE_COMMAND_UI(IDC_KEYREPEAT, OnUpdateKeyRepeat)
	ON_UPDATE_COMMAND_UI(IDC_SPEED, OnUpdateSpeedEdit)
	ON_UPDATE_COMMAND_UI(IDC_TEMPO, OnUpdateTempoEdit)
	ON_UPDATE_COMMAND_UI(IDC_ROWS, OnUpdateRowsEdit)
	ON_UPDATE_COMMAND_UI(IDC_FRAMES, OnUpdateFramesEdit)
	ON_UPDATE_COMMAND_UI(ID_NEXT_SONG, OnUpdateNextSong)
	ON_UPDATE_COMMAND_UI(ID_PREV_SONG, OnUpdatePrevSong)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_SWITCHTOTRACKINSTRUMENT, OnUpdateTrackerSwitchToInstrument)
	ON_UPDATE_COMMAND_UI(ID_VIEW_CONTROLPANEL, OnUpdateViewControlpanel)
	ON_UPDATE_COMMAND_UI(ID_EDIT_EXPANDPATTERNS, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_SHRINKPATTERNS, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_FRAMEEDITOR_TOP, OnUpdateFrameeditorTop)
	ON_UPDATE_COMMAND_UI(ID_FRAMEEDITOR_LEFT, OnUpdateFrameeditorLeft)
	ON_CBN_SELCHANGE(IDC_SUBTUNE, OnCbnSelchangeSong)
	ON_CBN_SELCHANGE(IDC_OCTAVE, OnCbnSelchangeOctave)
	ON_MESSAGE(WM_USER_DISPLAY_MESSAGE_STRING, OnDisplayMessageString)
	ON_MESSAGE(WM_USER_DISPLAY_MESSAGE_ID, OnDisplayMessageID)
	ON_COMMAND(ID_VIEW_TOOLBAR, &CMainFrame::OnViewToolbar)

	// // //
	ON_BN_CLICKED(IDC_BUTTON_GROOVE, OnToggleGroove)
	ON_BN_CLICKED(IDC_BUTTON_FIXTEMPO, OnToggleFixTempo)
	ON_BN_CLICKED(IDC_CHECK_COMPACT, OnClickedCompact)
	ON_COMMAND(ID_CMD_INST_NUM, OnTypeInstrumentNumber)
	ON_COMMAND(IDC_COMPACT_TOGGLE, OnToggleCompact)
	ON_UPDATE_COMMAND_UI(IDC_HIGHLIGHT1, OnUpdateHighlight1)
	ON_UPDATE_COMMAND_UI(IDC_HIGHLIGHT2, OnUpdateHighlight2)
	ON_NOTIFY(UDN_DELTAPOS, IDC_HIGHLIGHTSPIN1, OnDeltaposHighlightSpin1)
	ON_NOTIFY(UDN_DELTAPOS, IDC_HIGHLIGHTSPIN2, OnDeltaposHighlightSpin2)
	ON_COMMAND(ID_FILE_EXPORTROWS, OnFileExportRows)
	ON_COMMAND(ID_COPYAS_TEXT, OnEditCopyAsText)
	ON_COMMAND(ID_COPYAS_VOLUMESEQUENCE, OnEditCopyAsVolumeSequence)
	ON_COMMAND(ID_COPYAS_PPMCK, OnEditCopyAsPPMCK)
	ON_COMMAND(ID_EDIT_PASTEOVERWRITE, OnEditPasteOverwrite)
	ON_COMMAND(ID_SELECT_NONE, OnEditSelectnone)
	ON_COMMAND(ID_SELECT_ROW, OnEditSelectrow)
	ON_COMMAND(ID_SELECT_COLUMN, OnEditSelectcolumn)
	ON_COMMAND(ID_SELECT_PATTERN, OnEditSelectpattern)
	ON_COMMAND(ID_SELECT_FRAME, OnEditSelectframe)
	ON_COMMAND(ID_SELECT_CHANNEL, OnEditSelectchannel)
	ON_COMMAND(ID_SELECT_TRACK, OnEditSelecttrack)
	ON_COMMAND(ID_SELECT_OTHER, OnEditSelectother)
	ON_COMMAND(ID_EDIT_FIND_TOGGLE, OnEditFindToggle)
	ON_COMMAND(ID_FIND_NEXT, OnFindNext)
	ON_COMMAND(ID_FIND_PREVIOUS, OnFindPrevious)
	ON_COMMAND(ID_EDIT_GOTO, OnEditGoto)
	ON_COMMAND(ID_EDIT_SWAPCHANNELS, OnEditSwapChannels)
	ON_COMMAND(ID_EDIT_STRETCHPATTERNS, OnEditStretchpatterns)
	ON_COMMAND(ID_TRANSPOSE_CUSTOM, OnEditTransposeCustom)
	ON_COMMAND(ID_CLEANUP_REMOVEUNUSEDDPCMSAMPLES, OnEditRemoveUnusedSamples)
	ON_COMMAND(ID_CLEANUP_POPULATEUNIQUEPATTERNS, OnEditPopulateUniquePatterns)
	ON_COMMAND(ID_MODULE_DUPLICATECURRENTPATTERN, OnModuleDuplicateCurrentPattern)
	ON_COMMAND(ID_MODULE_GROOVE, OnModuleGrooveSettings)
	ON_COMMAND(ID_MODULE_BOOKMARK, OnModuleBookmarkSettings)
	ON_COMMAND(ID_MODULE_ESTIMATESONGLENGTH, OnModuleEstimateSongLength)
	ON_COMMAND(ID_TRACKER_PLAY_MARKER, OnTrackerPlayMarker)		// // // 050B
	ON_COMMAND(ID_TRACKER_SET_MARKER, OnTrackerSetMarker)		// // // 050B
	ON_COMMAND(ID_VIEW_AVERAGEBPM, OnTrackerDisplayAverageBPM)		// // // 050B
	ON_COMMAND(ID_VIEW_CHANNELSTATE, OnTrackerDisplayChannelState)		// // // 050B
	ON_COMMAND(ID_TOGGLE_MULTIPLEXER, OnToggleMultiplexer)
	ON_UPDATE_COMMAND_UI(IDC_FOLLOW_TOGGLE, OnUpdateToggleFollow)
	ON_UPDATE_COMMAND_UI(IDC_COMPACT_TOGGLE, OnUpdateToggleCompact)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_FIXTEMPO, OnUpdateToggleFixTempo)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_GROOVE, OnUpdateGrooveEdit)
	ON_UPDATE_COMMAND_UI(ID_COPYAS_TEXT, OnUpdateEditCopySpecial)
	ON_UPDATE_COMMAND_UI(ID_COPYAS_VOLUMESEQUENCE, OnUpdateEditCopySpecial)
	ON_UPDATE_COMMAND_UI(ID_COPYAS_PPMCK, OnUpdateEditCopySpecial)
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTEOVERWRITE, OnUpdateEditPasteOverwrite)
	ON_UPDATE_COMMAND_UI(ID_SELECT_ROW, OnUpdatePatternEditorSelected)
	ON_UPDATE_COMMAND_UI(ID_SELECT_COLUMN, OnUpdatePatternEditorSelected)
	ON_UPDATE_COMMAND_UI(ID_SELECT_CHANNEL, OnUpdateSelectMultiFrame)
	ON_UPDATE_COMMAND_UI(ID_SELECT_TRACK, OnUpdateSelectMultiFrame)
//	ON_UPDATE_COMMAND_UI(ID_SELECT_OTHER, OnUpdateCurrentSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_FIND_TOGGLE, OnUpdateEditFindToggle)
	ON_UPDATE_COMMAND_UI(ID_EDIT_INTERPOLATE, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REVERSE, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REPLACEINSTRUMENT, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_STRETCHPATTERNS, OnUpdateSelectionEnabled)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PLAY_MARKER, OnUpdateTrackerPlayMarker)		// // // 050B
	ON_UPDATE_COMMAND_UI(ID_VIEW_AVERAGEBPM, OnUpdateDisplayAverageBPM)		// // // 050B
	ON_UPDATE_COMMAND_UI(ID_VIEW_CHANNELSTATE, OnUpdateDisplayChannelState)		// // // 050B
	ON_UPDATE_COMMAND_UI(ID_TRACKER_DISPLAYREGISTERSTATE, OnUpdateDisplayRegisterState)
	ON_UPDATE_COMMAND_UI(ID_DECAY_FAST, OnUpdateDecayFast)		// // // 050B
	ON_UPDATE_COMMAND_UI(ID_DECAY_SLOW, OnUpdateDecaySlow)		// // // 050B
	ON_COMMAND(ID_KRAID1, OnEasterEggKraid1)		// Easter Egg
	ON_COMMAND(ID_KRAID2, OnEasterEggKraid2)
	ON_COMMAND(ID_KRAID3, OnEasterEggKraid3)
	ON_COMMAND(ID_KRAID4, OnEasterEggKraid4)
	ON_COMMAND(ID_KRAID5, OnEasterEggKraid5)
	// // // From CFamiTrackerView
	ON_COMMAND(ID_CMD_OCTAVE_NEXT, OnNextOctave)
	ON_COMMAND(ID_CMD_OCTAVE_PREVIOUS, OnPreviousOctave)
	ON_COMMAND(ID_TRACKER_PAL, OnTrackerPal)
	ON_COMMAND(ID_TRACKER_NTSC, OnTrackerNtsc)
	ON_COMMAND(ID_SPEED_DEFAULT, OnSpeedDefault)
	ON_COMMAND(ID_SPEED_CUSTOM, OnSpeedCustom)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_PAL, OnUpdateTrackerPal)
	ON_UPDATE_COMMAND_UI(ID_TRACKER_NTSC, OnUpdateTrackerNtsc)
	ON_UPDATE_COMMAND_UI(ID_SPEED_DEFAULT, OnUpdateSpeedDefault)
	ON_UPDATE_COMMAND_UI(ID_SPEED_CUSTOM, OnUpdateSpeedCustom)
	END_MESSAGE_MAP()


BOOL CMainFrame::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle , const RECT& rect , CWnd* pParentWnd , LPCTSTR lpszMenuName , DWORD dwExStyle , CCreateContext* pContext)
{
	CSettings *pSettings = theApp.GetSettings();
	RECT newrect;

	// Load stored position
	newrect.bottom	= pSettings->WindowPos.iBottom;
	newrect.left	= pSettings->WindowPos.iLeft;
	newrect.right	= pSettings->WindowPos.iRight;
	newrect.top		= pSettings->WindowPos.iTop;

	return CFrameWnd::Create(lpszClassName, lpszWindowName, dwStyle, newrect, pParentWnd, lpszMenuName, dwExStyle, pContext);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// Get the DPI setting
	if (CDC *pDC = GetDC()) {		// // //
		DPI::SetScale(pDC->GetDeviceCaps(LOGPIXELSX), pDC->GetDeviceCaps(LOGPIXELSY));
		ReleaseDC(pDC);
	}

	ResetUndo();		// // //

	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!CreateToolbars())
		return -1;

	if (!m_wndStatusBar.Create(this) || !m_wndStatusBar.SetIndicators(indicators, std::size(indicators))) {		// // //
		TRACE("Failed to create status bar\n");
		return -1;      // fail to create
	}
	m_wndStatusBar.SetPaneInfo(1, ID_INDICATOR_CHIP, SBPS_NORMAL, 250);		// // //
	m_wndStatusBar.SetPaneInfo(2, ID_INDICATOR_INSTRUMENT, SBPS_NORMAL, 100);		// // //

	if (!CreateDialogPanels())
		return -1;

	if (!CreateInstrumentToolbar()) {
		TRACE("Failed to create instrument toolbar\n");
		return -1;      // fail to create
	}

	/*
	// TODO: Delete these three lines if you don't want the toolbar to be dockable
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	EnableDocking(CBRS_ALIGN_ANY);
	DockControlBar(&m_wndToolBar);
	*/

	if (!CreateVisualizerWindow()) {
		TRACE("Failed to create sample window\n");
		return -1;      // fail to create
	}

	// Initial message, 100ms
	SetTimer(TMR_WELCOME, 100, NULL);

	// Periodic audio check, 500ms
	SetTimer(TMR_AUDIO_CHECK, 500, NULL);

	// Auto save
#ifdef AUTOSAVE
	SetTimer(TMR_AUTOSAVE, 1000, NULL);
#endif

	m_wndOctaveBar.CheckDlgButton(IDC_FOLLOW, theApp.GetSettings()->FollowMode);
	m_wndOctaveBar.CheckDlgButton(IDC_CHECK_COMPACT, false);		// // //
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT1, CSongData::DEFAULT_HIGHLIGHT.First, 0);		// // //
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT2, CSongData::DEFAULT_HIGHLIGHT.Second, 0);

	UpdateMenus();

	// Setup saved frame editor position
	SetFrameEditorPosition(theApp.GetSettings()->FrameEditPos);

#ifdef _DEBUG
	m_strTitle.Append(_T(" [DEBUG]"));
#endif
#ifdef WIP
	m_strTitle.Append(_T(" [BETA]"));
#endif

	if (AfxGetResourceHandle() != AfxGetInstanceHandle()) {		// // //
		// Prevent confusing bugs while developing
		m_strTitle.Append(_T(" [Localization enabled]"));
	}

	return 0;
}

bool CMainFrame::CreateToolbars()
{
	REBARBANDINFO rbi1;

	if (!m_wndToolBarReBar.Create(this)) {
		TRACE("Failed to create rebar\n");
		return false;      // fail to create
	}

	// Add the toolbar
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))  {
		TRACE("Failed to create toolbar\n");
		return false;      // fail to create
	}

	m_wndToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

	if (!m_wndOctaveBar.Create(this, (UINT)IDD_OCTAVE, CBRS_TOOLTIPS | CBRS_FLYBY, IDD_OCTAVE)) {
		TRACE("Failed to create octave bar\n");
		return false;      // fail to create
	}

	// // // TODO: 050B

	rbi1.cbSize		= sizeof(REBARBANDINFO);
	rbi1.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_SIZE;
	rbi1.fStyle		= RBBS_GRIPPERALWAYS;		// // // 050B
	rbi1.hwndChild	= m_wndToolBar;
	rbi1.cxMinChild	= DPI::SX(554);
	rbi1.cyMinChild	= DPI::SY(22);
	rbi1.cx			= DPI::SX(496);

	if (!m_wndToolBarReBar.GetReBarCtrl().InsertBand(-1, &rbi1)) {
		TRACE("Failed to create rebar\n");
		return false;      // fail to create
	}

	rbi1.cbSize		= sizeof(REBARBANDINFO);
	rbi1.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_SIZE;
	rbi1.fStyle		= RBBS_GRIPPERALWAYS;		// // // 050B
	rbi1.hwndChild	= m_wndOctaveBar;
	rbi1.cxMinChild	= DPI::SX(460);		// // //
	rbi1.cyMinChild	= DPI::SY(22);
	rbi1.cx			= DPI::SX(100);

	if (!m_wndToolBarReBar.GetReBarCtrl().InsertBand(-1, &rbi1)) {
		TRACE("Failed to create rebar\n");
		return false;      // fail to create
	}

	m_wndToolBarReBar.GetReBarCtrl().MinimizeBand(0);

	HBITMAP hbm = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_TOOLBAR_256), IMAGE_BITMAP, DPI::SX(352), DPI::SY(16), LR_CREATEDIBSECTION);
	m_bmToolbar.Attach(hbm);

	m_ilToolBar.Create(DPI::SX(16), DPI::SY(16), ILC_COLOR8 | ILC_MASK, 4, 4);
	m_ilToolBar.Add(&m_bmToolbar, MakeRGB(192, 192, 192));
	m_wndToolBar.GetToolBarCtrl().SetImageList(&m_ilToolBar);

	return true;
}

bool CMainFrame::CreateDialogPanels()
{
	//CDialogBar

	// Top area
	if (!m_wndControlBar.Create(this, IDD_MAINBAR, CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_MAINBAR)) {
		TRACE("Failed to create frame main bar\n");
		return false;
	}

	/////////
	if (!m_wndVerticalControlBar.Create(this, IDD_MAINBAR, CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_MAINBAR)) {
		TRACE("Failed to create frame main bar\n");
		return false;
	}

	m_wndVerticalControlBar.ShowWindow(SW_HIDE);
	/////////

	// Create frame controls
	m_wndFrameControls.SetFrameParent(this);
	m_wndFrameControls.Create(IDD_FRAMECONTROLS, &m_wndControlBar);
	m_wndFrameControls.ShowWindow(SW_SHOW);

	// Create frame editor
	m_pFrameEditor = std::make_unique<CFrameEditor>(this);		// // //

	CRect rect(12, 12, m_pFrameEditor->CalcWidth(MAX_CHANNELS_2A03), 162);		// // //
	DPI::ScaleRect(rect);		// // //

	if (!m_pFrameEditor->CreateEx(WS_EX_STATICEDGE, NULL, _T(""), WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL, rect, (CWnd*)&m_wndControlBar, 0)) {
		TRACE("Failed to create frame editor\n");
		return false;
	}

	// // // Find / replace panel
	m_pFindDlg = std::make_unique<CFindDlg>();
	if (!m_wndFindControlBar.Create(this, IDD_MAINBAR, CBRS_RIGHT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_MAINBAR)) {
		TRACE("Failed to create find dialog\n");
		return false;
	}
	m_wndFindControlBar.ShowWindow(SW_HIDE);
	if (!m_pFindDlg->Create(IDD_FIND, &m_wndFindControlBar)) {
		TRACE("Failed to create find / replace dialog\n");
		return false;
	}
	m_pFindDlg->ShowWindow(SW_SHOW);

	m_wndDialogBar.SetFrameParent(this);

	if (!m_wndDialogBar.Create(IDD_MAINFRAME, &m_wndControlBar)) {
		TRACE("Failed to create dialog bar\n");
		return false;
	}

	m_wndDialogBar.ShowWindow(SW_SHOW);

	// Subclass edit boxes
	m_cLockedEditSpeed.SubclassDlgItem(IDC_SPEED, &m_wndDialogBar);
	m_cLockedEditTempo.SubclassDlgItem(IDC_TEMPO, &m_wndDialogBar);
	m_cLockedEditLength.SubclassDlgItem(IDC_ROWS, &m_wndDialogBar);
	m_cLockedEditFrames.SubclassDlgItem(IDC_FRAMES, &m_wndDialogBar);
	m_cLockedEditStep.SubclassDlgItem(IDC_KEYSTEP, &m_wndDialogBar);
	m_cLockedEditHighlight1.SubclassDlgItem(IDC_HIGHLIGHT1, &m_wndOctaveBar);		// // //
	m_cLockedEditHighlight2.SubclassDlgItem(IDC_HIGHLIGHT2, &m_wndOctaveBar);		// // //
	m_cButtonGroove.SubclassDlgItem(IDC_BUTTON_GROOVE, &m_wndDialogBar);		// // //
	m_cButtonFixTempo.SubclassDlgItem(IDC_BUTTON_FIXTEMPO, &m_wndDialogBar);		// // //

	// Subclass and setup the instrument list

	m_pInstrumentList = std::make_unique<CInstrumentListCtrl>(this);		// // //
	m_pInstrumentList->SubclassDlgItem(IDC_INSTRUMENTS, &m_wndDialogBar);

	SetupColors();

	m_pInstrumentList->CreateImageList();		// // //
	m_pInstrumentList->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	// Title, author & copyright
	m_cBannerEditName.SubclassDlgItem(IDC_SONG_NAME, &m_wndDialogBar);
	m_cBannerEditArtist.SubclassDlgItem(IDC_SONG_ARTIST, &m_wndDialogBar);
	m_cBannerEditCopyright.SubclassDlgItem(IDC_SONG_COPYRIGHT, &m_wndDialogBar);

	m_cBannerEditName.SetLimitText(CFamiTrackerModule::METADATA_FIELD_LENGTH - 1);		// // //
	m_cBannerEditArtist.SetLimitText(CFamiTrackerModule::METADATA_FIELD_LENGTH - 1);
	m_cBannerEditCopyright.SetLimitText(CFamiTrackerModule::METADATA_FIELD_LENGTH - 1);

	CEdit *pInstName = static_cast<CEdit*>(m_wndDialogBar.GetDlgItem(IDC_INSTNAME));
	pInstName->SetLimitText(CInstrument::INST_NAME_MAX - 1);

	// New instrument editor

#ifdef NEW_INSTRUMENTPANEL
/*
	if (!m_wndInstrumentBar.Create(this, IDD_INSTRUMENTPANEL, CBRS_RIGHT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_INSTRUMENTPANEL)) {
		TRACE("Failed to create frame instrument bar\n");
	}

	m_wndInstrumentBar.ShowWindow(SW_SHOW);
*/
#endif

	// Frame bar
/*
	if (!m_wndFrameBar.Create(this, IDD_FRAMEBAR, CBRS_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY, IDD_FRAMEBAR)) {
		TRACE("Failed to create frame bar\n");
	}

	m_wndFrameBar.ShowWindow(SW_SHOW);
*/

	return true;
}

bool CMainFrame::CreateVisualizerWindow()
{
	CRect rect;		// // // 050B
	m_wndDialogBar.GetDlgItem(IDC_MAINFRAME_VISUALIZER)->GetWindowRect(&rect);
	GetDesktopWindow()->MapWindowPoints(&m_wndDialogBar, &rect);

	// Create the sample graph window
	m_pVisualizerWnd = std::make_unique<CVisualizerWnd>();

	if (!m_pVisualizerWnd->CreateEx(WS_EX_STATICEDGE, NULL, _T("Samples"), WS_CHILD | WS_VISIBLE, rect, (CWnd*)&m_wndDialogBar, 0))
		return false;

	// Assign this to the sound generator
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	if (pSoundGen)
		pSoundGen->SetVisualizerWindow(m_pVisualizerWnd.get());

	return true;
}

bool CMainFrame::CreateInstrumentToolbar()
{
	// Setup the instrument toolbar
	REBARBANDINFO rbi;

	if (!m_wndInstToolBarWnd.CreateEx(0, NULL, _T(""), WS_CHILD | WS_VISIBLE, DPI::Rect(310, 173, 184, 26), (CWnd*)&m_wndDialogBar, 0))
		return false;

	if (!m_wndInstToolReBar.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), &m_wndInstToolBarWnd, AFX_IDW_REBAR))
		return false;

	if (!m_wndInstToolBar.CreateEx(&m_wndInstToolReBar, TBSTYLE_FLAT | TBSTYLE_TRANSPARENT, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) || !m_wndInstToolBar.LoadToolBar(IDT_INSTRUMENT))
		return false;

	m_wndInstToolBar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);

	// Set 24-bit icons
	HBITMAP hbm = (HBITMAP)::LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDB_TOOLBAR_INST_256), IMAGE_BITMAP, DPI::SX(96), DPI::SY(16), LR_CREATEDIBSECTION);
	m_bmInstToolbar.Attach(hbm);
	m_ilInstToolBar.Create(DPI::SX(16), DPI::SY(16), ILC_COLOR24 | ILC_MASK, 4, 4);
	m_ilInstToolBar.Add(&m_bmInstToolbar, MakeRGB(255, 0, 255));
	m_wndInstToolBar.GetToolBarCtrl().SetImageList(&m_ilInstToolBar);

	char name = '\0';		// // //

	rbi.cbSize		= sizeof(REBARBANDINFO);
	rbi.fMask		= RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_STYLE | RBBIM_TEXT;
	rbi.fStyle		= RBBS_NOGRIPPER;
	rbi.cxMinChild	= DPI::SX(300);
	rbi.cyMinChild	= DPI::SY(30);
	rbi.lpText		= &name;
	rbi.cch			= 7;
	rbi.cx			= DPI::SX(300);
	rbi.hwndChild	= m_wndInstToolBar;

	m_wndInstToolReBar.InsertBand(-1, &rbi);

	// Route messages to this window
	m_wndInstToolBar.SetOwner(this);

	// Turn add new instrument button into a drop-down list
	m_wndInstToolBar.SetButtonStyle(0, TBBS_DROPDOWN);
	// Turn load instrument button into a drop-down list
	m_wndInstToolBar.SetButtonStyle(4, TBBS_DROPDOWN);

	return true;
}

void CMainFrame::ResizeFrameWindow()
{
	// Called when the number of channels has changed
	ASSERT(m_pFrameEditor != NULL);

	if (HasDocument()) {		// // //
		auto pView = static_cast<CFamiTrackerView *>(GetActiveView());		// // //
		int Channels = pView->GetSongView()->GetChannelOrder().GetChannelCount();
		int Height = 0, Width = 0;

		// Located to the right
		if (m_iFrameEditorPos == FRAME_EDIT_POS_TOP) {
			// Frame editor window
			Height = CFrameEditor::DEFAULT_HEIGHT;		// // // 050B
			Width = m_pFrameEditor->CalcWidth(Channels);

			m_pFrameEditor->MoveWindow(DPI::Rect(12, 12, Width, Height));		// // //

			// Move frame controls
			m_wndFrameControls.MoveWindow(DPI::Rect(10, Height + 21, 150, 26));		// // //
		}
		// Located to the left
		else {
			// Frame editor window
			CRect rect;
			m_wndVerticalControlBar.GetClientRect(&rect);

			Height = rect.Height() - DPI::SY(CPatternEditor::HEADER_HEIGHT - 2);		// // //
			Width = m_pFrameEditor->CalcWidth(Channels);

			m_pFrameEditor->MoveWindow(DPI::SX(2), DPI::SY(CPatternEditor::HEADER_HEIGHT + 1), DPI::SX(Width), Height);		// // //

			// Move frame controls
			m_wndFrameControls.MoveWindow(DPI::Rect(4, 10, 150, 26));
		}

		// Vertical control bar
		m_wndVerticalControlBar.m_sizeDefault.cx = DPI::SX(Width + 4);		// // // 050B
		m_wndVerticalControlBar.CalcFixedLayout(TRUE, FALSE);
		RecalcLayout();
	}


	int DialogStartPos = 0;
	if (m_iFrameEditorPos == FRAME_EDIT_POS_TOP) {
		CRect FrameEditorRect;
		m_pFrameEditor->GetClientRect(&FrameEditorRect);
		DialogStartPos = FrameEditorRect.right + DPI::SX(32);		// // // 050B
	}

	CRect ParentRect;
	m_wndControlBar.GetClientRect(&ParentRect);
	m_wndDialogBar.MoveWindow(DialogStartPos, DPI::SY(2), ParentRect.Width() - DialogStartPos, ParentRect.Height() - DPI::SY(4));		// // //

	CRect ControlRect;		// // //
	m_wndDialogBar.GetDlgItem(IDC_MAINFRAME_INST_TOOLBAR)->GetWindowRect(&ControlRect);
	GetDesktopWindow()->MapWindowPoints(&m_wndDialogBar, ControlRect);
	m_wndInstToolBarWnd.MoveWindow(ControlRect);

	m_pFrameEditor->RedrawWindow();
}

void CMainFrame::SetupColors()
{
	ASSERT(m_pInstrumentList != NULL);

	// Instrument list colors
	m_pInstrumentList->SetBkColor(theApp.GetSettings()->Appearance.iColBackground);
	m_pInstrumentList->SetTextBkColor(theApp.GetSettings()->Appearance.iColBackground);
	m_pInstrumentList->SetTextColor(theApp.GetSettings()->Appearance.iColPatternText);
	m_pInstrumentList->RedrawWindow();
}

void CMainFrame::SetTempo(int Tempo)
{
	CFamiTrackerDoc &Doc = GetDoc();
	int MinTempo = Doc.GetSpeedSplitPoint();
	Tempo = std::clamp(Tempo, MinTempo, MAX_TEMPO);
	if (Tempo != Doc.GetSongTempo(m_iTrack)) {		// // //
		Doc.ModifyIrreversible();
		Doc.SetSongTempo(m_iTrack, Tempo);
		theApp.GetSoundGenerator()->ResetTempo();

		if (m_wndDialogBar.GetDlgItemInt(IDC_TEMPO) != Tempo)
			m_wndDialogBar.SetDlgItemInt(IDC_TEMPO, Tempo, FALSE);
	}
}

void CMainFrame::SetSpeed(int Speed)
{
	CFamiTrackerDoc &Doc = GetDoc();
	int MaxSpeed = Doc.GetSongTempo(m_iTrack) ? Doc.GetSpeedSplitPoint() - 1 : 0xFF;
	Speed = std::clamp(Speed, MIN_SPEED, MaxSpeed);
	if (Speed != Doc.GetSongSpeed(m_iTrack) || Doc.GetSongGroove(m_iTrack)) {		// // //
		Doc.ModifyIrreversible();
		Doc.SetSongGroove(m_iTrack, false);
		Doc.SetSongSpeed(m_iTrack, Speed);
		theApp.GetSoundGenerator()->ResetTempo();

		if (m_wndDialogBar.GetDlgItemInt(IDC_SPEED) != Speed)
			m_wndDialogBar.SetDlgItemInt(IDC_SPEED, Speed, FALSE);
	}
}

void CMainFrame::SetGroove(int Groove) {
	CFamiTrackerDoc &Doc = GetDoc();
	Groove = std::clamp(Groove, 0, MAX_GROOVE - 1);
	if (Groove != Doc.GetSongSpeed(m_iTrack) || !Doc.GetSongGroove(m_iTrack)) {		// // //
		Doc.ModifyIrreversible();
		Doc.SetSongGroove(m_iTrack, true);
		Doc.SetSongSpeed(m_iTrack, Groove);
		theApp.GetSoundGenerator()->ResetTempo();

		if (m_wndDialogBar.GetDlgItemInt(IDC_SPEED) != Groove)
			m_wndDialogBar.SetDlgItemInt(IDC_SPEED, Groove, FALSE);
	}
}

void CMainFrame::SetRowCount(int Count)
{
	if (!GetDoc().IsFileLoaded())
		return;

	Count = std::clamp(Count, 1, MAX_PATTERN_LENGTH);		// // //
	if (AddAction(std::make_unique<CPActionPatternLen>(Count)))		// // //
		if (m_wndDialogBar.GetDlgItemInt(IDC_ROWS) != Count)
			m_wndDialogBar.SetDlgItemInt(IDC_ROWS, Count, FALSE);
}

void CMainFrame::SetFrameCount(int Count)
{
	if (!GetDoc().IsFileLoaded())
		return;

	Count = std::clamp(Count, 1, MAX_FRAMES);		// // //
	if (AddAction(std::make_unique<CFActionFrameCount>(Count)))		// // //
		if (m_wndDialogBar.GetDlgItemInt(IDC_FRAMES) != Count)
			m_wndDialogBar.SetDlgItemInt(IDC_FRAMES, Count, FALSE);
}

void CMainFrame::UpdateControls()
{
	m_wndDialogBar.UpdateDialogControls(&m_wndDialogBar, TRUE);
}

void CMainFrame::SetHighlightRows(const stHighlight &Hl)		// // //
{
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT1, Hl.First);
	m_wndOctaveBar.SetDlgItemInt(IDC_HIGHLIGHT2, Hl.Second);
}

void CMainFrame::DisplayOctave()
{
	CComboBox *pOctaveList = static_cast<CComboBox*>(m_wndOctaveBar.GetDlgItem(IDC_OCTAVE));
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pOctaveList->SetCurSel(GetSelectedOctave());		// // //
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

CFamiTrackerDoc &CMainFrame::GetDoc() {		// // //
	auto pDoc = dynamic_cast<CFamiTrackerDoc *>(GetActiveDocument());
	ASSERT_VALID(pDoc);
	return *pDoc;
}

const CFamiTrackerDoc &CMainFrame::GetDoc() const {		// // //
	return const_cast<CMainFrame *>(this)->GetDoc();
}

CSongData *CMainFrame::GetCurrentSong() {
	return GetDoc().GetSong(GetSelectedTrack());
}

const CSongData *CMainFrame::GetCurrentSong() const {
	return const_cast<CMainFrame *>(this)->GetCurrentSong();
}

bool CMainFrame::HasDocument() const {		// // //
	return const_cast<CMainFrame *>(this)->GetActiveDocument() != nullptr;
}


// CMainFrame message handlers

template <typename... T>
void CMainFrame::SetStatusText(LPCTSTR Text, T&&... args)		// // //
{
	if (!Text)
		return;

	char Buf[512] = { };
	_sntprintf_s(Buf, sizeof(Buf), _TRUNCATE, Text, std::forward<T>(args)...);
	m_wndStatusBar.SetWindowText(Buf);
}

void CMainFrame::ClearInstrumentList()
{
	// Remove all items from the instrument list
	m_pInstrumentList->DeleteAllItems();
	SetInstrumentEditName(_T(""));
}

void CMainFrame::NewInstrument(inst_type_t Inst)		// // //
{
	// Add new instrument to module
	CFamiTrackerDoc &Doc = GetDoc();

	if (unsigned Index = Doc.GetFreeInstrumentIndex(); Index != INVALID_INSTRUMENT) {
		if (auto pInst = Doc.GetInstrumentManager()->CreateNew(Inst)) {
			pInst->OnBlankInstrument();		// // //
			AddAction(std::make_unique<ModuleAction::CAddInst>(Index, std::move(pInst)));
		}
#ifdef _DEBUG
		else
			MessageBox(_T("(TODO) add instrument definitions for this chip"), _T("Stop"), MB_OK);
#endif
	}
	else
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR);
}

void CMainFrame::OnAddInstrument2A03()
{
	NewInstrument(INST_2A03);
}

void CMainFrame::OnAddInstrumentVRC6()
{
	NewInstrument(INST_VRC6);
}

void CMainFrame::OnAddInstrumentVRC7()
{
	NewInstrument(INST_VRC7);
}

void CMainFrame::OnAddInstrumentFDS()
{
	NewInstrument(INST_FDS);
}

void CMainFrame::OnAddInstrumentMMC5()
{
	NewInstrument(INST_2A03);
}

void CMainFrame::OnAddInstrumentN163()
{
	NewInstrument(INST_N163);
}

void CMainFrame::OnAddInstrumentS5B()
{
	NewInstrument(INST_S5B);
}

void CMainFrame::UpdateInstrumentList()
{
	// Rewrite the instrument list
	ClearInstrumentList();
	for (int i = 0; i < MAX_INSTRUMENTS; ++i)
		m_pInstrumentList->InsertInstrument(i);
	m_pInstrumentList->SelectInstrument(GetSelectedInstrument());		// // //

	UpdateInstrumentName();		// // //
}

void CMainFrame::SelectInstrument(int Index)
{
	// Set the selected instrument
	//
	// This might get called with non-existing instruments, in that case
	// set that instrument and clear the selection in the instrument list
	//

	if (Index == INVALID_INSTRUMENT)
		return;

	int ListCount = m_pInstrumentList->GetItemCount();

	// No instruments added
	if (ListCount == 0) {
		m_iInstrument = 0;
		SetInstrumentEditName(_T(""));
		return;
	}

	if (auto pInst = GetDoc().GetInstrument(Index)) {
		// Select instrument in list
		m_pInstrumentList->SelectInstrument(Index);

		// Set instrument name
		SetInstrumentEditName(pInst->GetName().data());

		// Update instrument editor
		if (m_wndInstEdit.IsOpened())
			m_wndInstEdit.SetCurrentInstrument(Index);
	}
	else {
		// Remove selection
		m_pInstrumentList->SetSelectionMark(-1);
		m_pInstrumentList->SetItemState(m_iInstrument, 0, LVIS_SELECTED | LVIS_FOCUSED);
		SetInstrumentEditName(_T(""));
		CloseInstrumentEditor();
	}

	// Save selected instrument
	m_iInstrument = Index;
}

int CMainFrame::GetSelectedInstrument() const
{
	// Returns selected instrument
	return m_iInstrument;
}

void CMainFrame::SwapInstruments(int First, int Second)
{
	AddAction(std::make_unique<ModuleAction::CSwapInst>(First, Second));		// // //
}

void CMainFrame::UpdateInstrumentName() const {		// // //
	if (auto pInst = GetDoc().GetInstrument(GetSelectedInstrument())) {
		LPCTSTR pName = (LPCTSTR)CA2CT(pInst->GetName().data());
		m_pInstrumentList->SetInstrumentName(GetSelectedInstrument(), pName);
		m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(pName);
	}
	else
		m_wndDialogBar.GetDlgItem(IDC_INSTNAME)->SetWindowText(_T(""));
}

void CMainFrame::OnNextInstrument()
{
	// Select next instrument in the list
	m_pInstrumentList->SelectNextItem();
}

void CMainFrame::OnPrevInstrument()
{
	// Select previous instrument in the list
	m_pInstrumentList->SelectPreviousItem();
}

void CMainFrame::OnNextOctave()		// // //
{
	int Octave = GetSelectedOctave();
	if (Octave < 7)
		SelectOctave(Octave + 1);
}

void CMainFrame::OnPreviousOctave()		// // //
{
	int Octave = GetSelectedOctave();
	if (Octave > 0)
		SelectOctave(Octave - 1);
}

static const int INST_DIGITS = 2;		// // //

void CMainFrame::OnTypeInstrumentNumber()		// // //
{
	m_iInstNumDigit = 1;
	m_iInstNumCurrent = 0;
	ShowInstrumentNumberText();
}

bool CMainFrame::TypeInstrumentNumber(int Digit)		// // //
{
	if (!m_iInstNumDigit) return false;
	if (Digit != -1) {
		m_iInstNumCurrent |= Digit << (4 * (INST_DIGITS - m_iInstNumDigit++));
		ShowInstrumentNumberText();
		if (m_iInstNumDigit > INST_DIGITS) {
			if (m_iInstNumCurrent >= 0 && m_iInstNumCurrent < MAX_INSTRUMENTS)
				SelectInstrument(m_iInstNumCurrent);
			else {
				SetMessageText(IDS_INVALID_INST_NUM);
				MessageBeep(MB_ICONWARNING);
			}
			m_iInstNumDigit = 0;
		}
	}
	else {
		SetMessageText(IDS_INVALID_INST_NUM);
		MessageBeep(MB_ICONWARNING);
		m_iInstNumDigit = 0;
	}
	return true;
}

void CMainFrame::ShowInstrumentNumberText()		// // //
{
	std::string digit;
	for (int i = 0; i < INST_DIGITS; ++i)
		if (i >= m_iInstNumDigit - 1)
			digit += '_';
		else
			digit += conv::to_digit((m_iInstNumCurrent >> (4 * (INST_DIGITS - i - 1))) & 0xF);

	CString msg;
	AfxFormatString1(msg, IDS_TYPE_INST_NUM, digit.c_str());
	SetMessageText(msg);
}

void CMainFrame::SetInstrumentEditName(const char *pText)		// // //
{
	m_wndDialogBar.SetDlgItemText(IDC_INSTNAME, pText);
}

void CMainFrame::SetIndicatorTime(int Min, int Sec, int MSec)
{
	static int LMin, LSec, LMSec;

	if (Min != LMin || Sec != LSec || MSec != LMSec) {
		LMin = Min;
		LSec = Sec;
		LMSec = MSec;
		CString String;
		String.Format(_T("%02i:%02i:%01i0"), Min, Sec, MSec);
		m_wndStatusBar.SetPaneText(6, String);
	}
}

void CMainFrame::SetIndicatorPos(int Frame, int Row)
{
	std::string String = theApp.GetSettings()->General.bRowInHex ?		// // //
		(conv::from_int_hex(Row, 2) + " / " + conv::from_int_hex(Frame, 2)) :
		(conv::from_int(Row, 3) + " / " + conv::from_int(Frame, 3));
	m_wndStatusBar.SetPaneText(7, String.c_str());
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	if (m_wndToolBarReBar.m_hWnd == NULL)
		return;

	m_wndToolBarReBar.GetReBarCtrl().MinimizeBand(0);

	if (nType != SIZE_MINIMIZED)
		ResizeFrameWindow();
}

void CMainFrame::OnInstNameChange()
{
	int SelIndex = m_pInstrumentList->GetSelectionMark();
	int SelInstIndex = m_pInstrumentList->GetInstrumentIndex(SelIndex);

	if (SelIndex == -1 || m_pInstrumentList->GetItemCount() == 0)
		return;

	if (SelInstIndex != m_iInstrument)	// Instrument selection out of sync, ignore name change
		return;

	if (auto pInst = GetDoc().GetInstrument(m_iInstrument)) {		// // //
		TCHAR Text[CInstrument::INST_NAME_MAX];
		auto pEdit = static_cast<CEdit *>(m_wndDialogBar.GetDlgItem(IDC_INSTNAME));
		pEdit->GetWindowText(Text, CInstrument::INST_NAME_MAX);
		// Update instrument list & document
		auto sel = pEdit->GetSel();
		CT2CA str(Text);
		if (AddAction(std::make_unique<ModuleAction::CInstName>(GetSelectedInstrument(), (const char *)str)))		// // //
			pEdit->SetSel(sel);
	}
}

void CMainFrame::OnAddInstrument()
{
	// Add new instrument to module

	// Chip type depends on selected channel
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	switch (GetChipFromChannel(pView->GetSelectedChannelID())) {		// // // TODO: remove eventually
	case sound_chip_t::APU:  return OnAddInstrument2A03();
	case sound_chip_t::VRC6: return OnAddInstrumentVRC6();
	case sound_chip_t::VRC7: return OnAddInstrumentVRC7();
	case sound_chip_t::FDS:  return OnAddInstrumentFDS();
	case sound_chip_t::MMC5: return OnAddInstrumentMMC5();
	case sound_chip_t::N163: return OnAddInstrumentN163();
	case sound_chip_t::S5B:  return OnAddInstrumentS5B();
	}
}

void CMainFrame::OnRemoveInstrument()
{
	// Remove from document
	int prev = GetSelectedInstrument();
	if (AddAction(std::make_unique<ModuleAction::CRemoveInst>(m_iInstrument))) {		// // //
//		m_pInstrumentList->RemoveInstrument(prev);
//		m_pInstrumentList->SelectInstrument(m_iInstrument);
	}
}

void CMainFrame::OnCloneInstrument()
{
	CFamiTrackerDoc &Doc = GetDoc();

	// No instruments in list
	if (m_pInstrumentList->GetItemCount() == 0)
		return;

	auto Manager = Doc.GetInstrumentManager();		// // //
	if (Manager->CloneInstrument(m_iInstrument, Manager->GetFirstUnused())) {
		Doc.ModifyIrreversible();		// // //
		Doc.UpdateAllViews(NULL, UPDATE_INSTRUMENT);
	}
	else
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR);
}

void CMainFrame::OnDeepCloneInstrument()
{
	CFamiTrackerDoc &Doc = GetDoc();

	// No instruments in list
	if (m_pInstrumentList->GetItemCount() == 0)
		return;

	auto Manager = Doc.GetInstrumentManager();		// // //
	if (Manager->DeepCloneInstrument(m_iInstrument, Manager->GetFirstUnused())) {
		Doc.ModifyIrreversible();		// // //
		Doc.UpdateAllViews(NULL, UPDATE_INSTRUMENT);
	}
	else
		AfxMessageBox(IDS_INST_LIMIT, MB_ICONERROR);
}

void CMainFrame::OnBnClickedEditInst()
{
	OpenInstrumentEditor();
}

void CMainFrame::OnEditInstrument()
{
	OpenInstrumentEditor();
}

bool CMainFrame::LoadInstrument(unsigned Index, const CString &filename) {		// // //
	const auto err = [this] (int ID) {
		AfxMessageBox(ID, MB_ICONERROR);
		return false;
	};

	auto &Doc = GetDoc();
	if (Index != INVALID_INSTRUMENT) {
		if (CSimpleFile file(filename, std::ios::in | std::ios::binary); file)
			return Doc.LoadInstrument(Index, file);
		return err(IDS_FILE_OPEN_ERROR);
	}
	return err(IDS_INST_LIMIT);
}

void CMainFrame::OnLoadInstrument()
{
	// Loads an instrument from a file

	CString filter = LoadDefaultFilter(IDS_FILTER_FTI, _T(".fti"));
	CFileDialog FileDialog(TRUE, _T("fti"), 0, OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT, filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_FTI);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	POSITION pos (FileDialog.GetStartPosition());

	auto &Im = *GetDoc().GetInstrumentManager();		// // //

	// Load multiple files
	while (pos) {
		int Index = Im.GetFirstUnused();		// // //
		if (!LoadInstrument(Index, FileDialog.GetNextPathName(pos)))
			return UpdateInstrumentList();
		SelectInstrument(Index);		// // //
	}
	UpdateInstrumentList();

	if (FileDialog.GetFileName().GetLength() == 0)		// // //
		theApp.GetSettings()->SetPath(FileDialog.GetPathName() + _T("\\"), PATH_FTI);
	else
		theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_FTI);
}

void CMainFrame::OnSaveInstrument()
{
#ifdef DISABLE_SAVE		// // //
	SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif
	// Saves instrument to a file

	const CFamiTrackerDoc &Doc = GetDoc();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());

	int Index = GetSelectedInstrument();
	if (Index == INVALID_INSTRUMENT)
		return;

	auto pInst = Doc.GetInstrument(Index);		// // //
	if (!pInst)
		return;

	auto Name = std::string(pInst->GetName());		// // //

	// Remove bad characters
	for (auto &ch : Name)
		if (ch == '/')
			ch = ' ';

	CString filter = LoadDefaultFilter(IDS_FILTER_FTI, _T(".fti"));
	CFileDialog FileDialog(FALSE, _T("fti"), Name.c_str(), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, filter);

	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_FTI);
	if (FileDialog.DoModal() == IDCANCEL)
		return;
	theApp.GetSettings()->SetPath(FileDialog.GetPathName(), PATH_FTI);

	CSimpleFile file(FileDialog.GetPathName(), std::ios::out | std::ios::binary);
	if (!file) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR, MB_ICONERROR);
		return;
	}

	pInst->SaveFTI(file);		// // //

	if (m_pInstrumentFileTree)
		m_pInstrumentFileTree->Changed();
}

void CMainFrame::OnDeltaposSpeedSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	CFamiTrackerDoc *pDoc = CFamiTrackerDoc::GetDoc();		// // //
	int NewSpeed = pDoc->GetSongSpeed(m_iTrack) - ((NMUPDOWN*)pNMHDR)->iDelta;

	if (!pDoc->GetSongGroove(m_iTrack)) {
		SetSpeed(NewSpeed);
		return;
	}
	else if (((NMUPDOWN*)pNMHDR)->iDelta < 0) {
		for (int i = pDoc->GetSongSpeed(m_iTrack) + 1; i < MAX_GROOVE; ++i)
			if (pDoc->HasGroove(i)) {
				SetGroove(i);
				return;
			}
	}
	else if (((NMUPDOWN*)pNMHDR)->iDelta > 0) {
		for (int i = pDoc->GetSongSpeed(m_iTrack) - 1; i >= 0; --i)
			if (pDoc->HasGroove(i)) {
				SetGroove(i);
				return;
			}
	}
}

void CMainFrame::OnDeltaposTempoSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int NewTempo = CFamiTrackerDoc::GetDoc()->GetSongTempo(m_iTrack) - ((NMUPDOWN*)pNMHDR)->iDelta;
	SetTempo(NewTempo);
}

void CMainFrame::OnDeltaposRowsSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int NewRows = CFamiTrackerDoc::GetDoc()->GetPatternLength(m_iTrack) - ((NMUPDOWN*)pNMHDR)->iDelta;
	SetRowCount(NewRows);
}

void CMainFrame::OnDeltaposFrameSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int NewFrames = CFamiTrackerDoc::GetDoc()->GetFrameCount(m_iTrack) - ((NMUPDOWN*)pNMHDR)->iDelta;
	SetFrameCount(NewFrames);
}

void CMainFrame::OnTrackerPlay()
{
	// Play
	theApp.StartPlayer(play_mode_t::Frame);
}

void CMainFrame::OnTrackerPlaypattern()
{
	// Loop pattern
	theApp.StartPlayer(play_mode_t::RepeatFrame);
}

void CMainFrame::OnTrackerPlayStart()
{
	// Play from start of song
	theApp.StartPlayer(play_mode_t::Song);
}

void CMainFrame::OnTrackerPlayCursor()
{
	// Play from cursor
	theApp.StartPlayer(play_mode_t::Cursor);
}

void CMainFrame::OnTrackerPlayMarker()		// // // 050B
{
	// Play from row marker
	if (static_cast<CFamiTrackerView*>(GetActiveView())->IsMarkerValid())
		theApp.StartPlayer(play_mode_t::Marker);
}

void CMainFrame::OnUpdateTrackerPlayMarker(CCmdUI *pCmdUI)		// // // 050B
{
	pCmdUI->Enable(static_cast<CFamiTrackerView*>(GetActiveView())->IsMarkerValid() ? TRUE : FALSE);
}

void CMainFrame::OnTrackerSetMarker()		// // // 050B
{
	auto pView = static_cast<CFamiTrackerView*>(GetActiveView());
	int Frame = pView->GetSelectedFrame();
	int Row = pView->GetSelectedRow();

	if (Frame == pView->GetMarkerFrame() && Row == pView->GetMarkerRow())
		pView->SetMarker(-1, -1);
	else
		pView->SetMarker(Frame, Row);
}

void CMainFrame::OnTrackerTogglePlay()
{
	// Toggle playback
	theApp.TogglePlayer();
}

void CMainFrame::OnTrackerStop()
{
	// Stop playback
	theApp.StopPlayer();
}

void CMainFrame::OnTrackerKillsound()
{
	theApp.LoadSoundConfig();
	theApp.SilentEverything();
}

bool CMainFrame::CheckRepeat() const
{
	static UINT LastTime, RepeatCounter;
	UINT CurrentTime = GetTickCount();

	if ((CurrentTime - LastTime) < REPEAT_TIME) {
		if (RepeatCounter < REPEAT_DELAY)
			RepeatCounter++;
	}
	else {
		RepeatCounter = 0;
	}

	LastTime = CurrentTime;

	return RepeatCounter == REPEAT_DELAY;
}

void CMainFrame::OnBnClickedIncFrame()
{
	if (static_cast<CFamiTrackerView*>(GetActiveView())->GetSelectedFrame() != GetFrameEditor()->GetEditFrame())		// // //
		return;
	int Add = (CheckRepeat() ? 4 : 1);
	if (ChangeAllPatterns())
		AddAction(std::make_unique<CFActionChangePatternAll>(Add));		// // //
	else
		AddAction(std::make_unique<CFActionChangePattern>(Add));
}

void CMainFrame::OnBnClickedDecFrame()
{
	if (static_cast<CFamiTrackerView*>(GetActiveView())->GetSelectedFrame() != GetFrameEditor()->GetEditFrame())		// // //
		return;
	int Remove = -(CheckRepeat() ? 4 : 1);
	if (ChangeAllPatterns())
		AddAction(std::make_unique<CFActionChangePatternAll>(Remove));		// // //
	else
		AddAction(std::make_unique<CFActionChangePattern>(Remove));
}

bool CMainFrame::ChangeAllPatterns() const
{
	return m_wndFrameControls.IsDlgButtonChecked(IDC_CHANGE_ALL) != 0 && !GetFrameEditor()->IsSelecting();		// // //
}

void CMainFrame::OnKeyRepeat()
{
	theApp.GetSettings()->General.bKeyRepeat = (m_wndDialogBar.IsDlgButtonChecked(IDC_KEYREPEAT) == 1);
}

void CMainFrame::OnDeltaposKeyStepSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	int Pos = m_wndDialogBar.GetDlgItemInt(IDC_KEYSTEP) - ((NMUPDOWN*)pNMHDR)->iDelta;
	m_wndDialogBar.SetDlgItemInt(IDC_KEYSTEP, std::clamp(Pos, 0, MAX_PATTERN_LENGTH));		// // //
}

void CMainFrame::OnEnKeyStepChange()
{
	int Step = m_wndDialogBar.GetDlgItemInt(IDC_KEYSTEP);
	static_cast<CFamiTrackerView*>(GetActiveView())->SetStepping(std::clamp(Step, 0, MAX_PATTERN_LENGTH));		// // //
}

void CMainFrame::OnCreateNSF()
{
	CExportDialog ExportDialog(this);
	ExportDialog.DoModal();
}

void CMainFrame::OnCreateWAV()
{
#ifdef DISABLE_SAVE		// // //
	SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	CCreateWaveDlg WaveDialog;
	WaveDialog.ShowDialog();
}

void CMainFrame::OnNextFrame()
{
	static_cast<CFamiTrackerView*>(GetActiveView())->SelectNextFrame();
}

void CMainFrame::OnPrevFrame()
{
	static_cast<CFamiTrackerView*>(GetActiveView())->SelectPrevFrame();
}

void CMainFrame::OnHelpPerformance()
{
	if (!m_pPerformanceDlg)		// // //
		m_pPerformanceDlg = std::make_unique<CPerformanceDlg>();
	if (!m_pPerformanceDlg->m_hWnd)
		m_pPerformanceDlg->Create(IDD_PERFORMANCE, this);
	if (!m_pPerformanceDlg->IsWindowVisible())
		m_pPerformanceDlg->CenterWindow();
	m_pPerformanceDlg->ShowWindow(SW_SHOW);
	m_pPerformanceDlg->SetFocus();
}

void CMainFrame::OnUpdateSBInstrument(CCmdUI *pCmdUI)
{
	auto String = conv::from_uint_hex(GetSelectedInstrument(), 2);		// // //
	unsigned int Split = static_cast<CFamiTrackerView*>(GetActiveView())->GetSplitInstrument();
	if (Split != MAX_INSTRUMENTS)
		String = conv::from_int_hex(Split, 2) + " / " + String;
	CString msg;
	AfxFormatString1(msg, ID_INDICATOR_INSTRUMENT, String.c_str());
	pCmdUI->Enable();
	pCmdUI->SetText(msg);
}

void CMainFrame::OnUpdateSBOctave(CCmdUI *pCmdUI)
{
	CString String;
	AfxFormatString1(String, ID_INDICATOR_OCTAVE, MakeIntString(GetSelectedOctave()));		// // //
	pCmdUI->Enable();
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBFrequency(CCmdUI *pCmdUI)
{
	const CFamiTrackerDoc &Doc = GetDoc();
	machine_t Machine = Doc.GetMachine();
	int EngineSpeed = Doc.GetEngineSpeed();
	CString String;

	if (EngineSpeed == 0)
		EngineSpeed = (Machine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;

	String.Format(_T("%i Hz"), EngineSpeed);

	pCmdUI->Enable();
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateSBTempo(CCmdUI *pCmdUI)
{
	static int Highlight = m_wndOctaveBar.GetDlgItemInt(IDC_HIGHLIGHT1);

	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	if (pSoundGen && !pSoundGen->IsBackgroundTask()) {
		CString String;
		String.Format(_T("%.2f BPM"), pSoundGen->GetCurrentBPM());		// // //
		pCmdUI->Enable();
		pCmdUI->SetText(String);
	}
}

void CMainFrame::OnUpdateSBChip(CCmdUI *pCmdUI)
{
	CString String;

	const CSoundChipSet &Chip = GetDoc().GetExpansionChip();

	if (!Chip.IsMultiChip())		// // //
		switch (Chip.WithoutChip(sound_chip_t::APU).GetSoundChip()) {
		case sound_chip_t::VRC6: String = _T(" Konami VRC6");       break;
		case sound_chip_t::VRC7: String = _T(" Konami VRC7");       break;
		case sound_chip_t::FDS:  String = _T(" Nintendo FDS");      break;
		case sound_chip_t::MMC5: String = _T(" Nintendo MMC5");     break;
		case sound_chip_t::N163: String = _T(" Namco 163");         break;
		case sound_chip_t::S5B:  String = _T(" Sunsoft 5B");        break;
		case sound_chip_t::NONE: String = _T(" No expansion chip"); break;
		}
	else {
		if (Chip.ContainsChip(sound_chip_t::VRC6))
			String += _T(" + VRC6");
		if (Chip.ContainsChip(sound_chip_t::VRC7))
			String += _T(" + VRC7");
		if (Chip.ContainsChip(sound_chip_t::FDS))
			String += _T(" + FDS");
		if (Chip.ContainsChip(sound_chip_t::MMC5))
			String += _T(" + MMC5");
		if (Chip.ContainsChip(sound_chip_t::N163))
			String += _T(" + N163");
		if (Chip.ContainsChip(sound_chip_t::S5B))
			String += _T(" + S5B");
		String.Delete(0, 3);
	}

	pCmdUI->Enable();
	pCmdUI->SetText(String);
}

void CMainFrame::OnUpdateInsertFrame(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc &Doc = GetDoc();

	if (!Doc.IsFileLoaded())
		return;

	pCmdUI->Enable(Doc.GetFrameCount(m_iTrack) < MAX_FRAMES);
}

void CMainFrame::OnUpdateRemoveFrame(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc &Doc = GetDoc();

	if (!Doc.IsFileLoaded())
		return;

	pCmdUI->Enable(Doc.GetFrameCount(m_iTrack) > 1);
}

void CMainFrame::OnUpdateDuplicateFrame(CCmdUI *pCmdUI)
{
	const CFamiTrackerDoc &Doc = GetDoc();

	if (!Doc.IsFileLoaded())
		return;

	pCmdUI->Enable(Doc.GetFrameCount(m_iTrack) < MAX_FRAMES);
}

void CMainFrame::OnUpdateModuleMoveframedown(CCmdUI *pCmdUI)
{
	const CFamiTrackerDoc &Doc = GetDoc();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());

	if (!Doc.IsFileLoaded())
		return;

	pCmdUI->Enable(!(pView->GetSelectedFrame() == (Doc.GetFrameCount(m_iTrack) - 1)));
}

void CMainFrame::OnUpdateModuleMoveframeup(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable(pView->GetSelectedFrame() > 0);
}

void CMainFrame::OnUpdateInstrumentNew(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetDoc().GetChannelCount() > 0 && m_pInstrumentList->GetItemCount() < MAX_INSTRUMENTS);		// // //
}

void CMainFrame::OnUpdateInstrumentRemove(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0);
}

void CMainFrame::OnUpdateInstrumentClone(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0 && m_pInstrumentList->GetItemCount() < MAX_INSTRUMENTS);
}

void CMainFrame::OnUpdateInstrumentDeepClone(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0 && m_pInstrumentList->GetItemCount() < MAX_INSTRUMENTS);
}

void CMainFrame::OnUpdateInstrumentLoad(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() < MAX_INSTRUMENTS);
}

void CMainFrame::OnUpdateInstrumentSave(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0);
}

void CMainFrame::OnUpdateInstrumentEdit(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pInstrumentList->GetItemCount() > 0);
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	CString text;
	switch (nIDEvent) {
		// Welcome message
		case TMR_WELCOME:
			AfxFormatString1(text, IDS_WELCOME_VER_FORMAT, Get0CCFTVersionString());
			SetMessageText(text);
			KillTimer(TMR_WELCOME);
			break;
		// Check sound player
		case TMR_AUDIO_CHECK:
			CheckAudioStatus();
			break;
#ifdef AUTOSAVE
		// Auto save
		case TMR_AUTOSAVE: {
			/*
				CFamiTrackerDoc *pDoc = dynamic_cast<CFamiTrackerDoc*>(GetActiveDocument());
				if (pDoc != NULL)
					pDoc->AutoSave();
					*/
			}
			break;
#endif
	}

	CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnUpdateKeyStepEdit(CCmdUI *pCmdUI)
{
	pCmdUI->SetText(MakeIntString(static_cast<CFamiTrackerView*>(GetActiveView())->GetStepping()));
}

void CMainFrame::OnUpdateSpeedEdit(CCmdUI *pCmdUI)
{
	if (!m_cLockedEditSpeed.IsEditable()) {
		if (m_cLockedEditSpeed.Update())
			SetSpeed(m_cLockedEditSpeed.GetValue());
		else {
			pCmdUI->SetText(MakeIntString(GetDoc().GetSongSpeed(m_iTrack)));
		}
	}
}

void CMainFrame::OnUpdateTempoEdit(CCmdUI *pCmdUI)
{
	if (!m_cLockedEditTempo.IsEditable()) {
		if (m_cLockedEditTempo.Update())
			SetTempo(m_cLockedEditTempo.GetValue());
		else {
			pCmdUI->SetText(MakeIntString(GetDoc().GetSongTempo(m_iTrack)));
		}
	}
}

void CMainFrame::OnUpdateRowsEdit(CCmdUI *pCmdUI)
{
	if (!m_cLockedEditLength.IsEditable()) {
		if (m_cLockedEditLength.Update())
			SetRowCount(m_cLockedEditLength.GetValue());
		else {
			pCmdUI->SetText(MakeIntString(GetDoc().GetPatternLength(m_iTrack)));
		}
	}
}

void CMainFrame::OnUpdateFramesEdit(CCmdUI *pCmdUI)
{
	if (!m_cLockedEditFrames.IsEditable()) {
		if (m_cLockedEditFrames.Update())
			SetFrameCount(m_cLockedEditFrames.GetValue());
		else {
			pCmdUI->SetText(MakeIntString(GetDoc().GetFrameCount(m_iTrack)));
		}
	}
}

void CMainFrame::OnUpdateHighlight1(CCmdUI *pCmdUI)		// // //
{
	if (!m_cLockedEditHighlight1.IsEditable()) {
		const CFamiTrackerDoc &Doc = GetDoc();
		if (m_cLockedEditHighlight1.Update())
			AddAction(std::make_unique<CPActionHighlight>(stHighlight {
				std::clamp(m_cLockedEditHighlight1.GetValue(), 0, MAX_PATTERN_LENGTH), Doc.GetHighlight(GetSelectedTrack()).Second, 0}));
		else
			pCmdUI->SetText(MakeIntString(Doc.GetHighlight(GetSelectedTrack()).First));
	}
}

void CMainFrame::OnUpdateHighlight2(CCmdUI *pCmdUI)		// // //
{
	if (!m_cLockedEditHighlight2.IsEditable()) {
		const CFamiTrackerDoc &Doc = GetDoc();
		if (m_cLockedEditHighlight2.Update())
			AddAction(std::make_unique<CPActionHighlight>(stHighlight {
				Doc.GetHighlight(GetSelectedTrack()).First, std::clamp(m_cLockedEditHighlight2.GetValue(), 0, MAX_PATTERN_LENGTH), 0}));
		else
			pCmdUI->SetText(MakeIntString(Doc.GetHighlight(GetSelectedTrack()).Second));
	}
}

void CMainFrame::OnFileGeneralsettings()
{
	CString Title;
	GetMessageString(IDS_CONFIG_WINDOW, Title);
	CPropertySheet ConfigWindow(Title, this, 0);

	CConfigGeneral		TabGeneral;
	CConfigVersion		TabVersion;		// // //
	CConfigAppearance	TabAppearance;
	CConfigMIDI			TabMIDI;
	CConfigSound		TabSound;
	CConfigShortcuts	TabShortcuts;
	CConfigMixer		TabMixer;

	ConfigWindow.m_psh.dwFlags	&= ~PSH_HASHELP;
	TabGeneral.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabVersion.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabAppearance.m_psp.dwFlags &= ~PSP_HASHELP;
	TabMIDI.m_psp.dwFlags		&= ~PSP_HASHELP;
	TabSound.m_psp.dwFlags		&= ~PSP_HASHELP;
	TabShortcuts.m_psp.dwFlags	&= ~PSP_HASHELP;
	TabMixer.m_psp.dwFlags		&= ~PSP_HASHELP;

	ConfigWindow.AddPage(&TabGeneral);
	ConfigWindow.AddPage(&TabVersion);
	ConfigWindow.AddPage(&TabAppearance);
	ConfigWindow.AddPage(&TabMIDI);
	ConfigWindow.AddPage(&TabSound);
	ConfigWindow.AddPage(&TabShortcuts);
	ConfigWindow.AddPage(&TabMixer);

	ConfigWindow.DoModal();

	static_cast<CFamiTrackerView*>(GetActiveView())->UpdateNoteQueues();		// // // auto arp setting
}

void CMainFrame::SetSongInfo(const CFamiTrackerDoc &doc)		// // //
{
	m_wndDialogBar.SetDlgItemText(IDC_SONG_NAME, doc.GetModuleName().data());
	m_wndDialogBar.SetDlgItemText(IDC_SONG_ARTIST, doc.GetModuleArtist().data());
	m_wndDialogBar.SetDlgItemText(IDC_SONG_COPYRIGHT, doc.GetModuleCopyright().data());
}

void CMainFrame::OnEnSongNameChange()
{
	CString str;
	auto pEdit = (CEdit *)m_wndDialogBar.GetDlgItem(IDC_SONG_NAME);
	pEdit->GetWindowText(str);
	auto sel = pEdit->GetSel();
	if (AddAction(std::make_unique<ModuleAction::CTitle>(str.GetString())))
		pEdit->SetSel(sel);
}

void CMainFrame::OnEnSongArtistChange()
{
	CString str;
	auto pEdit = (CEdit *)m_wndDialogBar.GetDlgItem(IDC_SONG_ARTIST);
	pEdit->GetWindowText(str);
	auto sel = pEdit->GetSel();
	if (AddAction(std::make_unique<ModuleAction::CArtist>(str.GetString())))
		pEdit->SetSel(sel);
}

void CMainFrame::OnEnSongCopyrightChange()
{
	CString str;
	auto pEdit = (CEdit *)m_wndDialogBar.GetDlgItem(IDC_SONG_COPYRIGHT);
	pEdit->GetWindowText(str);
	auto sel = pEdit->GetSel();
	if (AddAction(std::make_unique<ModuleAction::CCopyright>(str.GetString())))
		pEdit->SetSel(sel);
}

void CMainFrame::ChangeNoteState(int Note)
{
	m_wndInstEdit.ChangeNoteState(Note);
}

void CMainFrame::OpenInstrumentEditor()
{
	// Bring up the instrument editor for the selected instrument
	CFamiTrackerDoc &Doc = GetDoc();

	int Instrument = GetSelectedInstrument();

	if (Doc.IsInstrumentUsed(Instrument)) {
		if (m_wndInstEdit.IsOpened() == false) {
			m_wndInstEdit.SetInstrumentManager(Doc.GetInstrumentManager());		// // //
			m_wndInstEdit.Create(IDD_INSTRUMENT, this);
			m_wndInstEdit.SetCurrentInstrument(Instrument);
			m_wndInstEdit.ShowWindow(SW_SHOW);
		}
		else
			m_wndInstEdit.SetCurrentInstrument(Instrument);
		m_wndInstEdit.UpdateWindow();
	}
}

void CMainFrame::CloseInstrumentEditor()
{
	// Close the instrument editor, in case it was opened
	if (m_wndInstEdit.IsOpened())
		m_wndInstEdit.DestroyWindow();
}

void CMainFrame::CloseGrooveSettings()		// // //
{
	if (m_pGrooveDlg) {
		m_pGrooveDlg->DestroyWindow();
		m_pGrooveDlg.reset();
	}
}

void CMainFrame::CloseBookmarkSettings()		// // //
{
	if (m_pBookmarkDlg) {
		m_pBookmarkDlg->DestroyWindow();
		m_pBookmarkDlg.reset();
	}
}

void CMainFrame::UpdateBookmarkList(int Pos)		// // //
{
	if (m_pBookmarkDlg) {
		if (m_pBookmarkDlg->IsWindowVisible()) {
			m_pBookmarkDlg->LoadBookmarks(m_iTrack);
			m_pBookmarkDlg->SelectBookmark(Pos);
		}
	}
}

void CMainFrame::OnUpdateKeyRepeat(CCmdUI *pCmdUI)
{
	if (theApp.GetSettings()->General.bKeyRepeat)
		pCmdUI->SetCheck(1);
	else
		pCmdUI->SetCheck(0);
}

void CMainFrame::OnFileImportText()
{
	CString fileFilter = LoadDefaultFilter(IDS_FILTER_TXT, _T(".txt"));
	CFileDialog FileDialog(TRUE, 0, 0, OFN_HIDEREADONLY, fileFilter);

	if (GetActiveDocument()->SaveModified() == 0)
		return;

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	CFamiTrackerDoc &Doc = GetDoc();

	try {
		CTextExport { }.ImportFile(FileDialog.GetPathName(), Doc);
	}
	catch (std::runtime_error err) {
		AfxMessageBox(err.what(), MB_OK | MB_ICONEXCLAMATION);
	}

	SetSongInfo(Doc);		// // //
	Doc.SetModifiedFlag(FALSE);
	Doc.SetExceededFlag(false);		// // //
	// TODO figure out how to handle this case, call OnInitialUpdate??
	//Doc.UpdateAllViews(NULL, CHANGED_ERASE);		// Remove
	Doc.UpdateAllViews(NULL, UPDATE_INSTRUMENT);
	//Doc.UpdateAllViews(NULL, UPDATE_ENTIRE);		// TODO Remove
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(&Doc);
}

void CMainFrame::OnFileExportText()
{
#ifdef DISABLE_SAVE		// // //
	SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	CFamiTrackerDoc &Doc = GetDoc();
	CString	DefFileName = Doc.GetFileTitle();

	CString fileFilter = LoadDefaultFilter(IDS_FILTER_TXT, _T(".txt"));
	CFileDialog FileDialog(FALSE, _T(".txt"), DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, fileFilter);
	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	CTextExport Exporter;
	CString sResult = Exporter.ExportFile(FileDialog.GetPathName(), Doc);		// // //
	if (sResult.GetLength() > 0)
	{
		AfxMessageBox(sResult, MB_OK | MB_ICONEXCLAMATION);
	}
}

void CMainFrame::OnFileExportRows()		// // //
{
#ifdef DISABLE_SAVE		// // //
	SetMessageText(IDS_DISABLE_SAVE);
	return;
#endif

	CFamiTrackerDoc	*pDoc = (CFamiTrackerDoc*)GetActiveDocument();
	CString	DefFileName = pDoc->GetFileTitle();

	CFileDialog FileDialog(FALSE, _T(".csv"), DefFileName, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, _T("Comma-separated values (*.csv)|*.csv|All files|*.*||"));
	FileDialog.m_pOFN->lpstrInitialDir = theApp.GetSettings()->GetPath(PATH_NSF);

	if (FileDialog.DoModal() == IDCANCEL)
		return;

	CTextExport Exporter;
	CString sResult = Exporter.ExportRows(FileDialog.GetPathName(), *pDoc->GetModule());
	if (sResult.GetLength() > 0)
	{
		AfxMessageBox(sResult, MB_OK | MB_ICONEXCLAMATION);
	}
}

BOOL CMainFrame::DestroyWindow()
{
	// Store window position

	CRect WinRect;
	int State = STATE_NORMAL;

	GetWindowRect(WinRect);

	if (IsZoomed()) {
		State = STATE_MAXIMIZED;
		// Ignore window position if maximized
		WinRect.top = theApp.GetSettings()->WindowPos.iTop;
		WinRect.bottom = theApp.GetSettings()->WindowPos.iBottom;
		WinRect.left = theApp.GetSettings()->WindowPos.iLeft;
		WinRect.right = theApp.GetSettings()->WindowPos.iRight;
	}

	if (IsIconic()) {
		WinRect.top = WinRect.left = 100;
		WinRect.bottom = 920;
		WinRect.right = 950;
		DPI::ScaleRect(WinRect);		// // // 050B
	}

	// Save window position
	theApp.GetSettings()->SetWindowPos(WinRect.left, WinRect.top, WinRect.right, WinRect.bottom, State);

	return CFrameWnd::DestroyWindow();
}

BOOL CMainFrame::OnEraseBkgnd(CDC* pDC)
{
	return FALSE;
}

void CMainFrame::OnTrackerSwitchToInstrument()
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pView->SwitchToInstrument(!pView->SwitchToInstrument());
}

// // //

void CMainFrame::OnTrackerDisplayAverageBPM()		// // // 050B
{
	theApp.GetSettings()->Display.bAverageBPM = !theApp.GetSettings()->Display.bAverageBPM;
}

void CMainFrame::OnTrackerDisplayChannelState()		// // // 050B
{
	theApp.GetSettings()->Display.bChannelState = !theApp.GetSettings()->Display.bChannelState;
}

void CMainFrame::OnTrackerDisplayRegisterState()
{
	theApp.GetSettings()->Display.bRegisterState = !theApp.GetSettings()->Display.bRegisterState;		// // //
}

void CMainFrame::OnUpdateDisplayAverageBPM(CCmdUI *pCmdUI)		// // // 050B
{
	pCmdUI->SetCheck(theApp.GetSettings()->Display.bAverageBPM ? MF_CHECKED : MF_UNCHECKED);
}

void CMainFrame::OnUpdateDisplayChannelState(CCmdUI *pCmdUI)		// // // 050B
{
	pCmdUI->SetCheck(theApp.GetSettings()->Display.bChannelState ? MF_CHECKED : MF_UNCHECKED);
}

void CMainFrame::OnUpdateDisplayRegisterState(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->SetCheck(theApp.GetSettings()->Display.bRegisterState ? MF_CHECKED : MF_UNCHECKED);
}

void CMainFrame::OnUpdateTrackerSwitchToInstrument(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->SetCheck(pView->SwitchToInstrument() ? 1 : 0);
}

void CMainFrame::OnModuleModuleproperties()
{
	// Display module properties dialog
	CModulePropertiesDlg propertiesDlg;
	propertiesDlg.DoModal();
}

void CMainFrame::OnModuleChannels()
{
	CChannelsDlg channelsDlg;
	channelsDlg.DoModal();
}

void CMainFrame::OnModuleComments()
{
	CCommentsDlg commentsDlg;
	const CFamiTrackerDoc &Doc = GetDoc();
	commentsDlg.SetComment(Doc.GetModule()->GetComment());		// // //
	commentsDlg.SetShowOnLoad(Doc.GetModule()->ShowsCommentOnOpen());
	if (commentsDlg.DoModal() == IDOK && commentsDlg.IsChanged())
		AddAction(std::make_unique<ModuleAction::CComment>(commentsDlg.GetComment(), commentsDlg.GetShowOnLoad()));		// // //
}

void CMainFrame::OnModuleGrooveSettings()		// // //
{
	if (!m_pGrooveDlg) {
		m_pGrooveDlg = std::make_unique<CGrooveDlg>();
		m_pGrooveDlg->Create(IDD_GROOVE, this);
	}
	if (!m_pGrooveDlg->IsWindowVisible())
		m_pGrooveDlg->CenterWindow();
	m_pGrooveDlg->ShowWindow(SW_SHOW);
	m_pGrooveDlg->SetFocus();
}

void CMainFrame::OnModuleBookmarkSettings()		// // //
{
	if (!m_pBookmarkDlg) {
		m_pBookmarkDlg = std::make_unique<CBookmarkDlg>();
		m_pBookmarkDlg->Create(IDD_BOOKMARKS, this);
	}
	if (!m_pBookmarkDlg->IsWindowVisible())
		m_pBookmarkDlg->CenterWindow();
	m_pBookmarkDlg->ShowWindow(SW_SHOW);
	m_pBookmarkDlg->SetFocus();
	m_pBookmarkDlg->LoadBookmarks(m_iTrack);
}

void CMainFrame::OnModuleEstimateSongLength()		// // //
{
	CSongLengthScanner scanner {*GetDoc().GetModule(), *static_cast<CFamiTrackerView *>(GetActiveView())->GetSongView()};
	auto [Intro, Loop] = scanner.GetSecondsCount();
	auto [IntroRows, LoopRows] = scanner.GetRowCount();

	int Rate = GetDoc().GetFrameRate();

	const TCHAR *fmt = _T("Estimated duration:\n"
		"Intro: %lld:%02lld.%02lld (%d rows, %lld ticks)\n"
		"Loop: %lld:%02lld.%02lld (%d rows, %lld ticks)\n"
		"Tick counts are subject to rounding errors!");
	CString str;
	str.Format(fmt,
		static_cast<long long>(Intro + .5 / 6000) / 60,
		static_cast<long long>(Intro + .005) % 60,
		static_cast<long long>(Intro * 100 + .5) % 100,
		IntroRows,
		static_cast<long long>(Intro * Rate + .5),
		static_cast<long long>(Loop + .5 / 6000) / 60,
		static_cast<long long>(Loop + .005) % 60,
		static_cast<long long>(Loop * 100 + .5) % 100,
		LoopRows,
		static_cast<long long>(Loop * Rate + .5));
	AfxMessageBox(str);
}

void CMainFrame::UpdateTrackBox()
{
	// Fill the track box with all songs
	CComboBox		*pTrackBox	= static_cast<CComboBox*>(m_wndDialogBar.GetDlgItem(IDC_SUBTUNE));
	const CFamiTrackerModule &modfile = *GetDoc().GetModule();		// // //
	CString			Text;

	ASSERT(pTrackBox != NULL);

	pTrackBox->ResetContent();

	modfile.VisitSongs([&] (const CSongData &song, unsigned i) {
		auto sv = song.GetTitle();
		Text.Format(_T("#%i %.*s"), i + 1, sv.size(), sv.data());		// // //
		pTrackBox->AddString(Text);
	});

	int Count = modfile.GetSongCount();
	if (GetSelectedTrack() > (Count - 1))
		SelectTrack(Count - 1);

	pTrackBox->SetCurSel(GetSelectedTrack());
}

void CMainFrame::OnCbnSelchangeSong()
{
	CComboBox *pTrackBox = static_cast<CComboBox*>(m_wndDialogBar.GetDlgItem(IDC_SUBTUNE));
	int Track = pTrackBox->GetCurSel();
	SelectTrack(Track);
	GetActiveView()->SetFocus();
}

void CMainFrame::OnCbnSelchangeOctave()
{
	CComboBox *pTrackBox	= static_cast<CComboBox*>(m_wndOctaveBar.GetDlgItem(IDC_OCTAVE));
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	unsigned int Octave		= pTrackBox->GetCurSel();

	if (GetSelectedOctave() != Octave)		// // //
		SelectOctave(Octave);
}

void CMainFrame::OnRemoveFocus()
{
	GetActiveView()->SetFocus();
}

void CMainFrame::OnNextSong()
{
	int Tracks = GetDoc().GetModule()->GetSongCount();		// // //
	int Track = GetSelectedTrack();
	if (Track < (Tracks - 1))
		SelectTrack(Track + 1);
}

void CMainFrame::OnPrevSong()
{
	int Track = GetSelectedTrack();
	if (Track > 0)
		SelectTrack(Track - 1);
}

void CMainFrame::OnUpdateNextSong(CCmdUI *pCmdUI)
{
	int Tracks = GetDoc().GetModule()->GetSongCount();		// // //

	if (GetSelectedTrack() < (Tracks - 1))
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdatePrevSong(CCmdUI *pCmdUI)
{
	if (GetSelectedTrack() > 0)
		pCmdUI->Enable(TRUE);
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnClickedFollow()
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	bool FollowMode = m_wndOctaveBar.IsDlgButtonChecked(IDC_FOLLOW) != 0;
	theApp.GetSettings()->FollowMode = FollowMode;
	pView->SetFollowMode(FollowMode);
	pView->SetFocus();
}

void CMainFrame::OnToggleFollow()
{
	m_wndOctaveBar.CheckDlgButton(IDC_FOLLOW, !m_wndOctaveBar.IsDlgButtonChecked(IDC_FOLLOW));
	OnClickedFollow();
}

void CMainFrame::OnUpdateToggleFollow(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->SetCheck(m_wndOctaveBar.IsDlgButtonChecked(IDC_FOLLOW) != 0);
}

void CMainFrame::OnClickedCompact()		// // //
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	bool CompactMode = m_wndOctaveBar.IsDlgButtonChecked(IDC_CHECK_COMPACT) != 0;
	pView->SetCompactMode(CompactMode);
	pView->SetFocus();
}

void CMainFrame::OnToggleCompact()		// // //
{
	m_wndOctaveBar.CheckDlgButton(IDC_CHECK_COMPACT, !m_wndOctaveBar.IsDlgButtonChecked(IDC_CHECK_COMPACT));
	OnClickedCompact();
}

void CMainFrame::OnUpdateToggleCompact(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->SetCheck(m_wndOctaveBar.IsDlgButtonChecked(IDC_CHECK_COMPACT) != 0);
}

void CMainFrame::OnViewControlpanel()
{
	if (m_wndControlBar.IsVisible()) {
		m_wndControlBar.ShowWindow(SW_HIDE);
	}
	else {
		m_wndControlBar.ShowWindow(SW_SHOW);
		m_wndControlBar.UpdateWindow();
	}

	RecalcLayout();
	ResizeFrameWindow();
}

void CMainFrame::OnUpdateViewControlpanel(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_wndControlBar.IsVisible());
}

void CMainFrame::OnDeltaposHighlightSpin1(NMHDR *pNMHDR, LRESULT *pResult)		// // //
{
	if (HasDocument()) {
		stHighlight Hl = GetDoc().GetHighlight(GetSelectedTrack());
		Hl.First = std::clamp(Hl.First - ((NMUPDOWN*)pNMHDR)->iDelta, 0, MAX_PATTERN_LENGTH);
		AddAction(std::make_unique<CPActionHighlight>(Hl));
		theApp.GetSoundGenerator()->SetHighlightRows(Hl.First);		// // //
	}
}

void CMainFrame::OnDeltaposHighlightSpin2(NMHDR *pNMHDR, LRESULT *pResult)		// // //
{
	if (HasDocument()) {
		stHighlight Hl = GetDoc().GetHighlight(GetSelectedTrack());
		Hl.Second = std::clamp(Hl.Second - ((NMUPDOWN*)pNMHDR)->iDelta, 0, MAX_PATTERN_LENGTH);
		AddAction(std::make_unique<CPActionHighlight>(Hl));
	}
}

void CMainFrame::OnSelectPatternEditor()
{
	GetActiveView()->SetFocus();
}

void CMainFrame::OnSelectFrameEditor()
{
	m_pFrameEditor->EnableInput();
}

void CMainFrame::OnModuleInsertFrame()
{
	AddAction(std::make_unique<CFActionAddFrame>());		// // //
}

void CMainFrame::OnModuleRemoveFrame()
{
	AddAction(std::make_unique<CFActionRemoveFrame>());		// // //
}

void CMainFrame::OnModuleDuplicateFrame()
{
	AddAction(std::make_unique<CFActionDuplicateFrame>());		// // //
}

void CMainFrame::OnModuleDuplicateFramePatterns()
{
	AddAction(std::make_unique<CFActionCloneFrame>());		// // //
}

void CMainFrame::OnModuleMoveframedown()
{
	AddAction(std::make_unique<CFActionMoveDown>());		// // //
}

void CMainFrame::OnModuleMoveframeup()
{
	AddAction(std::make_unique<CFActionMoveUp>());		// // //
}

void CMainFrame::OnModuleDuplicateCurrentPattern()		// // //
{
	AddAction(std::make_unique<CFActionClonePatterns>());
}

// UI updates

void CMainFrame::OnUpdateEditCut(CCmdUI *pCmdUI)
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnUpdateEditCut(pCmdUI);
	else if (GetFocus() == GetFrameEditor())
		pCmdUI->Enable(TRUE);
}

void CMainFrame::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable((pView->IsSelecting() || GetFocus() == m_pFrameEditor.get()) ? 1 : 0);
}

void CMainFrame::OnUpdatePatternEditorSelected(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->Enable(GetFocus() == GetActiveView() ? 1 : 0);
}

void CMainFrame::OnUpdateEditCopySpecial(CCmdUI *pCmdUI)		// // //
{
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable((pView->IsSelecting() && GetFocus() == GetActiveView()) ? 1 : 0);
}

void CMainFrame::OnUpdateSelectMultiFrame(CCmdUI *pCmdUI)		// // //
{
	pCmdUI->Enable(theApp.GetSettings()->General.bMultiFrameSel ? TRUE : FALSE);
}

void CMainFrame::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	if (GetFocus() == GetActiveView())
		pCmdUI->Enable(static_cast<CFamiTrackerView*>(GetActiveView())->IsClipboardAvailable() ? 1 : 0);
	else if (GetFocus() == GetFrameEditor())
		pCmdUI->Enable(GetFrameEditor()->IsClipboardAvailable() ? 1 : 0);
}

void CMainFrame::OnUpdateEditDelete(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable((pView->IsSelecting() || GetFocus() == m_pFrameEditor.get()) ? 1 : 0);
}

void CMainFrame::OnHelpEffecttable()
{
	// Display effect table in help
	HtmlHelp((DWORD)_T("effect_list.htm"), HH_DISPLAY_TOPIC);
}

void CMainFrame::OnHelpFAQ()
{
	// Display FAQ in help
	HtmlHelp((DWORD)_T("faq.htm"), HH_DISPLAY_TOPIC);
}

CFrameEditor *CMainFrame::GetFrameEditor() const
{
	ASSERT(m_pFrameEditor);		// // //
	return m_pFrameEditor.get();
}

void CMainFrame::OnEditEnableMIDI()
{
	theApp.GetMIDI()->ToggleInput();
}

void CMainFrame::OnUpdateEditEnablemidi(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(theApp.GetMIDI()->IsAvailable());
	pCmdUI->SetCheck(theApp.GetMIDI()->IsOpened());
}

void CMainFrame::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CFrameWnd::OnShowWindow(bShow, nStatus);

	if (bShow == TRUE) {
		// Set the window state as saved in settings
		if (theApp.GetSettings()->WindowPos.iState == STATE_MAXIMIZED)
			CFrameWnd::ShowWindow(SW_MAXIMIZE);
	}
}

void CMainFrame::OnDestroy()
{
	TRACE("FrameWnd: Destroying main frame window\n");

	CSoundGen *pSoundGen = theApp.GetSoundGenerator();

	KillTimer(TMR_AUDIO_CHECK);

	// Clean up sound stuff
	if (pSoundGen && pSoundGen->IsRunning()) {
		// Remove visualizer window from sound generator
		pSoundGen->SetVisualizerWindow(NULL);
		// Kill the sound interface since the main window is being destroyed
		CEvent SoundEvent(FALSE, FALSE);		// // //
		pSoundGen->PostThreadMessage(WM_USER_CLOSE_SOUND, (WPARAM)&SoundEvent, NULL);
		// Wait for sound to close
		DWORD dwResult = ::WaitForSingleObject(SoundEvent.m_hObject, /*CSoundGen::AUDIO_TIMEOUT*/ 2000 + 1000);		// // //

		if (dwResult != WAIT_OBJECT_0)
			TRACE(_T("MainFrame: Error while waiting for sound to close!\n"));
	}

	CFrameWnd::OnDestroy();
}

int CMainFrame::GetSelectedTrack() const
{
	return m_iTrack;
}

void CMainFrame::SelectTrack(unsigned int Track)
{
	// Select a new track
	ASSERT(Track < MAX_TRACKS);

	CComboBox *pTrackBox = static_cast<CComboBox*>(m_wndDialogBar.GetDlgItem(IDC_SUBTUNE));
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());

	m_iTrack = Track;

	if (theApp.GetSoundGenerator()->IsPlaying() && Track != theApp.GetSoundGenerator()->GetPlayerTrack())		// // // 050B
		theApp.ResetPlayer();

	pTrackBox->SetCurSel(m_iTrack);
	//GetDoc().UpdateAllViews(NULL, CHANGED_TRACK);

	ResetUndo();		// // // 050B
	UpdateControls();

	pView->TrackChanged(m_iTrack);
	GetFrameEditor()->CancelSelection();		// // //
	GetFrameEditor()->Invalidate();
	OnUpdateFrameTitle(TRUE);

	if (m_pBookmarkDlg != NULL)		// // //
		m_pBookmarkDlg->LoadBookmarks(m_iTrack);
}

int CMainFrame::GetSelectedOctave() const		// // // 050B
{
	return m_iOctave;
}

void CMainFrame::SelectOctave(int Octave)		// // // 050B
{
	static_cast<CFamiTrackerView*>(GetActiveView())->AdjustOctave(Octave - GetSelectedOctave());
	m_iOctave = Octave;
	DisplayOctave();
}

BOOL CMainFrame::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	LPNMTOOLBAR lpnmtb = (LPNMTOOLBAR) lParam;

	// Handle new instrument menu
	switch (((LPNMHDR)lParam)->code) {
		case TBN_DROPDOWN:
			switch (lpnmtb->iItem) {
				case ID_INSTRUMENT_NEW:
					OnNewInstrumentMenu((LPNMHDR)lParam, pResult);
					return FALSE;
				case ID_INSTRUMENT_LOAD:
					OnLoadInstrumentMenu((LPNMHDR)lParam, pResult);
					return FALSE;
			}
	}

	return CFrameWnd::OnNotify(wParam, lParam, pResult);
}

void CMainFrame::OnNewInstrumentMenu(NMHDR* pNotifyStruct, LRESULT* result)
{
	CRect rect;
	::GetWindowRect(pNotifyStruct->hwndFrom, &rect);

	CMenu menu;
	menu.CreatePopupMenu();

	const CFamiTrackerDoc &Doc = GetDoc();
	CFamiTrackerView *pView = static_cast<CFamiTrackerView*>(GetActiveView());

	const CSoundChipSet &Chip = Doc.GetExpansionChip();		// // //
	sound_chip_t SelectedChip = GetChipFromChannel(pView->GetSelectedChannelID());		// // // where the cursor is located

	if (Chip.ContainsChip(sound_chip_t::APU))
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_2A03, _T("New 2A03 instrument"));
	if (Chip.ContainsChip(sound_chip_t::VRC6))
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_VRC6, _T("New VRC6 instrument"));
	if (Chip.ContainsChip(sound_chip_t::VRC7))
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_VRC7, _T("New VRC7 instrument"));
	if (Chip.ContainsChip(sound_chip_t::FDS))
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_FDS, _T("New FDS instrument"));
	if (Chip.ContainsChip(sound_chip_t::MMC5))
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_MMC5, _T("New MMC5 instrument"));
	if (Chip.ContainsChip(sound_chip_t::N163))
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_N163, _T("New Namco instrument"));
	if (Chip.ContainsChip(sound_chip_t::S5B))
		menu.AppendMenu(MF_STRING, ID_INSTRUMENT_ADD_S5B, _T("New Sunsoft instrument"));

	switch (SelectedChip) {
	case sound_chip_t::APU:  menu.SetDefaultItem(ID_INSTRUMENT_ADD_2A03); break;
	case sound_chip_t::VRC6: menu.SetDefaultItem(ID_INSTRUMENT_ADD_VRC6); break;
	case sound_chip_t::VRC7: menu.SetDefaultItem(ID_INSTRUMENT_ADD_VRC7); break;
	case sound_chip_t::FDS:  menu.SetDefaultItem(ID_INSTRUMENT_ADD_FDS);  break;
	case sound_chip_t::MMC5: menu.SetDefaultItem(ID_INSTRUMENT_ADD_MMC5); break;
	case sound_chip_t::N163: menu.SetDefaultItem(ID_INSTRUMENT_ADD_N163); break;
	case sound_chip_t::S5B:  menu.SetDefaultItem(ID_INSTRUMENT_ADD_S5B);  break;
	}

	menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, rect.left, rect.bottom, this);
}

void CMainFrame::OnLoadInstrumentMenu(NMHDR * pNotifyStruct, LRESULT * result)
{
	CRect rect;
	::GetWindowRect(pNotifyStruct->hwndFrom, &rect);

	// Build menu tree
	if (!m_pInstrumentFileTree)
		m_pInstrumentFileTree = std::make_unique<CInstrumentFileTree>();		// // //
	if (m_pInstrumentFileTree->ShouldRebuild())
		m_pInstrumentFileTree->BuildMenuTree(theApp.GetSettings()->InstrumentMenuPath);

	UINT retValue = m_pInstrumentFileTree->GetMenu().TrackPopupMenu(
		TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, rect.left, rect.bottom, this);
	switch (retValue) {
	case CInstrumentFileTree::MENU_BASE: // Open file
		OnLoadInstrument();
		break;
	case CInstrumentFileTree::MENU_BASE + 1: // Select dir
		SelectInstrumentFolder();
		break;
	default:
		if (retValue >= CInstrumentFileTree::MENU_BASE + 2) { // A file
			auto &Im = *GetDoc().GetInstrumentManager();		// // //
			int Index = Im.GetFirstUnused();
			if (!LoadInstrument(Index, m_pInstrumentFileTree->GetFile(retValue)))
				return;
			SelectInstrument(Index);
			UpdateInstrumentList();
		}
	}
}

void CMainFrame::SelectInstrumentFolder()
{
	BROWSEINFOA Browse;
	LPITEMIDLIST lpID;
	char Path[MAX_PATH];
	CString Title;

	Title.LoadString(IDS_INSTRUMENT_FOLDER);
	Browse.lpszTitle	= Title;
	Browse.hwndOwner	= m_hWnd;
	Browse.pidlRoot		= NULL;
	Browse.lpfn			= NULL;
	Browse.ulFlags		= BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
	Browse.pszDisplayName = Path;

	lpID = SHBrowseForFolder(&Browse);

	if (lpID != NULL) {
		SHGetPathFromIDList(lpID, Path);
		theApp.GetSettings()->InstrumentMenuPath = Path;
		m_pInstrumentFileTree->Changed();
	}
}

BOOL CMainFrame::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	switch (pCopyDataStruct->dwData) {
		case IPC_LOAD:
			// Load file
			if (_tcslen((LPCTSTR)pCopyDataStruct->lpData) > 0)
				theApp.OpenDocumentFile((LPCTSTR)pCopyDataStruct->lpData);
			return TRUE;
		case IPC_LOAD_PLAY:
			// Load file
			if (_tcslen((LPCTSTR)pCopyDataStruct->lpData) > 0)
				theApp.OpenDocumentFile((LPCTSTR)pCopyDataStruct->lpData);
			// and play
			if (CFamiTrackerDoc::GetDoc()->IsFileLoaded() &&
				!CFamiTrackerDoc::GetDoc()->HasLastLoadFailed())
				theApp.StartPlayer(play_mode_t::Song);		// // //
			return TRUE;
	}

	return CFrameWnd::OnCopyData(pWnd, pCopyDataStruct);
}

bool CMainFrame::AddAction(std::unique_ptr<CAction> pAction)		// // //
{
	ASSERT(m_pActionHandler);

	if (!m_pActionHandler->AddAction(*this, std::move(pAction)))
		return false;		// // //

	auto pDoc = dynamic_cast<CFTMComponentInterface *>(GetActiveDocument());		// // //
	pDoc->Modify(true);
	if (m_pActionHandler->ActionsLost())		// // //
		pDoc->ModifyIrreversible();

	return true;
}

void CMainFrame::ResetUndo()
{
	m_pActionHandler = std::make_unique<CActionHandler>(MAX_UNDO_LEVELS);		// // //
}

void CMainFrame::OnEditUndo()
{
	auto &doc = static_cast<CFamiTrackerDoc &>(*GetActiveDocument());
	m_pActionHandler->UndoLastAction(*this);		// // //
	doc.SetModifiedFlag(true);
	if (!m_pActionHandler->CanUndo() && !doc.GetExceededFlag())
		doc.SetModifiedFlag(false);
}

void CMainFrame::OnEditRedo()
{
	m_pActionHandler->RedoLastAction(*this);		// // //
	auto &doc = static_cast<CFamiTrackerDoc &>(*GetActiveDocument());
	doc.SetModifiedFlag(true); // TODO: not always
}

void CMainFrame::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pActionHandler && m_pActionHandler->CanUndo());		// // //
}

void CMainFrame::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pActionHandler && m_pActionHandler->CanRedo());		// // //
}

void CMainFrame::UpdateMenus()
{
	// Write new shortcuts to menus
	UpdateMenu(GetMenu());
}

void CMainFrame::UpdateMenu(CMenu *pMenu)
{
	CAccelerator *pAccel = theApp.GetAccelerator();

	for (UINT i = 0; i < static_cast<unsigned int>(pMenu->GetMenuItemCount()); ++i) {		// // //
		UINT state = pMenu->GetMenuState(i, MF_BYPOSITION);
		if (state & MF_POPUP) {
			// Update sub menu
			UpdateMenu(pMenu->GetSubMenu(i));
		}
		else if ((state & MF_SEPARATOR) == 0) {
			// Change menu name
			CString shortcut;
			UINT id = pMenu->GetMenuItemID(i);

			if (pAccel->GetShortcutString(id, shortcut)) {
				CString string;
				pMenu->GetMenuString(i, string, MF_BYPOSITION);

				int tab = string.Find(_T('\t'));

				if (tab != -1) {
					string = string.Left(tab);
				}

				string += shortcut;
				pMenu->ModifyMenu(i, MF_BYPOSITION, id, string);
			}
		}
	}
}

void CMainFrame::OnEditCut()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCut();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditCut();
}

void CMainFrame::OnEditCopy()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCopy();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditCopy();
}

void CMainFrame::OnEditCopyAsText()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCopyAsText();
	//else if (GetFocus() == GetFrameEditor())
	//	GetFrameEditor()->OnEditCopy();
}

void CMainFrame::OnEditCopyAsVolumeSequence()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCopyAsVolumeSequence();
}

void CMainFrame::OnEditCopyAsPPMCK()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCopyAsPPMCK();
}

void CMainFrame::OnEditPaste()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditPaste();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditPaste();
}

void CMainFrame::OnEditPasteOverwrite()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditPasteOverwrite();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditPasteOverwrite();
}

void CMainFrame::OnUpdateEditPasteOverwrite(CCmdUI *pCmdUI)		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnUpdateEditPaste(pCmdUI);
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnUpdateEditPasteOverwrite(pCmdUI);
}

void CMainFrame::OnEditDelete()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditCut();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditDelete();
}

void CMainFrame::OnEditSelectall()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectall();
	else if (GetFocus() == GetFrameEditor())		// // //
		GetFrameEditor()->OnEditSelectall();
}

void CMainFrame::OnEditSelectnone()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectnone();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->CancelSelection();
}

void CMainFrame::OnEditSelectrow()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectrow();
}

void CMainFrame::OnEditSelectcolumn()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectcolumn();
}

void CMainFrame::OnEditSelectpattern()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectpattern();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditSelectpattern();
}

void CMainFrame::OnEditSelectframe()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectframe();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditSelectframe();
}

void CMainFrame::OnEditSelectchannel()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelectchannel();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditSelectchannel();
}

void CMainFrame::OnEditSelecttrack()
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditSelecttrack();
	else if (GetFocus() == GetFrameEditor())
		GetFrameEditor()->OnEditSelectall();
}

void CMainFrame::OnEditSelectother()		// // //
{
	auto pView = static_cast<CFamiTrackerView*>(GetActiveView());
	auto pEditor = pView->GetPatternEditor();
	auto pDoc = static_cast<const CFamiTrackerDoc*>(GetActiveDocument());

	if (GetFocus() == pView) {
		if (pEditor->IsSelecting()) {
			const CSelection Sel = pEditor->GetSelection().GetNormalized();
			pEditor->CancelSelection();

			CFrameSelection NewSel;
			int Frames = pDoc->GetFrameCount(m_iTrack);
			NewSel.m_cpStart.m_iFrame = Sel.m_cpStart.m_iFrame;
			NewSel.m_cpEnd.m_iFrame = Sel.m_cpEnd.m_iFrame;
			if (NewSel.m_cpStart.m_iFrame < 0) {
				NewSel.m_cpStart.m_iFrame += Frames;
				NewSel.m_cpEnd.m_iFrame += Frames;
			}
			NewSel.m_cpEnd.m_iFrame = std::min(NewSel.m_cpEnd.m_iFrame, Frames - 1);
			NewSel.m_cpStart.m_iChannel = Sel.m_cpStart.m_iChannel;
			NewSel.m_cpEnd.m_iChannel = Sel.m_cpEnd.m_iChannel;

			m_pFrameEditor->SetSelection(NewSel);
		}
		else
			m_pFrameEditor->CancelSelection();
		m_pFrameEditor->EnableInput();
	}
	else if (GetFocus() == m_pFrameEditor.get()) {		// // //
		if (m_pFrameEditor->IsSelecting()) {
			const CFrameSelection Sel = m_pFrameEditor->GetSelection().GetNormalized();
			m_pFrameEditor->CancelSelection();

			CSelection NewSel;
			NewSel.m_cpStart.m_iFrame = Sel.GstFirstSelectedFrame();
			NewSel.m_cpStart.m_iChannel = Sel.GetFirstSelectedChannel();
			NewSel.m_cpEnd.m_iFrame = Sel.GetLastSelectedFrame();
			NewSel.m_cpEnd.m_iChannel = Sel.GetLastSelectedChannel();

			NewSel.m_cpStart.m_iRow = 0;
			NewSel.m_cpStart.m_iColumn = C_NOTE;
			NewSel.m_cpEnd.m_iRow = pView->GetSongView()->GetCurrentPatternLength(NewSel.m_cpEnd.m_iFrame) - 1;
			NewSel.m_cpEnd.m_iColumn = pEditor->GetChannelColumns(NewSel.m_cpEnd.m_iChannel);

			pEditor->SetSelection(NewSel);
			pEditor->UpdateSelectionCondition();
		}
		else
			pEditor->CancelSelection();
		pView->SetFocus();
	}
}

void CMainFrame::OnDecayFast()
{
	theApp.GetSettings()->MeterDecayRate = DECAY_FAST;
	theApp.GetSoundGenerator()->SetMeterDecayRate(DECAY_FAST);		// // // 050B
}

void CMainFrame::OnDecaySlow()
{
	theApp.GetSettings()->MeterDecayRate = DECAY_SLOW;
	theApp.GetSoundGenerator()->SetMeterDecayRate(DECAY_SLOW);		// // // 050B
}

void CMainFrame::OnUpdateDecayFast(CCmdUI *pCmdUI)		// // // 050B
{
	pCmdUI->SetCheck(theApp.GetSoundGenerator()->GetMeterDecayRate() == DECAY_FAST ? MF_CHECKED : MF_UNCHECKED);
}

void CMainFrame::OnUpdateDecaySlow(CCmdUI *pCmdUI)		// // // 050B
{
	pCmdUI->SetCheck(theApp.GetSoundGenerator()->GetMeterDecayRate() == DECAY_SLOW ? MF_CHECKED : MF_UNCHECKED);
}

void CMainFrame::OnEditExpandpatterns()
{
	if (GetFocus() == GetActiveView())		// // //
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditExpandPatterns();
}

void CMainFrame::OnEditShrinkpatterns()
{
	if (GetFocus() == GetActiveView())		// // //
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditShrinkPatterns();
}

void CMainFrame::OnEditStretchpatterns()		// // //
{
	if (GetFocus() == GetActiveView())
		static_cast<CFamiTrackerView*>(GetActiveView())->OnEditStretchPatterns();
}

void CMainFrame::OnEditTransposeCustom()		// // //
{
	CTransposeDlg TrspDlg;
	TrspDlg.SetTrack(GetSelectedTrack());
	if (TrspDlg.DoModal() == IDOK)
		ResetUndo();
}

void CMainFrame::OnEditClearPatterns()
{
	AddAction(std::make_unique<CPActionClearAll>(GetSelectedTrack()));		// // //
}

void CMainFrame::OnEditRemoveUnusedInstruments()
{
	// Removes unused instruments in the module

	CFamiTrackerDoc &Doc = GetDoc();

	if (AfxMessageBox(IDS_REMOVE_INSTRUMENTS, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	// Current instrument might disappear
	CloseInstrumentEditor();

	Doc.RemoveUnusedInstruments();

	// Update instrument list
	Doc.UpdateAllViews(NULL, UPDATE_INSTRUMENT);
}

void CMainFrame::OnEditRemoveUnusedPatterns()
{
	// Removes unused patterns in the module

	CFamiTrackerDoc &Doc = GetDoc();

	if (AfxMessageBox(IDS_REMOVE_PATTERNS, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	Doc.RemoveUnusedPatterns();
	Doc.ModifyIrreversible();		// // //
	ResetUndo();
	Doc.UpdateAllViews(NULL, UPDATE_PATTERN);
}

void CMainFrame::OnEditMergeDuplicatedPatterns()
{
	AddAction(std::make_unique<CFActionMergeDuplicated>());		// // //
}

void CMainFrame::OnUpdateSelectionEnabled(CCmdUI *pCmdUI)
{
	CFamiTrackerView *pView	= static_cast<CFamiTrackerView*>(GetActiveView());
	pCmdUI->Enable(pView->IsSelecting() ? 1 : 0);
}

void CMainFrame::OnUpdateCurrentSelectionEnabled(CCmdUI *pCmdUI)		// // //
{
	if (GetFocus() == GetActiveView())
		OnUpdateSelectionEnabled(pCmdUI);
	else if (GetFocus() == GetFrameEditor())
		pCmdUI->Enable(GetFrameEditor()->IsSelecting() ? 1 : 0);
}

void CMainFrame::SetFrameEditorPosition(int Position)
{
	// Change frame editor position
	m_iFrameEditorPos = Position;

	m_pFrameEditor->ShowWindow(SW_HIDE);

	switch (Position) {
		case FRAME_EDIT_POS_TOP:
			m_wndVerticalControlBar.ShowWindow(SW_HIDE);
			m_pFrameEditor->SetParent(&m_wndControlBar);
			m_wndFrameControls.SetParent(&m_wndControlBar);
			break;
		case FRAME_EDIT_POS_LEFT:
			m_wndVerticalControlBar.ShowWindow(SW_SHOW);
			m_pFrameEditor->SetParent(&m_wndVerticalControlBar);
			m_wndFrameControls.SetParent(&m_wndVerticalControlBar);
			break;
	}

	ResizeFrameWindow();

	m_pFrameEditor->ShowWindow(SW_SHOW);
	m_pFrameEditor->Invalidate();
	m_pFrameEditor->RedrawWindow();

	ResizeFrameWindow();	// This must be called twice or the editor disappears, I don't know why

	// Save to settings
	theApp.GetSettings()->FrameEditPos = Position;
}

void CMainFrame::SetControlPanelPosition(control_panel_pos_t Position)		// // // 050B
{
	m_iControlPanelPos = Position;
	if (m_iControlPanelPos)
		SetFrameEditorPosition(FRAME_EDIT_POS_LEFT);
/*
	auto Rect = DPI::Rect(193, 0, 193, 126);
	MapDialogRect(m_wndInstToolBarWnd, &Rect);

	switch (m_iControlPanelPos) {
	case CONTROL_PANEL_POS_TOP:
		m_wndToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_BORDER_BOTTOM | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_wndToolBar.CalcFixedLayout(TRUE, FALSE);
		break;
	case CONTROL_PANEL_POS_LEFT:
		m_wndToolBar.SetBarStyle(CBRS_ALIGN_LEFT | CBRS_BORDER_RIGHT | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_wndToolBar.CalcFixedLayout(TRUE, FALSE);
		break;
	case CONTROL_PANEL_POS_RIGHT:
		m_wndToolBar.SetBarStyle(CBRS_ALIGN_RIGHT | CBRS_BORDER_LEFT | CBRS_TOOLTIPS | CBRS_FLYBY);
		m_wndToolBar.CalcFixedLayout(TRUE, FALSE);
		break;
	}
*/
	// 0x462575

	ResizeFrameWindow();
	ResizeFrameWindow();
}

void CMainFrame::OnFrameeditorTop()
{
	SetFrameEditorPosition(FRAME_EDIT_POS_TOP);
}

void CMainFrame::OnFrameeditorLeft()
{
	SetFrameEditorPosition(FRAME_EDIT_POS_LEFT);
}

void CMainFrame::OnUpdateFrameeditorTop(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iFrameEditorPos == FRAME_EDIT_POS_TOP);
}

void CMainFrame::OnUpdateFrameeditorLeft(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_iFrameEditorPos == FRAME_EDIT_POS_LEFT);
}

void CMainFrame::OnToggleSpeed()
{
	CFamiTrackerDoc &Doc = GetDoc();
	int Speed = Doc.GetModule()->GetSpeedSplitPoint();

	if (Speed == CFamiTrackerModule::DEFAULT_SPEED_SPLIT_POINT)		// // //
		Speed = CFamiTrackerModule::OLD_SPEED_SPLIT_POINT;
	else
		Speed = CFamiTrackerModule::DEFAULT_SPEED_SPLIT_POINT;

	Doc.GetModule()->SetSpeedSplitPoint(Speed);
	Doc.ModifyIrreversible();		// // //
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(&Doc);

	SetStatusText(_T("Speed/tempo split-point set to %i"), Speed);
}

void CMainFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	CFamiTrackerDoc &Doc = GetDoc();

	CString title = Doc.GetTitle();

	// Add a star (*) for unsaved documents
	if (Doc.IsModified())
		title.Append(_T("*"));

	// Add name of subtune
	auto sv = GetCurrentSong()->GetTitle();
	title.AppendFormat(_T(" [#%i %.*s]"), m_iTrack + 1, sv.size(), sv.data());		// // //

	title.Append(_T(" - 0CC-FamiTracker "));		// // //
	title.Append(Get0CCFTVersionString());		// // //
	SetWindowText(title);
	// UpdateFrameTitleForDocument(title);
}

LRESULT CMainFrame::OnDisplayMessageString(WPARAM wParam, LPARAM lParam)
{
	AfxMessageBox((LPCTSTR)wParam, (UINT)lParam);
	return 0;
}

LRESULT CMainFrame::OnDisplayMessageID(WPARAM wParam, LPARAM lParam)
{
	AfxMessageBox((UINT)wParam, (UINT)lParam);
	return 0;
}

void CMainFrame::CheckAudioStatus()
{
	const DWORD TIMEOUT = 2000; // Display a message for 2 seconds

	// Monitor audio playback
	// TODO remove static variables
	static BOOL DisplayedError;
	static DWORD MessageTimeout;

	if (!theApp.GetSoundGenerator()) {
		// Should really never be displayed (only during debugging)
		SetMessageText(_T("Audio is not working"));
		return;
	}

	auto pDriver = theApp.GetSoundGenerator()->GetAudioDriver();		// // //
	if (!pDriver)
		return;

	// Wait for signals from the player thread
	if (pDriver->GetSoundTimeout()) {
		// No events from the audio pump
		SetMessageText(IDS_SOUND_FAIL);
		DisplayedError = TRUE;
		MessageTimeout = GetTickCount() + TIMEOUT;
	}
#ifndef _DEBUG
	else if (pDriver->DidBufferUnderrun()) {		// // //
		// Buffer underrun
		SetMessageText(IDS_UNDERRUN_MESSAGE);
		DisplayedError = TRUE;
		MessageTimeout = GetTickCount() + TIMEOUT;
	}
	else if (pDriver->WasAudioClipping()) {		// // //
		// Audio is clipping
		SetMessageText(IDS_CLIPPING_MESSAGE);
		DisplayedError = TRUE;
		MessageTimeout = GetTickCount() + TIMEOUT;
	}
#endif
	else {
		if (DisplayedError == TRUE && MessageTimeout < GetTickCount()) {
			// Restore message
			//SetMessageText(AFX_IDS_IDLEMESSAGE);
			DisplayedError = FALSE;
		}
	}
}

void CMainFrame::OnViewToolbar()
{
	BOOL Visible = m_wndToolBar.IsVisible();
	m_wndToolBar.ShowWindow(Visible ? SW_HIDE : SW_SHOW);
	m_wndToolBarReBar.ShowWindow(Visible ? SW_HIDE : SW_SHOW);
	RecalcLayout();
}

// // //

void CMainFrame::OnToggleMultiplexer()
{
	CSettings *pSettings = theApp.GetSettings();
	CSoundGen *pSoundGen = theApp.GetSoundGenerator();
	if (!pSettings->m_bNamcoMixing){
		pSettings->m_bNamcoMixing = true;
		pSoundGen->SetNamcoMixing(theApp.GetSettings()->m_bNamcoMixing);
		SetStatusText(_T("Namco 163 multiplexer emulation disabled"));
	}
	else{
		pSettings->m_bNamcoMixing = false;
		pSoundGen->SetNamcoMixing(theApp.GetSettings()->m_bNamcoMixing);
		SetStatusText(_T("Namco 163 multiplexer emulation enabled"));
	}
}

void CMainFrame::OnToggleGroove()
{
	CFamiTrackerDoc &Doc = GetDoc();
	Doc.SetSongGroove(m_iTrack, !Doc.GetSongGroove(m_iTrack));
	Doc.ModifyIrreversible();
	GetActiveView()->SetFocus();
}

void CMainFrame::OnUpdateGrooveEdit(CCmdUI *pCmdUI)
{
	CFamiTrackerDoc &Doc = GetDoc();
	int Speed = Doc.GetSongSpeed(m_iTrack);
	if (Doc.GetSongGroove(m_iTrack)) {
		m_cButtonGroove.SetWindowText(_T("Groove"));
		if (Speed > MAX_GROOVE - 1) Speed = MAX_GROOVE - 1;
		if (Speed < 0) Speed = 0;
	}
	else {
		m_cButtonGroove.SetWindowText(_T("Speed"));
		int MaxSpeed = Doc.GetSongTempo(m_iTrack) ? Doc.GetSpeedSplitPoint() - 1 : 0xFF;
		if (Speed > MaxSpeed) Speed = MaxSpeed;
		if (Speed < MIN_SPEED) Speed = MIN_SPEED;
	}
	if (Speed != Doc.GetSongSpeed(m_iTrack))
		Doc.ModifyIrreversible();
	Doc.SetSongSpeed(m_iTrack, Speed);
}

void CMainFrame::OnToggleFixTempo()
{
	CFamiTrackerDoc &Doc = GetDoc();
	Doc.SetSongTempo(m_iTrack, Doc.GetSongTempo(m_iTrack) ? 0 : 150);
	Doc.ModifyIrreversible();
	GetActiveView()->SetFocus();
}

void CMainFrame::OnUpdateToggleFixTempo(CCmdUI *pCmdUI)
{
	const CFamiTrackerDoc &Doc = GetDoc();

	if (int Tempo = Doc.GetSongTempo(m_iTrack)) {
		m_cButtonFixTempo.SetWindowText(_T("Tempo"));
		m_cLockedEditTempo.EnableWindow(true);
		m_wndDialogBar.GetDlgItem(IDC_TEMPO_SPIN)->EnableWindow(true);
	}
	else {
		m_cButtonFixTempo.SetWindowText(_T("Fixed"));
		m_cLockedEditTempo.EnableWindow(false);
		m_wndDialogBar.GetDlgItem(IDC_TEMPO_SPIN)->EnableWindow(false);
		CString str;
		str.Format(_T("%.2f"), static_cast<float>(Doc.GetFrameRate()) * 2.5);
		m_cLockedEditTempo.SetWindowText(str);
	}
}

void CMainFrame::OnEasterEggKraid1()			// Easter Egg
{
	if (m_iKraidCounter == 0) m_iKraidCounter++;
	else m_iKraidCounter = 0;
}

void CMainFrame::OnEasterEggKraid2()
{
	if (m_iKraidCounter == 1) m_iKraidCounter++;
	else m_iKraidCounter = 0;
}

void CMainFrame::OnEasterEggKraid3()
{
	if (m_iKraidCounter == 2) m_iKraidCounter++;
	else m_iKraidCounter = 0;
}

void CMainFrame::OnEasterEggKraid4()
{
	if (m_iKraidCounter == 3) m_iKraidCounter++;
	else m_iKraidCounter = 0;
}

void CMainFrame::OnEasterEggKraid5()
{
	if (m_iKraidCounter == 4) {
		if (AfxMessageBox(IDS_KRAID, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO) {
			m_iKraidCounter = 0;
			return;}
		CFamiTrackerDoc &doc = GetDoc();
		Kraid { }(doc);
		SelectTrack(0);
		SetSongInfo(doc);
		UpdateControls();
		UpdateInstrumentList();
		UpdateTrackBox();
		ResetUndo();
		ResizeFrameWindow();
		SetStatusText(_T(
			"Famitracker - Metroid - Kraid's Lair "
			"(Uploaded on Jun 9, 2010 http://www.youtube.com/watch?v=9yzCLy-fZVs) "
			"The FTM straight from the tutorial. - 8BitDanooct1"));
	}
	m_iKraidCounter = 0;
}

void CMainFrame::OnFindNext()
{
	m_pFindDlg->OnBnClickedButtonFindNext();
}

void CMainFrame::OnFindPrevious()
{
	m_pFindDlg->OnBnClickedButtonFindPrevious();
}

void CMainFrame::OnEditFindToggle()
{
	if (m_wndFindControlBar.IsWindowVisible())
		m_wndFindControlBar.ShowWindow(SW_HIDE);
	else {
		m_wndFindControlBar.ShowWindow(SW_SHOW);
		m_wndFindControlBar.Invalidate();
		m_wndFindControlBar.RedrawWindow();
	}
	ResizeFrameWindow();
}

void CMainFrame::OnUpdateEditFindToggle(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_wndFindControlBar.IsWindowVisible());
}

void CMainFrame::ResetFind()
{
	m_pFindDlg->Reset();
}

void CMainFrame::OnEditRemoveUnusedSamples()
{
	CFamiTrackerDoc &Doc = GetDoc();

	if (AfxMessageBox(IDS_REMOVE_SAMPLES, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDNO)
		return;

	CloseInstrumentEditor();
	Doc.RemoveUnusedSamples();
	Doc.ModifyIrreversible();		// // //
	ResetUndo();
	Doc.UpdateAllViews(NULL, UPDATE_PATTERN);
}

void CMainFrame::OnEditPopulateUniquePatterns()		// // //
{
	AddAction(std::make_unique<CPActionUniquePatterns>(GetSelectedTrack()));
}

void CMainFrame::OnEditGoto()
{
	CGotoDlg gotoDlg;
	gotoDlg.DoModal();
}

void CMainFrame::OnEditSwapChannels()
{
	CSwapDlg swapDlg;
	swapDlg.SetTrack(GetSelectedTrack());
	if (swapDlg.DoModal() == IDOK)
		ResetUndo();
}

// // // Moved from CFamiTrackerView

void CMainFrame::OnTrackerPal()
{
	CFamiTrackerDoc &Doc = GetDoc();

	Doc.GetModule()->SetMachine(PAL);
	Doc.ModifyIrreversible();
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(&Doc);		// // //
	m_wndInstEdit.SetRefreshRate(static_cast<float>(Doc.GetModule()->GetFrameRate()));		// // //
}

void CMainFrame::OnTrackerNtsc()
{
	CFamiTrackerDoc &Doc = GetDoc();

	Doc.GetModule()->SetMachine(NTSC);
	Doc.ModifyIrreversible();
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
	theApp.GetSoundGenerator()->DocumentPropertiesChanged(&Doc);		// // //
	m_wndInstEdit.SetRefreshRate(static_cast<float>(Doc.GetModule()->GetFrameRate()));		// // //
}

void CMainFrame::OnSpeedDefault()
{
	CFamiTrackerDoc &Doc = GetDoc();

	int Speed = 0;
	Doc.GetModule()->SetEngineSpeed(Speed);
	Doc.ModifyIrreversible();
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
	m_wndInstEdit.SetRefreshRate(static_cast<float>(Doc.GetModule()->GetFrameRate()));		// // //
}

void CMainFrame::OnSpeedCustom()
{
	CFamiTrackerDoc &Doc = GetDoc();
	CFamiTrackerModule &Module = *Doc.GetModule();

	CSpeedDlg SpeedDlg;

	machine_t Machine = Module.GetMachine();
	int Speed = Module.GetEngineSpeed();
	if (Speed == 0)
		Speed = (Machine == NTSC) ? CAPU::FRAME_RATE_NTSC : CAPU::FRAME_RATE_PAL;
	Speed = SpeedDlg.GetSpeedFromDlg(Speed);

	if (Speed == 0)
		return;

	Module.SetEngineSpeed(Speed);
	Doc.ModifyIrreversible();
	theApp.GetSoundGenerator()->LoadMachineSettings();		// // //
	m_wndInstEdit.SetRefreshRate(static_cast<float>(Module.GetFrameRate()));		// // //
}

void CMainFrame::OnUpdateTrackerPal(CCmdUI *pCmdUI)
{
	const CFamiTrackerDoc &Doc = GetDoc();

	pCmdUI->Enable(!Doc.GetModule()->HasExpansionChips() && !theApp.GetSoundGenerator()->IsPlaying());		// // //
	UINT item = Doc.GetMachine() == PAL ? ID_TRACKER_PAL : ID_TRACKER_NTSC;
	if (pCmdUI->m_pMenu != NULL)
		pCmdUI->m_pMenu->CheckMenuRadioItem(ID_TRACKER_NTSC, ID_TRACKER_PAL, item, MF_BYCOMMAND);
}

void CMainFrame::OnUpdateTrackerNtsc(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!theApp.GetSoundGenerator()->IsPlaying());		// // //
	UINT item = GetDoc().GetMachine() == NTSC ? ID_TRACKER_NTSC : ID_TRACKER_PAL;
	if (pCmdUI->m_pMenu != NULL)
		pCmdUI->m_pMenu->CheckMenuRadioItem(ID_TRACKER_NTSC, ID_TRACKER_PAL, item, MF_BYCOMMAND);
}

void CMainFrame::OnUpdateSpeedDefault(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!theApp.GetSoundGenerator()->IsPlaying());		// // //
	pCmdUI->SetCheck(GetDoc().GetEngineSpeed() == 0);
}

void CMainFrame::OnUpdateSpeedCustom(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(!theApp.GetSoundGenerator()->IsPlaying());		// // //
	pCmdUI->SetCheck(GetDoc().GetEngineSpeed() != 0);
}
