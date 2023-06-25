cmake_minimum_required(VERSION 3.14)
project(ClimateDisplay)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${PROJECT_SOURCE_DIR}")
include (tools)
include (FindGit)
include (FetchContent)

find_package(Git)

assertVariableSet(GIT_FOUND)

FetchContent_Declare(
    ssd1306
    GIT_REPOSITORY https://github.com/AndrewFromMelbourne/libSSD1306
    GIT_TAG master
#    SOURCE_DIR libSSD1306
)

FetchContent_MakeAvailable(ssd1306)
#FetchContent_GetProperties(ssd1306)

get_cmake_property(_variableNames VARIABLES)
list (SORT _variableNames)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

#if(NOT ssd1306_POPULATED)
#    message(STATUS "Fetching ssd1306")
#    FetchContent_Populate(ssd1306)
#endif()

#add_subdirectory(${ssd1306_SOURCE_DIR})

if (NOT MOSQUITTO_INCLUDE_DIR)
    find_path(MOSQUITTO_INCLUDE_DIR mosquitto.h)
endif()

if (NOT MOSQUITTO_LIB)
    message(STATUS "Searching. CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
    find_library(MOSQUITTO_LIB mosquitto REQUIRED)
endif()


add_executable(ClimateDisplay "")

set(myHeaders Config.h)
set(mySources ClimateDisplay.cpp Config.cpp)
target_sources (ClimateDisplay PUBLIC ${myHeaders} PRIVATE ${mySources})
target_include_directories(ClimateDisplay PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${MOSQUITTO_INCLUDE_DIR} ${ssd1306_SOURCE_DIR}/lib)
target_link_libraries(ClimateDisplay ${MOSQUITTO_LIB} SSD1306 )
