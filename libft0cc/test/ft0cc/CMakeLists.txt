set(TEST_SOURCES
	ft0cc/doc/groove.cpp)

add_executable(ft0cctest main.cpp ${TEST_SOURCES})
target_link_libraries(ft0cctest libgtest libgmock
	ft0cc)
add_test(NAME ft0cctest COMMAND
	ft0cctest)

if(COVERAGE)
	target_compile_options(ft0cctest PRIVATE --coverage)
	target_link_libraries(ft0cctest PRIVATE --coverage)
endif()
