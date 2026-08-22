cmake_minimum_required(VERSION 3.25)

if (NOT DEFINED REPOSITORY_ROOT OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "PackageReleaseTest requires REPOSITORY_ROOT and TEST_ROOT")
endif()

set(plugin_dir "${TEST_ROOT}/input/Spectral Relief.vst3")
set(installation_file "${TEST_ROOT}/INSTALLATION.md")
set(output_file "${TEST_ROOT}/Spectral-Relief-v0.1.0-Test.zip")
set(package_root "Spectral-Relief-v0.1.0-Test")

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${plugin_dir}/Contents")
file(WRITE "${plugin_dir}/Contents/fake-binary" "test plug-in payload")
file(WRITE "${installation_file}" "# Test installation guide\n")

execute_process(
    COMMAND ${CMAKE_COMMAND}
        -DREPOSITORY_ROOT=${REPOSITORY_ROOT}
        -DPLUGIN_DIR=${plugin_dir}
        -DINSTALLATION_FILE=${installation_file}
        -DOUTPUT_FILE=${output_file}
        -DPACKAGE_VERSION=v0.1.0
        -DPLATFORM_LABEL=Test
        -P ${REPOSITORY_ROOT}/cmake/PackageRelease.cmake
    RESULT_VARIABLE package_result
    OUTPUT_VARIABLE package_stdout
    ERROR_VARIABLE package_stderr)

if (NOT package_result EQUAL 0)
    message(FATAL_ERROR
        "Release packager failed (${package_result}):\n${package_stdout}\n${package_stderr}")
endif()

if (NOT EXISTS "${output_file}")
    message(FATAL_ERROR "Release packager did not create ${output_file}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar tf "${output_file}"
    RESULT_VARIABLE list_result
    OUTPUT_VARIABLE archive_listing
    ERROR_VARIABLE list_stderr)

if (NOT list_result EQUAL 0)
    message(FATAL_ERROR "Could not list release archive: ${list_stderr}")
endif()

function(require_archive_path relative_path)
    string(FIND "${archive_listing}" "${package_root}/${relative_path}" match_position)
    if (match_position EQUAL -1)
        message(FATAL_ERROR
            "Release archive is missing ${relative_path}. Contents:\n${archive_listing}")
    endif()
endfunction()

require_archive_path("Spectral Relief.vst3/Contents/fake-binary")
require_archive_path("LICENSE")
require_archive_path("THIRD_PARTY_NOTICES.md")
require_archive_path("INSTALLATION.md")
require_archive_path("SOURCE.md")

message(STATUS "Release package contents verified")
