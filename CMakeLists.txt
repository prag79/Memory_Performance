cmake_minimum_required(VERSION 3.0)
project(TeraSMemoryChip)

find_package(SystemCLanguage CONFIG REQUIRED)
set (CMAKE_CXX_STANDARD ${SystemC_CXX_STANDARD})

add_executable(test_systemc signal_example.cpp)
target_link_libraries(test_systemc SystemC::systemc)
include_directories(/opt/systemc/include
	            ./)
