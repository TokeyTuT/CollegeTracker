cmake_minimum_required(VERSION 3.16)

project(CollegeTracker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Sql LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Sql LinguistTools)

set(TS_FILES CollegeTracker_zh_CN.ts)

set(PROJECT_SOURCES
        src/main.cpp
        src/views/mainwindow.cpp
        src/views/mainwindow.h
        ui/mainwindow.ui
        ${TS_FILES}
)

set(EXTRA_SOURCES
    src/models/DatabaseMannager.h
    src/models/DatabaseMannager.cpp
    src/views/AddCourseDialog.h
    src/views/AddCourseDialog.cpp
    src/views/LoginDialog.h
    src/views/LoginDialog.cpp
    src/views/RegisterDialog.h
    src/views/RegisterDialog.cpp
    src/models/User.h
    src/models/User.cpp
    src/services/ResumeExporter.h
    src/services/ResumeExporter.cpp
    src/services/CsvUtils.h
    src/services/CsvUtils.cpp
    src/views/pages/HomePage.h
    src/views/pages/HomePage.cpp
    src/views/pages/CoursePage.h
    src/views/pages/CoursePage.cpp
    src/views/pages/ExperiencePage.h
    src/views/pages/ExperiencePage.cpp
    src/views/pages/ResumePage.h
    src/views/pages/ResumePage.cpp
    src/views/dialogs/PhotoCropDialog.h
    src/views/dialogs/PhotoCropDialog.cpp
    resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(CollegeTracker
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${EXTRA_SOURCES}
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(CollegeTracker SHARED
            ${PROJECT_SOURCES}
            ${EXTRA_SOURCES}
        )
    else()
        add_executable(CollegeTracker
            ${PROJECT_SOURCES}
            ${EXTRA_SOURCES}
        )
    endif()
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()


target_include_directories(CollegeTracker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/pages
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
)

target_link_libraries(CollegeTracker PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.CollegeTracker)
endif()
set_target_properties(CollegeTracker PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS CollegeTracker
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(CollegeTracker)
endif()
