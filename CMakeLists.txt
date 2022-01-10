cmake_minimum_required (VERSION 3.11)
project(minimocas-tools)

if (EXISTS "$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
  message(STATUS "Enabling physycom settings")
  include("$ENV{WORKSPACE}/sysconfig/cmake/physycom_config.cmake")
else()
  message(STATUS "Unable to find physycom settings file")
endif()
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INSTALL_RPATH ${CMAKE_CURRENT_LIST_DIR}/bin)

if (UNIX AND NOT APPLE)
  find_package(LibRt REQUIRED)
endif()
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(Boost 1.65.0 COMPONENTS system filesystem REQUIRED)
find_package(CURLpp)

if(MSVC)
  add_definitions(-DNOMINMAX)
endif()

if(MSVC)
  #add_definitions(/WX)
else()
  add_definitions(-Wall -Wextra -Wpedantic)
  add_definitions(-Wno-reorder -Wno-implicit-fallthrough -Wno-unused-local-typedefs)
  add_definitions(-Wno-unused-parameter -Wno-unused-function)
  if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9)
    add_definitions(-Wno-maybe-uninitialized)
  endif()
endif()

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/src/
  ${CMAKE_CURRENT_LIST_DIR}/src/app
)

if(UNIX AND NOT APPLE)
  list(APPEND common_libs LIBRT::LIBRT)
endif()

if(UNIX)
  set(APPEND common_libs ${CMAKE_DL_LIBS})
endif()

add_subdirectory(minimocas)

if ( (UNIX OR MSVC) AND NOT APPLE )
  find_package(OpenMP)
  if (OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  endif()
endif()

if(MSVC)
  #add_definitions(/WX)
else()
  add_definitions(-Wall -Wextra -Wpedantic)
  add_definitions(-Wno-reorder -Wno-implicit-fallthrough -Wno-unused-local-typedefs)
  add_definitions(-Wno-unused-parameter -Wno-unused-function)
endif()

find_package(FLTK 1.3.3 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(
  ${CMAKE_CURRENT_LIST_DIR}/minimocas/src/lib
  ${CMAKE_CURRENT_LIST_DIR}/minimocas/src/jsoncons/include
  ${CMAKE_CURRENT_LIST_DIR}/minimocas/src/utils
  ${FLTK_INCLUDE_DIR}
)

add_library(minitest_deps
  ${CMAKE_CURRENT_LIST_DIR}/src/app/minitest_layout.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/app/minitest_scene.cpp
)
add_executable(minitest
  ${CMAKE_CURRENT_LIST_DIR}/src/app/minitest.cpp
)
target_link_libraries(minitest
  minitest_deps
  carto
  ${FLTK_LIBRARIES}
  ${OPENGL_LIBRARIES}
  Threads::Threads
  ${common_libs}
)

add_executable(miniedit
  ${CMAKE_CURRENT_LIST_DIR}/src/app/miniedit.cpp
)
target_link_libraries(miniedit
  carto
  Threads::Threads
  ${common_libs}
)

add_executable(vici
  ${CMAKE_CURRENT_LIST_DIR}/src/app/vici.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/app/vici_layout.cpp
  ${CMAKE_CURRENT_LIST_DIR}/src/app/vici_scene.cpp
)
target_link_libraries(vici
  carto
  simulation_deps
  ${FLTK_LIBRARIES}
  ${OPENGL_LIBRARIES}
  Threads::Threads
  ${common_libs}
)

 #add_executable(vicistats
 #  ${CMAKE_CURRENT_LIST_DIR}/src/app/vicistats.cpp
 #)
 #target_link_libraries(vicistats
 #  carto
 #  simulation_deps
 #  Boost::system
 #  Threads::Threads
 #  ${common_libs}
 #)

#add_executable(miniplex
#  ${CMAKE_CURRENT_LIST_DIR}/src/app/miniplex.cpp
#  ${CMAKE_CURRENT_LIST_DIR}/src/app/miniplex_layout.cpp
#  ${CMAKE_CURRENT_LIST_DIR}/src/app/miniplex_scene.cpp
#)
#target_link_libraries(miniplex
#  carto
#  multiplex_deps
#  Threads::Threads
#  ${FLTK_LIBRARIES}
#  ${OPENGL_LIBRARIES}
#  ${common_libs}
#)

set(WORKSPACE $ENV{WORKSPACE})
string(REPLACE "\\" "/" WORKSPACE ${WORKSPACE})

add_subdirectory(${WORKSPACE}/pybind11 pybind11)
pybind11_add_module(pysim
  ${CMAKE_CURRENT_LIST_DIR}/src/app/simulation_python_interface.cpp
)
target_link_libraries(pysim PRIVATE
  simulation_deps
  Boost::system
  Boost::filesystem
)

install(TARGETS pysim DESTINATION ${CMAKE_CURRENT_LIST_DIR}/bin)
