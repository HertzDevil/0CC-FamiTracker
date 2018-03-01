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

#include "SequenceParser.h"
#include "Sequence.h"
#include "PatternNote.h"
#include "sv_regex.h"
#include "NumConv.h"
#include "NoteName.h"

// // //

std::string CSeqConversionDefault::ToString(char Value) const
{
	return std::to_string(Value);
}

bool CSeqConversionDefault::ToValue(std::string_view sv)
{
	m_bReady = false;
	if (sv == "$$") {
		m_bHex = true;
		return true;
	}

	if (!GetNextTerm(sv, m_iCurrentValue))
		return false;
	m_iTargetValue = m_iCurrentValue;
	m_iRepeat = m_iValueDiv = 1;
	if (!sv.empty() && sv.front() == ':') {
		if (sv.remove_prefix(1), sv.empty())
			return false;
		if (!GetNextInteger(sv, m_iValueDiv))
			return false;
		if (m_iValueDiv <= 0)
			return false;
		if (!sv.empty() && sv.front() == ':') {
			if (sv.remove_prefix(1), sv.empty())
				return false;
			if (!GetNextTerm(sv, m_iTargetValue))
				return false;
		}
	}
	if (!sv.empty() && sv.front() == '\'') {
		if (sv.remove_prefix(1), sv.empty())
			return false;
		if (!GetNextInteger(sv, m_iRepeat))
			return false;
	}
	if (!sv.empty() || m_iRepeat <= 0)
		return false;
	m_iValueInc = m_iTargetValue - m_iCurrentValue;
	m_iRepeatCounter = m_iCounter = m_iValueMod = 0;
	return m_bReady = true;
}

bool CSeqConversionDefault::IsReady() const
{
	return m_bReady;
}

char CSeqConversionDefault::GetValue()
{
	// do not use float division
	int Val = m_iCurrentValue;
	if (Val > m_iMaxValue) Val = m_iMaxValue;
	if (Val < m_iMinValue) Val = m_iMinValue;
	if (++m_iRepeatCounter >= m_iRepeat) {
		m_iValueMod += m_iValueInc;
		m_iCurrentValue += m_iValueMod / m_iValueDiv;
		m_iValueMod %= m_iValueDiv;
		m_iRepeatCounter = 0;
		if (++m_iCounter >= m_iValueDiv)
			m_bReady = false;
	}
	return Val;
}

void CSeqConversionDefault::OnStart()
{
	m_bHex = false;
}

void CSeqConversionDefault::OnFinish()
{
	m_bReady = false;
}

bool CSeqConversionDefault::GetNextInteger(std::string_view &sv, int &Out, bool Signed) const
{
	re::svmatch m;

	if (m_bHex) {
		static const std::regex HEX_RE {R"(^([\+-]?)[0-9A-Fa-f]+)", std::regex_constants::optimize};
		if (!std::regex_search(sv.begin(), sv.end(), m, HEX_RE))
			return false;
		if (Signed && !m[1].length())
			return false;
		if (auto val = conv::to_int(re::sv_from_submatch(m[0]), 16))
			Out = *val;
		else
			return false;
	}
	else {
		static const std::regex NUMBER_RE {R"(^([\+-]?)[0-9]+)", std::regex_constants::optimize};
		static const std::regex HEX_PREFIX_RE {R"(([\+-]?)[\$x]([0-9A-Fa-f]+))", std::regex_constants::optimize}; // do not allow 0x prefix
		if (std::regex_search(sv.begin(), sv.end(), m, HEX_PREFIX_RE)) {
			if (Signed && !m[1].length())
				return false;
			if (auto val = conv::to_int(re::sv_from_submatch(m[2]), 16))
				Out = *val;
			else
				return false;
			if (m[1] == "-")
				Out = -Out;
		}
		else if (std::regex_search(sv.begin(), sv.end(), m, NUMBER_RE)) {
			if (Signed && !m[1].length())
				return false;
			if (auto val = conv::to_int(re::sv_from_submatch(m[0])))
				Out = *val;
			else
				return false;
		}
		else
			return false;
	}

	sv.remove_prefix(std::distance(sv.begin(), m.suffix().first));
	return true;
}

