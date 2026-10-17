cmake_minimum_required(VERSION 3.21)
project(ShopConsole VERSION 1.7.0 LANGUAGES CXX)

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
    src/CheckoutPage.h
    src/CheckoutPage.cpp
    src/OverviewPage.h
    src/OverviewPage.cpp
    src/OnlineOrdersPage.h
    src/OnlineOrdersPage.cpp
    src/ShopTime.h
    src/ApplicationsPage.h
    src/ApplicationsPage.cpp
    src/FlowLayout.h
    src/FlowLayout.cpp
    src/PosWindow.h
    src/PosWindow.cpp
    src/DiscountDialog.h
    src/DiscountDialog.cpp
    src/PaymentDialog.h
    src/PaymentDialog.cpp
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
    src/HeldSalesDialog.h
    src/HeldSalesDialog.cpp
    src/DailySummaryPage.h
    src/DailySummaryPage.cpp
    src/AuditLogPage.h
    src/AuditLogPage.cpp
    src/CategoriesPage.h
    src/CategoriesPage.cpp
    src/CustomOrdersPage.h
    src/CustomOrdersPage.cpp
    src/MessagesPage.h
    src/MessagesPage.cpp
)

qt_add_resources(ShopConsole "icons"
    PREFIX "/icons"
    BASE "resources/icons"
    FILES
        resources/icons/inventory.svg
        resources/icons/employees.svg
        resources/icons/salary.svg
        resources/icons/daily-summary.svg
        resources/icons/audit-log.svg
        resources/icons/software.svg
        resources/icons/categories.svg
        resources/icons/orders.svg
        resources/icons/help.svg
        resources/icons/checkout.svg
        resources/icons/overview.svg
        resources/icons/online-orders.svg
        resources/icons/applications.svg
)

target_link_libraries(ShopConsole PRIVATE Qt6::Widgets Qt6::Network Qt6::Svg)
# Publishable key only, never the secret key (it bypasses RLS). Empty = read .env.
set(BUILTIN_SUPABASE_URL "" CACHE STRING "Supabase URL compiled into the app")
set(BUILTIN_SUPABASE_ANON_KEY "" CACHE STRING "Supabase publishable (anon) key compiled into the app")
if(BUILTIN_SUPABASE_ANON_KEY MATCHES "^sb_secret_")
    message(FATAL_ERROR "BUILTIN_SUPABASE_ANON_KEY is a secret key — use the publishable key.")
endif()

# Release builds pass the full tag version (e.g. 1.6.0-rc1) so a pre-release
# knows it's older than the final 1.6.0; local builds use the project version.
if(NOT APP_VERSION_LABEL)
    set(APP_VERSION_LABEL "${PROJECT_VERSION}")
endif()

target_compile_definitions(ShopConsole PRIVATE
    APP_VERSION="${APP_VERSION_LABEL}"
    BUILTIN_SUPABASE_URL="${BUILTIN_SUPABASE_URL}"
    BUILTIN_SUPABASE_ANON_KEY="${BUILTIN_SUPABASE_ANON_KEY}"
)

# update.ps1 must sit next to the exe for self-update.
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
