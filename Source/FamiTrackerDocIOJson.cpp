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

#include "FamiTrackerDocIOJson.h"
#include "FamiTrackerModule.h"
#include "FamiTrackerEnv.h"
#include "SoundChipService.h"
#include "SongData.h"
#include "Bookmark.h"
#include "BookmarkCollection.h"
#include "ChannelMap.h"
#include "Instrument2A03.h"
#include "InstrumentVRC7.h"
#include "InstrumentFDS.h"
#include "InstrumentN163.h"
#include "InstrumentManager.h"
#include "Sequence.h"
#include "SequenceCollection.h"
#include "SequenceManager.h"
#include "DSampleManager.h"
#include "ft0cc/doc/dpcm_sample.hpp"
#include "ft0cc/doc/groove.hpp"
#include "ft0cc/doc/pattern_note.hpp"
#include <optional>
#include "clip.h"
#include "EffectName.h"

using json = nlohmann::json;
using namespace std::string_literals;
using namespace std::string_view_literals;

namespace {

template <typename T, typename U>
U between(T v, U lo, U hi) {
	if (v < static_cast<T>(lo) || static_cast<T>(hi) < v)
		throw std::invalid_argument {"Expected value between [" +
			std::to_string(lo) + ", " + std::to_string(hi) + "], got " + std::to_string(v)};
	return static_cast<U>(v);
}

template <typename T, typename KeyT>
auto get_maybe(const json &j, const KeyT &k) {
	if (auto it = j.find(k); it != j.end())
		return it->template get<T>();
	return T { };
}

template <typename T>
T json_get_between(const json &j, const std::string &k, T lo, T hi) {
	auto v = j.at(k).get<json::number_integer_t>();
	if (v < static_cast<json::number_integer_t>(lo) || static_cast<json::number_integer_t>(hi) < v)
		throw std::invalid_argument {"Value at " + k + " must be between [" +
			std::to_string(lo) + ", " + std::to_string(hi) + "], got " + std::to_string(v)};
	return static_cast<T>(v);
}

template <typename T>
T json_get_between(const json &j, std::size_t k, T lo, T hi) {
	auto v = j.at(k).get<json::number_integer_t>();
	if (v < static_cast<json::number_integer_t>(lo) || static_cast<json::number_integer_t>(hi) < v)
		throw std::invalid_argument {"Value at " + std::to_string(k) + " must be between [" +
			std::to_string(lo) + ", " + std::to_string(hi) + "], got " + std::to_string(v)};
	return static_cast<T>(v);
}

template <typename T, typename KeyT>
auto get_maybe(const json &j, const KeyT &k, T&& def) {
	auto it = j.find(k);
	return it != j.end() ? it->template get<T>() : std::forward<T>(def);
}

template <typename F, typename KeyT>
void json_maybe(const json &j, const KeyT &k, F f) {
	if (auto it = j.find(k); it != j.end())
		f(*it);
}

} // namespace

namespace nlohmann {

template <typename T>
struct adl_serializer<std::optional<T>> {
	static void to_json(json &j, const std::optional<T> &x) {
		if (x.has_value())
			j = *x;
		else
			j = nullptr;
	}

	static void from_json(const json &j, std::optional<T> &x) {
		if (j.is_null())
			x = std::nullopt;
		else
			x = j.get<T>();
	}
};

} // namespace nlohmann

namespace {

constexpr std::string_view GetChipName(inst_type_t inst_type) noexcept {
	switch (inst_type) {
	case inst_type_t::INST_2A03: return "2A03"sv;
	case inst_type_t::INST_VRC6: return "VRC6"sv;
	case inst_type_t::INST_VRC7: return "VRC7"sv;
	case inst_type_t::INST_FDS:  return "FDS"sv;
	case inst_type_t::INST_N163: return "N163"sv;
	case inst_type_t::INST_S5B:  return "5B"sv;
	}
	return ""sv;
}

} // namespace

