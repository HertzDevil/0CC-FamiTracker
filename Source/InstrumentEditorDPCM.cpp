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

#include "InstrumentEditorDPCM.h"
#include "FamiTracker.h"
#include "FamiTrackerEnv.h"		// // //
#include "FamiTrackerDoc.h"
#include "FamiTrackerModule.h"		// // //
#include "FamiTrackerView.h"
#include "SampleEditorView.h"
#include "SampleEditorDlg.h"
#include "SeqInstrument.h"		// // //
#include "Instrument2A03.h"		// // //
#include "DSampleManager.h"		// // //
#include "PCMImport.h"
#include "Settings.h"
#include "SoundGen.h"
#include "ft0cc/doc/dpcm_sample.hpp"		// // //
#include "PatternNote.h"		// // // note names
#include <algorithm>		// // //
#include "NumConv.h"		// // //
#include "str_conv/str_conv.hpp"		// // //
#include "NoteName.h"		// // //

LPCWSTR NO_SAMPLE_STR = L"(no sample)";

// Derive a new class from CFileDialog with implemented preview of DMC files

class CDMCFileSoundDialog : public CFileDialog
{
public:
	CDMCFileSoundDialog(BOOL bOpenFileDialog, LPCWSTR lpszDefExt = NULL, LPCWSTR lpszFileName = NULL, DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, LPCWSTR lpszFilter = NULL, CWnd* pParentWnd = NULL, DWORD dwSize = 0);
	virtual ~CDMCFileSoundDialog();

	static const int DEFAULT_PREVIEW_PITCH = 15;

protected:
	virtual void OnFileNameChange();
	CStringW m_strLastFile;
};

//	CFileSoundDialog

CDMCFileSoundDialog::CDMCFileSoundDialog(BOOL bOpenFileDialog, LPCWSTR lpszDefExt, LPCWSTR lpszFileName, DWORD dwFlags, LPCWSTR lpszFilter, CWnd* pParentWnd, DWORD dwSize)
	: CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize)
{
}

CDMCFileSoundDialog::~CDMCFileSoundDialog()
{
	// Stop any possible playing sound
	//PlaySound(NULL, NULL, SND_NODEFAULT | SND_SYNC);
}

void CDMCFileSoundDialog::OnFileNameChange()
{
	// Preview DMC file
	if (!GetFileExt().CompareNoCase(L"dmc") && Env.GetSettings()->General.bWavePreview) {
		DWORD dwAttrib = GetFileAttributesW(GetPathName());
		if (!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY) && GetPathName() != m_strLastFile) {
			CFile file(GetPathName(), CFile::modeRead);
			auto size = std::min<unsigned>(static_cast<unsigned>(file.GetLength()), ft0cc::doc::dpcm_sample::max_size);
			std::vector<uint8_t> pBuf(size);
			file.Read(pBuf.data(), size);
			Env.GetSoundGenerator()->PreviewSample(std::make_shared<ft0cc::doc::dpcm_sample>(pBuf, ""),
				0, DEFAULT_PREVIEW_PITCH);		// // //
			file.Close();
			m_strLastFile = GetPathName();
		}
	}

	CFileDialog::OnFileNameChange();
}

// CInstrumentDPCM dialog

IMPLEMENT_DYNAMIC(CInstrumentEditorDPCM, CInstrumentEditPanel)
CInstrumentEditorDPCM::CInstrumentEditorDPCM(CWnd* pParent) : CInstrumentEditPanel(CInstrumentEditorDPCM::IDD, pParent), m_pInstrument(NULL)
{
}

CInstrumentEditorDPCM::~CInstrumentEditorDPCM()
{
}

void CInstrumentEditorDPCM::DoDataExchange(CDataExchange* pDX)
{
	CInstrumentEditPanel::DoDataExchange(pDX);
}

CDSampleManager *CInstrumentEditorDPCM::GetDSampleManager() const {		// // //
	return GetDocument()->GetModule()->GetDSampleManager();
}


