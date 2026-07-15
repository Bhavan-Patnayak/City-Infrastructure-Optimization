cmake_minimum_required(VERSION 3.15)
project(CityNetworkDesign VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Auto-download the nlohmann_json library from GitHub
include(FetchContent)
FetchContent_Declare(json 
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

include_directories(src)

# Add our source files
add_executable(network_designer 
    src/main.cpp
    src/graph/Graph.cpp
)

# Link the JSON library to our program
target_link_libraries(network_designer PRIVATE nlohmann_json::nlohmann_json)