void to_json(json &j, const CPatternData &pattern) {
	j = json::array();

	for (auto [note, row] : with_index(pattern.Rows()))
		if (note != ft0cc::doc::pattern_note { })
			j.push_back(json {
				{"row", row},
				{"note", json(note)},
			});
}

void to_json(json &j, const CTrackData &track) {
	auto frame_list = json::array();
	for (unsigned f = 0; f < MAX_FRAMES; ++f)
		frame_list.push_back(track.GetFramePattern(f));

	j = json {
		{"effect_columns", track.GetEffectColumnCount()},
		{"frame_list", std::move(frame_list)},
		{"patterns", json::array()},
	};

	track.VisitPatterns([&] (const CPatternData &pattern, std::size_t index) {
		if (pattern.GetNoteCount(pattern.GetMaximumSize()) > 0) {
			j["patterns"].push_back(json {
				{"index", index},
				{"notes", json(pattern)},
			});
		}
	});
}

void to_json(json &j, const stHighlight &hl) {
	j = json::array({hl.First, hl.Second});
}

void to_json(json &j, const CBookmark &bm) {
	j = json {
		{"name", bm.m_sName},
		{"highlight", json(bm.m_Highlight)},
		{"frame", bm.m_iFrame},
		{"row", bm.m_iRow},
		{"persist", bm.m_bPersist},
	};
}

void to_json(json &j, const CBookmarkCollection &bmcol) {
	j = json::array();
	for (const auto &bm : bmcol)
		j.push_back(json(*bm));
}

void to_json(json &j, const CSongData &song) {
	j = json {
		{"speed", song.GetSongSpeed()},
		{"tempo", song.GetSongTempo()},
		{"frames", song.GetFrameCount()},
		{"rows", song.GetPatternLength()},
		{"title", std::string {song.GetTitle()}},
		{"uses_groove", song.GetSongGroove()},
		{"tracks", json::array()},
		{"highlight", {song.GetRowHighlight().First, song.GetRowHighlight().Second}},
		{"bookmarks", json(song.GetBookmarks())},
	};

	song.VisitTracks([&] (const CTrackData &track, stChannelID ch) {
		auto tj = json(track);
		auto &frame_list = tj["frame_list"];
		frame_list.erase(frame_list.begin() + song.GetFrameCount(), frame_list.end());
		tj["chip"] = std::string {FTEnv.GetSoundChipService()->GetChipShortName(ch.Chip)};
		tj["subindex"] = ch.Subindex;
		j["tracks"].push_back(std::move(tj));
	});
}

void to_json(json &j, const CChannelOrder &order) {
	j = json::array();
	order.ForeachChannel([&] (stChannelID ch) {
		j.push_back(json {
			{"chip", std::string {FTEnv.GetSoundChipService()->GetChipShortName(ch.Chip)}},
			{"subindex", ch.Subindex},
		});
	});
}

