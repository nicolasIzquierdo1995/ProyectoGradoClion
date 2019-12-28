cmake_minimum_required(VERSION 3.15)
project(ProyectoGradoClion)

set(CMAKE_CXX_STANDARD 14)
set(FIND_HDF_COMPONENTS C CXX shared)
set (LINK_LIBS ${LINK_LIBS} ${CMAKE_SOURCE_DIR}/libs)


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs/hdf5/include)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs/hdf5/lib)
file(GLOB SOURCES
        headers/*.hpp
        sources/*.cpp
        )



add_executable(ProyectoGradoClion ${SOURCES})
target_link_libraries (ProyectoGradoClion PRIVATE ${LINK_LIBS})