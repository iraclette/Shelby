cmake_minimum_required(VERSION 3.21)
project(ShopConsole VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)
qt_standard_project_setup()

qt_add_executable(ShopConsole WIN32
    src/main.cpp
    src/Config.h
    src/Config.cpp
    src/SupabaseClient.h
    src/SupabaseClient.cpp
    src/LoginWindow.h
    src/LoginWindow.cpp
    src/AdminWindow.h
    src/AdminWindow.cpp
    src/PosWindow.h
    src/PosWindow.cpp
    src/ProductDialog.h
    src/ProductDialog.cpp
    src/ProductPhotosDialog.h
    src/ProductPhotosDialog.cpp
    src/UpdateChecker.h
    src/UpdateChecker.cpp
)

target_link_libraries(ShopConsole PRIVATE Qt6::Widgets Qt6::Network)
target_compile_definitions(ShopConsole PRIVATE APP_VERSION="${PROJECT_VERSION}")

install(TARGETS ShopConsole
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

qt_generate_deploy_app_script(
    TARGET ShopConsole
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
