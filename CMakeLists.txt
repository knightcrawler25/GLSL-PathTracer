cmake_minimum_required (VERSION 2.8)
project (PathTracer)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "Configuration types" FORCE)

SET(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


file(GLOB_RECURSE SRC_FILES
    ${CMAKE_SOURCE_DIR}/PathTracer/*.h
    ${CMAKE_SOURCE_DIR}/PathTracer/*.cpp
)
file(GLOB EXT_FILES
    ${CMAKE_SOURCE_DIR}/thirdparty/SOIL/src/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/SOIL/src/*.c
    ${CMAKE_SOURCE_DIR}/thirdparty/RadeonRays/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/RadeonRays/*.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/*.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/imguizmo/*.h
    ${CMAKE_SOURCE_DIR}/thirdparty/imguizmo/*.cpp	
)

if (NOT EMSCRIPTEN)
file(GLOB EXT_FILES_SPECIFIC
    ${CMAKE_SOURCE_DIR}/thirdparty/gl3w/GL/*.c
)
set(SDL2_LIBRARIES "SDL2" "SDL2main")
endif()


set(SDL2_INCLUDE_DIRS  ${CMAKE_SOURCE_DIR}/thirdparty/SDL2/include)
set(SDL2_LIBDIR ${CMAKE_SOURCE_DIR}/thirdparty/SDL2/lib/x64)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/PathTracer
  ${CMAKE_SOURCE_DIR}/thirdparty/glm
  ${CMAKE_SOURCE_DIR}/thirdparty/imgui
  ${CMAKE_SOURCE_DIR}/thirdparty/RadeonRays
  ${CMAKE_SOURCE_DIR}/thirdparty/glew/include
  ${CMAKE_SOURCE_DIR}/thirdparty/SOIL/include
  ${CMAKE_SOURCE_DIR}/thirdparty/gl3w
  ${CMAKE_SOURCE_DIR}/thirdparty/imguizmo
  ${SDL2_INCLUDE_DIRS}
)

link_directories(
  ${SDL2_LIBDIR}
)
find_package(OpenGL)


#######################################################################################

#set (USE_MSVC_RUNTIME_LIBRARY_DLL true)
#IF (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)
#    #We statically link to reduce dependancies
#    FOREACH(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RETAIL)
#        IF(${flag_var} MATCHES "/MD")
#            STRING(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
#        ENDIF(${flag_var} MATCHES "/MD")
#        IF(${flag_var} MATCHES "/MDd")
#            STRING(REGEX REPLACE "/MDd" "/MTd" ${flag_var} "${${flag_var}}")
#        ENDIF(${flag_var} MATCHES "/MDd")
#    ENDFOREACH(flag_var)
#ENDIF (NOT USE_MSVC_RUNTIME_LIBRARY_DLL)

#######################################################################################

foreach(f ${SRC_FILES})
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

set(ENABLE_HIDECONSOLE_BUILD TRUE CACHE BOOL "TRUE to hide console for Windows sys")

if(WINDOWS)
  set(GUI_TYPE WIN32)
elseif(MACOSX)
  set(GUI_TYPE MACOSX_BUNDLE)
endif()

if(MSVC)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()


SET(LINK_OPTIONS " ")
SET(EXE_NAME "PathTracer")

ADD_EXECUTABLE(${EXE_NAME} ${SRC_FILES} ${EXT_FILES} ${EXT_FILES_SPECIFIC})

if(WINDOWS)
TARGET_LINK_LIBRARIES(${EXE_NAME} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES})
else()
TARGET_LINK_LIBRARIES(${EXE_NAME} ${OPENGL_LIBRARIES} ${SDL2_LIBRARIES} dl)
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

set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin )
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin )
set_target_properties(${EXE_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin )
set_target_properties(${EXE_NAME} PROPERTIES DEBUG_POSTFIX "_d")
set_target_properties(${EXE_NAME} PROPERTIES RELWITHDEBINFO_POSTFIX "RelWithDebInfo")
set_target_properties(${EXE_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

#--------------------------------------------------------------------
# Hide the console window in visual studio projects
#--------------------------------------------------------------------
if(ENABLE_HIDECONSOLE_BUILD)
if(MSVC)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
endif()
endif()

if(WINDOWS)
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS")
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
endif()

if (EMSCRIPTEN)
add_definitions(-DEMSCRIPTEN)
add_definitions(-D_X86_)
set(EMS_OPTS "-s USE_SDL=2 -s USE_WEBGL2=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s BINARYEN_TRAP_MODE=clamp --bind -O2 --no-heap-copy -g4 --source-map-base localhost:8000")
add_definitions(${EMS_OPTS})
set(EMS_PRELOAD " --preload-file assets --preload-file shaders")
set(EMS_LINK "${EMS_OPTS} ${EMS_PRELOAD}")
set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS ${EMS_LINK})

add_custom_command(TARGET ${EXE_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/bin/assets ${CMAKE_CURRENT_BINARY_DIR}/assets
)
add_custom_command(TARGET ${EXE_NAME} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/PathTracer/shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders
)

add_custom_command(TARGET ${EXE_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/PathTracer/index.html ${CMAKE_CURRENT_BINARY_DIR}/index.html
)


endif()

if(ENABLE_HIDECONSOLE_BUILD)
MESSAGE(STATUS "Console is hidden")
else()
MESSAGE(STATUS "Console is visible")
endif()