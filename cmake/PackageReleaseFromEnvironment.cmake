cmake_minimum_required(VERSION 3.25)

foreach(required_environment_variable
        SR_REPOSITORY_ROOT
        SR_PLUGIN_DIR
        SR_INSTALLATION_FILE
        SR_OUTPUT_DIRECTORY
        SR_PACKAGE_VERSION
        SR_PLATFORM_LABEL)
    if ("$ENV{${required_environment_variable}}" STREQUAL "")
        message(FATAL_ERROR
            "PackageReleaseFromEnvironment.cmake requires ${required_environment_variable}")
    endif()
endforeach()

set(PACKAGE_VERSION "$ENV{SR_PACKAGE_VERSION}")
set(PLATFORM_LABEL "$ENV{SR_PLATFORM_LABEL}")

foreach(label PACKAGE_VERSION PLATFORM_LABEL)
    if (NOT "${${label}}" MATCHES "^[A-Za-z0-9][A-Za-z0-9._-]*$")
        message(FATAL_ERROR "${label} contains unsafe archive-name characters")
    endif()
endforeach()

set(output_directory "$ENV{SR_OUTPUT_DIRECTORY}")
if (NOT IS_ABSOLUTE "${output_directory}")
    message(FATAL_ERROR "SR_OUTPUT_DIRECTORY must be an absolute path")
endif()

set(REPOSITORY_ROOT "$ENV{SR_REPOSITORY_ROOT}")
set(PLUGIN_DIR "$ENV{SR_PLUGIN_DIR}")
set(INSTALLATION_FILE "$ENV{SR_INSTALLATION_FILE}")
set(OUTPUT_FILE
    "${output_directory}/Spectral-Relief-${PACKAGE_VERSION}-${PLATFORM_LABEL}.zip")

include("${CMAKE_CURRENT_LIST_DIR}/PackageRelease.cmake")