void to_json(json &j, const CFamiTrackerModule &modfile) {
	j = json {
		{"metadata", {
			{"title", std::string {modfile.GetModuleName()}},
			{"artist", std::string {modfile.GetModuleArtist()}},
			{"copyright", std::string {modfile.GetModuleCopyright()}},
			{"comment", std::string {modfile.GetComment()}},
			{"show_comment_on_open", modfile.ShowsCommentOnOpen()},
		}},
		{"global", {
			{"machine", modfile.GetMachine() == machine_t::PAL ? "pal" : "ntsc"},
			{"engine_speed", modfile.GetEngineSpeed()},
			{"vibrato_style", modfile.GetVibratoStyle() == vibrato_t::Up ? "old" : "new"},
			{"linear_pitch", modfile.GetLinearPitch()},
			{"fxx_split_point", modfile.GetSpeedSplitPoint()},
			{"detune", {
				{"semitones", modfile.GetTuningSemitone()},
				{"cents", modfile.GetTuningCent()},
			}},
		}},
		{"channels", json(modfile.GetChannelOrder())},
		{"songs", json::array()},
		{"instruments", json::array()},
		{"sequences", json::array()},
		{"dpcm_samples", json(*modfile.GetInstrumentManager()->GetDSampleManager())},
		{"detunes", json::array()},
		{"grooves", json::array()},
	};

	modfile.VisitSongs([&] (const CSongData &song) {
		auto sj = json(song);
		unsigned i = 0;
		while (i < sj["tracks"].size()) {
			bool hasChannel = false;
			for (const auto &cj : j["channels"])
				if (cj["chip"] == sj["tracks"][i]["chip"] && cj["subindex"] == sj["tracks"][i]["subindex"]) {
					hasChannel = true;
					++i;
					break;
				}

			if (!hasChannel)
				sj["tracks"].erase(i);
		}
		j["songs"].push_back(std::move(sj));
	});

	for (unsigned i = 0; i < MAX_INSTRUMENTS; ++i)
		if (auto pInst = modfile.GetInstrumentManager()->GetInstrument(i)) {
			auto ij = json(*pInst);
			ij["index"] = i;
			j["instruments"].push_back(std::move(ij));
		}

	for (int i = 0; i < 6; ++i)
		for (int n = 0; n < NOTE_COUNT; ++n)
			if (auto offs = modfile.GetDetuneOffset(i, n))
				j["detunes"].push_back(json {
					{"table_id", i},
					{"note", n},
					{"offset", offs},
				});

	for (unsigned i = 0; i < MAX_GROOVE; ++i)
		if (auto pGroove = modfile.GetGroove(i)) {
			auto gj = json(*pGroove);
			gj["index"] = i;
			j["grooves"].push_back(std::move(gj));
		}

	const auto InsertSequences = [&] (inst_type_t inst_type) {
		const CSequenceManager &smanager = *modfile.GetInstrumentManager()->GetSequenceManager(inst_type);
		auto name = std::string {GetChipName(inst_type)};
		for (auto t : enum_values<sequence_t>())
			if (const auto *seqcol = smanager.GetCollection(t))
				for (unsigned i = 0; i < MAX_SEQUENCES; ++i)
					if (auto pSeq = seqcol->GetSequence(i)) {
						auto sj = json(*pSeq);
						sj["chip"] = name;
						sj["macro_id"] = value_cast(t);
						sj["index"] = i;
						j["sequences"].push_back(std::move(sj));
					}
	};

	InsertSequences(INST_2A03);
	InsertSequences(INST_VRC6);
//	InsertSequences(INST_FDS);
	InsertSequences(INST_N163);
	InsertSequences(INST_S5B);
}

void to_json(json &j, const CSequence &seq) {
	j = json {
		{"items", json::array()},
		{"setting_id", value_cast(seq.GetSetting())},
	};
	for (unsigned i = 0; i < seq.GetItemCount(); ++i)
		j["items"].push_back(seq.GetItem(i));

	if (auto loop = seq.GetLoopPoint(); loop != (unsigned)-1)
		j["loop"] = loop;
	if (auto release = seq.GetReleasePoint(); release != (unsigned)-1)
		j["release"] = release;
}

void to_json(json &j, const CDSampleManager &dmanager) {
	j = json::array();
	for (unsigned i = 0; i < CDSampleManager::MAX_DSAMPLES; ++i)
		if (auto sample = dmanager.GetDSample(i)) {
			auto dj = json(*sample);
			dj["index"] = i;
			j.push_back(std::move(dj));
		}
}



namespace {

void to_json_common(json &j, const CInstrument &inst) {
	j = json {
		{"name", std::string {inst.GetName()}},
		{"chip", std::string {GetChipName(inst.GetType())}},
	};
}

} // namespace

