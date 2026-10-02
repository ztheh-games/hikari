set( CMAKE_INCLUDE_CURRENT_DIR ON )

set( TEST_BASE_DIR "${PROJECT_SOURCE_DIR}/tests" )
set( ENGINE_BASE_DIR "${PROJECT_SOURCE_DIR}/engine" )

set( INCLUDE_DIRS
    ${TEST_BASE_DIR}/include
    ${ENGINE_BASE_DIR}/include
)

set( REQUIRED_HIKARI_SOURCE_FILES
    ${ENGINE_BASE_DIR}/src/hikari/client/game/events/EventBus.cpp
    ${ENGINE_BASE_DIR}/src/hikari/client/game/events/EventBusImpl.cpp
    ${ENGINE_BASE_DIR}/src/hikari/client/game/events/EventListenerDelegate.cpp
    ${ENGINE_BASE_DIR}/src/hikari/client/game/events/EventData.cpp
    ${ENGINE_BASE_DIR}/src/hikari/client/game/events/BaseEventData.cpp
    ${ENGINE_BASE_DIR}/src/hikari/core/game/CollisionInfo.cpp
    ${ENGINE_BASE_DIR}/src/hikari/core/game/Direction.cpp
    ${ENGINE_BASE_DIR}/src/hikari/core/game/Movable.cpp
    ${ENGINE_BASE_DIR}/src/hikari/core/util/Log.cpp
)

set( TEST_SOURCE_FILES
    ${REQUIRED_HIKARI_SOURCE_FILES}
    src/test/Main.cpp
    src/test/TestVector2.cpp
    src/test/TestPoint2D.cpp
    src/test/TestRectangle2D.cpp
    src/test/TestBoundingBox.cpp
    src/test/TestGeometryUtils.cpp
    src/test/TestEventBusImpl.cpp
    src/test/TestCollisionSelection.cpp
    src/test/TestMovable.cpp
)

include_directories( ${INCLUDE_DIRS} )

add_executable( tests ${TEST_SOURCE_FILES} ${INCLUDE_DIRS} )

add_executable(runtime-tests
    src/test/Main.cpp
    src/test/TestClientRuntime.cpp
    src/test/TestGraphics.cpp
    src/test/TestAudio.cpp
)
target_link_libraries(runtime-tests PRIVATE hikari-runtime)
target_include_directories(runtime-tests PRIVATE ${TEST_BASE_DIR}/include)
target_compile_definitions(runtime-tests PRIVATE
    HIKARI_TEST_CONTENT_DIR="${PROJECT_SOURCE_DIR}/content"
)

add_executable(gpu-tests src/test/Main.cpp src/test/TestGPU.cpp)
target_link_libraries(gpu-tests PRIVATE hikari-runtime)
target_include_directories(gpu-tests PRIVATE ${TEST_BASE_DIR}/include)
target_compile_definitions(gpu-tests PRIVATE HIKARI_TEST_CONTENT_DIR="${PROJECT_SOURCE_DIR}/content")
add_test(NAME core COMMAND tests)
add_test(NAME runtime COMMAND runtime-tests)
set_tests_properties(runtime PROPERTIES ENVIRONMENT "SDL_AUDIO_DRIVER=dummy")
add_test(NAME gpu COMMAND gpu-tests)
set_tests_properties(gpu PROPERTIES LABELS gpu ENVIRONMENT "SDL_AUDIO_DRIVER=dummy")
