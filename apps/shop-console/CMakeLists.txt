cmake_minimum_required(VERSION 3.21)
project(ShopConsole VERSION 1.2.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Svg)
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
    src/DiscountDialog.h
    src/DiscountDialog.cpp
    src/ProductDialog.h
    src/ProductDialog.cpp
    src/ProductPhotosDialog.h
    src/ProductPhotosDialog.cpp
    src/UpdateChecker.h
    src/UpdateChecker.cpp
    src/InventoryPage.h
    src/InventoryPage.cpp
    src/EmployeesPage.h
    src/EmployeesPage.cpp
    src/EmployeeDialog.h
    src/EmployeeDialog.cpp
    src/SalaryPage.h
    src/SalaryPage.cpp
    src/PaySalaryDialog.h
    src/PaySalaryDialog.cpp
    src/SoftwarePage.h
    src/SoftwarePage.cpp
    src/ReturnDialog.h
    src/ReturnDialog.cpp
    src/TransferStockDialog.h
    src/TransferStockDialog.cpp
)

qt_add_resources(ShopConsole "icons"
    PREFIX "/icons"
    BASE "resources/icons"
    FILES
        resources/icons/inventory.svg
        resources/icons/employees.svg
        resources/icons/salary.svg
        resources/icons/software.svg
)

target_link_libraries(ShopConsole PRIVATE Qt6::Widgets Qt6::Network Qt6::Svg)
target_compile_definitions(ShopConsole PRIVATE APP_VERSION="${PROJECT_VERSION}")

# UpdateChecker::downloadAndInstall() looks for this next to the exe — kept
# next to the build output locally too so the two never drift apart, even
# though exercising the self-update flow for real needs an actual GitHub
# release (a signed asset URL), not just a local build.
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scripts/update.ps1 DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

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