BEGIN_MESSAGE_MAP(CInstrumentEditorDPCM, CInstrumentEditPanel)
	ON_BN_CLICKED(IDC_LOAD, &CInstrumentEditorDPCM::OnBnClickedLoad)
	ON_BN_CLICKED(IDC_UNLOAD, &CInstrumentEditorDPCM::OnBnClickedUnload)
	ON_BN_CLICKED(IDC_IMPORT, &CInstrumentEditorDPCM::OnBnClickedImport)
	ON_BN_CLICKED(IDC_SAVE, &CInstrumentEditorDPCM::OnBnClickedSave)
	ON_BN_CLICKED(IDC_LOOP, &CInstrumentEditorDPCM::OnBnClickedLoop)
	ON_BN_CLICKED(IDC_ADD, &CInstrumentEditorDPCM::OnBnClickedAdd)
	ON_BN_CLICKED(IDC_REMOVE, &CInstrumentEditorDPCM::OnBnClickedRemove)
	ON_BN_CLICKED(IDC_EDIT, &CInstrumentEditorDPCM::OnBnClickedEdit)
	ON_BN_CLICKED(IDC_PREVIEW, &CInstrumentEditorDPCM::OnBnClickedPreview)
	ON_CBN_SELCHANGE(IDC_PITCH, &CInstrumentEditorDPCM::OnCbnSelchangePitch)
	ON_CBN_SELCHANGE(IDC_SAMPLES, &CInstrumentEditorDPCM::OnCbnSelchangeSamples)
	ON_NOTIFY(NM_CLICK, IDC_SAMPLE_LIST, &CInstrumentEditorDPCM::OnNMClickSampleList)
	ON_NOTIFY(NM_CLICK, IDC_TABLE, &CInstrumentEditorDPCM::OnNMClickTable)
	ON_NOTIFY(NM_DBLCLK, IDC_SAMPLE_LIST, &CInstrumentEditorDPCM::OnNMDblclkSampleList)
	ON_NOTIFY(NM_RCLICK, IDC_SAMPLE_LIST, &CInstrumentEditorDPCM::OnNMRClickSampleList)
	ON_NOTIFY(NM_RCLICK, IDC_TABLE, &CInstrumentEditorDPCM::OnNMRClickTable)
	ON_NOTIFY(NM_DBLCLK, IDC_TABLE, &CInstrumentEditorDPCM::OnNMDblclkTable)
	ON_NOTIFY(UDN_DELTAPOS, IDC_DELTA_SPIN, &CInstrumentEditorDPCM::OnDeltaposDeltaSpin)
	ON_EN_CHANGE(IDC_LOOP_POINT, &CInstrumentEditorDPCM::OnEnChangeLoopPoint)
	ON_EN_CHANGE(IDC_DELTA_COUNTER, &CInstrumentEditorDPCM::OnEnChangeDeltaCounter)
END_MESSAGE_MAP()

// CInstrumentDPCM message handlers

