add_executable(dozi_mastering_probe Main.cpp)
target_link_libraries(dozi_mastering_probe PRIVATE Dozi::Core)
target_compile_options(dozi_mastering_probe PRIVATE -Wall -Wextra -Wpedantic -Werror)
