cmake_minimum_required(VERSION 3.25)

foreach(required_variable STAGING_ROOT PACKAGE_NAME OUTPUT_FILE)
    if (NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "CreateArchive.cmake requires ${required_variable}")
    endif()
endforeach()

if (NOT PACKAGE_NAME MATCHES "^[A-Za-z0-9][A-Za-z0-9._-]*$")
    message(FATAL_ERROR "PACKAGE_NAME contains unsafe archive-name characters")
endif()

if (NOT IS_DIRECTORY "${STAGING_ROOT}/${PACKAGE_NAME}")
    message(FATAL_ERROR "Package staging directory does not exist")
endif()

file(GLOB_RECURSE archive_paths
    LIST_DIRECTORIES true
    RELATIVE "${STAGING_ROOT}"
    "${STAGING_ROOT}/${PACKAGE_NAME}/*")
list(SORT archive_paths)
list(PREPEND archive_paths "${PACKAGE_NAME}")

file(ARCHIVE_CREATE
    OUTPUT "${OUTPUT_FILE}"
    PATHS ${archive_paths}
    FORMAT zip
    MTIME "2026-01-01 00:00:00 UTC")
