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
 * Free Software Foundation, either version XX of the License, or (at your
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

#if __cplusplus >= 201703L
// conforming C++17 compiler
#	if __has_include(<filesystem>)
#		include <filesystem>
		namespace fs = std::filesystem;
#	else
#		include <experimental/filesystem>
		namespace fs = std::experimental::filesystem::v1;
#	endif
#elif defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 191426329
// VS2017 version 15.7 preview 3 and above
#	include <filesystem>
	namespace fs = std::filesystem;
#elif defined(_MSVC_LANG) && _MSVC_LANG > 201402L
// older versions of VS2017
#	include <experimental/filesystem>
	namespace fs = std::experimental::filesystem::v1;
#elif defined(__has_include)
// more C++17 support
#	if __has_include(<filesystem>)
#		include <filesystem>
		namespace fs = std::filesystem;
#	elif __has_include(<experimental/filesystem>)
#		include <experimental/filesystem>
		namespace fs = std::experimental::filesystem::v1;
#	elif __has_include(<boost/filesystem.hpp>)
#		include <boost/filesystem.hpp>
		namespace fs = boost::filesystem;
#	endif
#else
// do not assume the presence of <experimental/filesystem> here
#	include <boost/filesystem.hpp>
	namespace fs = boost::filesystem;
#endif
