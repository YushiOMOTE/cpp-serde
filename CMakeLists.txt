cmake_minimum_required(VERSION 3.11)

project("serde")

set(VERSION 0.1.0)

option(SERDE_BUILD_TESTS "Enable unit tests" OFF)
option(SERDE_BUILD_SAMPLE "Enable sample" ON)
option(SERDE_INSTALL "Enable install" OFF)

if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(SERDE_BUILD_TESTS OFF)
  set(SERDE_BUILD_SAMPLE OFF)
  set(SERDE_INSTALL OFF)
endif()

add_library(serde INTERFACE)
target_include_directories(serde INTERFACE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>)

if (SERDE_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(test)
endif()

if (SERDE_BUILD_SAMPLE)
  add_compile_options(-std=c++17 -Wall -Wextra -pedantic)
  add_executable(serde-sample sample.cpp)
  target_link_libraries(serde-sample yaml-cpp serde)
endif()

if (SERDE_INSTALL)
  include(CMakePackageConfigHelpers)
  set(TARGETS_EXPORT_NAME ${PROJECT_NAME}Targets)
  configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/config.cmake.in"
    "${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION lib/cmake/${PROJECT_NAME})

  install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/${PROJECT_NAME} DESTINATION include)
  install(TARGETS ${PROJECT_NAME} EXPORT ${TARGETS_EXPORT_NAME})
  install(EXPORT ${TARGETS_EXPORT_NAME} FILE ${TARGETS_EXPORT_NAME}.cmake DESTINATION lib/cmake/${PROJECT_NAME})
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake" DESTINATION lib/cmake/${PROJECT_NAME})
endif()
