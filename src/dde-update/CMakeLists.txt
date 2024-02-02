
set(PROJECT_NAME dde-update)
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

target_include_directories(${PROJECT_NAME} PRIVATE
    ${DtkWidget_INCLUDE_DIRS}
    ${DFrameworkDBus_INCLUDE_DIRS}
    ${QGSettings_INCLUDE_DIRS}
    "../common"
    "../common/common"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Qt5Widgets_LIBRARIES}
    ${Qt5DBus_LIBRARIES}
    ${DtkWidget_LIBRARIES}
    ${DtkCore_LIBRARIES}
    ${DFrameworkDBus_LIBRARIES}
    ${QGSettings_LIBRARIES}
)

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
# 需要在greeter之前启动的配置和脚本
install(FILES "misc/99_system_upgrade_conf.json" DESTINATION share/dde-session-shell/greeters.d/launch.conf.d)
install(PROGRAMS "misc/system_upgrade_check.sh" DESTINATION bin)
install(FILES "misc/dde-update-autostart.desktop" DESTINATION /etc/xdg/autostart)
install(PROGRAMS "misc/98deepin-upgrade-check" DESTINATION /etc/X11/Xsession.d/)
install(PROGRAMS "misc/run-kwayland-check-wrapper.sh" DESTINATION bin)
install(PROGRAMS "misc/run-kwayland-check.sh" DESTINATION bin)
