cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{COPILOT_WORKSPACE_PATH} AND NOT "$ENV{COPILOT_WORKSPACE_PATH}" STREQUAL "")
    set(workspace_path "$ENV{COPILOT_WORKSPACE_PATH}")
else()
    get_filename_component(workspace_path "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(executable_names hikari hikari.exe)
set(output_directories
    "${workspace_path}/build/engine/Release"
    "${workspace_path}/build/engine"
    "${workspace_path}/build/Release"
    "${workspace_path}/build"
)

foreach(output_directory IN LISTS output_directories)
    foreach(executable_name IN LISTS executable_names)
        set(candidate "${output_directory}/${executable_name}")
        if(EXISTS "${candidate}" AND NOT IS_DIRECTORY "${candidate}")
            set(hikari_executable "${candidate}")
            set(hikari_working_directory "${output_directory}")
            break()
        endif()
    endforeach()

    if(DEFINED hikari_executable)
        break()
    endif()
endforeach()

if(NOT DEFINED hikari_executable)
    message(FATAL_ERROR
        "Hikari has not been built. Run the \"Build Hikari\" script first."
    )
endif()

message(STATUS "Launching ${hikari_executable}")
execute_process(
    COMMAND "${hikari_executable}"
    WORKING_DIRECTORY "${hikari_working_directory}"
    RESULT_VARIABLE run_result
)

if(NOT run_result EQUAL 0)
    message(FATAL_ERROR "Hikari exited with result ${run_result}.")
endif()
