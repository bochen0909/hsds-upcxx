cmake_minimum_required(VERSION 3.10)

project(Sparc-MPI VERSION 0.1.0 LANGUAGES CXX)

#specify the C++ standard
set(CMAKE_STANDARD 11)
set(CMAKE_STANDARD_REQUIRED True)
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set (MIMIR_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extlib/Mimir/src)
configure_file(SparcConfig.h.in SparcConfig.h)

add_executable(test_utils src/test_utils.cpp)
add_executable(test_kmer src/test_kmer.cpp src/kmer.cpp)

add_executable(kmer_counting src/kmer_counting.cpp src/kmer.cpp)

target_include_directories(test_utils PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
target_include_directories(test_kmer PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
                           
find_package(Boost 1.52 REQUIRED)
                          
find_package(MPI 3.0 REQUIRED)

if(MPI_FOUND)
	find_library(MIMIR_LIBRARY mimir HINTS  ${MIMIR_SOURCE_DIR})
endif()

if(MPI_FOUND)
	target_include_directories(kmer_counting PUBLIC "${MIMIR_SOURCE_DIR}" )
	target_include_directories(kmer_counting PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
	target_link_libraries(kmer_counting PUBLIC ${MIMIR_LIBRARY})
    target_link_libraries(kmer_counting PUBLIC ${MPI_CXX_LIBRARIES})
endif()                                              

