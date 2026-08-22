cmake_minimum_required(VERSION 3.25)

foreach(required_variable
        REPOSITORY_ROOT
        PLUGIN_DIR
        INSTALLATION_FILE
        OUTPUT_FILE
        PACKAGE_VERSION
        PLATFORM_LABEL)
    if (NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "PackageRelease.cmake requires ${required_variable}")
    endif()
endforeach()

if (NOT IS_DIRECTORY "${PLUGIN_DIR}")
    message(FATAL_ERROR "VST3 bundle does not exist: ${PLUGIN_DIR}")
endif()

get_filename_component(plugin_name "${PLUGIN_DIR}" NAME)
if (NOT plugin_name STREQUAL "Spectral Relief.vst3")
    message(FATAL_ERROR
        "Expected a bundle named 'Spectral Relief.vst3', got '${plugin_name}'")
endif()

foreach(required_file LICENSE THIRD_PARTY_NOTICES.md)
    if (NOT EXISTS "${REPOSITORY_ROOT}/${required_file}")
        message(FATAL_ERROR "Required public file does not exist: ${required_file}")
    endif()
endforeach()

if (NOT EXISTS "${INSTALLATION_FILE}")
    message(FATAL_ERROR "Installation guide does not exist: ${INSTALLATION_FILE}")
endif()

foreach(label PACKAGE_VERSION PLATFORM_LABEL)
    if (NOT "${${label}}" MATCHES "^[A-Za-z0-9][A-Za-z0-9._-]*$")
        message(FATAL_ERROR "${label} contains unsafe archive-name characters")
    endif()
endforeach()

get_filename_component(output_directory "${OUTPUT_FILE}" DIRECTORY)
if (output_directory STREQUAL "")
    set(output_directory "${CMAKE_CURRENT_BINARY_DIR}")
endif()

set(package_name "Spectral-Relief-${PACKAGE_VERSION}-${PLATFORM_LABEL}")
set(staging_root "${output_directory}/.spectral-relief-package-stage")
set(package_directory "${staging_root}/${package_name}")

file(REMOVE_RECURSE "${staging_root}")
file(MAKE_DIRECTORY "${package_directory}")
file(COPY "${PLUGIN_DIR}" DESTINATION "${package_directory}")
configure_file("${REPOSITORY_ROOT}/LICENSE" "${package_directory}/LICENSE" COPYONLY)
configure_file(
    "${REPOSITORY_ROOT}/THIRD_PARTY_NOTICES.md"
    "${package_directory}/THIRD_PARTY_NOTICES.md"
    COPYONLY)
configure_file("${INSTALLATION_FILE}" "${package_directory}/INSTALLATION.md" COPYONLY)

file(WRITE "${package_directory}/SOURCE.md"
    "# Corresponding source code\n\n"
    "Spectral Relief ${PACKAGE_VERSION} is free software licensed under "
    "AGPL-3.0-only.\n\n"
    "The complete corresponding source for this binary release is available at:\n\n"
    "https://github.com/nijatburjiyev/spectral-relief/tree/${PACKAGE_VERSION}\n")

file(MAKE_DIRECTORY "${output_directory}")
file(REMOVE "${OUTPUT_FILE}")
execute_process(
    COMMAND ${CMAKE_COMMAND}
        "-DSTAGING_ROOT=${staging_root}"
        "-DPACKAGE_NAME=${package_name}"
        "-DOUTPUT_FILE=${OUTPUT_FILE}"
        -P "${CMAKE_CURRENT_LIST_DIR}/CreateArchive.cmake"
    WORKING_DIRECTORY "${staging_root}"
    RESULT_VARIABLE archive_result
    OUTPUT_VARIABLE archive_stdout
    ERROR_VARIABLE archive_stderr)

if (NOT archive_result EQUAL 0)
    message(FATAL_ERROR
        "Could not create release archive (${archive_result}):\n"
        "${archive_stdout}\n${archive_stderr}")
endif()

file(REMOVE_RECURSE "${staging_root}")
message(STATUS "Created release archive: ${OUTPUT_FILE}")
