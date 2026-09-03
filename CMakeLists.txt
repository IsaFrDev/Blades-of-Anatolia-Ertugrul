cmake_minimum_required(VERSION 3.15)
project(ErtugrulGame CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

if(MSVC)
    add_compile_options(/W3 /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# game/legacy/ ataylab chetda qoldirilgan (eski prototip)
set(ERT_SOURCES
    game/src/core/Math.cpp
    game/src/gfx/Texture.cpp
    game/src/gfx/Font.cpp
    game/src/gfx/Mesh.cpp
    game/src/gfx/Skin.cpp
    game/src/gfx/ShadowMap.cpp
    game/src/gfx/Pbr.cpp
    game/src/audio/Audio.cpp
    game/src/audio/Sfx.cpp
    game/src/audio/Voice.cpp
    game/src/loc/Loc.cpp
    game/src/game/Episodes.cpp
    game/src/game/Cutscene.cpp
    game/src/game/Character.cpp
    game/src/game/Encounter.cpp
    game/src/game/Enemy.cpp
    game/src/game/Combat.cpp
    game/src/game/Projectile.cpp
    game/src/world/Terrain.cpp
    game/src/world/Level.cpp
    game/src/world/Physics.cpp
    game/src/ui/Menu.cpp
    game/src/app/Input.cpp
    game/src/app/Bindings.cpp
    game/src/app/App.cpp
    game/src/app/Win32Main.cpp
)

add_executable(ertugrul ${ERT_SOURCES})
target_include_directories(ertugrul PRIVATE game/include)

if(WIN32)
    target_link_libraries(ertugrul PRIVATE
        opengl32 glu32          # 3D
        gdi32 gdiplus           # shrift atlasi + JPEG/PNG dekodlash
        winmm                   # waveOut ovoz
        ole32 oleaut32 uuid     # SAPI (TTS) uchun COM
        user32 shell32
    )
    # STATIK runtime. Aks holda exe libstdc++-6.dll / libgcc_s_seh-1.dll ni PATH dan
    # qidiradi va Git Bash ostida C:/Program Files/Git/mingw64/bin dagi BOSHQA
    # versiyani yuklab, App::loadConfig ichida (std::string ABI) segfault berardi.
    # Endi exe hech qanday MinGW DLL siz ishlaydi — Git Bash, PowerShell, Explorer.
    target_link_options(ertugrul PRIVATE -static -static-libgcc -static-libstdc++)
else()
    message(FATAL_ERROR "Bu qurilish faqat Windows uchun (Win32 + WGL).")
endif()

# Ishga tushirish papkasi loyiha ildizi bo'lishi kerak (data/, assets/, localization/ shu yerda)
set_target_properties(ertugrul PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build"
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
