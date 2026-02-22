cmake_minimum_required(VERSION 3.22)

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

FetchContent_MakeAvailable(spdlog Boost)

file(GLOB src "src/*.cpp")
file(GLOB logger "src/logger/*.cpp")
file(GLOB parse "src/csv_parser/*.cpp")
file(GLOB out_writer "src/out_writer/*.cpp")

add_executable(CVSParser ${src} ${logger} ${parse} ${out_writer})
target_link_libraries(CVSParser PRIVATE spdlog::spdlog Boost::program_options Boost::accumulators)