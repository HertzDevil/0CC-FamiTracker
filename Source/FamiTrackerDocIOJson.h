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

#include "ext/json/json.hpp"

class CPatternData;
class CTrackData;
struct stHighlight;
class CBookmark;
class CBookmarkCollection;
class CSongData;
class CChannelOrder;
class CFamiTrackerModule;
class CSequence;
class CDSampleManager;

class CInstrument;
class CSeqInstrument;
class CInstrument2A03;
class CInstrumentVRC7;
class CInstrumentFDS;
class CInstrumentN163;

void to_json(nlohmann::json &j, const CPatternData &pattern);
void to_json(nlohmann::json &j, const CTrackData &track);
void to_json(nlohmann::json &j, const stHighlight &hl);
void to_json(nlohmann::json &j, const CBookmark &bm);
void to_json(nlohmann::json &j, const CBookmarkCollection &bmcol);
void to_json(nlohmann::json &j, const CSongData &song);
void to_json(nlohmann::json &j, const CChannelOrder &order);
void to_json(nlohmann::json &j, const CFamiTrackerModule &modfile);
void to_json(nlohmann::json &j, const CSequence &seq);
void to_json(nlohmann::json &j, const CDSampleManager &dmanager);

void to_json(nlohmann::json &j, const CInstrument &inst);
void to_json(nlohmann::json &j, const CSeqInstrument &inst);
void to_json(nlohmann::json &j, const CInstrument2A03 &inst);
void to_json(nlohmann::json &j, const CInstrumentVRC7 &inst);
void to_json(nlohmann::json &j, const CInstrumentFDS &inst);
void to_json(nlohmann::json &j, const CInstrumentN163 &inst);

void from_json(const nlohmann::json &j, CPatternData &pattern);
void from_json(const nlohmann::json &j, CTrackData &track);
void from_json(const nlohmann::json &j, stHighlight &hl);
void from_json(const nlohmann::json &j, CBookmark &bm);
void from_json(const nlohmann::json &j, CBookmarkCollection &bmcol);
void from_json(const nlohmann::json &j, CSongData &song);
void from_json(const nlohmann::json &j, CChannelOrder &order);
void from_json(const nlohmann::json &j, CFamiTrackerModule &modfile);
void from_json(const nlohmann::json &j, CSequence &seq);
void from_json(const nlohmann::json &j, CDSampleManager &dmanager);

namespace ft0cc::doc {

class dpcm_sample;
class groove;
class pattern_note;

void to_json(nlohmann::json &j, const dpcm_sample &dpcm);
void to_json(nlohmann::json &j, const groove &groove);
void to_json(nlohmann::json &j, const pattern_note &note);

void from_json(const nlohmann::json &j, dpcm_sample &dpcm);
void from_json(const nlohmann::json &j, groove &groove);
void from_json(const nlohmann::json &j, pattern_note &note);

} // namespace ft0cc::doc
