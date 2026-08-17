add_executable(dozi_mastering_tests MasteringTests.cpp)
target_link_libraries(dozi_mastering_tests PRIVATE Dozi::Core)
target_compile_options(dozi_mastering_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
add_test(NAME dozi_mastering_tests COMMAND dozi_mastering_tests)
