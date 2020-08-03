cmake_minimum_required(VERSION 3.10)

project(Sparc-MPI VERSION 0.1.0 LANGUAGES CXX)

#specify the C++ standard
set(CMAKE_STANDARD 11)
set(CMAKE_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -s")


option(BUILD_SPARC_MIMIR "build Mimir version of Sparc" OFF)
option(BUILD_SPARC_MRMPI "build MR-MPI version of Sparc" OFF)
option(BUILD_SPARC_MPI "build pure MPI version of Sparc" OFF)
option(BUILD_SPARC_UPCXX "build UPCXX version of Sparc" OFF)
option(BUILD_SPARC_BCL "build bcl version of Sparc" OFF)

option(BUILD_WITH_LEVELDB "build MPI version of Sparc with leveldb support" OFF)
option(BUILD_WITH_MPIClient "build MPI version of Sparc with mpi client" OFF)

if(NOT DEFINED BCL_BACKEND)
	set(BCL_BACKEND gasnet_ex) #mpi, shmem, gasnet_ex
endif()

set(LOCAL_EXT_PREFIX_DIR ${PROJECT_SOURCE_DIR}/extlib/local)       

add_custom_target(sparc_mimir)
add_custom_target(sparc_mrmpi)
add_custom_target(sparc_mpi)
add_custom_target(sparc_upcxx)
add_custom_target(sparc_bcl)

include(cmake/CMakefile.external)

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
if(BUILD_SPARC_MPI)
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
	
	find_package(MPI 3.0 REQUIRED)
	find_package(ZLIB REQUIRED)

	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib" )
	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib64" )

	add_executable(edge_generating_upcxx 
		src/edge_generating_upcxx.cpp 
		src/kmer.cpp src/sparc/log.cpp src/sparc/utils.cpp
		)

	add_executable(kmer_counting_upcxx 
		src/kmer_counting_upcxx.cpp 
		src/kmer.cpp src/sparc/log.cpp src/sparc/utils.cpp
		)
	
	add_executable(kmer_read_mapping_upcxx 
		src/kmer_read_mapping_upcxx.cpp 
		src/kmer.cpp src/sparc/log.cpp src/sparc/utils.cpp
		)

	add_executable(lpav1_upcxx
		src/lpav1_upcxx.cpp 
		src/kmer.cpp src/sparc/log.cpp src/sparc/utils.cpp
		)

						
	foreach(EXEC edge_generating_upcxx kmer_counting_upcxx kmer_read_mapping_upcxx lpav1_upcxx)
		add_dependencies(${EXEC} spdloglib gzstreamlib)
		target_include_directories(${EXEC} PUBLIC ${UPCXX_INCLUDE_DIRS} "${LOCAL_EXT_PREFIX_DIR}/include" )
		target_compile_definitions(${EXEC} PUBLIC ${UPCXX_DEFINITIONS})
		target_compile_definitions(${EXEC} PUBLIC "-DUSE_UPCXXCLIENT")
        target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
		target_link_libraries(${EXEC} PUBLIC spdlog gzstream ZLIB::ZLIB ${UPCXX_LIBRARIES})
	    set_target_properties(${EXEC} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	    add_dependencies(sparc_upcxx ${EXEC})
	endforeach(EXEC)
	
endif()


############# bcl programs ###########################

if(BUILD_SPARC_BCL)
	string( TOLOWER "${BCL_BACKEND}" BCL_BACKEND )

	message("using backend ${BCL_BACKEND}")
	
	find_package(Threads REQUIRED)
	set(BCL_DEPENDIES bcllib)
	
	set (UPCXX_VERBOSE ON)
	include(cmake/UPCXXConfig.cmake)
	if (NOT EXISTS "${UPCXX_META_EXECUTABLE}")
	   message( FATAL_ERROR "cannot find upcxx" )
	endif()
	

	if(BCL_BACKEND STREQUAL "shmem")
		set(BCL_DEFINITIONS ${BCL_DEFINITIONS} -DGASNET_EX -DGASNET_SEQ)
		set(BCL_LIBRARIES shmem shcoll shmemc-ucx shmemt shmemu pmix ucp)
		link_directories(${EXEC} PUBLIC "$ENV{HOME}/local2/lib" )
		include_directories(${EXEC} PUBLIC "$ENV{HOME}/local2/include" )
		set(BCL_DEFINITIONS ${BCL_DEFINITIONS} -DSHMEM)
	elseif(BCL_BACKEND STREQUAL "gasnet_ex")
		set(BCL_INCLUDED_DIR ${UPCXX_INCLUDE_DIRS})
		set(BCL_DEFINITIONS ${BCL_DEFINITIONS} -DGASNET_EX -DGASNET_SEQ)
		set(BCL_LIBRARIES ${UPCXX_LIBRARIES})
	elseif(BCL_BACKEND STREQUAL "mpi")
		set(BCL_INCLUDED_DIR ${MPI_CXX_INCLUDE_DIRS})
	else()
		message( FATAL_ERROR "unknown backend ${BCL_BACKEND}" )
	endif()
	
	find_package(MPI 3.0 REQUIRED)
	find_package(ZLIB REQUIRED)

	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib" )
	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib64" )

	set(NAME "kmer_counting_bcl_${BCL_BACKEND}")
	add_executable( ${NAME} 
		src/kmer_counting_bcl.cpp 
		src/kmer.cpp src/sparc/log.cpp src/sparc/utils.cpp
		)
						
	foreach(EXEC ${NAME} )
		target_compile_features(${EXEC} PRIVATE cxx_std_17)
		add_dependencies(${EXEC} spdloglib gzstreamlib ${BCL_DEPENDIES})
		target_include_directories(${EXEC} PUBLIC ${BCL_INCLUDED_DIR} "${LOCAL_EXT_PREFIX_DIR}/include" )
		target_compile_definitions(${EXEC} PUBLIC ${BCL_DEFINITIONS})
		target_compile_definitions(${EXEC} PUBLIC "-DUSE_BCLCLIENT")
        target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
		target_link_libraries(${EXEC} PUBLIC spdlog gzstream ZLIB::ZLIB ${BCL_LIBRARIES})
	    set_target_properties(${EXEC} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	    add_dependencies(sparc_bcl ${EXEC})
	endforeach(EXEC)
	
endif()






