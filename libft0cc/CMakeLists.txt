cmake_minimum_required(VERSION 3.0)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project(libft0cc)
enable_testing()

#set(CMAKE_CXX_STANDARD 17 CACHE STRING
#	"The C++ standard whose features are requested to build this target." FORCE)
#set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL
#	"Boolean describing whether the value of CXX_STANDARD is a requirement." FORCE)
set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL
	"A flag specifying whether compiler specific extensions should be used." FORCE)

SET(COVERAGE OFF CACHE BOOL "Enables coverage reports.")

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
	add_compile_options(-std=c++17)
	add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

set(SOURCES
	src/ft0cc/doc/groove.cpp)

add_library(ft0cc STATIC ${SOURCES})
target_include_directories(ft0cc PUBLIC include)
if(COVERAGE)
	target_compile_options(ft0cc PRIVATE --coverage)
	target_link_libraries(ft0cc --coverage)
endif()

add_subdirectory(test)
