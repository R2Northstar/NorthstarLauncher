if(NOT TARGET uWebSockets)
    set(UWEBSOCKETS_BASE_DIR "${PROJECT_SOURCE_DIR}/primedev/thirdparty/uWebSockets")

    find_package(OpenSSL REQUIRED)

    add_library(uWebSockets STATIC
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src/bsd.c
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src/context.c
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src/loop.c
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src/quic.c
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src/socket.c
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src/udp.c
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src/crypto/openssl.c
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src/crypto/sni_tree.cpp
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src/eventing/libuv.c
    )

    target_include_directories(uWebSockets PUBLIC
        ${UWEBSOCKETS_BASE_DIR}/uSockets/src
        ${UWEBSOCKETS_BASE_DIR}/src
        ${LIBUV_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIRS}
    )

    target_link_libraries(uWebSockets PUBLIC
        ${OPENSSL_LIBRARIES}
        ${LIBUV_LIBRARIES}
        ${ZLIB_LIBRARIES}
        ${CMAKE_THREAD_LIBS_INIT}
    )

    target_link_libraries(uWebSockets PRIVATE OpenSSL::SSL OpenSSL::Crypto)

    set(uWebSockets_FOUND TRUE)
endif()
