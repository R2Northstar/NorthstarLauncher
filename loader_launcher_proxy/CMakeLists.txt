add_library(launcher SHARED
    dllmain.cpp
)

target_link_libraries(launcher LINK_PUBLIC shlwapi msvcr120)

target_compile_features(launcher PUBLIC
	cxx_std_20
)

set_target_properties(launcher PROPERTIES PREFIX "")