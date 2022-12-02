# CMakeList.txt : CMake project for NorthstarLauncher, include source and define
# project specific logic here.
#

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")

# Add source to this project's executable.
add_executable (NorthstarLauncher "main.cpp" ${APP_ICON_RESOURCE_WINDOWS})

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET NorthstarLauncher PROPERTY CXX_STANDARD 20)
endif()

target_link_libraries(NorthstarLauncher wsock32 ws2_32 shlwapi)

# TODO: Add tests and install targets if needed.