BOOL CInstrumentEditorDPCM::OnInitDialog()
{
	CInstrumentEditPanel::OnInitDialog();

	m_iSelectedNote = -1;		// // //

	m_cTableListCtrl.SubclassDlgItem(IDC_TABLE, this);		// // //
	m_cSampleListCtrl.SubclassDlgItem(IDC_SAMPLE_LIST, this);
	m_cComboSampleName.SubclassDlgItem(IDC_SAMPLES, this);
	m_cComboSamplePitch.SubclassDlgItem(IDC_PITCH, this);
	m_cEditDeltaCounter.SubclassDlgItem(IDC_DELTA_COUNTER, this);
	m_cSpinDeltaCounter.SubclassDlgItem(IDC_DELTA_SPIN, this);

	CRect r;		// // // 050B
	m_cTableListCtrl.GetClientRect(&r);
	int Width = r.Width() - ::GetSystemMetrics(SM_CXHSCROLL);
	m_cTableListCtrl.DeleteAllItems();
	m_cTableListCtrl.InsertColumn(0, L"Key", LVCFMT_LEFT, static_cast<int>(.2 * Width));
	m_cTableListCtrl.InsertColumn(1, L"Pitch", LVCFMT_LEFT, static_cast<int>(.23 * Width));
	m_cTableListCtrl.InsertColumn(2, L"Sample", LVCFMT_LEFT, static_cast<int>(.57 * Width));
	m_cTableListCtrl.SendMessageW(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	m_cSampleListCtrl.GetClientRect(&r);
	Width = r.Width();
	m_cSampleListCtrl.DeleteAllItems();
	m_cSampleListCtrl.InsertColumn(0, L"#", LVCFMT_LEFT, static_cast<int>(.2 * Width));		// // //
	m_cSampleListCtrl.InsertColumn(1, L"Name", LVCFMT_LEFT, static_cast<int>(.55 * Width));
	m_cSampleListCtrl.InsertColumn(2, L"Size", LVCFMT_LEFT, static_cast<int>(.25 * Width));
	m_cSampleListCtrl.SendMessageW(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	for (int i = 0; i < 16; ++i)
		m_cComboSamplePitch.AddString(conv::to_wide(std::to_string(i)).data());
	m_cComboSamplePitch.SetCurSel(15);

	CheckDlgButton(IDC_LOOP, 0);

	m_cTableListCtrl.DeleteAllItems();		// // //
	for (int i = 0; i < NOTE_COUNT; ++i)
		m_cTableListCtrl.InsertItem(i, conv::to_wide(GetNoteString(GET_NOTE(i), GET_OCTAVE(i))).data());
	m_cTableListCtrl.GetItemRect(0, &r, 2);		// // //
	m_cTableListCtrl.Scroll({0, DEFAULT_OCTAVE * NOTE_RANGE * r.Height()});

	BuildSampleList();
	m_iSelectedSample = 0;

	m_cSpinDeltaCounter.SetRange(-1, 127);
	m_cSpinDeltaCounter.SetPos(-1);

	SetDlgItemTextW(IDC_DELTA_COUNTER, L"Off");

	static_cast<CButton*>(GetDlgItem(IDC_ADD))->SetIcon(::LoadIconW(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_LEFT)));
	static_cast<CButton*>(GetDlgItem(IDC_REMOVE))->SetIcon(::LoadIconW(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_RIGHT)));

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CInstrumentEditorDPCM::BuildKeyList()
{
	for (int i = 0; i < NOTE_COUNT; ++i)
		UpdateKey(i);
}

void CInstrumentEditorDPCM::UpdateCurrentKey()		// // //
{
	UpdateKey(m_iSelectedNote);
}

void CInstrumentEditorDPCM::UpdateKey(int Index)
{
	CStringW NameStr = NO_SAMPLE_STR;
	CStringW PitchStr = L"-";

	if (m_pInstrument->GetSampleIndex(Index) != CInstrument2A03::NO_DPCM) {
		int Pitch = m_pInstrument->GetSamplePitch(Index);
		auto pSample = m_pInstrument->GetDSample(Index);		// // //
		NameStr = !pSample ? L"(n/a)" : conv::to_wide(pSample->name()).data();
		PitchStr = FormattedW(L"%i %s", Pitch & 0x0F, (Pitch & 0x80) ? L"L" : L"");
	}

	m_cTableListCtrl.SetItemText(Index, 1, PitchStr);
	m_cTableListCtrl.SetItemText(Index, 2, NameStr);
}

int CInstrumentEditorDPCM::GetSelectedSampleIndex() const {		// // //
	if (int Index = m_cSampleListCtrl.GetSelectionMark(); Index != -1)
		return m_cSampleListCtrl.GetItemData(Index);
	return -1;
}

void CInstrumentEditorDPCM::BuildSampleList()
{
	m_cSampleListCtrl.DeleteAllItems();
	m_cComboSampleName.ResetContent();

	unsigned int Size(0), Index(0);

	m_cComboSampleName.AddString(NO_SAMPLE_STR);
	m_cComboSampleName.SetItemData(0, (unsigned)-1);

	for (int i = 0; i < MAX_DSAMPLES; ++i) {
		if (auto pDSample = GetDSampleManager()->GetDSample(i)) {		// // //
			m_cSampleListCtrl.InsertItem(Index, conv::to_wide(conv::sv_from_int(i)).data());
			auto name = conv::to_wide(pDSample->name());
			m_cSampleListCtrl.SetItemText(Index, 1, name.data());
			m_cSampleListCtrl.SetItemText(Index, 2, FormattedW(L"%u", pDSample->size()));
			m_cSampleListCtrl.SetItemData(Index, i);		// // //
			m_cComboSampleName.AddString(FormattedW(L"%02i - %.*s", i, name.size(), name.data()));
			m_cComboSampleName.SetItemData(Index + 1, i);		// // //
			Size += pDSample->size();
			++Index;
		}
	}

	CStringW Text;
	AfxFormatString3(Text, IDS_DPCM_SPACE_FORMAT,
		conv::to_wide(conv::from_int(Size / 0x400)).data(),		// // //
		conv::to_wide(conv::from_int((MAX_SAMPLE_SPACE - Size) / 0x400)).data(),
		conv::to_wide(conv::from_int(MAX_SAMPLE_SPACE / 0x400)).data());

	SetDlgItemTextW(IDC_SPACE, Text);
}

// When saved in NSF, the samples has to be aligned at even 6-bits addresses
//#define ADJUST_FOR_STORAGE(x) (((x >> 6) + (x & 0x3F ? 1 : 0)) << 6)
// TODO: I think I was wrong
#define ADJUST_FOR_STORAGE(x) (x)

bool CInstrumentEditorDPCM::LoadSample(const CStringW &FilePath, const CStringW &FileName)
{
	CFile SampleFile;

	if (!SampleFile.Open(FilePath, CFile::modeRead)) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR);
		return false;
	}

	// Clip file if too large
	int Size = std::min<int>((int)SampleFile.GetLength(), ft0cc::doc::dpcm_sample::max_size);
	int AddSize = 0;

	// Make sure size is compatible with DPCM hardware
	if ((Size & 0xF) != 1) {
		AddSize = 0x10 - ((Size + 0x0F) & 0x0F);
	}

	std::vector<uint8_t> pBuf(Size + AddSize);		// // //
	SampleFile.Read(pBuf.data(), Size);
	SampleFile.Close();

	if (!InsertSample(std::make_shared<ft0cc::doc::dpcm_sample>(pBuf, conv::to_utf8(FileName))))
		return false;

	BuildSampleList();

	return true;
}

