set(TEST_SOURCES
	doc/groove_test.cpp
	doc/inst_sequence_test.cpp)

add_executable(ft0cctest test_main.cpp ${TEST_SOURCES})
target_link_libraries(ft0cctest libgtest libgmock
	ft0cc)
add_test(NAME ft0cctest COMMAND
	ft0cctest)

if(COVERAGE)
	target_compile_options(ft0cctest PRIVATE --coverage)
	target_link_libraries(ft0cctest --coverage)
endif()
