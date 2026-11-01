cmake_minimum_required(VERSION 3.20)

project(MyFolderMenu VERSION 1.0.0 LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Static CRT on MSVC so the release executable has no Visual C++ runtime DLL
# dependency. The MinGW build uses -static at link time (see below).
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

option(MFM_BUILD_TESTS "Build the automated test suite" ON)

# Keep version.txt as the single source of truth for the binary version.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/version.txt" MFM_VERSION LIMIT_COUNT 1)
string(STRIP "${MFM_VERSION}" MFM_VERSION)
if(NOT MFM_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "version.txt must contain X.Y.Z (found '${MFM_VERSION}')")
endif()
string(REPLACE "." ";" MFM_VERSION_PARTS "${MFM_VERSION}")
list(GET MFM_VERSION_PARTS 0 MFM_VERSION_MAJOR)
list(GET MFM_VERSION_PARTS 1 MFM_VERSION_MINOR)
list(GET MFM_VERSION_PARTS 2 MFM_VERSION_PATCH)
set(MFM_VERSION_COMMA "${MFM_VERSION_MAJOR},${MFM_VERSION_MINOR},${MFM_VERSION_PATCH},0")
set(MFM_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/resources")

configure_file("${MFM_RESOURCES_DIR}/MyFolderMenu.rc.in"
               "${CMAKE_CURRENT_BINARY_DIR}/generated/MyFolderMenu.rc" @ONLY)

set(MFM_CORE_SOURCES
    src/core/strings.cpp
    src/core/paths.cpp
    src/core/config.cpp
    src/core/theme.cpp
    src/core/startup.cpp
    src/core/shell_icons.cpp
    src/core/version.cpp
)

set(MFM_APP_SOURCES
    src/app/app_state.cpp
    src/app/fonts.cpp
    src/app/tray.cpp
    src/app/menu_view.cpp
    src/app/menu_model.cpp
    src/app/actions.cpp
    src/app/main_window.cpp
)

set(MFM_UI_SOURCES
    src/ui/dialog_common.cpp
    src/ui/settings_dialog.cpp
    src/ui/about_dialog.cpp
)

add_library(mfm_lib STATIC
    ${MFM_CORE_SOURCES}
    ${MFM_APP_SOURCES}
    ${MFM_UI_SOURCES}
)

target_include_directories(mfm_lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_compile_definitions(mfm_lib PUBLIC
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    UNICODE
    _UNICODE
    _WIN32_WINNT=0x0A00
    WINVER=0x0A00
)
target_compile_definitions(mfm_lib PUBLIC MFM_VERSION="${MFM_VERSION}")

set(MFM_WIN_LIBS user32 gdi32 shell32 ole32 comdlg32 advapi32 comctl32 uxtheme dwmapi version)
target_link_libraries(mfm_lib PUBLIC ${MFM_WIN_LIBS})

if(MINGW OR CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mfm_lib PRIVATE -Wall -Wextra)
elseif(MSVC)
    target_compile_options(mfm_lib PRIVATE /W4 /permissive- /utf-8)
endif()

add_executable(MyFolderMenu WIN32
    src/main.cpp
    "${CMAKE_CURRENT_BINARY_DIR}/generated/MyFolderMenu.rc"
)
target_link_libraries(MyFolderMenu PRIVATE mfm_lib)

if(MSVC)
    # The manifest is embedded by the resource script; stop the linker from
    # adding a second RT_MANIFEST resource (CVT1100 duplicate resource).
    target_link_options(MyFolderMenu PRIVATE /MANIFEST:NO)
endif()

if(MINGW)
    target_link_options(MyFolderMenu PRIVATE
        -municode
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

if(MFM_BUILD_TESTS)
    enable_testing()

    add_executable(mfm_tests
        tests/test_main.cpp
        tests/test_util.cpp
        tests/test_strings.cpp
        tests/test_paths.cpp
        tests/test_config.cpp
        tests/test_theme.cpp
        tests/test_startup.cpp
        tests/test_version.cpp
        tests/test_menu.cpp
        tests/test_menu_view.cpp
        tests/test_dialog.cpp
    )
    target_link_libraries(mfm_tests PRIVATE mfm_lib)
    target_include_directories(mfm_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")

    if(MSVC)
        # The tests contain accented expected strings; compile them as UTF-8 too.
        target_compile_options(mfm_tests PRIVATE /utf-8)
    endif()

    if(MINGW)
        target_link_options(mfm_tests PRIVATE
            -static
            -static-libgcc
            -static-libstdc++
        )
    endif()

    add_test(NAME mfm_tests COMMAND mfm_tests)
endif()

install(TARGETS MyFolderMenu RUNTIME DESTINATION .)
install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/version.txt"
    DESTINATION .
)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/RELEASE_${MFM_VERSION}.md"
        DESTINATION . OPTIONAL)

set(CPACK_PACKAGE_NAME "MyFolderMenu")
set(CPACK_PACKAGE_VENDOR "adrian.barbosa@gmail.com")
set(CPACK_PACKAGE_VERSION "${MFM_VERSION}")
set(CPACK_PACKAGE_FILE_NAME "MyFolderMenu-${MFM_VERSION}-win64")
set(CPACK_GENERATOR "ZIP")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)
