include(FindPackageHandleStandardArgs)

if (WIN32)
	# Find include files
	find_path(
		ASSIMP_INCLUDE_DIR
		NAMES assimp/scene.h
		PATHS
			$ENV{PROGRAMFILES}/include
			${ASSIMP_ROOT_DIR}/include
		DOC "The directory where assimp/scene.h resides")

	# Find library files
	find_library(
		ASSIMP_LIBRARY_RELEASE
		NAMES assimp-vc140-mt
		PATHS
			$ENV{PROGRAMFILES}/lib
			${ASSIMP_ROOT_DIR}/lib
			${LM_EXTERNAL_LIBRARY_PATH}/Release)

	find_library(
		ASSIMP_LIBRARY_DEBUG
		NAMES assimp-vc140-mtd
		PATHS
			$ENV{PROGRAMFILES}/lib
			${ASSIMP_ROOT_DIR}/lib
			${LM_EXTERNAL_LIBRARY_PATH}/Debug)
else()
	# Find include files
	find_path(
		ASSIMP_INCLUDE_DIR
		NAMES assimp/scene.h
		PATHS
			/usr/include
			/usr/local/include
			/sw/include
			/opt/local/include
		DOC "The directory where assimp/scene.h resides")

	# Find library files
	find_library(
		ASSIMP_LIBRARY
		NAMES assimp
		PATHS
			/usr/lib64
			/usr/lib
			/usr/local/lib64
			/usr/local/lib
			/sw/lib
			/opt/local/lib
			${ASSIMP_ROOT_DIR}/lib
		DOC "The Assimp library")
endif()

if (WIN32)
	# Handle REQUIRD argument, define *_FOUND variable
	find_package_handle_standard_args(assimp DEFAULT_MSG ASSIMP_INCLUDE_DIR ASSIMP_LIBRARY_RELEASE ASSIMP_LIBRARY_DEBUG)

	# Define ASSIMP_LIBRARIES and ASSIMP_INCLUDE_DIRS
	if (ASSIMP_FOUND)
		set(ASSIMP_LIBRARIES_RELEASE ${ASSIMP_LIBRARY_RELEASE})
		set(ASSIMP_LIBRARIES_DEBUG ${ASSIMP_LIBRARY_DEBUG})
		set(ASSIMP_LIBRARIES debug ${ASSIMP_LIBRARIES_DEBUG} optimized ${ASSIMP_LIBRARY_RELEASE})
		set(ASSIMP_INCLUDE_DIRS ${ASSIMP_INCLUDE_DIR})
	endif()

	# Hide some variables
	mark_as_advanced(ASSIMP_INCLUDE_DIR ASSIMP_LIBRARY_RELEASE ASSIMP_LIBRARY_DEBUG)
else()
	find_package_handle_standard_args(assimp DEFAULT_MSG ASSIMP_INCLUDE_DIR ASSIMP_LIBRARY)
	if (ASSIMP_FOUND)
		set(ASSIMP_LIBRARIES ${ASSIMP_LIBRARY})
		set(ASSIMP_INCLUDE_DIRS ${ASSIMP_INCLUDE_DIR})
	endif()
	mark_as_advanced(ASSIMP_INCLUDE_DIR ASSIMP_LIBRARY)
endif()
