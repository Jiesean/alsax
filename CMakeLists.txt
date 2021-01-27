cmake_minimum_required(VERSION 3.7)
project(sai_micbasex)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BUILD_TINYALSA "Build sai_micbasex with tinyalsa" OFF)
option(BUILD_LINUXALSA "Build sai_micbasex with linux alsa" OFF)
option(WRITE_DATA_FILE "write data to file" OFF)
option(BUILD_STATIC_LIB "Build static library" OFF)
option(BUILD_BOOST_TEST "Build boost test sample" OFF)

if(${CMAKE_BUILD_TYPE} STREQUAL debug)
	add_definitions(-DDEBUG)
endif()

if(WRITE_DATA_FILE)
    add_definitions(-DWRITE_DATA_FILE)
endif()

include_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/inc" 
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/downsample/inc"
)

link_directories(
    "${CMAKE_CURRENT_SOURCE_DIR}/thridlib/${SYSTEM_DETAILS}"
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/downsample/src DOWNSAMPLE_SRC)
set(LIB_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src/sai_micbasex.cpp ${DOWNSAMPLE_SRC})
if(BUILD_STATIC_LIB)
    add_library(${PROJECT_NAME} STATIC ${LIB_SRC})
else()
    add_library(${PROJECT_NAME} SHARED ${LIB_SRC})
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_executable(sample_basex ${CMAKE_CURRENT_SOURCE_DIR}/sample/sample_basex.cpp)
if(BUILD_BOOST_TEST)
    add_executable(sample_boosttest ${CMAKE_CURRENT_SOURCE_DIR}/sample/sample_boosttest.cpp)
endif()

if(BUILD_TINYALSA)
	add_definitions(-DPCMTINYALSA)
    target_link_libraries(${PROJECT_NAME} tinyalsa log)
	target_link_libraries(sample_basex tinyalsa ${PROJECT_NAME})
    if(BUILD_BOOST_TEST)
    	target_link_libraries(sample_boosttest tinyalsa ${PROJECT_NAME} libboost_unit_test_framework.a)
    endif()
endif()

if(BUILD_LINUXALSA)
	add_definitions(-DPCMALSA)
    target_link_libraries(${PROJECT_NAME} asound)
	target_link_libraries(sample_basex asound ${PROJECT_NAME})
    if(BUILD_BOOST_TEST)
    	target_link_libraries(sample_boosttest asound ${PROJECT_NAME} libboost_unit_test_framework.a)
    endif()
endif()



