cmake_minimum_required(VERSION 3.25)

if (NOT DEFINED REPOSITORY_ROOT OR NOT DEFINED TEST_ROOT)
    message(FATAL_ERROR "PackageReleaseTest requires REPOSITORY_ROOT and TEST_ROOT")
endif()

set(plugin_dir "${TEST_ROOT}/input/Spectral Relief.vst3")
set(installation_file "${TEST_ROOT}/INSTALLATION.md")
set(output_directory "${TEST_ROOT}/output")
set(output_file "${output_directory}/Spectral-Relief-v0.1.0-Test.zip")
set(package_root "Spectral-Relief-v0.1.0-Test")

file(REMOVE_RECURSE "${TEST_ROOT}")
file(MAKE_DIRECTORY "${plugin_dir}/Contents")
file(WRITE "${plugin_dir}/Contents/fake-binary" "test plug-in payload")
file(WRITE "${installation_file}" "# Test installation guide\n")

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
        "SR_REPOSITORY_ROOT=${REPOSITORY_ROOT}"
        "SR_PLUGIN_DIR=${plugin_dir}"
        "SR_INSTALLATION_FILE=${installation_file}"
        "SR_OUTPUT_DIRECTORY=${output_directory}"
        "SR_PACKAGE_VERSION=v0.1.0"
        "SR_PLATFORM_LABEL=Test"
        ${CMAKE_COMMAND} -P ${REPOSITORY_ROOT}/cmake/PackageReleaseFromEnvironment.cmake
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

file(SHA256 "${output_file}" first_archive_hash)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E env
        "SR_REPOSITORY_ROOT=${REPOSITORY_ROOT}"
        "SR_PLUGIN_DIR=${plugin_dir}"
        "SR_INSTALLATION_FILE=${installation_file}"
        "SR_OUTPUT_DIRECTORY=${output_directory}"
        "SR_PACKAGE_VERSION=v0.1.0"
        "SR_PLATFORM_LABEL=Test"
        ${CMAKE_COMMAND} -P ${REPOSITORY_ROOT}/cmake/PackageReleaseFromEnvironment.cmake
    RESULT_VARIABLE second_package_result
    OUTPUT_VARIABLE second_package_stdout
    ERROR_VARIABLE second_package_stderr)

if (NOT second_package_result EQUAL 0)
    message(FATAL_ERROR
        "Second release package failed (${second_package_result}):\n"
        "${second_package_stdout}\n${second_package_stderr}")
endif()

file(SHA256 "${output_file}" second_archive_hash)
if (NOT first_archive_hash STREQUAL second_archive_hash)
    message(FATAL_ERROR
        "Identical package inputs produced different archives: "
        "${first_archive_hash} != ${second_archive_hash}")
endif()

execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar tf "${output_file}"
    RESULT_VARIABLE list_result
    OUTPUT_VARIABLE archive_listing
    ERROR_VARIABLE list_stderr)

if (NOT list_result EQUAL 0)
    message(FATAL_ERROR "Could not list release archive: ${list_stderr}")
endif()

string(REPLACE "\r\n" "\n" normalized_listing "${archive_listing}")
string(REPLACE "\n" ";" archive_entries "${normalized_listing}")
list(FILTER archive_entries EXCLUDE REGEX "^$")
set(unique_archive_entries ${archive_entries})
list(REMOVE_DUPLICATES unique_archive_entries)
list(LENGTH archive_entries archive_entry_count)
list(LENGTH unique_archive_entries unique_archive_entry_count)
if (NOT archive_entry_count EQUAL unique_archive_entry_count)
    message(FATAL_ERROR "Release archive contains duplicate entries")
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

set(extract_directory "${TEST_ROOT}/extracted")
file(MAKE_DIRECTORY "${extract_directory}")
file(ARCHIVE_EXTRACT INPUT "${output_file}" DESTINATION "${extract_directory}")
file(READ "${extract_directory}/${package_root}/SOURCE.md" source_contents)
string(FIND
    "${source_contents}"
    "https://github.com/nijatburjiyev/spectral-relief/tree/v0.1.0"
    source_url_position)
if (source_url_position EQUAL -1)
    message(FATAL_ERROR "SOURCE.md does not link to the exact release tag")
endif()

message(STATUS "Release package contents verified")
