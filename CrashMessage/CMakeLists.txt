# CrashMessage

add_executable(CrashMessage
               "main.cpp"
)

set_target_properties(CrashMessage PROPERTIES
                      RUNTIME_OUTPUT_DIRECTORY ${NS_BINARY_DIR}/bin
                      OUTPUT_NAME CrashMsg
                      LINK_FLAGS "/MANIFEST:NO /DEBUG /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
)
