cmake_minimum_required(VERSION 3.10)

project(Sparc-MPI VERSION 0.1.0 LANGUAGES CXX)

#specify the C++ standard
set(CMAKE_STANDARD 11)
set(CMAKE_STANDARD_REQUIRED True)
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set (MIMIR_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extlib/Mimir/src)
set (MRMPI_SOURCE_DIR ${PROJECT_SOURCE_DIR}/extlib/mrmpi-7Apr14/src)

configure_file(src/SparcConfig.h.in src/SparcConfig.h)

add_executable(test_utils src/test_utils.cpp)
add_executable(test_kmer src/test_kmer.cpp src/kmer.cpp)

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
	find_library(MRMPI_LIBRARY mrmpi HINTS  ${MRMPI_SOURCE_DIR})
endif()

if(NOT MIMIR_LIBRARY AND NOT MRMPI_LIBRARY)
  message( FATAL_ERROR "Can find neither Mimir or MR-MPI" )
endif()

##############	MRMPI programs	###########################
if(MRMPI_LIBRARY)
	add_executable(kmer_counting_mrmpi src/kmer_counting_mrmpi.cpp src/kmer.cpp)
	add_executable(kmer_read_mapping_mrmpi src/kmer_read_mapping_mrmpi.cpp src/kmer.cpp)
	add_executable(edge_generating_mrmpi src/edge_generating_mrmpi.cpp src/kmer.cpp)

endif()

if(MRMPI_LIBRARY)
	foreach(EXEC kmer_counting_mrmpi kmer_read_mapping_mrmpi edge_generating_mrmpi)
		target_include_directories(${EXEC} PUBLIC "${MRMPI_SOURCE_DIR}" )
		target_link_libraries(${EXEC} PUBLIC ${MRMPI_LIBRARY})
		target_include_directories(${EXEC} PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
	    target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
	endforeach(EXEC)
	
endif()                                              

##############	MIMIR programs	###########################
if(MIMIR_LIBRARY)
	add_executable(kmer_counting_mimir src/kmer_counting_mimir.cpp src/kmer.cpp)
	add_executable(kmer_read_mapping_mimir src/kmer_read_mapping_mimir.cpp src/kmer.cpp)
	add_executable(edge_generating_mimir src/edge_generating_mimir.cpp src/kmer.cpp)
endif()

if(MIMIR_LIBRARY)
	foreach(EXEC kmer_counting_mimir kmer_read_mapping_mimir edge_generating_mimir)
		target_include_directories(${EXEC} PUBLIC "${MIMIR_SOURCE_DIR}" )
		target_link_libraries(${EXEC} PUBLIC ${MIMIR_LIBRARY})
		target_include_directories(${EXEC} PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
	    target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
	endforeach(EXEC)
	
endif()                                              


############# mpi programs ###########################
include(ExternalProject)

ExternalProject_Add(snappy
  PREFIX ${Sparc-MPI_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/google/snappy.git
  GIT_TAG 1.1.8
  INSTALL_DIR ${Sparc-MPI_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> .. && make 
  INSTALL_COMMAND  cd build && make install 
  )
  
ExternalProject_Add(leveldb
  DEPENDS snappy
  PREFIX ${Sparc-MPI_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/google/leveldb.git
  GIT_TAG 1.22
  INSTALL_DIR ${Sparc-MPI_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  PATCH_COMMAND git apply ${Sparc-MPI_SOURCE_DIR}/patch/leveldb.patch
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  
  mkdir -p build && cd build && 
  cmake -DCMAKE_BUILD_TYPE=Release
  -DLEVELDB_BUILD_TESTS=OFF
  -DLEVELDB_BUILD_BENCHMARKS=OFF
  -DLOCAL_PREFIX_PATH=<INSTALL_DIR>
  -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> .. && make 
  INSTALL_COMMAND  cd build && make install 
  )
