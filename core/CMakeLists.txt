project(qsmapcore)

add_definitions(-DQSMAP_APP_LIBRARY_EXPORTS)
add_definitions(-DQXlsx_SHAREDLIB)
add_definitions(-DQXlsx_EXPORTS)

# 将当前目录下所有源码文件添加到变量app_src
aux_source_directory(${PROJECT_SOURCE_DIR}/src core_src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/QXlsx/source QXLSX_src)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/qtpropertybrowser qtpropertybrowser_src)

# 包含需要MOC的源码
set(EXTERNAL_QT_SRCS
${CMAKE_SOURCE_DIR}/include/qsmapapp.h
${CMAKE_SOURCE_DIR}/include/qtpropertybrowser.h
${CMAKE_SOURCE_DIR}/include/qttreepropertybrowser.h
${CMAKE_SOURCE_DIR}/include/qtvariantproperty.h
${CMAKE_SOURCE_DIR}/include/qtpropertymanager.h
${CMAKE_SOURCE_DIR}/include/qsmapprojectview.h
${CMAKE_SOURCE_DIR}/include/xlsxdocument.h
)

set(QXLSX_SRC_FILES
    ${QXLSX_src}
    ./src/QXlsx/header/xlsxabstractooxmlfile_p.h
    ./src/QXlsx/header/xlsxchartsheet_p.h
    ./src/QXlsx/header/xlsxdocpropsapp_p.h
    ./src/QXlsx/header/xlsxformat_p.h
    ./src/QXlsx/header/xlsxsharedstrings_p.h
    ./src/QXlsx/header/xlsxworkbook_p.h
    ./src/QXlsx/header/xlsxabstractsheet_p.h
    ./src/QXlsx/header/xlsxcolor_p.h
    ./src/QXlsx/header/xlsxdocpropscore_p.h
    ./src/QXlsx/header/xlsxmediafile_p.h
    ./src/QXlsx/header/xlsxsimpleooxmlfile_p.h
    ./src/QXlsx/header/xlsxworksheet_p.h
    ./src/QXlsx/header/xlsxcellformula_p.h
    ./src/QXlsx/header/xlsxconditionalformatting_p.h
    ./src/QXlsx/header/xlsxdocument_p.h
    ./src/QXlsx/header/xlsxnumformatparser_p.h
    ./src/QXlsx/header/xlsxstyles_p.h
    ./src/QXlsx/header/xlsxzipreader_p.h
    ./src/QXlsx/header/xlsxcell_p.h
    ./src/QXlsx/header/xlsxcontenttypes_p.h
    ./src/QXlsx/header/xlsxdrawinganchor_p.h
    ./src/QXlsx/header/xlsxrelationships_p.h
    ./src/QXlsx/header/xlsxtheme_p.h
    ./src/QXlsx/header/xlsxzipwriter_p.h
    ./src/QXlsx/header/xlsxchart_p.h
    ./src/QXlsx/header/xlsxdatavalidation_p.h
    ./src/QXlsx/header/xlsxdrawing_p.h
    ./src/QXlsx/header/xlsxrichstring_p.h
    ./src/QXlsx/header/xlsxutility_p.h
)

set (APP_SRCS
${core_src}
${qtpropertybrowser_src}
${QXLSX_SRC_FILES}
${EXTERNAL_QT_SRCS}
resources.qrc
src/qtpropertybrowser/qtpropertybrowser.qrc
)

include_directories(
  ${PROJECT_SOURCE_DIR}/src/QXlsx/header
)

add_library(${PROJECT_NAME} SHARED ${APP_SRCS})

if (NOT WIN32)
    # Strict iterators can't be used on Windows, they lead to a link error
    # when application code iterates over a QVector<QPoint> for instance, unless
    # Qt itself was also built with strict iterators.
    # See example at https://bugreports.qt.io/browse/AUTOSUITE-946
    target_compile_definitions(${PROJECT_NAME} PRIVATE QT_STRICT_ITERATORS)
endif()

# Add the Qt5 Widgets for linking
target_link_libraries(${PROJECT_NAME}
Qt5::Widgets
Qt5::Xml
Qt5::Core
Qt5::Gui
Qt5::GuiPrivate
Qt5::Network
Qt5::OpenGL
qgis_core
qgis_gui
qgis_app)
