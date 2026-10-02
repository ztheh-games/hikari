if(NOT DEFINED SHADERCROSS OR NOT EXISTS "${SHADERCROSS}")
    message(FATAL_ERROR
        "Select an existing native shadercross CLI with HIKARI_SHADERCROSS_EXECUTABLE. "
        "Use recodr's completed host cache or an installed compiler. No compiler bootstrap is attempted.")
endif()
if(NOT DEFINED SHADER_DIR)
    message(FATAL_ERROR "SHADER_DIR is required.")
endif()
foreach(shader IN ITEMS quad textured palette fade)
    if(shader STREQUAL "quad")
        set(stage vertex)
    else()
        set(stage fragment)
    endif()
    foreach(format IN ITEMS SPIRV DXIL MSL)
        if(format STREQUAL "SPIRV")
            set(extension spv)
        else()
            string(TOLOWER "${format}" extension)
        endif()
        execute_process(
            COMMAND "${SHADERCROSS}" "${SHADER_DIR}/${shader}.hlsl"
                --source HLSL --dest "${format}" --stage "${stage}" --entrypoint main
                --output "${SHADER_DIR}/${shader}.${extension}"
            RESULT_VARIABLE result
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Shader compilation failed: ${shader}/${format} (${result}).")
        endif()
    endforeach()
    execute_process(
        COMMAND "${SHADERCROSS}" "${SHADER_DIR}/${shader}.spv"
            --source SPIRV --dest JSON --stage "${stage}" --entrypoint main
            --output "${SHADER_DIR}/${shader}.json"
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "Shader reflection failed: ${shader} (${result}).")
    endif()
endforeach()
