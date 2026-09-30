cmake_minimum_required(VERSION 3.20)

if(DEFINED ENV{COPILOT_WORKSPACE_PATH} AND NOT "$ENV{COPILOT_WORKSPACE_PATH}" STREQUAL "")
    set(workspace_path "$ENV{COPILOT_WORKSPACE_PATH}")
else()
    get_filename_component(workspace_path "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
endif()

set(build_path "${workspace_path}/build")

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        -S "${workspace_path}"
        -B "${build_path}"
        -D CMAKE_BUILD_TYPE=Release
    RESULT_VARIABLE configure_result
)

if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Hikari configuration failed with exit code ${configure_result}.")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        --build "${build_path}"
        --config Release
    RESULT_VARIABLE build_result
)

if(NOT build_result EQUAL 0)
    message(FATAL_ERROR "Hikari build failed with exit code ${build_result}.")
endif()
