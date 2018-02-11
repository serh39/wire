file(GLOB_RECURSE TESTS_SOURCES *.cpp)
add_executable(libwire-tests ${TESTS_SOURCES})
target_link_libraries(libwire-tests libwire gtest)
add_test(NAME libwire COMMAND libwire-tests)
