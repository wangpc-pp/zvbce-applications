cmake_minimum_required(VERSION 3.20)

project(zvbext_applications LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(ZVBEXT_APPLICATIONS_STATIC_LINK
       "Link the RISC-V test executables statically" ON)

function(add_application_runner target implementation march)
  add_executable(${target} run.c ${implementation})
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_compile_options(
    ${target}
    PRIVATE
      -march=${march}
      -menable-experimental-extensions
      -Wall
      -Wextra
      -Werror)
  target_link_options(
    ${target}
    PRIVATE
      -march=${march}
      -menable-experimental-extensions)

  if(ZVBEXT_APPLICATIONS_STATIC_LINK)
    target_link_options(${target} PRIVATE -static)
  endif()
endfunction()

add_application_runner(run-scalar reference.c rv64gc)
add_application_runner(run-rvv rvv_without_zvbext.c rv64gcv)
add_application_runner(run-zvbext rvv_with_zvbext.c rv64gcv_zvbext0p1)

add_custom_target(
  run
  COMMAND
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run-qemu.sh"
    "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS run-scalar run-rvv run-zvbext
  USES_TERMINAL)

add_custom_target(
  instruction-count
  COMMAND
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/count-instructions.sh"
    "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS run-scalar run-rvv run-zvbext
  USES_TERMINAL)
