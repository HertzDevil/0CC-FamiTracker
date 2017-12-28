cmake_minimum_required(VERSION 3.0)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project(libft0cc)
enable_testing()

if(${CMAKE_VERSION} VERSION_GREATER "3.8.1")
	set(CMAKE_CXX_STANDARD 17 CACHE STRING
		"The C++ standard whose features are requested to build this target." FORCE)
	set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL
		"Boolean describing whether the value of CXX_STANDARD is a requirement." FORCE)
endif()
set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL
	"A flag specifying whether compiler specific extensions should be used." FORCE)

set(COVERAGE OFF CACHE BOOL "Enables coverage reports.")

if(MSVC)
	add_compile_options(/std:c++17 /permissive- /Zc:forScope /Zc:inline /Zc:rvalueCast /Za)
	add_compile_options(/W4 /WX /sdl)
	add_definitions(-DUNICODE -D_UNICODE)
	set(variables
		CMAKE_CXX_FLAGS_DEBUG
		CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS_MINSIZEREL
	)
	foreach(variable ${variables})
		if(${variable} MATCHES "/MD")
			string(REGEX REPLACE "/MD" "/MT" ${variable} "${${variable}}")
		endif()
	endforeach()
else()
	if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
		set(CLANG_DEFAULT_CXX_STDLIB libc++)
	endif()
	add_compile_options(-std=c++17)
	add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

add_subdirectory(src)
add_subdirectory(test)
