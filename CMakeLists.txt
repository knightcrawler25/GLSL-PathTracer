cmake_minimum_required (VERSION 2.8)
project (PathTracer)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Configuration types" FORCE)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE SHADERS
    ${CMAKE_SOURCE_DIR}/src/shaders/*.glsl
)

file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

file(GLOB_RECURSE EXT_FILES
    ${CMAKE_SOURCE_DIR}/thirdparty/stb/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imguizmo/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/imguizmo/*.cpp  
    ${CMAKE_SOURCE_DIR}/thirdparty/RadeonRays/*.h 
    ${CMAKE_SOURCE_DIR}/thirdparty/RadeonRays/*.cpp     
    ${CMAKE_SOURCE_DIR}/thirdparty/oidn/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/oidn/*.hpp
    ${CMAKE_SOURCE_DIR}/thirdparty/tinydir/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/tinyobjloader/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/tinygltf/*.h
)

if(APPLE)
list(APPEND EXT_FILES ${CMAKE_SOURCE_DIR}/thirdparty/gl3wApple/GL/gl3w.c)
else()
list(APPEND EXT_FILES ${CMAKE_SOURCE_DIR}/thirdparty/gl3w/GL/gl3w.c)
endif()

set(SRCS ${SRC_FILES} ${EXT_FILES} ${SHADERS})

if(APPLE)
find_package(SDL2 REQUIRED)
else()
set(SDL2_LIBRARIES "SDL2" "SDL2main")
set(SDL2_INCLUDE_DIRS  ${CMAKE_SOURCE_DIR}/thirdparty/SDL2/include)
set(SDL2_LIBDIR ${CMAKE_SOURCE_DIR}/thirdparty/SDL2/lib/x64)
endif()
set(OIDN_LIBRARIES "OpenImageDenoise")
set(OIDN_LIBDIR ${CMAKE_SOURCE_DIR}/thirdparty/oidn/lib)

include_directories(
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/loaders
    ${CMAKE_SOURCE_DIR}/src/tests
    ${CMAKE_SOURCE_DIR}/src/math
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui
    ${CMAKE_SOURCE_DIR}/thirdparty/glew/include
    ${CMAKE_SOURCE_DIR}/thirdparty/stb
    ${CMAKE_SOURCE_DIR}/thirdparty/imguizmo
    ${CMAKE_SOURCE_DIR}/thirdparty/RadeonRays
    ${CMAKE_SOURCE_DIR}/thirdparty/tinydir
    ${CMAKE_SOURCE_DIR}/thirdparty/tinyobjloader
    ${CMAKE_SOURCE_DIR}/thirdparty/oidn/include
    ${CMAKE_SOURCE_DIR}/thirdparty/tinygltf
    ${SDL2_INCLUDE_DIRS}
)

if(APPLE)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/gl3wApple)
else()
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/gl3w)
endif()

link_directories(
  ${SDL2_LIBDIR}
  ${OIDN_LIBDIR}
)
find_package(OpenGL)

foreach(f ${SRCS})
    # Get the path of the file relative to ${DIRECTORY},
    # then alter it (not compulsory)
    file(RELATIVE_PATH SRCGR ${CMAKE_SOURCE_DIR} ${f})
    set(SRCGR "Sources/${SRCGR}")

    # Extract the folder, ie remove the filename part
    string(REGEX REPLACE "(.*)(/[^/]*)$" "\\1" SRCGR ${SRCGR})

    # Source_group expects \\ (double antislash), not / (slash)
    string(REPLACE / \\ SRCGR ${SRCGR})
    source_group("${SRCGR}" FILES ${f})
endforeach()

if(WIN32)
  set(GUI_TYPE WIN32)
elseif(MACOSX)
  set(GUI_TYPE MACOSX_BUNDLE)
endif()

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()


SET(LINK_OPTIONS " ")
SET(EXE_NAME "PathTracer")

ADD_EXECUTABLE(${EXE_NAME} ${SRCS})

if(WIN32)
TARGET_LINK_LIBRARIES(${EXE_NAME} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${OIDN_LIBRARIES})
else()
TARGET_LINK_LIBRARIES(${EXE_NAME} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} ${OIDN_LIBRARIES} dl)
endif()

#--------------------------------------------------------------------
# preproc
#--------------------------------------------------------------------
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-DBX_CONFIG_ENABLE_MSVC_LEVEL4_WARNINGS=1)
add_definitions(-D__STDC_LIMIT_MACROS)
add_definitions(-D__STDC_CONSTANT_MACROS)
add_definitions(-DIMGUI_DISABLE_OBSOLETE_FUNCTIONS)
if(MSVC)
add_definitions(-DWIN32)
add_definitions(-D_WIN32)
#add_definitions(-DNOMINMAX)
endif()
add_definitions(-DUSE_DL_PREFIX)
add_definitions(-DGLEW_STATIC)

#--------------------------------------------------------------------
# output dirs
#--------------------------------------------------------------------

set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_BINARY_DIR} )
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_BINARY_DIR} )
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_CURRENT_BINARY_DIR} )
set_target_properties(${EXE_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${EXE_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX "RelWithDebInfo")
set_target_properties(${EXE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

if(WIN32)
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
endif()

#add_custom_command(TARGET ${EXE_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
#)
#add_custom_command(TARGET ${EXE_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
#)

if(WIN32)
add_custom_command(TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${SDL2_LIBDIR}/SDL2.dll ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${OIDN_LIBDIR}/OpenImageDenoise.dll ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy ${OIDN_LIBDIR}/tbb.dll ${CMAKE_CURRENT_BINARY_DIR}
)
endif()