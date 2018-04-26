target_include_directories(ft0cc PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(ft0cc)

if(COVERAGE)
	target_compile_options(ft0cc PRIVATE --coverage)
	target_link_libraries(ft0cc --coverage)
endif()
