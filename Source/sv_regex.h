// CC0
// see also http://open-std.org/JTC1/SC22/WG21/docs/papers/2017/p0506r2.pdf

#pragma once

#include <regex>
#include <string_view>

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

using svmatch = std::match_results<std::string_view::const_iterator>;
using svsub_match = std::sub_match<std::string_view::const_iterator>;
using svregex_iterator = std::regex_iterator<std::string_view::const_iterator>;
using wsvmatch = std::match_results<std::wstring_view::const_iterator>;
using wsvsub_match = std::sub_match<std::wstring_view::const_iterator>;
using wsvregex_iterator = std::regex_iterator<std::wstring_view::const_iterator>;
