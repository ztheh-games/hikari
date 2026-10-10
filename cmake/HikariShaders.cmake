set(HIKARI_SHADERCROSS_EXECUTABLE "" CACHE FILEPATH
    "Existing native SDL_shadercross CLI (for example recodr's completed shared cache)")
add_custom_target(regenerate-shaders
    COMMAND "${CMAKE_COMMAND}"
        "-DSHADERCROSS=${HIKARI_SHADERCROSS_EXECUTABLE}"
        "-DSHADER_DIR=${PROJECT_SOURCE_DIR}/content/assets/shaders/gpu"
        -P "${PROJECT_SOURCE_DIR}/cmake/RegenerateShaders.cmake"
    VERBATIM
)
foreach(shader IN ITEMS quad textured palette fade)
    foreach(extension IN ITEMS spv dxil msl json)
        if(NOT EXISTS "${PROJECT_SOURCE_DIR}/content/assets/shaders/gpu/${shader}.${extension}")
            message(FATAL_ERROR "Missing packaged GPU shader: ${shader}.${extension}. Regenerate shaders with the cached host CLI.")
        endif()
    endforeach()
endforeach()
