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

#include <regex>
#include <string_view>

namespace re {

template <typename CharT, typename Traits, typename ReTraits>
auto string_gmatch(std::basic_string_view<CharT, Traits> sv, const std::basic_regex<CharT, ReTraits> &re) {
	using sv_t = std::basic_string_view<CharT, Traits>;
	using it_t = typename sv_t::const_iterator;
	struct rng {
		rng(sv_t sv, const std::basic_regex<CharT, ReTraits> &re) :
			b_(sv.begin(), sv.end(), re) { }
		decltype(auto) begin() {
			return b_;
		}
		decltype(auto) end() {
			return e_;
		}
	private:
		std::regex_iterator<it_t, CharT, ReTraits> b_, e_;
	};
	return rng {sv, re};
}

static inline const std::regex words {R"(\S+)", std::regex_constants::optimize};

template <typename T>
auto tokens(T sv) {
	return string_gmatch(std::string_view {sv}, words);
}

using svmatch = std::match_results<std::string_view::const_iterator>;
using svsub_match = std::sub_match<std::string_view::const_iterator>;
using svregex_iterator = std::regex_iterator<std::string_view::const_iterator>;
using wsvmatch = std::match_results<std::wstring_view::const_iterator>;
using wsvsub_match = std::sub_match<std::wstring_view::const_iterator>;
using wsvregex_iterator = std::regex_iterator<std::wstring_view::const_iterator>;

} // namespace re
