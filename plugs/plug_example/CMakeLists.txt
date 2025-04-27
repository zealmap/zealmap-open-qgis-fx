project(qsmap_plug_example)

########################################################
# Files
aux_source_directory(${PROJECT_SOURCE_DIR}/src plug_prj_src)

set (PLUG_PRJ_SRCS
${plug_prj_src}
)

########################################################
# Build
add_library(${PROJECT_NAME} MODULE ${PLUG_PRJ_SRCS})

target_link_libraries(${PROJECT_NAME}
Qt5::Widgets
Qt5::Xml
Qt5::Core
Qt5::Gui
Qt5::Network
Qt5::OpenGL
qgis_core
qgis_app
qgis_gui
qsmapcore
)

add_dependencies(${PROJECT_NAME} qsmapcore)
