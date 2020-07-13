cmake_minimum_required(VERSION 3.10)

project(Sparc-MPI VERSION 0.1.0 LANGUAGES CXX)

#specify the C++ standard
set(CMAKE_STANDARD 11)
set(CMAKE_STANDARD_REQUIRED True)
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)


option(BUILD_SPARC_MIMIR "build Mimir version of Sparc" ON)
option(BUILD_SPARC_MRMPI "build MR-MPI version of Sparc" ON)
option(BUILD_SPARC_MPI "build pure MPI version of Sparc" ON)

set(LOCAL_EXT_PREFIX_DIR ${PROJECT_SOURCE_DIR}/extlib/local)       
########## external project ##################

include(ExternalProject)

ExternalProject_Add(mrmpilib
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/extlib/mrmpi-7Apr14/src
  INSTALL_DIR ${LOCAL_EXT_PREFIX_DIR}
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  make 
  INSTALL_COMMAND  INSTALL_DIR=<INSTALL_DIR> make install
)
set_target_properties(mrmpilib PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(mimirlib
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/extlib/Mimir/
  INSTALL_DIR ${LOCAL_EXT_PREFIX_DIR}
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  make 
  INSTALL_COMMAND  make install
)
set_target_properties(mimirlib PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(gzstream
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  URL ${PROJECT_SOURCE_DIR}/extlib/gzstream.tgz
  INSTALL_DIR ${LOCAL_EXT_PREFIX_DIR}
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  make 
  INSTALL_COMMAND  "" 
  )
set_target_properties(gzstream PROPERTIES EXCLUDE_FROM_ALL TRUE)


ExternalProject_Add(libzmq
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/zeromq/libzmq.git
  GIT_TAG v4.3.2
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  mkdir -p build && cd build && cmake  -DBUILD_SHARED=OFF -DBUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>  ..  
  INSTALL_COMMAND  cd build && make install 
  )
set_target_properties(libzmq PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(cppzmqlib
  DEPENDS libzmq
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/zeromq/cppzmq.git
  GIT_TAG v4.6.0
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  mkdir -p build && cd build && cmake  -DCPPZMQ_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>  ..  
  INSTALL_COMMAND  cd build && make install 
  )
set_target_properties(cppzmqlib PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(zmqpplib
  DEPENDS libzmq
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/zeromq/zmqpp.git
  GIT_TAG 4.2.0
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  PATCH_COMMAND git apply ${PROJECT_SOURCE_DIR}/patch/zmqpp.patch
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND 
  make CXXFLAGS=-I<INSTALL_DIR>/include   libzmqpp.a   
  INSTALL_COMMAND make install PREFIX=<INSTALL_DIR> BUILD_SHARED=NO 
  )
set_target_properties(zmqpplib PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(snappy
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/google/snappy.git
  GIT_TAG 1.1.8
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> .. && make 
  INSTALL_COMMAND  cd build && make install 
  )
set_target_properties(snappy PROPERTIES EXCLUDE_FROM_ALL TRUE)
  
ExternalProject_Add(leveldblib
  DEPENDS snappy
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/google/leveldb.git
  GIT_TAG 1.22
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  PATCH_COMMAND git apply ${PROJECT_SOURCE_DIR}/patch/leveldb.patch
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
set_target_properties(leveldblib PROPERTIES EXCLUDE_FROM_ALL TRUE)


######### sparc mpi ##########################



configure_file(src/SparcConfig.h.in src/SparcConfig.h)

add_executable(test_utils src/test_utils.cpp)
add_executable(test_kmer src/test_kmer.cpp src/kmer.cpp)

target_include_directories(test_utils PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
target_include_directories(test_kmer PUBLIC
                           "${PROJECT_BINARY_DIR}"
                           )
                           


##############	MRMPI programs	###########################
if(BUILD_SPARC_MRMPI)
	#find_package(Boost 1.52 REQUIRED)
	find_package(MPI 3.0 REQUIRED)
	
	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib" )
	
	add_executable(kmer_counting_mrmpi src/kmer_counting_mrmpi.cpp src/kmer.cpp)
	add_executable(kmer_read_mapping_mrmpi src/kmer_read_mapping_mrmpi.cpp src/kmer.cpp)
	add_executable(edge_generating_mrmpi src/edge_generating_mrmpi.cpp src/kmer.cpp)

	foreach(EXEC kmer_counting_mrmpi kmer_read_mapping_mrmpi edge_generating_mrmpi)
		add_dependencies(${EXEC} mrmpilib)
		target_include_directories(${EXEC} PUBLIC "${LOCAL_EXT_PREFIX_DIR}/include/mrmpi" )
		target_link_libraries(${EXEC} PUBLIC mrmpi)
		target_include_directories(${EXEC} PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
	    target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
	endforeach(EXEC)
	
endif()                                

##############	MIMIR programs	###########################
if(BUILD_SPARC_MIMIR)
	
	find_package(MPI 3.0 REQUIRED)
	
	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib" )
	
	add_executable(kmer_counting_mimir src/kmer_counting_mimir.cpp src/kmer.cpp)
	add_executable(kmer_read_mapping_mimir src/kmer_read_mapping_mimir.cpp src/kmer.cpp)
	add_executable(edge_generating_mimir src/edge_generating_mimir.cpp src/kmer.cpp)

	foreach(EXEC kmer_counting_mimir kmer_read_mapping_mimir edge_generating_mimir)
		add_dependencies(${EXEC} mimirlib)
		target_include_directories(${EXEC} PUBLIC "${LOCAL_EXT_PREFIX_DIR}/include" )
		target_link_libraries(${EXEC} PUBLIC mimir)
		target_include_directories(${EXEC} PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
	    target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
	endforeach(EXEC)
endif()                                              


############# mpi programs ###########################
if(BUILD_SPARC_MIMIR)
	find_package(MPI 3.0 REQUIRED)
	
	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib" )
	
	add_executable(edge_generating_mpi src/edge_generating_mpi.cpp src/kmer.cpp)

	foreach(EXEC edge_generating_mpi)
		add_dependencies(${EXEC} leveldblib cppzmqlib zmqpplib)
		target_include_directories(${EXEC} PUBLIC "${LOCAL_EXT_PREFIX_DIR}/include" )
		target_include_directories(${EXEC} PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
		target_link_libraries(${EXEC} PUBLIC zmq leveldb)
	    target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
	endforeach(EXEC)
endif()
