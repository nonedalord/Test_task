cmake_minimum_required(VERSION 3.22)

project(CSVParser)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.17.0
)
FetchContent_MakeAvailable(spdlog)

file(GLOB src "src/*.cpp")
file(GLOB logger "src/logger/*.cpp")

add_executable(CVSParser ${src} ${logger})
target_link_libraries(CVSParser PRIVATE spdlog::spdlog)