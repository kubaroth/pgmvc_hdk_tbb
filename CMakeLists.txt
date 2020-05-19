cmake_minimum_required(VERSION 3.13.0)

project(pgmvc)

set(HOUDINI_VERSION_MAJOR 17)
set(HOUDINI_VERSION_MINOR 5)
set(HOUDINI_VERSION_PATCH 425)

message ("HOUDINI VERSION: " $ENV{HOUDINI_VERSION_MAJOR}.$ENV{HOUDINI_VERSION_MINOR}.$ENV{HOUDINI_VERSION_PATCH})
set (HOUDINI_VERSION $ENV{HOUDINI_VERSION_MAJOR}.$ENV{HOUDINI_VERSION_MINOR}.$ENV{HOUDINI_VERSION_PATCH} )



add_definitions(
       -DVERSION=\"${HOUDINI_VERSION}\"
       -DHOUDINI_VERSION=${HOUDINI_VERSION}
       -DHOUDINI_MAJOR_RELEASE=${HOUDINI_VERSION_MAJOR}
       -DHOUDINI_MINOR_RELEASE=${HOUDINI_VERSION_MINOR}
       -DHOUDINI_BUILD_VERSION=${HOUDINI_VERSION_PATCH}
       -D_GNU_SOURCE
       -DLINUX
       -DAMD64
       -DSIZEOF_VOID_P=8
       -DFBX_ENABLED=1
       -DOPENCL_ENABLED=1
       -DOPENVDB_ENABLED=1
       -DSESI_LITTLE_ENDIAN
       -DENABLE_THREADS
       -DUSE_PTHREADS
       -DMAKING_DSO
       -D_REENTRANT
       -D_FILE_OFFSET_BITS=64
       -DGCC4
       -DGCC3
       )

set(HOUDINI_LIBRARIES
       HoudiniUI
       HoudiniUT
       HoudiniOPZ
       HoudiniOP1
       HoudiniOP2
       HoudiniOP3
       HoudiniSIM
       HoudiniGEO
       HoudiniPRM
       HoudiniRAY
       hboost_system
       tbb
       
       # GLU 
       # GL 
       # X11 
       # Xext 
       # Xi 
       # dl 
       pthread
       )

set(CMAKE_CXX_FLAGS "-fopenmp")
     
add_library(${PROJECT_NAME}_capture_all SHARED
  capture_pgmvc.cpp
  )

add_library(${PROJECT_NAME}_deform_all SHARED
  deform_pgmvc.cpp
  )


target_include_directories(${PROJECT_NAME}_capture_all BEFORE PUBLIC $ENV{HFS}/toolkit/include/)
target_include_directories(${PROJECT_NAME}_deform_all BEFORE PUBLIC $ENV{HFS}/toolkit/include/)

target_link_directories(${PROJECT_NAME}_capture_all BEFORE PUBLIC $ENV{HFS}/dsolib)
target_link_directories(${PROJECT_NAME}_deform_all BEFORE PUBLIC $ENV{HFS}/dsolib)

target_link_libraries(${PROJECT_NAME}_capture_all ${HOUDINI_LIBRARIES})
target_link_libraries(${PROJECT_NAME}_deform_all ${HOUDINI_LIBRARIES})
