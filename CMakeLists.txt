project(spork)

cmake_minimum_required(VERSION 3.0)
include(FindPkgConfig)
pkg_check_modules(CURLPP REQUIRED curlpp)
find_package(Threads REQUIRED)

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

# Create a meta target to create spork
add_custom_target(build_spork COMMENT A target that builds spork.)

message(STATUS "Creating build rules for Spork:")
set(THREADS_PREFER_PTHREAD_FLAG ON)
# target_link_libraries(spork PRIVATE Threads::Threads)
# Define example executable
add_executable(spork spork.cpp statelist.cpp)
# Link example against curlpp
target_link_libraries(spork ${CURLPP_LDFLAGS})
# Uncomment this for debugging
# set(CMAKE_BUILD_TYPE Debug)
# make the meta target depend on this example.
add_dependencies(build_spork spork)