bool CInstrumentEditorDPCM::InsertSample(std::shared_ptr<ft0cc::doc::dpcm_sample> pNewSample)		// // //
{
	auto *pManager = GetDSampleManager();
	int FreeSlot = pManager->GetFirstFree();

	// Out of sample slots
	if (FreeSlot == -1)
		return false;

	int Size = pManager->GetTotalSize();

	if ((Size + pNewSample->size()) > MAX_SAMPLE_SPACE) {
		CStringW message;
		AfxFormatString1(message, IDS_OUT_OF_SAMPLEMEM_FORMAT, conv::to_wide(conv::sv_from_int(MAX_SAMPLE_SPACE / 1024)).data());
		AfxMessageBox(message, MB_ICONERROR);
	}
	else if (pManager->SetDSample(FreeSlot, std::move(pNewSample)))		// // //
		GetDocument()->ModifyIrreversible();

	return true;
}

void CInstrumentEditorDPCM::OnBnClickedLoad()
{
	CStringW fileFilter = LoadDefaultFilter(IDS_FILTER_DMC, L".dmc");
	CDMCFileSoundDialog OpenFileDialog(TRUE, 0, 0, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER, fileFilter);

	OpenFileDialog.m_pOFN->lpstrInitialDir = Env.GetSettings()->GetPath(PATH_DMC);

	if (OpenFileDialog.DoModal() == IDCANCEL)
		return;

	Env.GetSettings()->SetPath(OpenFileDialog.GetPathName(), PATH_DMC);

	if (OpenFileDialog.GetFileName().GetLength() == 0) {
		Env.GetSettings()->SetPath(OpenFileDialog.GetPathName() + L"\\", PATH_DMC);		// // //
		// Multiple files
		POSITION Pos = OpenFileDialog.GetStartPosition();
		while (Pos) {
			CStringW Path = OpenFileDialog.GetNextPathName(Pos);
			CStringW FileName = Path.Right(Path.GetLength() - Path.ReverseFind('\\') - 1);
			LoadSample(Path, FileName);
		}
	}
	else {
		// Single file
		LoadSample(OpenFileDialog.GetPathName(), OpenFileDialog.GetFileName());
	}
}

