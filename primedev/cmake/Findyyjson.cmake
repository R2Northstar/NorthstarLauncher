if(NOT yyjson_FOUND)
    check_init_submodule(${PROJECT_SOURCE_DIR}/primedev/thirdparty/yyjson)

    add_subdirectory(${PROJECT_SOURCE_DIR}/primedev/thirdparty/yyjson yyjson)
    set(yyjson_FOUND 1)
endif()