bool CSeqConversionDefault::GetNextTerm(std::string_view &sv, int &Out)
{
	return GetNextInteger(sv, Out);
}



std::string CSeqConversion5B::ToString(char Value) const
{
	std::string Str = std::to_string(Value & 0x1F);
	auto m = enum_cast<s5b_mode_t>((unsigned char)Value);
	if ((m & s5b_mode_t::Square) == s5b_mode_t::Square)
		Str.push_back('t');
	if ((m & s5b_mode_t::Noise) == s5b_mode_t::Noise)
		Str.push_back('n');
	if ((m & s5b_mode_t::Envelope) == s5b_mode_t::Envelope)
		Str.push_back('e');
	return Str;
}

bool CSeqConversion5B::ToValue(std::string_view sv)
{
	m_iEnableFlags = -1;
	return CSeqConversionDefault::ToValue(sv);
}

char CSeqConversion5B::GetValue()
{
	return CSeqConversionDefault::GetValue() | m_iEnableFlags;
}

bool CSeqConversion5B::GetNextTerm(std::string_view &sv, int &Out)
{
	if (!GetNextInteger(sv, Out))
		return false;

	static const std::regex S5B_FLAGS_RE {R"(^[TtNnEe]*)"};
	if (re::svmatch m; std::regex_search(sv.begin(), sv.end(), m, S5B_FLAGS_RE)) {
		if (m_iEnableFlags == -1) {
			m_iEnableFlags = 0;
			auto flags = re::sv_from_submatch(m[0]);
			if (flags.find_first_of("Tt") != std::string::npos)
				m_iEnableFlags |= value_cast(s5b_mode_t::Square);
			if (flags.find_first_of("Nn") != std::string::npos)
				m_iEnableFlags |= value_cast(s5b_mode_t::Noise);
			if (flags.find_first_of("Ee") != std::string::npos)
				m_iEnableFlags |= value_cast(s5b_mode_t::Envelope);
		}
		sv.remove_prefix(std::distance(sv.begin(), m.suffix().first));
	}
	return true;
}



std::string CSeqConversionArpScheme::ToString(char Value) const		// // //
{
	int Offset = m_iMinValue + ((Value - m_iMinValue) & 0x3F);
	auto Scheme = enum_cast<arp_scheme_mode_t>((unsigned char)(Value & 0xC0));
	if (!Offset) {
		switch (Scheme) {
		case arp_scheme_mode_t::X: return "x";
		case arp_scheme_mode_t::Y: return "y";
		case arp_scheme_mode_t::NegY: return "-y";
		case arp_scheme_mode_t::none: return "0";
		}
	}
	std::string Str = std::to_string(Offset);
	switch (Scheme) {
	case arp_scheme_mode_t::X: Str += "+x"; break;
	case arp_scheme_mode_t::Y: Str += "+y"; break;
	case arp_scheme_mode_t::NegY: Str += "-y"; break;
	case arp_scheme_mode_t::none: break;
	}
	return Str;
}

bool CSeqConversionArpScheme::ToValue(std::string_view sv)
{
	m_iArpSchemeFlag = -1;
	return CSeqConversionDefault::ToValue(sv);
}

char CSeqConversionArpScheme::GetValue()
{
	return (CSeqConversionDefault::GetValue() & 0x3F) | m_iArpSchemeFlag;
}

bool CSeqConversionArpScheme::GetNextTerm(std::string_view &sv, int &Out)
{
	const auto SchemeFunc = [&] (std::string_view str) {
		if (m_iArpSchemeFlag != -1)
			return;
		m_iArpSchemeFlag = 0;
		if (str == "x" || str == "+x")
			m_iArpSchemeFlag = static_cast<unsigned char>(arp_scheme_mode_t::X);
		else if (str == "y" || str == "+y")
			m_iArpSchemeFlag = static_cast<unsigned char>(arp_scheme_mode_t::Y);
		else if (str == "-y")
			m_iArpSchemeFlag = static_cast<unsigned char>(arp_scheme_mode_t::NegY);
	};

	re::svmatch m;
	static const std::regex SCHEME_HEAD_RE {R"(^(x|y|-y))"};
	static const std::regex SCHEME_TAIL_RE {R"(^(\+x|\+y|-y)?)"};

	if (std::regex_search(sv.begin(), sv.end(), m, SCHEME_HEAD_RE)) {
		SchemeFunc(re::sv_from_submatch(m[0]));
		sv.remove_prefix(std::distance(sv.begin(), m.suffix().first));
		Out = 0;
		GetNextInteger(sv, Out, true); // optional
	}
	else {
		if (!GetNextInteger(sv, Out))
			return false;
		if (std::regex_search(sv.begin(), sv.end(), m, SCHEME_TAIL_RE)) {
			SchemeFunc(re::sv_from_submatch(m[0])); // optional
			sv.remove_prefix(std::distance(sv.begin(), m.suffix().first));
		}
	}
	return true;
}



