cmake_minimum_required(VERSION 3.24)
project(DYETechDemo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/DYEngine/cmake/Functions/")
include(RedirectOutputTo)

#include(DYEngine/cmake/Functions/RedirectOutputTo.cmake)
if (CMAKE_BUILD_TYPE MATCHES Debug)
    RedirectOutputTo("build/Debug")
    add_compile_definitions(DYE_DEBUG)
    add_compile_definitions(DYE_OPENGL_DEBUG)
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    RedirectOutputTo("build/Release")
endif ()

add_subdirectory(DYEngine/DYEngine)
add_subdirectory(DYEngine/DYEntity)
add_subdirectory(DYEngine/DYEditor)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/appicon.rc")
set(SOURCE_FILES
        src/DYETechDemoApp.cpp
        src/ColliderManager.cpp
        src/GizmosRippleEffectManager.cpp
        src/WindowParticlesManager.cpp
        src/Layers/MainMenuLayer.cpp
        src/Layers/CollisionTestLayer.cpp
        src/Layers/PongLayer.cpp
        src/Layers/LandTheBallLayer.cpp
        src/Components/Transform.cpp
        src/Components/Sprite.cpp
        src/Components/Collider.cpp
        src/Components/Velocity.cpp
        src/Components/Hittable.cpp
        src/Components/AttachableToPaddle.cpp
        src/Objects/PlayerPaddle.cpp
        src/Objects/Camera.cpp
        src/Objects/PongPlayer.cpp
        src/Objects/PongHomebase.cpp
        src/Objects/WindowCamera.cpp
        src/Objects/SpriteButton.cpp
        src/Objects/SpriteUnsignedNumber.cpp
        src/Objects/Wall.cpp
        src/Objects/PongBall.cpp
        src/Objects/LandBall.cpp)

set(HEADER_FILES
        src/DYETechDemoApp.h
        src/ColliderManager.h
        src/GizmosRippleEffectManager.h
        src/WindowParticlesManager.h
        src/Layers/MainMenuLayer.h
        src/Layers/CollisionTestLayer.h
        src/Layers/PongLayer.h
        src/Layers/LandTheBallLayer.h
        src/Components/Transform.h
        src/Components/Sprite.h
        src/Components/Collider.h
        src/Components/Velocity.h
        src/Components/Hittable.h
        src/Components/AttachableToPaddle.h
        src/Objects/PlayerPaddle.h
        src/Objects/Camera.h
        src/Objects/PongPlayer.h
        src/Objects/PongHomebase.h
        src/Objects/WindowCamera.h
        src/Objects/SpriteButton.h
        src/Objects/SpriteUnsignedNumber.h
        src/Objects/Wall.h
        src/Objects/PongBall.h
        src/Objects/LandBall.h)

add_executable(DYETechDemo ${SOURCE_FILES} ${HEADER_FILES} ${APP_ICON_RESOURCE_WINDOWS})

target_include_directories(DYETechDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(DYETechDemo DYEngine)
target_link_libraries(DYETechDemo DYEntity)
target_link_libraries(DYETechDemo DYEditor)

# Link libstdc++ statically to the executable
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lwinpthread -Wl,--no-whole-archive")
target_link_libraries(DYETechDemo -static-libgcc)

# Copy assets to the output directory
set(BINARY_ASSETS
        assets/Sprite_Grid.png

        assets/Sprite_DYELogo.png

        assets/Sprite_RulePong.png
        assets/Sprite_RuleLandTheBall.png
        assets/Sprite_SubtitleExit.png

        assets/Sprite_ButtonLandTB_Selected.png
        assets/Sprite_ButtonLandTB_Deselected.png
        assets/Sprite_ButtonPong_Selected.png
        assets/Sprite_ButtonPong_Deselected.png
        assets/Sprite_ButtonExit_Selected.png
        assets/Sprite_ButtonExit_Deselected.png
        assets/Sprite_MenuButtonPrompt.png

        assets/Sprite_Digit_0.png
        assets/Sprite_Digit_1.png
        assets/Sprite_Digit_2.png
        assets/Sprite_Digit_3.png
        assets/Sprite_Digit_4.png
        assets/Sprite_Digit_5.png
        assets/Sprite_Digit_6.png
        assets/Sprite_Digit_7.png
        assets/Sprite_Digit_8.png
        assets/Sprite_Digit_9.png

        assets/Sprite_GameOver.png
        assets/Sprite_PressToDropBall.png
        assets/Sprite_PreviousHighScoreText.png
        assets/Sprite_NewHighScoreText.png
        assets/Sprite_HighScoreText.png

        assets/Sprite_Pong.png
        assets/Sprite_PongBorder.png
        assets/Sprite_DottedLine.png
        assets/Sprite_PongPaddle.png
        assets/Sprite_P1Wins.png
        assets/Sprite_P2Wins.png)

include(DYEngine/cmake/Functions/CopyAssets.cmake)
CopyBinaryAssets(${BINARY_ASSETS})