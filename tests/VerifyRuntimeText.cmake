file(GLOB_RECURSE runtime_sources
    "${SOURCE_ROOT}/src/*.cpp"
    "${SOURCE_ROOT}/src/*.h")

set(non_ascii_files "")
foreach(source IN LISTS runtime_sources)
    file(READ "${source}" contents HEX)
    string(LENGTH "${contents}" hex_length)
    math(EXPR last_byte "${hex_length} - 2")
    foreach(offset RANGE 0 ${last_byte} 2)
        string(SUBSTRING "${contents}" ${offset} 2 byte)
        if(byte MATCHES "^[89a-fA-F]")
            list(APPEND non_ascii_files "${source}")
            break()
        endif()
    endforeach()
endforeach()

if(non_ascii_files)
    list(REMOVE_DUPLICATES non_ascii_files)
    list(JOIN non_ascii_files "\n  " formatted)
    message(FATAL_ERROR "Runtime source text must remain rendering-safe ASCII:\n  ${formatted}")
endif()
