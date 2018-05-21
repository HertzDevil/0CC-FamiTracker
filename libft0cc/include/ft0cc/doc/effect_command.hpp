/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * 0CC-FamiTracker is (C) 2014-2018 HertzDevil
 *
 * Alternatively, the contents of this file may be used under the terms
 * of the GNU General Public License Version 2, as described below:
 *
 * This file is free software: you may copy, redistribute and/or modify
 * it under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/. */


#pragma once

#include "ft0cc/cpputil/strong_ordering.hpp"
#include "ft0cc/doc/effect_type.hpp"

namespace ft0cc::doc {

using effect_param_t = std::uint8_t;

struct effect_command {
	effect_type fx {effect_type::none};
	effect_param_t param {0u};

	constexpr int compare(const effect_command &other) const noexcept {
		if (fx < other.fx)
			return -1;
		if (fx > other.fx)
			return 1;
		if (fx == effect_type::none)
			return 0;
		if (param < other.param)
			return -1;
		if (param > other.param)
			return 1;
		return 0;
	}
};

ENABLE_STRONG_ORDERING(effect_command)

} // namespace ft0cc::doc
