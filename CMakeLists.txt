cmake_minimum_required(VERSION 3.14)

project(PVEClient
    VERSION 0.1.0
    DESCRIPTION "PVE 云桌面瘦客户端"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 查找 Qt5 组件
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
)

# 源文件
set(SOURCES
    src/main.cpp

    # 核心模块
    src/core/ConnectionInfo.cpp
    src/core/ConfigManager.cpp
    src/core/ProcessRunner.cpp
    src/core/ConnectionManager.cpp

    # PVE API
    src/pve/PveApiClient.cpp
    src/pve/PveAuthManager.cpp

    # 协议启动器
    src/protocol/RdpLauncher.cpp
    src/protocol/SpiceLauncher.cpp

    # UI
    src/ui/MainWindow.cpp
    src/ui/LoginDialog.cpp
    src/ui/ConnectionListWidget.cpp
    src/ui/ConnectionEditDialog.cpp
    src/ui/SettingsDialog.cpp
)

set(HEADERS
    # 核心模块
    src/core/ConnectionInfo.h
    src/core/ConfigManager.h
    src/core/ProcessRunner.h
    src/core/ConnectionManager.h

    # PVE API
    src/pve/PveApiClient.h
    src/pve/PveAuthManager.h

    # 协议启动器
    src/protocol/AbstractLauncher.h
    src/protocol/RdpLauncher.h
    src/protocol/SpiceLauncher.h

    # UI
    src/ui/MainWindow.h
    src/ui/LoginDialog.h
    src/ui/ConnectionListWidget.h
    src/ui/ConnectionEditDialog.h
    src/ui/SettingsDialog.h
)

# 资源文件
set(RESOURCES
    resources/resources.qrc
)

# 可执行文件
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

# 链接 Qt 库
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
)

# 头文件搜索路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

# Windows 特定设置：隐藏控制台窗口（Release 模式）
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE $<CONFIG:Release>
    )
endif()
