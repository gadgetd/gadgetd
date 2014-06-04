CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(gadgetd)
########################################################
# Generation options:
# -DBUILD_DOC_ONLY - only doxygen documentation is build
# -DBUILD_DOC - build also doxygen documentation
# -DSUPPORT_FFS_LEGACY_API - use legacy ffs API
########################################################

########################################################
# NOTE:
# Remember to add all directories with files to DOC_SRC_DIRS_IN list
#
########################################################

IF(BUILD_DOC_ONLY)
	SET(BUILD_EXECUTABLE FALSE)
	SET(BUILD_DOC TRUE)
ELSE(BUILD_DOC_ONLY)
	SET(BUILD_EXECUTABLE TRUE)
ENDIF(BUILD_DOC_ONLY)

IF(BUILD_EXECUTABLE)

	SET(GADGETD_SRCS
		src/gadgetd.c
		src/gadgetd-create.c
		src/gadgetd-remove.c
		src/gadgetd-config.c
		src/strdelim.c
		src/ffs-service.c
	)

	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
        INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

	SET(PREFIX ${CMAKE_INSTALL_PREFIX})
	SET(PACKAGE ${PROJECT_NAME})
	SET(BINDIR "${PREFIX}/bin")
	SET(DATADIR "${PREFIX}/share/${PACKAGE}/data")

	SET(PKG_MODULES
		libusbg
		glib-2.0
	)

	INCLUDE(FindPkgConfig)
	pkg_check_modules(pkgs REQUIRED ${PKG_MODULES})

	FOREACH(flag ${pkgs_CFLAGS})
		SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
	ENDFOREACH(flag)

	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -lusbg -g -Wall")
	MESSAGE("FLAGS: ${CMAKE_C_FLAGS}")

	ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
	ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE}\"")
	ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")

	IF(SUPPORT_FFS_LEGACY_API)
	        ADD_DEFINITIONS("-D__FFS_LEGACY_API_SUPPORT")
	ENDIF(SUPPORT_FFS_LEGACY_API)

	ADD_EXECUTABLE(${PROJECT_NAME} ${GADGETD_SRCS})
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} "-ldl -lgio-2.0 -lgobject-2.0 -lglib-2.0")

	INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})


ENDIF(BUILD_EXECUTABLE)

IF(BUILD_DOC)
	SET( DOC_SRC_DIRS_IN
		${CMAKE_SOURCE_DIR}/include
		${CMAKE_SOURCE_DIR}/src
	)

	FOREACH(doc_dir ${DOC_SRC_DIRS_IN})
		SET(DOC_SRC_DIRS "${DOC_SRC_DIRS} ${doc_dir}")
	ENDFOREACH(doc_dir)

	FIND_PACKAGE(Doxygen REQUIRED)

	get_filename_component( DOXYGEN_DOC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} PATH)

	#adjust the doxygen configuration for this project
	configure_file(${CMAKE_SOURCE_DIR}/doxygen/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/doxygen/Doxyfile @ONLY)

	#build the documentation
	add_custom_target( doc ALL
		${DOXYGEN_EXECUTABLE}
		${CMAKE_CURRENT_BINARY_DIR}/doxygen/Doxyfile
		WORKING_DIRECTORY ${CMAKE_DOXYGEN_DIRECTORY}
		COMMENT "Generating documentation with Doxygen" VERBATIM
	)

ENDIF(BUILD_DOC)
