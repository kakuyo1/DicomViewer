cmake_minimum_required(VERSION 3.21)

project(DicomViewer VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Concurrent)          # 先探测系统里是 Qt6 还是 Qt5，把版本号存进 QT_VERSION_MAJOR
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Concurrent)

# VTK
find_package(VTK CONFIG REQUIRED COMPONENTS
    CommonCore
    RenderingCore
    RenderingOpenGL2
    GUISupportQt
)

# DCMTK
set(DCMTK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dcmtk/lib/cmake/dcmtk")
find_package(DCMTK CONFIG REQUIRED)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE headers CONFIGURE_DEPENDS src/*.h src/*.hpp)

add_executable(DicomViewer
    ${sources}
    ${headers}
)

set_target_properties(DicomViewer PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
    BUILD_RPATH "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dcmtk/lib"  # 让程序在构建目录运行时能找到 third_party/dcmtk/lib 下的动态库
)

target_include_directories(DicomViewer
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/spdlog/include
)

target_link_libraries(DicomViewer
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Concurrent
        DCMTK::dcmdata
        VTK::CommonCore
        VTK::RenderingCore
        VTK::RenderingOpenGL2
        VTK::GUISupportQt
)
