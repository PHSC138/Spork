project(scaling-spork)

cmake_minimum_required(VERSION 3.0)
include(FindPkgConfig)
pkg_check_modules(CURLPP REQUIRED curlpp)

set(CMAKE_CXX_STANDARD 17) # C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
# set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11


file(GLOB ExamplesFileList "*example*.cpp")

# Create a meta target to create examples
add_custom_target(build_all_examples COMMENT A target that requires all the examples.)

message(STATUS "Creating build rules for Examples:")
foreach(Example ${ExamplesFileList})
  message(STATUS "\tCreating build rule for ${Example}")
  # Get the name without extension or directory
  get_filename_component(ExampleName ${Example} NAME_WE)
  # Define example executable
  add_executable(${ExampleName} ${Example})
  # Link example against curlpp
  target_link_libraries(${ExampleName} ${CURLPP_LDFLAGS})
  # make the meta target depend on this example.
  add_dependencies(build_all_examples ${ExampleName})
endforeach(Example ${ExamplesFileList})

# Create a meta target to create scaling-spork
add_custom_target(build_scaling-spork COMMENT A target that builds scaling-spork.)

message(STATUS "Creating build rules for Scaling-Spork:")
# Define example executable
add_executable(scaling-spork scaling-spork.cpp)
# Link example against curlpp
target_link_libraries(scaling-spork ${CURLPP_LDFLAGS})
# make the meta target depend on this example.
add_dependencies(build_scaling-spork scaling-spork)
