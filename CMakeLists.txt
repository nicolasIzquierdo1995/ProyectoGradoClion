cmake_minimum_required(VERSION 3.15)
project(ProyectoGradoClion)

set(CMAKE_CXX_STANDARD 14)

find_package (HDF5 PATHS ${CMAKE_SOURCE_DIR}/libs/hdf5/share/cmake/hdf5 COMPONENTS C CXX static)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs/hdf5/lib)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs/hdf5/lib)

file(GLOB SOURCES
        ${CMAKE_SOURCE_DIR}/headers/*.hpp
        ${CMAKE_SOURCE_DIR}/sources/*.cpp
        )

add_executable(ProyectoGradoClion ${SOURCES})
target_link_libraries (ProyectoGradoClion ${HDF5_CXX_STATIC_LIBRARY})