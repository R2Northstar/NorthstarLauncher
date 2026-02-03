if(NOT imgui-ws_FOUND)
    check_init_submodule(${PROJECT_SOURCE_DIR}/primedev/thirdparty/imgui-ws)

    set(ZLIB_LIBRARIES ZLIB::ZLIB)
    set(IMTUI_LIBRARY_TYPE STATIC)
    add_subdirectory(${PROJECT_SOURCE_DIR}/primedev/thirdparty/imgui-ws imgui-ws)
    set(imgui-ws_FOUND
        1
        PARENT_SCOPE
        )
endif()
