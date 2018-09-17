add_compile_options(-std=c++17 -Wall -Wextra -pedantic)

include(ExternalProject)

set(PREFIX ${CMAKE_BINARY_DIR}/tmp)
include_directories(include ${PREFIX}/include)

ExternalProject_Add(googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.8.0
  CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${PREFIX}"
  )

set(TESTS
  basics
)

foreach(t ${TESTS})
  set(TEST test-${t})
  add_executable(${TEST} ${TEST}.cpp)
  add_dependencies(${TEST} googletest)
  target_link_libraries(${TEST} serde yaml-cpp ${PREFIX}/lib/libgtest.a ${PREFIX}/lib/libgtest_main.a pthread)
  add_test(NAME ${TEST} COMMAND ${TEST} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endforeach(t)
