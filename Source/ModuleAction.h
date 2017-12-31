/*
** FamiTracker - NES/Famicom sound tracker
** Copyright (C) 2005-2014  Jonathan Liss
**
** 0CC-FamiTracker is (C) 2014-2017 HertzDevil
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

#include "Action.h"
#include <string>
#include <memory>

class CInstrument;

// // // global document actions

class CModuleAction : public CAction {
	void SaveUndoState(const CMainFrame &MainFrm) override;
	void SaveRedoState(const CMainFrame &MainFrm) override;
	void RestoreUndoState(CMainFrame &MainFrm) const override;
	void RestoreRedoState(CMainFrame &MainFrm) const override;
};

namespace ModuleAction {

class CComment : public CModuleAction {
public:
	CComment(const std::string &comment, bool show) :
		newComment_(comment), newShow_(show) {
	}
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	void UpdateViews(CMainFrame &MainFrm) const override;

	std::string oldComment_;
	std::string newComment_;
	bool oldShow_ = false;
	bool newShow_ = false;
};

class CTitle : public CModuleAction {
public:
	CTitle(std::string_view str);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &other) override;
	void UpdateViews(CMainFrame &MainFrm) const override;

	std::string oldStr_;
	std::string newStr_;
};

class CArtist : public CModuleAction {
public:
	CArtist(std::string_view str);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &other) override;
	void UpdateViews(CMainFrame &MainFrm) const override;

	std::string oldStr_;
	std::string newStr_;
};

class CCopyright : public CModuleAction {
public:
	CCopyright(std::string_view str);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &other) override;
	void UpdateViews(CMainFrame &MainFrm) const override;

	std::string oldStr_;
	std::string newStr_;
};

class CAddInst : public CModuleAction {
public:
	CAddInst(unsigned index, std::shared_ptr<CInstrument> pInst);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	void UpdateViews(CMainFrame &MainFrm) const override;

	std::shared_ptr<CInstrument> inst_;
	unsigned index_;
	unsigned prev_;
};

class CRemoveInst : public CModuleAction {
public:
	CRemoveInst(unsigned index);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	void UpdateViews(CMainFrame &MainFrm) const override;

	std::shared_ptr<CInstrument> inst_;
	unsigned index_;
	unsigned nextIndex_;
};

class CInstName : public CModuleAction {
public:
	CInstName(unsigned index, std::string_view str);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	bool Merge(const CAction &other) override;
	void UpdateViews(CMainFrame &MainFrm) const override;

	unsigned index_;
	std::string oldStr_;
	std::string newStr_;
};

class CSwapInst : public CModuleAction {
public:
	CSwapInst(unsigned left, unsigned right);
private:
	bool SaveState(const CMainFrame &MainFrm) override;
	void Undo(CMainFrame &MainFrm) override;
	void Redo(CMainFrame &MainFrm) override;
	void UpdateViews(CMainFrame &MainFrm) const override;

	unsigned left_;
	unsigned right_;
};

} // namespace ModuleAction
