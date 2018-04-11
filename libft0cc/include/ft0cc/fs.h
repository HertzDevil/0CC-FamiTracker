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
