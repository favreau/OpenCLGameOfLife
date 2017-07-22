cmake_minimum_required(VERSION 2.6)

set(PROJECT_NAME OpenCLGameOfLife)
project(${PROJECT_NAME})

set(${PROJECT_NAME}_VERSION_MAJOR 0)
set(${PROJECT_NAME}_VERSION_MINOR 1)
set(${PROJECT_NAME}_VERSION_PATCH 0)

set(CPACK_PACKAGE_VERSION_MAJOR "${${PROJECT_NAME}_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${${PROJECT_NAME}_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${${PROJECT_NAME}_VERSION_PATCH}")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

# ================================================================================
# Project
# ================================================================================

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif(NOT CMAKE_BUILD_TYPE)

# Windows' math include does not define constants by default.
# Set this definition so it does.
# Also set NOMINMAX so the min and max functions are not overwritten with macros.
IF(MSVC)
    add_definitions(-D_USE_MATH_DEFINES)
    add_definitions(-DNOMINMAX)
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
ENDIF()

# ================================================================================
# GL
# ================================================================================
find_package(OpenGL REQUIRED SYSTEM)
if (OpenGL_FOUND)
    message(STATUS "OpenGL found")
	include_directories(${OPENGL_INCLUDE_DIR})
else()
	message(ERROR " OpenGL not found!")
endif()

# ================================================================================
# GLUT
# ================================================================================
find_package(FREEGLUT REQUIRED SYSTEM)
if (FREEGLUT_FOUND)
    message(STATUS "FreeGlut found " ${FREEGLUT_LIBRARIES})
	include_directories(${FREEGLUT_INCLUDE_DIR})
else()
	message(ERROR " FreeGlut not found!")
endif()
	
# ================================================================================
# GLEW
# ================================================================================
find_package(GLEW REQUIRED SYSTEM)
if (GLEW_FOUND)
    message(STATUS "Glew found " ${GLEW_LIBRARIES})
	include_directories(${GLEW_INCLUDE_DIR})
else()
	message(ERROR " GLEW not found!")
endif()

# ================================================================================
# OpenCL
# ================================================================================
set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS TRUE)
find_package(OpenCL REQUIRED SYSTEM)
if (OPENCL_FOUND)
	list(APPEND FIND_PACKAGES_DEFINES USE_OPENCL)
	message(STATUS "OpenCL found and selected for build")
	include_directories(${OPENCL_INCLUDE_DIRS})
else()
	message(ERROR " OpenCL not found. Project will not be built with that technology")
endif()	

# ================================================================================
# Project
# ================================================================================
add_subdirectory(gol)
add_subdirectory(apps)
