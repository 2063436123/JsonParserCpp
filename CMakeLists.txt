cmake_minimum_required(VERSION 3.16)
project(JSONParser)

set(CMAKE_CXX_STANDARD 20)

add_executable(JSONParser main.cpp JsonParser.cpp JsonParserImpl.cpp)
target_link_libraries(JSONParser gtest pthread)