void to_json(json &j, const CInstrument &inst) {
	to_json_common(j, inst);

	if (auto p2A03 = dynamic_cast<const CInstrument2A03 *>(&inst))
		to_json(j, *p2A03);
	else if (auto pVRC7 = dynamic_cast<const CInstrumentVRC7 *>(&inst))
		to_json(j, *pVRC7);
	else if (auto pFDS = dynamic_cast<const CInstrumentFDS *>(&inst))
		to_json(j, *pFDS);
	else if (auto pN163 = dynamic_cast<const CInstrumentN163 *>(&inst))
		to_json(j, *pN163);
	else if (auto pSeq = dynamic_cast<const CSeqInstrument *>(&inst))
		to_json(j, *pSeq);
}

void to_json(json &j, const CSeqInstrument &inst) {
	j["sequence_flags"] = json::array();
	for (auto t : enum_values<sequence_t>())
		if (inst.GetSeqEnable(t))
			j["sequence_flags"].push_back(json {
				{"macro_id", value_cast(t)},
				{"seq_index", inst.GetSeqIndex(t)},
			});
}

void to_json(json &j, const CInstrument2A03 &inst) {
	to_json(j, static_cast<const CSeqInstrument &>(inst));

	j["dpcm_map"] = json::array();
	for (int n = 0; n < NOTE_COUNT; ++n)
		if (auto d_index = inst.GetSampleIndex(n); d_index != CInstrument2A03::NO_DPCM)
			j["dpcm_map"].push_back(json {
				{"dpcm_index", d_index},
				{"pitch", inst.GetSamplePitch(n) & 0x0Fu},
				{"loop", inst.GetSampleLoop(n)},
				{"delta", inst.GetSampleDeltaValue(n)},
			});
}

void to_json(json &j, const CInstrumentVRC7 &inst) {
	if (inst.GetPatch() > 0)
		j["patch"] = inst.GetPatch();
	else {
		j["patch"] = json::array();
		for (int i = 0; i < 8; ++i)
			j["patch"].push_back(inst.GetCustomReg(i));
	}
}

void to_json(json &j, const CInstrumentFDS &inst) {
	const auto InsertSeq = [&] (sequence_t seq_type) {
		if (inst.GetSeqEnable(seq_type)) {
			auto sj = json(*inst.GetSequence(seq_type));
			sj["macro_id"] = value_cast(seq_type);
			j["sequences"].push_back(std::move(sj));
		}
	};

	j["sequences"] = json::array();
	InsertSeq(sequence_t::Volume);
	InsertSeq(sequence_t::Arpeggio);
	InsertSeq(sequence_t::Pitch);

	j["wave"] = inst.GetSamples();

	if (inst.GetModulationEnable())
		j["modulation"] = json {
			{"table", inst.GetModTable()},
			{"rate", inst.GetModulationSpeed()},
			{"depth", inst.GetModulationDepth()},
			{"delay", inst.GetModulationDelay()},
		};
}

void to_json(json &j, const CInstrumentN163 &inst) {
	to_json(j, static_cast<const CSeqInstrument &>(inst));

	j["waves"] = json::array();
	for (int i = 0; i < inst.GetWaveCount(); ++i)
		j["waves"].push_back(inst.GetSamples(i));
	j["wave_position"] = inst.GetWavePos();
}



