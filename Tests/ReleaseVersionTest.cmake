cmake_minimum_required(VERSION 3.25)

if (NOT DEFINED REPOSITORY_ROOT)
    message(FATAL_ERROR "ReleaseVersionTest requires REPOSITORY_ROOT")
endif()

function(run_version_check release_tag expected_result)
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E env
            "SR_REPOSITORY_ROOT=${REPOSITORY_ROOT}"
            "SR_RELEASE_TAG=${release_tag}"
            ${CMAKE_COMMAND} -P ${REPOSITORY_ROOT}/cmake/ValidateReleaseVersion.cmake
        RESULT_VARIABLE actual_result
        OUTPUT_VARIABLE check_stdout
        ERROR_VARIABLE check_stderr)

    if (expected_result STREQUAL "success" AND NOT actual_result EQUAL 0)
        message(FATAL_ERROR
            "Expected ${release_tag} to pass release validation:\n"
            "${check_stdout}\n${check_stderr}")
    endif()

    if (expected_result STREQUAL "failure" AND actual_result EQUAL 0)
        message(FATAL_ERROR "Expected ${release_tag} to fail release validation")
    endif()
endfunction()

run_version_check("v0.1.0" success)
run_version_check("v0.2.0" failure)
run_version_check("v0.1.0-preview" failure)
run_version_check("not-a-version" failure)

message(STATUS "Release tag validation verified")
