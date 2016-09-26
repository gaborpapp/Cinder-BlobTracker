cmake_minimum_required( VERSION 3.0 FATAL_ERROR )
set( CMAKE_VERBOSE_MAKEFILE ON )

project( BlobTracker )
set( APP_NAME "${PROJECT_NAME}App" )

get_filename_component( CINDER_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../.." ABSOLUTE )

get_filename_component( APP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../" ABSOLUTE )

set( APP_SRCS
	${APP_DIR}/src/BlobTrackerApp.cpp
)

include( "${CINDER_PATH}/proj/cmake/modules/cinderMakeApp.cmake" )

ci_make_app(
	APP_NAME ${APP_NAME}
	SOURCES ${APP_SRCS}
	CINDER_PATH ${CINDER_PATH}
	BLOCKS
		../../../../../Cinder-BlobTracker
		../../../../../Cinder-OpenCV3
)