namespace ft0cc::doc {

void to_json(json &j, const dpcm_sample &dpcm) {
	j = json {
		{"name", std::string {dpcm.name()}},
		{"samples", json::array()},
	};
	for (std::size_t i = 0, n = dpcm.size(); i < n; ++i)
		j["values"].push_back(dpcm.sample_at(i));
}

void to_json(json &j, const groove &groove) {
	j = json {
		{"values", json::array()},
	};
	for (auto x : groove)
		j["values"].push_back(x);
}

void to_json(json &j, const pattern_note &note) {
	switch (note.note()) {
	case ft0cc::doc::pitch::none: j["kind"] = "none"; break;
	case ft0cc::doc::pitch::halt: j["kind"] = "halt"; break;
	case ft0cc::doc::pitch::release: j["kind"] = "release"; break;
	case ft0cc::doc::pitch::echo:
		j["kind"] = "echo";
		j["value"] = note.oct();
		break;
	default:
		if (is_note(note.note())) {
			j["kind"] = "note";
			j["value"] = note.midi_note();
		}
	}

	if (note.vol() < MAX_VOLUME)
		j["volume"] = note.vol();
	if (note.inst() < MAX_INSTRUMENTS)
		j["inst_index"] = note.inst();
	else if (note.inst() == HOLD_INSTRUMENT)
		j["inst_index"] = -1;

	for (const auto &cmd_ : note.fx_cmds())
		if (cmd_.fx != effect_type::none) {
			j["effects"] = json::array();
			for (const auto &[fx, param] : note.fx_cmds())
				if (fx != effect_type::none)
					j["effects"].push_back(json {
						{"column", fx},
						{"name", std::string {EFF_CHAR[value_cast(fx)]}},
						{"param", param},
					});
			break;
		}
}

} // namespace ft0cc::doc



namespace ft0cc::doc {

void from_json(const json &j, dpcm_sample &dpcm) {
	json_maybe(j, "name", [&] (std::string &&name) {
		dpcm.rename(name);
	});

	json_maybe(j, "samples", [&] (std::vector<int64_t> &&samps) {
		dpcm.resize(std::min(samps.size(), dpcm_sample::max_size));
		for (std::size_t i = 0; i < dpcm.size(); ++i)
			dpcm.set_sample_at(i, clip<dpcm_sample::sample_t>(samps[i]));
	});
}

void from_json(const json &j, groove &g) {
	json_maybe(j, "values", [&] (std::vector<int64_t> &&vals) {
		g.resize(std::min(vals.size(), groove::max_size));
		for (std::size_t i = 0; i < g.size(); ++i)
			g.set_entry(i, clip<groove::entry_type>(vals[i]));
	});
}

void from_json(const json &j, pattern_note &note) {
	json_maybe(j, "kind", [&] (std::string &&kind) {
		if (kind == "note") {
			int midiNote = json_get_between(j, "value", 0, 95);
			note.set_note(pitch_from_midi(midiNote));
			note.set_oct(oct_from_midi(midiNote));
		}
		else if (kind == "halt")
			note.set_note(ft0cc::doc::pitch::halt);
		else if (kind == "release")
			note.set_note(ft0cc::doc::pitch::release);
		else if (kind == "echo") {
			note.set_note(ft0cc::doc::pitch::echo);
			note.set_oct(json_get_between(j, "value", (std::size_t)0u, ECHO_BUFFER_LENGTH - 1));
		}
		else if (kind == "none")
			note.set_note(ft0cc::doc::pitch::none);
	});

	if (j.count("volume"))
		note.set_vol(json_get_between(j, "volume", 0, MAX_VOLUME - 1));

	if (j.count("inst_index")) {
		auto inst = json_get_between(j, "inst_index", -1, MAX_INSTRUMENTS - 1);
		if (inst == -1)
			note.set_inst(HOLD_INSTRUMENT);
		else
			note.set_inst(inst);
	}

	json_maybe(j, "effects", [&] (std::vector<json> &&fxj) {
		for (const auto &fx : fxj) {
			int col = json_get_between(fx, "column", 0, MAX_EFFECT_COLUMNS - 1);
			auto ch = fx.at("name").get<std::string>();
			if (ch.size() != 1u)
				throw std::invalid_argument {"Effect name must be 1 character long"};
			ft0cc::doc::effect_type effect = FTEnv.GetSoundChipService()->TranslateEffectName(ch.front(), sound_chip_t::APU);
			if (effect == effect_type::none)
				throw std::invalid_argument {"Invalid effect name"};
			note.set_fx_cmd(col, {effect, static_cast<uint8_t>(json_get_between(fx, "param", 0, 255))});
		}
	});
}

} // namespace ft0cc::doc