void CInstrumentEditorDPCM::OnBnClickedUnload()
{
	int nItem = -1;

	if (m_iSelectedSample == MAX_DSAMPLES)
		return;

	int SelCount = m_cSampleListCtrl.GetSelectedCount();
	if (!SelCount)
		return;

	for (int i = 0; i < SelCount; ++i) {
		nItem = m_cSampleListCtrl.GetNextItem(nItem, LVNI_SELECTED);
		ASSERT(nItem != -1);
		int n = m_cSampleListCtrl.GetItemData(nItem);		// // //
		Env.GetSoundGenerator()->CancelPreviewSample();
		GetDSampleManager()->RemoveDSample(n);		// // //
	}

	BuildSampleList();
}

void CInstrumentEditorDPCM::OnNMClickSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (int Index = GetSelectedSampleIndex(); Index != -1)		// // //
		m_iSelectedSample = Index;
	*pResult = 0;
}

void CInstrumentEditorDPCM::OnBnClickedImport()
{
	CPCMImport	ImportDialog;

	if (auto pImported = ImportDialog.ShowDialog()) {		// // //
		InsertSample(std::move(pImported));
		BuildSampleList();
	}
}

void CInstrumentEditorDPCM::OnCbnSelchangePitch()
{
	if (m_iSelectedNote == -1)
		return;

	int Pitch = m_cComboSamplePitch.GetCurSel();

	if (IsDlgButtonChecked(IDC_LOOP))
		Pitch |= 0x80;

	m_pInstrument->SetSamplePitch(m_iSelectedNote, Pitch);
	GetDocument()->ModifyIrreversible();		// // //

	UpdateCurrentKey();
}

void CInstrumentEditorDPCM::OnNMClickTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_iSelectedNote = m_cTableListCtrl.GetSelectionMark();		// // //

	int Sample = m_pInstrument->GetSampleIndex(m_iSelectedNote);
	int Pitch = m_pInstrument->GetSamplePitch(m_iSelectedNote);
	int Delta = m_pInstrument->GetSampleDeltaValue(m_iSelectedNote);

	if (Sample != CInstrument2A03::NO_DPCM)
		m_cComboSampleName.SelectString(0, FormattedW(L"%02i - %s", Sample, (LPCWSTR)m_cTableListCtrl.GetItemText(m_cTableListCtrl.GetSelectionMark(), 2)));
	else
		m_cComboSampleName.SetCurSel(0);

	if (Sample >= 0) {
		m_cComboSamplePitch.SetCurSel(Pitch & 0x0F);

		CheckDlgButton(IDC_LOOP, m_pInstrument->GetSampleLoop(m_iSelectedNote) ? BST_CHECKED : BST_UNCHECKED);

		if (Delta == -1)
			m_cEditDeltaCounter.SetWindowTextW(L"Off");
		else
			SetDlgItemInt(IDC_DELTA_COUNTER, Delta, FALSE);

		m_cSpinDeltaCounter.SetPos(Delta);
	}

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnCbnSelchangeSamples()
{
	int PrevSample = m_pInstrument->GetSampleIndex(m_iSelectedNote);
	int Sel = m_cComboSampleName.GetCurSel();
	unsigned Sample = CInstrument2A03::NO_DPCM;

	if (Sel > 0) {
		Sample = m_cComboSampleName.GetItemData(Sel);		// // //
		if (PrevSample == CInstrument2A03::NO_DPCM)
			m_pInstrument->SetSamplePitch(m_iSelectedNote, m_cComboSamplePitch.GetCurSel());
	}

	m_pInstrument->SetSampleIndex(m_iSelectedNote, Sample);
	GetDocument()->ModifyIrreversible();		// // //

	UpdateCurrentKey();
}

