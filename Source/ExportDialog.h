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
#include "../resource.h"		// // //
#include <optional>		// // //
#include "ft0cc/cpputil/fs.hpp"		// // //

class CCompiler;		// // //
class CExportDialog;
class CSimpleFile;		// // //

typedef void (CExportDialog::*exportFunc_t)();

// CExportDialog dialog

class CExportDialog : public CDialog
{
	DECLARE_DYNAMIC(CExportDialog)

public:
	CExportDialog(CWnd* pParent = NULL);   // standard constructor
	virtual ~CExportDialog();

// Dialog Data
	enum { IDD = IDD_EXPORT };

protected:
	// Remember last option when dialog is closed
	static inline int m_iExportOption = 0;		// // //

	CComboBox m_cExportTypes;		// // //

protected:
	static const exportFunc_t DEFAULT_EXPORT_FUNCS[];
	static const LPCWSTR	  DEFAULT_EXPORT_NAMES[];		// // //
	static const LPCWSTR	  DEFAULT_FILTERS[][2];		// // //
	static const int		  DEFAULT_EXPORTERS;

	static const LPCWSTR	  DPCMS_FILTER[2];

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void CreateNSF();
	void CreateNES();
	void CreateBIN();
	void CreatePRG();
	void CreateASM();
	void CreateNSFe();		// // //

	int GetMachineType() const;		// // //
	void UpdateMetadata(CCompiler &compiler);		// // //

	std::optional<CSimpleFile> OpenFile(const fs::path &fileName);		// // //

	// void (*F)(CFile &)
	template <typename F>
	void WithFile(const fs::path &initFName, F f);
	// void (*F)(CFile &)
	template <typename F>
	void WithFile(const fs::path &initFName, const CStringW &filterName, const CStringW &filterExt, F f);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedClose();
	afx_msg void OnBnClickedExport();
	afx_msg void OnBnClickedPlay();
};
