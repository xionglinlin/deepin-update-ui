set(PROJECT_NAME dcc-update-plugin)
project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_FLAGS "-g -Wall")

# Find the library
find_package(PkgConfig REQUIRED)
find_package(DtkWidget REQUIRED)
find_package(DtkGui REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets DBus Network Concurrent REQUIRED)
find_package(DdeControlCenter REQUIRED)

pkg_check_modules(QGSettings REQUIRED gsettings-qt)
pkg_check_modules(DFrameworkDBus REQUIRED dframeworkdbus)

message(STATUS "dde-control-center version:" ${DdeControlCenter_VERSION})

# 比较版本号
if(DdeControlCenter_VERSION VERSION_GREATER "5.7.5")
  add_definitions(-DDCC_MODULE_CATEGORY_VALID)
  message(STATUS "dcc module category is valid")
endif()

file(GLOB_RECURSE SRCS
    "*.h"
    "*.cpp"
    "*.qrc"
)

add_library(${PROJECT_NAME} SHARED ${SRCS})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5DBus_INCLUDE_DIRS}
    ${Qt5Network_INCLUDE_DIRS}
    ${Qt5Concurrent_INCLUDE_DIRS}
    ${DFrameworkDBus_INCLUDE_DIRS}
    ${QGSettings_INCLUDE_DIRS}
    ${DtkCore_INCLUDE_DIRS}
    ${DtkWidget_INCLUDE_DIRS}
    ${DtkGui_INCLUDE_DIRS}
    ${DdeControlCenter_INCLUDE_DIR}
    "module"
    "window"
    "window/widgets"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Qt5Core_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${Qt5Network_LIBRARIES}
    ${Qt5Concurrent_LIBRARIES}
    ${DFrameworkDBus_LIBRARIES}
    ${QGSettings_LIBRARIES}
    ${DtkCore_LIBRARIES}
    ${DtkWidget_LIBRARIES}
    ${DtkGui_LIBRARIES}
    ${DdeControlCenter_LIBRARIES}
)

install(FILES "misc/org.deepin.dde.control-center.update.json" DESTINATION share/dsg/configs)
install(TARGETS ${PROJECT_NAME} DESTINATION lib/dde-control-center/modules)