std::shared_ptr<const ft0cc::doc::dpcm_sample> CInstrumentEditorDPCM::GetSelectedSample() {		// // //
	int Index = GetSelectedSampleIndex();		// // //
	return Index != -1 ? GetDSampleManager()->GetDSample(Index) : nullptr;
}

void CInstrumentEditorDPCM::SetSelectedSample(std::shared_ptr<ft0cc::doc::dpcm_sample> pSample) const		// // //
{
	if (int Index = GetSelectedSampleIndex(); Index != -1)		// // //
		GetDSampleManager()->SetDSample(Index, std::move(pSample));
}

void CInstrumentEditorDPCM::OnBnClickedSave()
{
	CStringW	Path;
	CFile	SampleFile;

	int Index = GetSelectedSampleIndex();		// // //
	if (Index == -1)
		return;
	auto pDSample = GetDSampleManager()->GetDSample(Index);		// // //
	if (!pDSample)
		return;

	CStringW fileFilter = LoadDefaultFilter(IDS_FILTER_DMC, L".dmc");
	CFileDialog SaveFileDialog(FALSE, L"dmc", conv::to_wide(pDSample->name()).data(), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, fileFilter);

	SaveFileDialog.m_pOFN->lpstrInitialDir = Env.GetSettings()->GetPath(PATH_DMC);

	if (SaveFileDialog.DoModal() == IDCANCEL)
		return;

	Env.GetSettings()->SetPath(SaveFileDialog.GetPathName(), PATH_DMC);

	Path = SaveFileDialog.GetPathName();

	if (!SampleFile.Open(Path, CFile::modeWrite | CFile::modeCreate)) {
		AfxMessageBox(IDS_FILE_OPEN_ERROR);
		return;
	}

	SampleFile.Write(pDSample->data(), pDSample->size());
	SampleFile.Close();
}

void CInstrumentEditorDPCM::OnBnClickedLoop()
{

	int Pitch = m_pInstrument->GetSamplePitch(m_iSelectedNote) & 0x0F;

	if (IsDlgButtonChecked(IDC_LOOP))
		Pitch |= 0x80;

	m_pInstrument->SetSamplePitch(m_iSelectedNote, Pitch);
	GetDocument()->ModifyIrreversible();		// // //

	UpdateCurrentKey();
}

void CInstrumentEditorDPCM::SelectInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = std::dynamic_pointer_cast<CInstrument2A03>(pInst);
	ASSERT(m_pInstrument);

	BuildKeyList();
}

BOOL CInstrumentEditorDPCM::PreTranslateMessage(MSG* pMsg)
{
	if (IsWindowVisible()) {
		switch (pMsg->message) {
		case WM_KEYDOWN:
			if (pMsg->wParam == VK_ESCAPE)		// // //
				break;
			if (GetFocus() != &m_cEditDeltaCounter) {
				// Select DPCM channel
				CFamiTrackerView::GetView()->SelectChannel(4);
				PreviewNote((unsigned char)pMsg->wParam);		// // //
				return TRUE;
			}
			break;
		case WM_KEYUP:
			if (GetFocus() != &m_cEditDeltaCounter) {
				PreviewRelease((unsigned char)pMsg->wParam);		// // //
				return TRUE;
			}
			break;
		}
	}

	return CInstrumentEditPanel::PreTranslateMessage(pMsg);
}

