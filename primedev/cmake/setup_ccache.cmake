
list(
    APPEND
    COMPILERS
    "C"
    "CXX"
    )

message(CHECK_START "Looking for ccache")

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(CHECK_PASS "found")

    foreach(compiler ${COMPILERS})
        set(COMPILER_VAR "CMAKE_${compiler}_COMPILER")

        # Resolve any links to the currently used compiler
        get_filename_component(compiler_path "${COMPILER_VAR}" REALPATH)

        # Get the executable name of the compiler
        get_filename_component(compiler_name "${compiler_path}" NAME)

        if (NOT ${compiler_name} MATCHES "ccache")
            message(STATUS "Using ccache for ${compiler} compiler")
            set("${COMPILER_VAR}_LAUNCHER" "${CCACHE_PROGRAM}" CACHE FILEPATH "Path to compiler launch program, e.g. ccache")
        endif()
    endforeach()
else()
    message(CHECK_FAIL "not found")
endif()
