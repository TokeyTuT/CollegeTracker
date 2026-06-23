cmake_minimum_required(VERSION 3.16)

if(APPLE AND NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING
        "Minimum supported macOS version")
endif()

project(CollegeTracker VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql LinguistTools)


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
    src/views/dialogs/AddCourseDialog.h
    src/views/dialogs/AddCourseDialog.cpp
    src/views/dialogs/LoginDialog.h
    src/views/dialogs/LoginDialog.cpp
    src/views/dialogs/RegisterDialog.h
    src/views/dialogs/RegisterDialog.cpp
    src/models/User.h
    src/models/User.cpp
    src/services/ResumeExporter.h
    src/services/ResumeExporter.cpp
    src/services/ResumeTemplateRegistry.h
    src/services/ResumeTemplateRegistry.cpp
    src/services/CsvUtils.h
    src/services/CsvUtils.cpp
    src/services/AvatarUtils.h
    src/services/AvatarUtils.cpp
    src/services/AppDataPaths.h
    src/services/AppDataPaths.cpp
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

qt_add_executable(CollegeTracker
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
    ${EXTRA_SOURCES}
)



target_include_directories(CollegeTracker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/pages
    ${CMAKE_CURRENT_SOURCE_DIR}/src/views/dialogs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/models
    ${CMAKE_CURRENT_SOURCE_DIR}/src/controllers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/services
)

target_link_libraries(CollegeTracker PRIVATE
    Qt6::Widgets
    Qt6::Sql
)

set_target_properties(CollegeTracker PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.collegetracker.CollegeTracker
    MACOSX_BUNDLE_BUNDLE_NAME CollegeTracker
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

qt_finalize_executable(CollegeTracker)
