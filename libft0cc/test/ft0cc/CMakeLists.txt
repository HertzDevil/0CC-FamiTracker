file(GLOB_RECURSE TEST_SOURCES *.cpp)

add_executable(ft0cctest ${TEST_SOURCES})
target_link_libraries(ft0cctest libgtest libgmock
	ft0cc)
add_test(NAME ft0cctest COMMAND
	ft0cctest)
