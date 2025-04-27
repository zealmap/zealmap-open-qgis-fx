project(qsmapapp)

# Make this a GUI application on Windows
if(WIN32)
  set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# 将当前目录下所有源码文件添加到变量app_src
aux_source_directory(${PROJECT_SOURCE_DIR}/src app_src)

set (APP_SRCS
${app_src}
)

add_executable(${PROJECT_NAME} ${APP_SRCS})

# Add the Qt5 Widgets for linking
target_link_libraries(${PROJECT_NAME}
Qt5::Widgets
Qt5::Xml
Qt5::Core
Qt5::Gui
Qt5::Network
qsmapcore)

add_dependencies(${PROJECT_NAME} qsmapcore)
