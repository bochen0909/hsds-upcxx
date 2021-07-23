cmake_minimum_required(VERSION 3.10)

project(HSDS-UPCXX VERSION 0.1.0 LANGUAGES CXX)

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

option(BUILD_SPARC_UPCXX "build UPCXX version of Sparc" ON)



set(LOCAL_EXT_PREFIX_DIR ${PROJECT_SOURCE_DIR}/extlib/local)       

add_custom_target(hsds_upcxx ALL)

include(cmake/CMakefile.external)


############# upcxx programs ###########################
if(BUILD_SPARC_UPCXX)
	############# check UPCXX ##############################
	set (UPCXX_VERBOSE ON)
	include(cmake/UPCXXConfig.cmake)
	if (NOT EXISTS "${UPCXX_META_EXECUTABLE}")
	   message( FATAL_ERROR "cannot find upcxx" )
	endif()
	############# end check UPCXX ##############################
	
	find_package(MPI 2.0 REQUIRED COMPONENTS CXX)
	find_package(ZLIB REQUIRED)

	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib" )
	link_directories( "${LOCAL_EXT_PREFIX_DIR}/lib64" )

	add_executable(edge_generating_upcxx 
		src/edge_generating_upcxx.cpp 
		src/kmer.cpp src/log.cpp src/utils.cpp
		)

	add_executable(kmer_counting_upcxx 
		src/kmer_counting_upcxx.cpp 
		src/kmer.cpp src/log.cpp src/utils.cpp
		)
	
	add_executable(kmer_read_mapping_upcxx 
		src/kmer_read_mapping_upcxx.cpp 
		src/kmer.cpp src/log.cpp src/utils.cpp
		)

	add_executable(lpav1_upcxx
		src/lpav1_upcxx.cpp 
		src/kmer.cpp src/log.cpp src/utils.cpp
		)

						
	foreach(EXEC edge_generating_upcxx kmer_counting_upcxx kmer_read_mapping_upcxx lpav1_upcxx)
		add_dependencies(${EXEC} spdloglib gzstreamlib)
		target_include_directories(${EXEC} PUBLIC ${UPCXX_INCLUDE_DIRS} "${LOCAL_EXT_PREFIX_DIR}/include" )
		target_compile_definitions(${EXEC} PUBLIC ${UPCXX_DEFINITIONS})
		target_compile_definitions(${EXEC} PUBLIC "-DUSE_UPCXXCLIENT")
        target_link_libraries(${EXEC} PUBLIC ${MPI_CXX_LIBRARIES})
		target_link_libraries(${EXEC} PUBLIC spdlog gzstream ZLIB::ZLIB ${UPCXX_LIBRARIES})
	    set_target_properties(${EXEC} PROPERTIES EXCLUDE_FROM_ALL TRUE)
	    add_dependencies(hsds_upcxx ${EXEC})
	endforeach(EXEC)
	
endif()

############# unit test ###########################
add_executable(test_kmer src/test_kmer.cpp src/kmer.cpp src/utils.cpp)
add_executable(test_utils src/test_utils.cpp src/utils.cpp)

