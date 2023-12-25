
set(PROJECT_NAME dde-abrecovery)
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
find_package(Qt5 COMPONENTS Core Widgets DBus REQUIRED)
find_package(DdeControlCenter REQUIRED)

pkg_check_modules(QGSettings REQUIRED gsettings-qt)
pkg_check_modules(DFrameworkDBus REQUIRED dframeworkdbus)

file(GLOB_RECURSE SRCS
    "*.h"
    "*.cpp"
    "*.qrc"
    "../common/*.h"
    "../common/*.cpp"
)

add_executable(${PROJECT_NAME} ${SRCS})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${DtkWidget_INCLUDE_DIRS}
    ${DFrameworkDBus_INCLUDE_DIRS}
    ${Qt5Gui_PRIVATE_INCLUDE_DIRS}
    ${PROJECT_BINARY_DIR}
    ${DtkWidget_INCLUDE_DIRS}
    "../common"
    "../common/common"
)

target_link_libraries(${PROJECT_NAME}
    ${DFrameworkDBus_LIBRARIES}
    ${DtkWidget_LIBRARIES}
    ${Qt5Widgets_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
# 需要在greeter之前启动的配置和脚本
install(FILES "misc/98_ab_rollback_conf.json" DESTINATION share/dde-session-shell/greeters.d/launch.conf.d)
install(PROGRAMS "misc/ab_rollback_check.sh" DESTINATION bin)
