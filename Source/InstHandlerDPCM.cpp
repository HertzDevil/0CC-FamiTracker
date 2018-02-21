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

#include "InstHandlerDPCM.h"
#include "Instrument2A03.h"
#include "ChannelHandlerInterface.h"

CInstHandlerDPCM::CInstHandlerDPCM(CChannelHandlerInterface *pInterface) :
	CInstHandler(pInterface, 0)
{
}

void CInstHandlerDPCM::LoadInstrument(std::shared_ptr<CInstrument> pInst)
{
	m_pInstrument = pInst;
}

void CInstHandlerDPCM::TriggerInstrument()
{
	CChannelHandlerInterfaceDPCM *pInterface = dynamic_cast<CChannelHandlerInterfaceDPCM*>(m_pInterface);
	if (pInterface == nullptr) return;
	if (auto pDPCMInst = std::dynamic_pointer_cast<const CInstrument2A03>(m_pInstrument)) {
		const int Val = m_pInterface->GetNote();
		if (auto pSamp = pDPCMInst->GetDSample(Val)) {
			pInterface->WriteDCOffset(pDPCMInst->GetSampleDeltaValue(Val));
			pInterface->SetLoopOffset(pDPCMInst->GetSampleLoopOffset(Val));
			pInterface->PlaySample(pSamp, pDPCMInst->GetSamplePitch(Val));
		}
	}
}

void CInstHandlerDPCM::ReleaseInstrument()
{
}

void CInstHandlerDPCM::UpdateInstrument()
{
}
