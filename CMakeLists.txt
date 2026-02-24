cmake_minimum_required(VERSION 3.23)

project(CSVParser)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.17.0
)

set(BOOST_INCLUDE_LIBRARIES program_options accumulators)
set(BOOST_ENABLE_CMAKE ON)
FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.7z
    USES_TERMINAL_DOWNLOAD TRUE
    DOWNLOAD_NO_EXTRACT FALSE
)

FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)

set(BUILD_GMOCK ON CACHE BOOL "Build gmock" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "Disable install of googletest" FORCE)

FetchContent_MakeAvailable(spdlog Boost tomlplusplus googletest)

file(GLOB src "src/*.cpp")
file(GLOB logger "src/logger/*.cpp")
file(GLOB parse "src/csv_parser/*.cpp")
file(GLOB out_writer "src/out_writer/*.cpp")
file(GLOB conf_reader "src/config_reader/*.cpp")

add_executable(CSVParser ${src} ${logger} ${parse} ${out_writer} ${conf_reader})

target_link_libraries(CSVParser PRIVATE
    spdlog::spdlog
    Boost::program_options
    Boost::accumulators
    tomlplusplus::tomlplusplus
)

if (BUILD_TESTING)
    enable_testing()
    file(GLOB tests_src "tests/*.cpp")
    add_executable(CSVParserTests ${tests_src} ${out_writer} ${parse} ${logger})
    target_link_libraries(CSVParserTests
        PRIVATE
        gtest
        gtest_main
        gmock
        gmock_main
        Boost::accumulators
        spdlog::spdlog
    )
    add_test(NAME CSVParserTests COMMAND CSVParserTests)
endif()
