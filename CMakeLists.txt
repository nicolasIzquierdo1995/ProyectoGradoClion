cmake_minimum_required(VERSION 3.15)
project(ProyectoGradoClion)

set(CMAKE_CXX_STANDARD 14)
set(FIND_HDF_COMPONENTS C CXX shared)
set(GCC_COVERAGE_COMPILE_FLAGS "--coverage")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS} " )
set(CMAKE_EXE_LINKER_FLAGS "${GCC_COVERAGE_COMPILE_FLAGS}" )

find_package (HDF5 PATHS ${CMAKE_SOURCE_DIR}/libs/hdf5/share/cmake/hdf5 COMPONENTS ${FIND_HDF_COMPONENTS})
set (LINK_LIBS ${LINK_LIBS} ${HDF5_CXX_SHARED_LIBRARY})
find_package(Boost COMPONENTS system filesystem REQUIRED)

find_package(Threads)


INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs/hdf5/include)
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/libs/hdf5)
file(GLOB SOURCES
        ${CMAKE_SOURCE_DIR}/headers/*.hpp
        ${CMAKE_SOURCE_DIR}/sources/*.cpp
        )



add_executable(ProyectoGradoClion ${SOURCES})
target_link_libraries (ProyectoGradoClion PRIVATE ${LINK_LIBS} ${CMAKE_THREAD_LIBS_INIT} ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_SYSTEM_LIBRARY})