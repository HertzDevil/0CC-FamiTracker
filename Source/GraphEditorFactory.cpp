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

#include "GraphEditorFactory.h"
#include "GraphEditor.h"
#include "GraphEditorComponentImpl.h"
#include "Sequence.h"
#include "Instrument.h"
#include "DPI.h"

std::unique_ptr<CGraphEditor> CGraphEditorFactory::Make(CWnd *parent, CRect region, std::shared_ptr<CSequence> pSeq, int instType, int maxVol, int maxDuty) const {
	switch (pSeq->GetSequenceType()) {
	case sequence_t::Volume: {
		int items = instType == INST_VRC6 && pSeq->GetSetting() == SETTING_VOL_64_STEPS ? 0x3F : maxVol;
		return MakeBarEditor(parent, region, std::move(pSeq), items);
	}
	case sequence_t::Arpeggio:
		return MakeCellEditor(parent, region, std::move(pSeq), 21);
	case sequence_t::Pitch:
	case sequence_t::HiPitch:
		return MakePitchEditor(parent, region, std::move(pSeq));
	case sequence_t::DutyCycle:
		if (instType == INST_S5B)
			return MakeNoiseEditor(parent, region, std::move(pSeq), 0x20);
		else
			return MakeBarEditor(parent, region, std::move(pSeq), maxDuty);
	}

	return nullptr;
}

std::unique_ptr<CGraphEditor> CGraphEditorFactory::MakeBarEditor(CWnd *parent, CRect region, std::shared_ptr<CSequence> pSeq, int items) const {
	auto pEditor = std::make_unique<CGraphEditor>(std::move(pSeq));
	if (pEditor->CreateEx(NULL, NULL, L"", WS_CHILD, region, parent, 0) == -1)
		return nullptr;

	CRect graphRect = region;
	graphRect.left += DPI::SX(CGraphEditor::GRAPH_LEFT);		// // //
	graphRect.top += DPI::SY(5);
	graphRect.bottom -= DPI::SY(16);
	auto &graph = pEditor->MakeGraphComponent<GraphEditorComponents::CBarGraph>(*pEditor, graphRect, items);

	CRect bottomRect = region;		// // //
	bottomRect.left += DPI::SX(CGraphEditor::GRAPH_LEFT);
	bottomRect.top = bottomRect.bottom - DPI::SY(16);
	pEditor->MakeGraphComponent<GraphEditorComponents::CLoopReleaseBar>(*pEditor, bottomRect, graph);

	return pEditor;
}

std::unique_ptr<CGraphEditor> CGraphEditorFactory::MakeCellEditor(CWnd *parent, CRect region, std::shared_ptr<CSequence> pSeq, int items) const {
	auto pEditor = std::make_unique<CArpeggioGraphEditor>(std::move(pSeq));
	if (pEditor->CreateEx(NULL, NULL, L"", WS_CHILD, region, parent, 0) == -1)
		return nullptr;

	CRect graphRect = region;
	graphRect.left += DPI::SX(CGraphEditor::GRAPH_LEFT);		// // //
	graphRect.top += DPI::SY(5);
	graphRect.bottom -= DPI::SY(16);
	auto &graph = pEditor->MakeGraphComponent<GraphEditorComponents::CCellGraph>(*pEditor, graphRect, items);

	CRect bottomRect = region;		// // //
	bottomRect.left += DPI::SX(CGraphEditor::GRAPH_LEFT);
	bottomRect.top = bottomRect.bottom - DPI::SY(16);
	pEditor->MakeGraphComponent<GraphEditorComponents::CLoopReleaseBar>(*pEditor, bottomRect, graph);

	return pEditor;
}

std::unique_ptr<CGraphEditor> CGraphEditorFactory::MakePitchEditor(CWnd *parent, CRect region, std::shared_ptr<CSequence> pSeq) const {
	auto pEditor = std::make_unique<CGraphEditor>(std::move(pSeq));
	if (pEditor->CreateEx(NULL, NULL, L"", WS_CHILD, region, parent, 0) == -1)
		return nullptr;

	CRect graphRect = region;
	graphRect.left += DPI::SX(CGraphEditor::GRAPH_LEFT);		// // //
	graphRect.top += DPI::SY(5);
	graphRect.bottom -= DPI::SY(16);
	auto &graph = pEditor->MakeGraphComponent<GraphEditorComponents::CPitchGraph>(*pEditor, graphRect);

	CRect bottomRect = region;		// // //
	bottomRect.left += DPI::SX(CGraphEditor::GRAPH_LEFT);
	bottomRect.top = bottomRect.bottom - DPI::SY(16);
	pEditor->MakeGraphComponent<GraphEditorComponents::CLoopReleaseBar>(*pEditor, bottomRect, graph);

	return pEditor;
}

std::unique_ptr<CGraphEditor> CGraphEditorFactory::MakeNoiseEditor(CWnd *parent, CRect region, std::shared_ptr<CSequence> pSeq, int items) const {
	auto pEditor = std::make_unique<CGraphEditor>(std::move(pSeq));
	if (pEditor->CreateEx(NULL, NULL, L"", WS_CHILD, region, parent, 0) == -1)
		return nullptr;

	CRect graphRect = region;
	graphRect.left += DPI::SX(CGraphEditor::GRAPH_LEFT);		// // //
	graphRect.top += DPI::SY(5);
	graphRect.bottom -= DPI::SY(16) + GraphEditorComponents::CNoiseSelector::BUTTON_HEIGHT * 3;
	auto &graph = pEditor->MakeGraphComponent<GraphEditorComponents::CNoiseGraph>(*pEditor, graphRect, items);

	CRect flagsRect = region;		// // //
	flagsRect.left += DPI::SX(CGraphEditor::GRAPH_LEFT);
	flagsRect.bottom -= DPI::SY(16);
	flagsRect.top = graphRect.bottom;
	pEditor->MakeGraphComponent<GraphEditorComponents::CNoiseSelector>(*pEditor, flagsRect, graph);

	CRect bottomRect = region;		// // //
	bottomRect.left += DPI::SX(CGraphEditor::GRAPH_LEFT);
	bottomRect.top = bottomRect.bottom - DPI::SY(16);
	pEditor->MakeGraphComponent<GraphEditorComponents::CLoopReleaseBar>(*pEditor, bottomRect, graph);

	return pEditor;
}
