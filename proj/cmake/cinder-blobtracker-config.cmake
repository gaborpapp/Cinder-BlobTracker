if ( NOT TARGET Cinder-BlobTracker )
	get_filename_component( CINDER_BLOBTRACKER_PATH "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE )

	get_filename_component( CINDER_OPENCV4_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../Cinder-OpenCV4" ABSOLUTE )
	find_package( Cinder-OpenCV4 PATHS CINDER_OPENCV4_PATH NO_DEFAULT_PATH )

	set( CINDER_BLOBTRACKER_INCLUDES
		${CINDER_BLOBTRACKER_PATH}/include
	)
	set( CINDER_BLOBTRACKER_SOURCES
		${CINDER_BLOBTRACKER_PATH}/src/mndl/blobtracker/BlobTracker.cpp
		${CINDER_BLOBTRACKER_PATH}/src/mndl/blobtracker/DebugDrawer.cpp
	)

	add_library( Cinder-BlobTracker ${CINDER_BLOBTRACKER_SOURCES} )

	target_include_directories( Cinder-BlobTracker PUBLIC "${CINDER_BLOBTRACKER_INCLUDES}"
		PRIVATE "${CINDER_OPENCV4_PATH}/include" ${Cinder-OpenCV4_INCLUDES} )

	target_include_directories( Cinder-BlobTracker SYSTEM BEFORE PUBLIC "${CINDER_PATH}/include" )

	target_link_libraries( Cinder-BlobTracker PRIVATE ${Cinder-OpenCV4_LIBRARIES} )

	if( NOT TARGET cinder )
		include( "${CINDER_PATH}/proj/cmake/configure.cmake" )
		find_package( cinder REQUIRED PATHS
			"${CINDER_PATH}/${CINDER_LIB_DIRECTORY}"
			"$ENV{CINDER_PATH}/${CINDER_LIB_DIRECTORY}" )
	endif()
	target_link_libraries( Cinder-BlobTracker PRIVATE cinder )
endif()
