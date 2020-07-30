cmake_minimum_required(VERSION 3.10)

project(Sparc-MPI VERSION 0.1.0 LANGUAGES CXX)

#specify the C++ standard
set(CMAKE_STANDARD 11)
set(CMAKE_STANDARD_REQUIRED True)
set (CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s")


option(BUILD_SPARC_MIMIR "build Mimir version of Sparc" ON)
option(BUILD_SPARC_MRMPI "build MR-MPI version of Sparc" ON)
option(BUILD_SPARC_MPI "build pure MPI version of Sparc" ON)
option(BUILD_SPARC_UPCXX "build UPCXX version of Sparc" ON)

option(BUILD_WITH_LEVELDB "build MPI version of Sparc with leveldb support" OFF)
option(BUILD_WITH_MPIClient "build MPI version of Sparc with mpi client" OFF)

set(LOCAL_EXT_PREFIX_DIR ${PROJECT_SOURCE_DIR}/extlib/local)       

add_custom_target(sparc_mimir ALL)
add_custom_target(sparc_mrmpi ALL)
add_custom_target(sparc_mpi ALL)
add_custom_target(sparc_upcxx ALL)

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
  CONFIGURE_COMMAND  ./autogen.sh 
  BUILD_COMMAND  ./configure --prefix <INSTALL_DIR> && make 
  INSTALL_COMMAND  make install
)
set_target_properties(mimirlib PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(gzstreamlib
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  URL ${PROJECT_SOURCE_DIR}/extlib/gzstream.tgz
  INSTALL_DIR ${LOCAL_EXT_PREFIX_DIR}
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  make 
  INSTALL_COMMAND  mkdir -p <INSTALL_DIR>/include && mkdir -p <INSTALL_DIR>/lib && cp gzstream.h <INSTALL_DIR>/include && cp libgzstream.a <INSTALL_DIR>/lib
  )
set_target_properties(gzstreamlib PROPERTIES EXCLUDE_FROM_ALL TRUE)


ExternalProject_Add(spdloglib
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.7.0
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>  ..  
  INSTALL_COMMAND  cd build && make install 
  )
set_target_properties(spdloglib PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(tsl_robin_map_lib
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/Tessil/robin-map.git
  GIT_TAG v0.6.3
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>  ..  
  INSTALL_COMMAND  cd build && make install 
  )
set_target_properties(tsl_robin_map_lib PROPERTIES EXCLUDE_FROM_ALL TRUE)

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

ExternalProject_Add(snappylib
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/google/snappy.git
  GIT_TAG 1.1.8
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  mkdir -p build && cd build && cmake -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> .. && make 
  INSTALL_COMMAND  cd build && make install 
  )
set_target_properties(snappylib PROPERTIES EXCLUDE_FROM_ALL TRUE)
  
ExternalProject_Add(lz4lib
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  GIT_REPOSITORY https://github.com/lz4/lz4.git
  GIT_TAG v1.9.2
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND  ""  
  INSTALL_COMMAND  cd lib && make install BUILD_SHARED=no PREFIX=<INSTALL_DIR> 
  )
set_target_properties(lz4lib PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(rocksdblib
  DEPENDS lz4lib snappylib
  PREFIX ${PROJECT_SOURCE_DIR}/extlib
  URL  https://github.com/facebook/rocksdb/archive/v6.10.2.tar.gz
  URL_MD5 f1c5debbb2bdaaf275309e1d78839fb2
  INSTALL_DIR ${PROJECT_SOURCE_DIR}/extlib/local
  BUILD_IN_SOURCE 1
  CONFIGURE_COMMAND  "" 
  BUILD_COMMAND
  test -f DONE ||  
  ROCKSDB_DISABLE_BZIP=1 ROCKSDB_DISABLE_ZLIB=1 
  CFLAGS=-I<INSTALL_DIR>/include PLATFORM_LDFLAGS=-L<INSTALL_DIR>/lib
  make static_lib AM_DEFAULT_VERBOSITY=1 -j4  && touch DONE
  INSTALL_COMMAND  test -f DONE2 || make install-static INSTALL_PATH=<INSTALL_DIR> && touch DONE2 
  )
set_target_properties(rocksdblib PROPERTIES EXCLUDE_FROM_ALL TRUE)

ExternalProject_Add(leveldblib
  DEPENDS snappylib
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
  -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
  -DCMAKE_CC_COMPILER=${CMAKE_CC_COMPILER}
  -DLEVELDB_BUILD_TESTS=OFF
  -DLEVELDB_BUILD_BENCHMARKS=OFF
  -DLOCAL_PREFIX_PATH=<INSTALL_DIR>
  -DCMAKE_EXE_LINKER_FLAGS=-L<INSTALL_DIR>/lib64
  -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR> .. && make 
  INSTALL_COMMAND  cd build && make install 
  )
set_target_properties(leveldblib PROPERTIES EXCLUDE_FROM_ALL TRUE)


######### sparc mpi ##########################



configure_file(src/SparcConfig.h.in src/SparcConfig.h)

add_executable(test_utils src/test_utils.cpp src/sparc/utils.cpp)
add_executable(test_kmer src/test_kmer.cpp src/kmer.cpp src/sparc/utils.cpp)

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
	
	add_executable(kmer_counting_mrmpi src/kmer_counting_mrmpi.cpp src/kmer.cpp src/sparc/utils.cpp)
	add_executable(kmer_read_mapping_mrmpi src/kmer_read_mapping_mrmpi.cpp src/kmer.cpp src/sparc/utils.cpp)
	add_executable(edge_generating_mrmpi src/edge_generating_mrmpi.cpp src/kmer.cpp src/sparc/utils.cpp)

	foreach(EXEC kmer_counting_mrmpi kmer_read_mapping_mrmpi edge_generating_mrmpi)
		add_dependencies(${EXEC} mrmpilib)
		target_include_directories(${EXEC} PUBLIC "${LOCAL_EXT_PREFIX_DIR}/include/mrmpi" )
		target_link_libraries(${EXEC} PUBLIC mrmpi)
		target_include_directories(${EXEC} PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
	    target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
	    set_target_properties(${EXEC} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	    add_dependencies(sparc_mrmpi ${EXEC})
	endforeach(EXEC)
	
endif()                                

##############	MIMIR programs	###########################
if(BUILD_SPARC_MIMIR)
	
	find_package(MPI 3.0 REQUIRED)
	
	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib" )
	
	add_executable(kmer_counting_mimir src/kmer_counting_mimir.cpp src/kmer.cpp src/sparc/utils.cpp)
	add_executable(kmer_read_mapping_mimir src/kmer_read_mapping_mimir.cpp src/kmer.cpp src/sparc/utils.cpp)
	add_executable(edge_generating_mimir src/edge_generating_mimir.cpp src/kmer.cpp src/sparc/utils.cpp)
	
	foreach(EXEC kmer_counting_mimir kmer_read_mapping_mimir edge_generating_mimir)
		add_dependencies(${EXEC} mimirlib)
		target_include_directories(${EXEC} PUBLIC "${LOCAL_EXT_PREFIX_DIR}/include" )
		target_link_libraries(${EXEC} PUBLIC mimir)
		target_include_directories(${EXEC} PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
	    target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
	    set_target_properties(${EXEC} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	    add_dependencies(sparc_mimir ${EXEC})
	endforeach(EXEC)
endif()                                              


############# mpi programs ###########################
if(BUILD_SPARC_MIMIR)
	find_package(MPI 3.0 REQUIRED)
	find_package(ZLIB REQUIRED)
	find_package(Threads REQUIRED)

	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib" )
	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib64" )

	add_executable(test_lz4string src/test_lz4string.cpp src/sparc/LZ4String.cpp src/sparc/Message.cpp)
	target_include_directories(test_lz4string PUBLIC "${LOCAL_EXT_PREFIX_DIR}/include" )
	target_link_libraries(test_lz4string PUBLIC lz4 zmqpp zmq Threads::Threads)

	add_executable(test_check_input_files src/test_check_input_files.cpp src/sparc/utils.cpp src/kmer.cpp src/sparc/log.cpp)
	target_include_directories(test_check_input_files PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
	target_include_directories(test_check_input_files PUBLIC "${LOCAL_EXT_PREFIX_DIR}/include" )
	target_link_libraries(test_check_input_files PUBLIC spdlog ${MPI_CXX_LIBRARIES})

	
	add_executable(kmer_counting_mpi 
		src/kmer_counting_mpi.cpp 
		src/kmer.cpp src/sparc/log.cpp 
		src/sparc/KmerCountingListener.cpp
		src/sparc/KmerCountingClient.cpp
		src/sparc/AbstractClient.cpp
		src/sparc/AbstractListener.cpp
		src/sparc/Message.cpp
		src/sparc/DBHelper.cpp
		src/sparc/LZ4String.cpp
		src/sparc/utils.cpp
		)
	add_executable(kmer_read_mapping_mpi 
		src/kmer_read_mapping_mpi.cpp 
		src/kmer.cpp src/sparc/log.cpp 
		src/sparc/KmerCountingListener.cpp
		src/sparc/KmerCountingClient.cpp
		src/sparc/AbstractClient.cpp
		src/sparc/AbstractListener.cpp		
		src/sparc/DBHelper.cpp
		src/sparc/Message.cpp
		src/sparc/LZ4String.cpp
		src/sparc/utils.cpp
		)	
		
	add_executable(edge_generating_mpi 
		src/edge_generating_mpi.cpp 
		src/kmer.cpp src/sparc/log.cpp 
		src/sparc/EdgeGeneratingListener.cpp
		src/sparc/EdgeGeneratingClient.cpp
		src/sparc/AbstractClient.cpp		
		src/sparc/AbstractListener.cpp
		src/sparc/DBHelper.cpp
		src/sparc/LZ4String.cpp
		src/sparc/Message.cpp
		src/sparc/utils.cpp
		)	

	add_executable(merge_intrmed_mpi 
		src/merge_intrmed_mpi.cpp
		src/sparc/MergeClient.cpp
		src/sparc/MergeListener.cpp		 
		src/sparc/KmerCountingListener.cpp
		src/sparc/KmerCountingClient.cpp
		src/sparc/AbstractListener.cpp
		src/sparc/AbstractClient.cpp
		src/sparc/DBHelper.cpp
		src/sparc/LZ4String.cpp
		src/sparc/Message.cpp
		src/sparc/utils.cpp
		src/kmer.cpp src/sparc/log.cpp		
		)	

	add_executable(lpav1_mpi 
		src/lpav1_mpi.cpp
		src/sparc/LPAState.cpp
		src/sparc/LPAClient.cpp
		src/sparc/AbstractClient.cpp
		src/sparc/AbstractListener.cpp
		src/sparc/LPAListener.cpp
		src/sparc/DBHelper.cpp
		src/sparc/Message.cpp
		src/sparc/LZ4String.cpp
		src/sparc/utils.cpp
		src/kmer.cpp src/sparc/log.cpp		
		)	
					
					
					
	foreach(EXEC kmer_counting_mpi kmer_read_mapping_mpi edge_generating_mpi merge_intrmed_mpi lpav1_mpi)
		if(BUILD_WITH_MPIClient)
			target_sources(${EXEC} PRIVATE src/sparc/MPIClient.cpp src/sparc/MPIListener.cpp)
			target_compile_definitions(${EXEC} PUBLIC "-DUSE_MPICLIENT")
		else()
			target_sources( ${EXEC} PRIVATE src/sparc/ZMQClient.cpp src/sparc/ZMQListener.cpp )
		endif()
		if(BUILD_WITH_LEVELDB)
			target_sources( ${EXEC} src/sparc/LevelDBHelper.cpp)
			add_dependencies(${EXEC} leveldblib)
			target_link_libraries(${EXEC} PUBLIC leveldb snappy)
			set_target_properties(${EXEC} PROPERTIES COMPILE_FLAGS "-DBUILD_WITH_LEVELDB")
		endif()
		add_dependencies(${EXEC} lz4lib zmqpplib spdloglib gzstreamlib)
		target_include_directories(${EXEC} PUBLIC "${LOCAL_EXT_PREFIX_DIR}/include" )
		target_include_directories(${EXEC} PUBLIC "${MPI_CXX_INCLUDE_DIRS}" )
		target_link_libraries(${EXEC} PUBLIC zmqpp zmq lz4 spdlog gzstream ZLIB::ZLIB Threads::Threads dl)
	    target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
	    set_target_properties(${EXEC} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	    add_dependencies(sparc_mpi ${EXEC})
	endforeach(EXEC)
	
	#some specials
	#set_target_properties(merge_intrmed_mpi PROPERTIES COMPILE_OPTIONS -DUSE_TSL_ROBIN_MAP )
	target_compile_definitions(merge_intrmed_mpi PUBLIC "-DUSE_TSL_ROBIN_MAP")
	add_dependencies(merge_intrmed_mpi tsl_robin_map_lib)
	
	
endif()





############# upcxx programs ###########################
if(BUILD_SPARC_UPCXX)
	############# check UPCXX ##############################
	set (UPCXX_VERBOSE ON)
	include(cmake/UPCXXConfig.cmake)
	if (NOT EXISTS "${UPCXX_META_EXECUTABLE}")
	   message( FATAL_ERROR "cannot find upcxx" )
	endif()
	############# end check UPCXX ##############################
	
	find_package(ZLIB REQUIRED)

	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib" )
	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib64" )

	add_executable(edge_generating_upc 
		src/edge_generating_upc.cpp 
		src/kmer.cpp src/sparc/log.cpp src/sparc/utils.cpp
		)

	add_executable(kmer_counting_upc 
		src/kmer_counting_upc.cpp 
		src/kmer.cpp src/sparc/log.cpp src/sparc/utils.cpp
		)
	
	add_executable(kmer_read_mapping_upc 
		src/kmer_read_mapping_upc.cpp 
		src/kmer.cpp src/sparc/log.cpp src/sparc/utils.cpp
		)

	add_executable(lpav1_upc 
		src/lpav1_upc.cpp 
		src/kmer.cpp src/sparc/log.cpp src/sparc/utils.cpp
		)

						
	foreach(EXEC edge_generating_upc kmer_counting_upc kmer_read_mapping_upc lpav1_upc)
		add_dependencies(${EXEC} spdloglib gzstreamlib)
		target_include_directories(${EXEC} PUBLIC ${UPCXX_INCLUDE_DIRS} "${LOCAL_EXT_PREFIX_DIR}/include" )
		target_compile_definitions(${EXEC} PUBLIC ${UPCXX_DEFINITIONS})
		target_compile_definitions(merge_intrmed_mpi PUBLIC "-DUSE_UPCXXCLIENT")
		target_link_libraries(${EXEC} PUBLIC spdlog gzstream ZLIB::ZLIB ${UPCXX_LIBRARIES})
	    set_target_properties(${EXEC} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	    add_dependencies(sparc_upcxx ${EXEC})
	endforeach(EXEC)
	
endif()




