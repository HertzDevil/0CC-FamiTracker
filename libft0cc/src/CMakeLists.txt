set(SOURCES
	ft0cc/doc/groove.cpp
	ft0cc/doc/inst_sequence.cpp
	ft0cc/doc/dpcm_sample.cpp)

add_library(ft0cc STATIC ${SOURCES})
target_include_directories(ft0cc PUBLIC ${CMAKE_SOURCE_DIR}/include)
if(COVERAGE)
	target_compile_options(ft0cc PRIVATE --coverage)
	target_link_libraries(ft0cc --coverage)
endif()
