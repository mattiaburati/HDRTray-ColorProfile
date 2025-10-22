configure_file(version.rc.in "${CMAKE_CURRENT_BINARY_DIR}/generated/version.rc")

# Option to embed external tools as resources
option(EMBED_TOOLS "Embed dispwin.exe and winddcutil.exe as resources" OFF)

# Add source to this project's executable.
add_executable(HDRTray)
target_sources(HDRTray PRIVATE
               "HDRTray.h"
               "HDRTray.cpp"
               "HDRTray.manifest"
               "HDRTray.rc"
               "NotifyIcon.hpp"
               "NotifyIcon.cpp"
               "ColorProfileManager.hpp"
               "ColorProfileManager.cpp"
               "ConfigManager.hpp"
               "ConfigManager.cpp"
               )
target_compile_definitions(HDRTray PRIVATE UNICODE _UNICODE)

# Check if tools exist and add EMBED_TOOLS definition if requested
if(EMBED_TOOLS)
    set(DISPWIN_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/tools/dispwin.exe")
    set(WINDDCUTIL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/resources/tools/winddcutil.exe")

    if(EXISTS "${DISPWIN_PATH}" AND EXISTS "${WINDDCUTIL_PATH}")
        target_compile_definitions(HDRTray PRIVATE EMBED_TOOLS)
        message(STATUS "Embedding external tools: dispwin.exe and winddcutil.exe")
    else()
        message(WARNING "EMBED_TOOLS is ON but tool files not found in resources/tools/")
        message(WARNING "  dispwin.exe: ${DISPWIN_PATH}")
        message(WARNING "  winddcutil.exe: ${WINDDCUTIL_PATH}")
        message(WARNING "Build will continue without embedded tools")
    endif()
endif()
target_include_directories(HDRTray PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_link_libraries(HDRTray PRIVATE common Windows10Colors comctl32)
set_target_properties(HDRTray PROPERTIES
                      WIN32_EXECUTABLE ON
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
