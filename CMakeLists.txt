cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
set(PROJECT u3d_module)
project(${PROJECT} LANGUAGES CXX)

if(NOT BUILD_NUMBER)
  message(SEND_ERROR "Please set BUILD_NUMBER")
endif()

#################################################################################
# Sources
#################################################################################
set(SOURCES
  src/u3d_robot_module.cpp
)

add_library(${PROJECT} SHARED ${SOURCES})

#################################################################################
# Properties
#################################################################################

target_compile_definitions(${PROJECT} PRIVATE
  BUILD_NUMBER=${BUILD_NUMBER}
)

set_property(TARGET ${PROJECT} PROPERTY CXX_STANDARD 11)

target_compile_options(${PROJECT} PRIVATE
  -Wall -Werror
  -pedantic
  -fno-strict-aliasing
  -Wno-write-strings
)

#if(UNIX AND (X86_64 OR X86)) need detect X86
  target_compile_options(${PROJECT} PUBLIC -m32)
  set_target_properties(${PROJECT} PROPERTIES LINK_FLAGS -m32)
#endif()

set_target_properties(${PROJECT} PROPERTIES
  PREFIX ""
  DEBUG_POSTFIX "_debug"
)

#################################################################################
# Libraries
#################################################################################

if(NOT U3D_FUNCTION_MODULE_PATH)
  set(U3D_FUNCTION_MODULE_PATH "../u3d_function_module/messages_functions")
endif()

if(NOT U3D_FUNCTION_MODULE_PATH STREQUAL U3D_FUNCTION_MODULE_PATH_INTERNAL)
  unset(U3D_FUNCTION_MODULE_CONFIG)
endif()
set(U3D_FUNCTION_MODULE_PATH_INTERNAL ${U3D_FUNCTION_MODULE_PATH} CACHE INTERNAL "" FORCE)

find_file(U3D_FUNCTION_MODULE_CONFIG NAMES "messages_functions.cmake" PATHS "${U3D_FUNCTION_MODULE_PATH}/build/install/config" NO_DEFAULT_PATH)
if(U3D_FUNCTION_MODULE_CONFIG)
  include(${U3D_FUNCTION_MODULE_CONFIG})
  target_link_libraries(${PROJECT} PRIVATE messages_functions)
else()
  message(SEND_ERROR "Please set U3D_FUNCTION_MODULE_PATH, current value: ${U3D_FUNCTION_MODULE_PATH}")
endif()


###############################
## Module_headers
###############################
if(NOT MODULE_HEADERS_PATH)
  set(MODULE_HEADERS_PATH "../../../libraries/public/module_headers")
endif()

if(NOT MODULE_HEADERS_PATH STREQUAL MODULE_HEADERS_PATH_INTERNAL)
  unset(MODULE_HEADERS_DIR)
endif()
set(MODULE_HEADERS_PATH_INTERNAL ${MODULE_HEADERS_PATH} CACHE INTERNAL "" FORCE)

find_path(MODULE_HEADERS_DIR NAMES "current/module.h" PATHS ${MODULE_HEADERS_PATH} NO_DEFAULT_PATH)
if(MODULE_HEADERS_DIR)
  target_include_directories(${PROJECT} PRIVATE "${MODULE_HEADERS_DIR}/100")
else()
  message(SEND_ERROR "Please set MODULE_HEADERS_PATH, current value: ${MODULE_HEADERS_PATH}")
endif()

###############################
## SimpleIni
###############################
if(NOT SIMPLE_INI_PATH)
  set(SIMPLE_INI_PATH "../../../libraries/3rdparty/simpleini")
endif()

if(NOT SIMPLE_INI_PATH STREQUAL SIMPLE_INI_PATH_INTERNAL)
  unset(SIMPLE_INI_DIR)
endif()
set(SIMPLE_INI_PATH_INTERNAL ${SIMPLE_INI_PATH} CACHE INTERNAL "" FORCE)

find_path(SIMPLE_INI_DIR NAMES "SimpleIni.h" PATHS "${SIMPLE_INI_PATH}" NO_DEFAULT_PATH)
if(SIMPLE_INI_DIR)
  target_include_directories(${PROJECT} PRIVATE ${SIMPLE_INI_DIR})
else()
  message(SEND_ERROR "Please set SIMPLE_INI_PATH, current value: ${SIMPLE_INI_PATH}")
endif()

###############################
## Boost
###############################
if (NOT BOOST_ROOT)
  set(BOOST_ROOT "../../../libraries/3rdparty/boost")
endif()

if (NOT BOOST_ROOT STREQUAL BOOST_ROOT_INTERNAL)
  unset(Boost_FOUND)
  unset(Boost_INCLUDE_DIRS)
  unset(Boost_LIBRARIES)
endif()
set(BOOST_ROOT_INTERNAL ${BOOST_ROOT} CACHE INTERNAL "" FORCE)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_NO_BOOST_CMAKE ON)

set(BOOST_COMPONENTS)
list(APPEND BOOST_COMPONENTS thread)

find_package(Boost 1.60.0 COMPONENTS ${BOOST_COMPONENTS})
if(Boost_FOUND)
  target_include_directories(${PROJECT} PRIVATE ${Boost_INCLUDE_DIRS})
  target_link_libraries(${PROJECT} PRIVATE ${Boost_LIBRARIES})
else()
  message(SEND_ERROR "Please set or correct BOOST_ROOT, current value: ${BOOST_ROOT}")
endif()

#################################################################################
# Install
#################################################################################

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install)
target_include_directories(${PROJECT} INTERFACE $<INSTALL_INTERFACE:include>)

install(TARGETS ${PROJECT} EXPORT ${PROJECT} RUNTIME DESTINATION lib COMPONENT Runtime)