void CInstrumentEditorDPCM::OnBnClickedAdd()
{
	// Add sample to key list

	int Pitch = m_cComboSamplePitch.GetCurSel();

	if (GetDSampleManager()->IsSampleUsed(m_iSelectedSample)) {		// // //
		m_pInstrument->SetSampleIndex(m_iSelectedNote, m_iSelectedSample);
		m_pInstrument->SetSamplePitch(m_iSelectedNote, Pitch);
		GetDocument()->ModifyIrreversible();		// // //
		UpdateCurrentKey();
	}

	if (m_iSelectedNote < NOTE_COUNT - 1 && m_iSelectedSample < MAX_DSAMPLES) {
		m_cSampleListCtrl.SetItemState(m_iSelectedSample, 0, LVIS_FOCUSED | LVIS_SELECTED);
		m_cTableListCtrl.SetItemState(m_iSelectedNote, 0, LVIS_FOCUSED | LVIS_SELECTED);
		if (m_iSelectedSample < m_cSampleListCtrl.GetItemCount() - 1)
			++m_iSelectedSample;
		++m_iSelectedNote;
		m_cSampleListCtrl.SetSelectionMark(m_iSelectedSample);
		m_cTableListCtrl.SetSelectionMark(m_iSelectedNote);
		m_cSampleListCtrl.SetItemState(m_iSelectedSample, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		m_cTableListCtrl.SetItemState(m_iSelectedNote, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}

void CInstrumentEditorDPCM::OnBnClickedRemove()
{
	// Remove sample from key list
	m_pInstrument->SetSampleIndex(m_iSelectedNote, CInstrument2A03::NO_DPCM);
	GetDocument()->ModifyIrreversible();		// // //
	UpdateCurrentKey();

	if (m_iSelectedNote > 0) {		// // //
		m_cTableListCtrl.SetItemState(m_iSelectedNote, 0, LVIS_FOCUSED | LVIS_SELECTED);
		--m_iSelectedNote;
		m_cTableListCtrl.SetSelectionMark(m_iSelectedNote);
		m_cTableListCtrl.SetItemState(m_iSelectedNote, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
	}
}


void CInstrumentEditorDPCM::OnEnChangeLoopPoint()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CInstrumentEditPanel::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here

	/*
	int Pitch = GetDlgItemInt(IDC_LOOP_POINT, 0, 0);
	m_pInstrument->SetSampleLoopOffset(m_iOctave, m_iSelectedKey, Pitch);
	*/
}

void CInstrumentEditorDPCM::OnBnClickedEdit()
{
	auto pSample = GetSelectedSample();		// // //
	if (!pSample)
		return;

	CSampleEditorDlg Editor(this, std::make_shared<ft0cc::doc::dpcm_sample>(*pSample));		// // // copy sample here

	INT_PTR nRes = Editor.DoModal();

	if (nRes == IDOK) {
		// Save edited sample
		SetSelectedSample(std::make_shared<ft0cc::doc::dpcm_sample>(*Editor.GetDSample()));		// // //
		GetDocument()->ModifyIrreversible();		// // //
	}

	// Update sample list
	BuildSampleList();
}

void CInstrumentEditorDPCM::OnNMDblclkSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Preview sample when double-clicking the sample list
	OnBnClickedPreview();
	*pResult = 0;
}

void CInstrumentEditorDPCM::OnBnClickedPreview()
{
	if (auto pSample = GetSelectedSample())		// // //
		Env.GetSoundGenerator()->PreviewSample(std::move(pSample), 0, 15);
}

void CInstrumentEditorDPCM::OnNMRClickSampleList(NMHDR *pNMHDR, LRESULT *pResult)
{
	CMenu *pPopupMenu, PopupMenuBar;
	CPoint point;

	GetCursorPos(&point);
	PopupMenuBar.LoadMenuW(IDR_SAMPLES_POPUP);
	pPopupMenu = PopupMenuBar.GetSubMenu(0);
	pPopupMenu->SetDefaultItem(IDC_PREVIEW);
	pPopupMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnNMRClickTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Create a popup menu for key list with samples
	m_iSelectedNote = m_cTableListCtrl.GetSelectionMark();		// // //

	CPoint point;
	CMenu PopupMenu;
	GetCursorPos(&point);
	PopupMenu.CreatePopupMenu();
	PopupMenu.AppendMenuW(MFT_STRING, (int)CInstrument2A03::NO_DPCM + 2, NO_SAMPLE_STR);		// // //

	// Fill menu
	for (int i = 0; i < MAX_DSAMPLES; ++i)
		if (auto pDSample = GetDSampleManager()->GetDSample(i))		// // //
			PopupMenu.AppendMenuW(MFT_STRING, i + 2, conv::to_wide(pDSample->name()).data());

	UINT Result = PopupMenu.TrackPopupMenu(TPM_RIGHTBUTTON | TPM_RETURNCMD, point.x, point.y, this);

	if (Result - 2 == CInstrument2A03::NO_DPCM) {
		// Remove sample
		m_pInstrument->SetSampleIndex(m_iSelectedNote, CInstrument2A03::NO_DPCM);
		GetDocument()->ModifyIrreversible();		// // //
		UpdateCurrentKey();
	}
	else if (Result) {
		// Add sample
		int Pitch = m_cComboSamplePitch.GetCurSel();
		m_pInstrument->SetSampleIndex(m_iSelectedNote, Result - 2);
		m_pInstrument->SetSamplePitch(m_iSelectedNote, Pitch);
		GetDocument()->ModifyIrreversible();		// // //
		UpdateCurrentKey();
	}

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnNMDblclkTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	// Preview sample from key table

	if (unsigned Sample = m_pInstrument->GetSampleIndex(m_iSelectedNote); Sample != CInstrument2A03::NO_DPCM) {		// // //
		int Pitch = m_pInstrument->GetSamplePitch(m_iSelectedNote) & 0x0F;
		CStringW sampleName = m_cTableListCtrl.GetItemText(m_iSelectedNote, 2);		// // //

		auto pSample = GetDSampleManager()->GetDSample(Sample);
		if (pSample && pSample->size() > 0u && sampleName != NO_SAMPLE_STR)
			Env.GetSoundGenerator()->PreviewSample(std::move(pSample), 0, Pitch);
	}

	*pResult = 0;
}

void CInstrumentEditorDPCM::OnEnChangeDeltaCounter()
{
	BOOL Trans;

	if (!m_pInstrument)
		return;

	int Value = GetDlgItemInt(IDC_DELTA_COUNTER, &Trans, FALSE);

	if (Trans == TRUE)
		Value = std::clamp(Value, 0, 127);
	else
		Value = -1;

	if (m_pInstrument->GetSampleDeltaValue(m_iSelectedNote) != Value) {
		m_pInstrument->SetSampleDeltaValue(m_iSelectedNote, Value);
		GetDocument()->ModifyIrreversible();		// // //
	}
}

void CInstrumentEditorDPCM::OnDeltaposDeltaSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	int Value = 0;

	if (!m_pInstrument)
		return;

	if (pNMUpDown->iPos <= 0 && pNMUpDown->iDelta < 0) {
		m_cEditDeltaCounter.SetWindowTextW(L"Off");
		Value = -1;
	}
	else {
		Value = pNMUpDown->iPos + pNMUpDown->iDelta;
		if (Value < 0)
			Value = 0;
		if (Value > 127)
			Value = 127;
		SetDlgItemInt(IDC_DELTA_COUNTER, Value, FALSE);
	}

	m_pInstrument->SetSampleDeltaValue(m_iSelectedNote, Value);
	GetDocument()->ModifyIrreversible();		// // //

	*pResult = 0;
}
