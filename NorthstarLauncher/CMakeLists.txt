add_executable(NorthstarLauncher
    main.cpp
    resources.rc
)

target_link_libraries(NorthstarLauncher LINK_PUBLIC ws2_32 shlwapi msvcr120)

target_compile_features(NorthstarLauncher PUBLIC
	cxx_std_20
)