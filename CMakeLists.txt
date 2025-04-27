cmake_minimum_required(VERSION 3.20)

project(qsmap_project)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /std:c++17")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8 /std:c++17")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
  # disable macros that offend std::numeric_limits<T>::min()/max()
  add_definitions(-DNOMINMAX)
  add_definitions(-D_USE_MATH_DEFINES)
  add_definitions(-DWIN32)

  set(CMAKE_WIN32_EXECUTABLE ON)
  set(CMAKE_MFC_EXECUTABLE ON)
  find_package(MFC REQUIRED)
endif()

#set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/../../install)

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)#寮哄埗浣跨敤17鏍囧噯

# Turn on automatic invocation of the MOC, UIC & RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Save environment variable into the CMake but with the proper path separator
file(TO_CMAKE_PATH "$ENV{QGIS_LIB_PATH}" QGIS_LIB_PATH)
file(TO_CMAKE_PATH "$ENV{QGIS_INCLUDE_PATH}" QGIS_INCLUDE_PATH)
# Following commands could use the created CMake variable
include_directories(
  ${QGIS_INCLUDE_PATH}
  ${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/ui
  ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/src
  ${CMAKE_SOURCE_DIR}/include
)

link_directories(
  ${QGIS_LIB_PATH}
)

# There may be a way to tell up front if Qt5 is going to be found, but I haven't found
# a foolproof way to do it yet, so settle for the default error message for now.
#if(NOT CMAKE_PREFIX_PATH AND NOT Qt5Widgets_DIR)
#    message(WARNING "CMAKE_PREFIX_PATH is not defined, so find_package may not work. Set the CMAKE_PREFIX_PATH "
#            "environment variable to the install prefix of Qt 5, either on the command line as "
#            "-DCMAKE_PREFIX_PATH=\"path/to/Qt5/lib/cmake\" or with set(CMAKE_PREFIX_PATH path/to/Qt5/lib/cmake)")
#endif(NOT CMAKE_PREFIX_PATH AND NOT Qt5Widgets_DIR)

# Add a compiler flag
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# Find the QtWidgets library
find_package(Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt5 REQUIRED COMPONENTS Xml)
find_package(Qt5 REQUIRED COMPONENTS Core)
find_package(Qt5 REQUIRED COMPONENTS Gui)
find_package(Qt5 REQUIRED COMPONENTS Network)
find_package(Qt5 REQUIRED COMPONENTS OpenGL)
# MODULE 琛ㄧず浠呬娇鐢ㄦā鍧楁ā寮忥紝涓嶄娇鐢ㄩ厤缃ā寮?
#find_package(QGIS REQUIRED MODULE)

list(APPEND CMAKE_AUTOUIC_SEARCH_PATHS "ui")

# 杈撳嚭鐩綍锛?{PROJECT_BINARY_DIR}涓篊make鍛戒护鎵ц鐨勪綅缃?
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)#瀛樻斁闈欐€佸簱鐨勪綅缃紝浠ュ強MSVC涓姩鎬佸簱鐨刲ib鏂囦欢鐨勪綅缃€?
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)#瀛樻斁鍔ㄦ€佸簱鐨勪綅缃?
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)#鍙墽琛屾枃浠朵綅缃?

install(DIRECTORY "${CMAKE_SOURCE_DIR}/include" DESTINATION ${PROJECT_BINARY_DIR}/install
    FILES_MATCHING PATTERN "*.h")
install(DIRECTORY "${PROJECT_BINARY_DIR}/bin" DESTINATION ${PROJECT_BINARY_DIR}/install
    FILES_MATCHING PATTERN "*.dll" PATTERN "*.exe" PATTERN "*.so")
install(DIRECTORY "${PROJECT_BINARY_DIR}/lib" DESTINATION ${PROJECT_BINARY_DIR}/install
    FILES_MATCHING PATTERN "*.lib" PATTERN "*.a")

# 娣诲姞瀛愮洰褰曪紝瀛愮洰褰曚笅蹇呴』鏈塁MakeLists.txt
add_subdirectory(app)
add_subdirectory(core)
add_subdirectory(plugs)
