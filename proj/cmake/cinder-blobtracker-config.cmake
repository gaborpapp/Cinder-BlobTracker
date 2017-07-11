if ( NOT TARGET Cinder-BlobTracker )
	get_filename_component( CINDER_BLOBTRACKER_PATH "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE )
	# FIXME: Cinder-OpenCV3 is required to be placed next to Cinder-BlobTracker
	get_filename_component( CINDER_OPENCV3_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../Cinder-OpenCV3" ABSOLUTE )

	set( CINDER_BLOBTRACKER_INCLUDES
		${CINDER_BLOBTRACKER_PATH}/include
	)
	set( CINDER_BLOBTRACKER_SOURCES
		${CINDER_BLOBTRACKER_PATH}/src/mndl/blobtracker/BlobTracker.cpp
		${CINDER_BLOBTRACKER_PATH}/src/mndl/blobtracker/DebugDrawer.cpp
	)

	add_library( Cinder-BlobTracker ${CINDER_BLOBTRACKER_SOURCES} )

	target_compile_options( Cinder-BlobTracker PUBLIC "-std=c++11" )

	target_include_directories( Cinder-BlobTracker PUBLIC "${CINDER_BLOBTRACKER_INCLUDES}"
			PRIVATE "${CINDER_OPENCV3_PATH}/include" )
	if( CINDER_MAC )
		target_include_directories( Cinder-BlobTracker
			PRIVATE "${CINDER_OPENCV3_PATH}/include/macosx" )
	endif()

	target_include_directories( Cinder-BlobTracker SYSTEM BEFORE PUBLIC "${CINDER_PATH}/include" )

	if( NOT TARGET cinder )
		include( "${CINDER_PATH}/proj/cmake/configure.cmake" )
		find_package( cinder REQUIRED PATHS
			"${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
			"$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}" )
	endif()
	target_link_libraries( Cinder-BlobTracker PRIVATE cinder )
endif()
