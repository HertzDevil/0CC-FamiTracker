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
#include <type_traits>		// // //
#include "ft0cc/cpputil/fs.hpp"		// // //

// #define AUTOSAVE
// #define DISABLE_SAVE		// // //

// External classes
class CFamiTrackerModule;		// // //
class CDocumentFile;

// // // + move core data fields into CFamiTrackerModule
// // // + move high-level pattern operations to CSongView
// // // + use CFamiTrackerModule / CSongData / CSongView directly

class CFamiTrackerDoc : public CDocument
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

	fs::path		GetFileTitle() const;		// // //

	//
	// Document file I/O
	//
	bool			IsFileLoaded() const;
	bool			HasLastLoadFailed() const;
	void			CreateEmpty();		// // //

	bool			GetExceededFlag() const;		// // //
	void			SetExceededFlag(bool Exceed = 1);

	void			Modify(bool Change);
	void			ModifyIrreversible();

	// Synchronization

	// T (*F)()
	// T must be void or default-constructible
	template <typename F>
	auto Locked(F f) const {		// // //
		using Ret = std::invoke_result_t<F>;
		if (CSingleLock lock(&m_csDocumentLock); lock.Lock())
			return f();
		else if constexpr (!std::is_void_v<Ret>)
			return Ret { };
	}

	// T (*F)()
	// T must be void or default-constructible
	template <typename F>
	auto Locked(F f, DWORD dwTimeOut) const {
		using Ret = std::invoke_result_t<F>;
		if (CSingleLock lock(&m_csDocumentLock); lock.Lock(dwTimeOut))
			return f();
		else if constexpr (!std::is_void_v<Ret>)
			return Ret { };
	}

	static std::unique_ptr<CFamiTrackerDoc> LoadImportFile(LPCWSTR lpszPathName);		// // // TODO: use module class directly

private:

	//
	// File management functions (load/save)
	//

	BOOL			SaveDocument(LPCWSTR lpszPathName) const;
	BOOL			OpenDocument(LPCWSTR lpszPathName);

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

	bool			m_bForceBackup = false;
	bool			m_bBackupDone = true;
	bool			m_bExceeded = false;			// // //

#ifdef AUTOSAVE
	// Auto save
	int				m_iAutoSaveCounter;
	CStringW			m_sAutoSaveFile;
#endif

	// Thread synchronization
	mutable CMutex	m_csDocumentLock;

// Overrides
public:
	BOOL OnNewDocument() override;
	BOOL OnSaveDocument(LPCWSTR lpszPathName) override;
	BOOL OnOpenDocument(LPCWSTR lpszPathName) override;
	void OnCloseDocument() override;
	void DeleteContents() override;
	void SetModifiedFlag(BOOL bModified = 1) override;

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnFileSaveAs();
	afx_msg void OnFileSave();
};
