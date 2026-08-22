cmake_minimum_required(VERSION 3.25)

if ("$ENV{SR_REPOSITORY_ROOT}" STREQUAL "" OR "$ENV{SR_RELEASE_TAG}" STREQUAL "")
    message(FATAL_ERROR
        "ValidateReleaseVersion.cmake requires SR_REPOSITORY_ROOT and SR_RELEASE_TAG")
endif()

set(repository_root "$ENV{SR_REPOSITORY_ROOT}")
set(release_tag "$ENV{SR_RELEASE_TAG}")

if (NOT release_tag MATCHES "^v[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR
        "Release tag '${release_tag}' must use strict vMAJOR.MINOR.PATCH syntax")
endif()

file(READ "${repository_root}/CMakeLists.txt" cmake_contents)
string(REGEX MATCH
    "project\\([ \t\r\n]*SpectralRelief[ \t\r\n]+VERSION[ \t\r\n]+([0-9]+\\.[0-9]+\\.[0-9]+)"
    project_declaration
    "${cmake_contents}")

if (project_declaration STREQUAL "")
    message(FATAL_ERROR "Could not read the SpectralRelief project version")
endif()

set(expected_tag "v${CMAKE_MATCH_1}")
if (NOT release_tag STREQUAL expected_tag)
    message(FATAL_ERROR
        "Release tag '${release_tag}' does not match project version '${expected_tag}'")
endif()

message(STATUS "Release tag ${release_tag} matches the project version")