std::string CSeqConversionArpFixed::ToString(char Value) const
{
	return GetNoteString(GET_NOTE(static_cast<unsigned char>(Value)), GET_OCTAVE(static_cast<unsigned char>(Value)));
}

bool CSeqConversionArpFixed::GetNextTerm(std::string_view &sv, int &Out)
{
	std::string_view sv_(sv);
	if (CSeqConversionDefault::GetNextTerm(sv_, Out)) {
		sv = sv_;
		return true;
	}

	re::svmatch m;
	static const std::regex FIXED_RE {R"(([A-Ga-g])([+\-#b]*)([0-9]+))"};
	static const int NOTE_OFFSET[] = {9, 11, 0, 2, 4, 5, 7};
	if (!std::regex_match(sv.begin(), sv.end(), m, FIXED_RE))
		return false;

	char ch = re::sv_from_submatch(m[1])[0];
	int Note = NOTE_OFFSET[ch >= 'a' ? ch - 'a' : ch - 'A'];
	auto accstr = re::sv_from_submatch(m[2]);
	for (const auto &acc : accstr) {
		switch (acc) {
		case '+': case '#': ++Note; break;
		/*case '-':*/ case 'b': --Note; break;
		}
	}

	if (auto oct = conv::to_uint(re::sv_from_submatch(m[3])))
		Note += NOTE_RANGE * *oct;
	else
		return false;

	Out = Note;

	sv.remove_prefix(std::distance(sv.begin(), m.suffix().first));
	return true;
}



void CSequenceParser::SetSequence(std::shared_ptr<CSequence> pSeq)
{
	m_pSequence = std::move(pSeq);
}

void CSequenceParser::SetConversion(std::unique_ptr<CSeqConversionBase> pConv)
{
	m_pConversion = std::move(pConv);
}

void CSequenceParser::ParseSequence(std::string_view sv)
{
	auto Setting = static_cast<seq_setting_t>(m_pSequence->GetSetting());
	m_pSequence->Clear();
	m_pSequence->SetSetting(Setting);
	m_iPushedCount = 0;

	const auto PushFunc = [&] () {
		while (m_pConversion->IsReady()) {
			m_pSequence->SetItem(m_iPushedCount, m_pConversion->GetValue());
			if (++m_iPushedCount >= MAX_SEQUENCE_ITEMS)
				break;
		}
	};

	int Loop = -1, Release = -1;

	m_pConversion->OnStart();
	PushFunc();
	for (auto x : re::tokens(sv)) {
		auto str = re::sv_from_submatch(x[0]);
		if (str == "|")
			Loop = m_iPushedCount;
		else if (str == "/")
			Release = m_iPushedCount;
		else if (m_pConversion->ToValue(str))
			PushFunc();
	}
	m_pConversion->OnFinish();
	PushFunc();
	m_pSequence->SetItemCount(m_iPushedCount);
	m_pSequence->SetLoopPoint(Loop);
	m_pSequence->SetReleasePoint(Release);
}

std::string CSequenceParser::PrintSequence() const
{
	std::string str;

	const int Loop = m_pSequence->GetLoopPoint();
	const int Release = m_pSequence->GetReleasePoint();

	for (int i = 0, Count = m_pSequence->GetItemCount(); i < Count; ++i) {
		if (i == Loop)
			str.append("| ");
		if (i == Release)
			str.append("/ ");
		str.append(m_pConversion->ToString(m_pSequence->GetItem(i)));
		str.push_back(' ');
	}

	return str;
}
