cmake_minimum_required(VERSION 3.15)
project(ProyectoGradoClion)

set(CMAKE_CXX_STANDARD 14)
set(FIND_HDF_COMPONENTS C CXX shared)

find_package (HDF5 NAMES "hdf5" COMPONENTS ${FIND_HDF_COMPONENTS})
if (HDF5_FOUND)
    if (HDF5_shared_C_FOUND)
        set (LINK_LIBS ${LINK_LIBS} ${HDF5_C_SHARED_LIBRARY})
    endif ()
    if (HDF5_shared_CXX_FOUND)
        set (LINK_LIBS ${LINK_LIBS} ${HDF5_CXX_SHARED_LIBRARY})
    endif ()
else ()
    message (FATAL_ERROR " HDF5 is Required")
endif ()
INCLUDE_DIRECTORIES (${HDF5_INCLUDE_DIR})

file(GLOB SOURCES
        headers/*.hpp
        sources/*.cpp
        )



add_executable(ProyectoGradoClion ${SOURCES})
target_link_libraries (ProyectoGradoClion PRIVATE ${LINK_LIBS})