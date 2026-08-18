cmake_minimum_required(VERSION 3.20)

project(zvbext_applications LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ZVBEXT_APPLICATIONS_STATIC_LINK
       "Link the RISC-V test executables statically" ON)

function(configure_riscv_target target march)
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

function(add_application_runner target implementation march)
  add_executable(${target} run.c ${implementation})
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  configure_riscv_target(${target} ${march})
endfunction()

add_application_runner(run-scalar reference.c rv64gc)
add_application_runner(run-rvv rvv_without_zvbext.c rv64gcv)
add_application_runner(run-zvbext rvv_with_zvbext.c rv64gcv_zvbext0p1)

add_library(
  p3104-common OBJECT
  p3104/examples.cpp
  p3104/batch.hpp
  p3104/bit_permutations.hpp)
add_library(p3104-portable-backend OBJECT p3104/batch_portable.cpp)
add_library(p3104-zvbext-backend OBJECT p3104/batch_zvbext.cpp)
foreach(target p3104-common p3104-portable-backend p3104-zvbext-backend)
  target_include_directories(
    ${target}
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/p3104")
endforeach()
configure_riscv_target(p3104-common rv64gc)
configure_riscv_target(p3104-portable-backend rv64gc)
configure_riscv_target(p3104-zvbext-backend rv64gcv_zvbext0p1)

add_executable(
  run-p3104
  $<TARGET_OBJECTS:p3104-common>
  $<TARGET_OBJECTS:p3104-portable-backend>)
add_executable(
  run-p3104-zvbext
  $<TARGET_OBJECTS:p3104-common>
  $<TARGET_OBJECTS:p3104-zvbext-backend>)
configure_riscv_target(run-p3104 rv64gc)
configure_riscv_target(run-p3104-zvbext rv64gcv_zvbext0p1)

add_custom_target(
  run
  COMMAND
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/run-qemu.sh"
    "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS
    run-scalar
    run-rvv
    run-zvbext
    run-p3104
    run-p3104-zvbext
  USES_TERMINAL)

add_custom_target(
  instruction-count
  COMMAND
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/count-instructions.sh"
    "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS
    run-scalar
    run-rvv
    run-zvbext
    run-p3104
    run-p3104-zvbext
  USES_TERMINAL)
