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


#pragma once

#include "stdafx.h"		// // //
#include <memory>		// // //
#include "DocumentInterface.h"

// #define AUTOSAVE
// #define DISABLE_SAVE		// // //

// External classes
class CFamiTrackerModule;		// // //
class CDocumentFile;
class CSoundChipSet;		// // //

// // // TODO:
// // // + move core data fields into CFamiTrackerModule
// // // + move high-level pattern operations to CSongView
// // // + use CFamiTrackerModule / CSongData / CSongView directly
// // // - move action handler into CFamiTrackerDoc

class CFamiTrackerDoc : public CDocument, public CDocumentInterface
{
	struct ctor_t { };		// // //

public:
	CFamiTrackerDoc(ctor_t) : CFamiTrackerDoc() { }

protected: // create from serialization only
	CFamiTrackerDoc();
	DECLARE_DYNCREATE(CFamiTrackerDoc)

	virtual ~CFamiTrackerDoc();

	// Static functions
public:
	static CFamiTrackerDoc *GetDoc();


	// Other
#ifdef AUTOSAVE
	void AutoSave();
#endif

	//
	// Public functions
	//
public:
	// // // implementation
	CFamiTrackerModule *GetModule() noexcept;
	const CFamiTrackerModule *GetModule() const noexcept;

	CString			GetFileTitle() const;

	//
	// Document file I/O
	//
	bool			IsFileLoaded() const;
	bool			HasLastLoadFailed() const;
	void			CreateEmpty();		// // //

	bool			GetExceededFlag() const;		// // //
	void			SetExceededFlag(bool Exceed = 1);

	void			Modify(bool Change) override;
	void			ModifyIrreversible() override;

	// Synchronization
	BOOL			LockDocument() const;
	BOOL			LockDocument(DWORD dwTimeout) const;
	BOOL			UnlockDocument() const;

	static std::unique_ptr<CFamiTrackerDoc> LoadImportFile(LPCTSTR lpszPathName);		// // // TODO: use module class directly

	void			SelectExpansionChip(const CSoundChipSet &chips, unsigned n163chs);		// // //

private:

	//
	// File management functions (load/save)
	//

	BOOL			SaveDocument(LPCTSTR lpszPathName) const;
	BOOL			OpenDocument(LPCTSTR lpszPathName);

	BOOL			OpenDocumentOld(CFile *pOpenFile);
	BOOL			OpenDocumentNew(CDocumentFile &DocumentFile);

#ifdef AUTOSAVE
	void			SetupAutoSave();
	void			ClearAutoSave();
#endif

	//
	// Private variables
	//
private:
	std::unique_ptr<CFamiTrackerModule> module_;		// // // implementation

	// State variables
	bool			m_bFileLoaded = false;			// Is a file loaded?
	bool			m_bFileLoadFailed = false;		// Last file load operation failed
	unsigned int	m_iFileVersion;					// Loaded file version

	bool			m_bForceBackup;
	bool			m_bBackupDone;
	bool			m_bExceeded = false;			// // //

#ifdef AUTOSAVE
	// Auto save
	int				m_iAutoSaveCounter;
	CString			m_sAutoSaveFile;
#endif

	// Thread synchronization
	mutable CMutex	m_csDocumentLock;

// Overrides
public:
	BOOL OnNewDocument() override;
	BOOL OnSaveDocument(LPCTSTR lpszPathName) override;
	BOOL OnOpenDocument(LPCTSTR lpszPathName) override;
	void OnCloseDocument() override;
	void DeleteContents() override;
	void SetModifiedFlag(BOOL bModified = 1) override;

// Implementation
private:
#ifdef _DEBUG
	void AssertValid() const override;
	void Dump(CDumpContext& dc) const override;
#endif

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
};
