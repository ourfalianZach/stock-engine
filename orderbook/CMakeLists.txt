cmake_minimum_required(VERSION 3.20) # Minimum required CMake version
project(orderbook CXX) # Project name and language
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20) # Use C++20 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Enforce C++20 standard

add_executable(orderbook
  src/main.cpp
  src/order_book.cpp
) # Define the executable target named 'orderbook'

target_include_directories(orderbook PRIVATE include)
target_include_directories(orderbook PRIVATE include) # Include directory for header files

# Helpful warnings (works on clang/gcc)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(orderbook PRIVATE -Wall -Wextra -Wpedantic)
endif()