CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(gadgetd)
########################################################
# Generation options:
# -DBUILD_DOC_ONLY - only doxygen documentation is build
# -DBUILD_DOC - build also doxygen documentation
# -DSUPPORT_FFS_LEGACY_API - use legacy ffs API
# -DBUILD_EXAMPLES - build also sample applications
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
		src/gadgetd-common.c
		src/gadgetd-introspection.c
		src/strdelim.c
		src/gadgetd-ffs-func.c
		src/gadget-daemon.c
		src/gadget-manager.c
		src/gadgetd-gadget-object.c
		src/gadget-strings.c
		src/gadget-descriptors.c
		src/gadget-function-manager.c
		src/gadgetd-function-object.c
		src/dbus-function-ifaces/gadgetd-serial-function-iface.c
		src/gadget-config-manager.c
		src/gadgetd-config-object.c
		src/gadgetd-core.c
		src/dbus-config-ifaces/gadget-config.c
		src/gadgetd-functions.c
		src/gadgetd-udc-object.c
		src/dbus-function-ifaces/gadgetd-function-iface.c
		src/gadgetd-udc-iface.c
	)

	SET(FFS-DAEMON-SRC
	        src/libffs-daemon/ffs-daemon.c
	)

	INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
        INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

	SET(PREFIX ${CMAKE_INSTALL_PREFIX})
	SET(PACKAGE ${PROJECT_NAME})
	SET(BINDIR "${PREFIX}/bin")
	SET(LIBDIR "${PREFIX}/lib")
	SET(INCLUDEDIR "${PREFIX}/include")
	SET(DATADIR "${PREFIX}/share/${PACKAGE}/data")

	SET(PKG_MODULES
		libusbgx
		glib-2.0
		libconfig
		gio-unix-2.0
	)

	INCLUDE(FindPkgConfig)
	pkg_check_modules(pkgs REQUIRED ${PKG_MODULES})

	FOREACH(flag ${pkgs_CFLAGS})
		SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
	ENDFOREACH(flag)

	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -g -Wall")
	MESSAGE("FLAGS: ${CMAKE_C_FLAGS}")

	ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
	ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE}\"")
	ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")

	IF(SUPPORT_FFS_LEGACY_API)
	        ADD_DEFINITIONS("-D__FFS_LEGACY_API_SUPPORT")
	ENDIF(SUPPORT_FFS_LEGACY_API)

	# DBUS code generation using gdbus-codegen
	SET(CODEGEN_PREFIX
                gadgetd-gdbus-codegen
	)

        SET(CODEGEN_COMMAND
                gdbus-codegen
		--interface-prefix org.usb.device.
		--generate-c-code ${CODEGEN_PREFIX}
		--c-generate-object-manager
		--c-namespace Gadgetd
		${CMAKE_CURRENT_SOURCE_DIR}/xml/gadgetd.xml
	)

	ADD_CUSTOM_COMMAND(
	        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/${CODEGEN_PREFIX}.c
	        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/${CODEGEN_PREFIX}.h
		COMMAND ${CODEGEN_COMMAND}
		COMMAND mv ${CODEGEN_PREFIX}.c ${CMAKE_CURRENT_SOURCE_DIR}/src/
		COMMAND mv ${CODEGEN_PREFIX}.h ${CMAKE_CURRENT_SOURCE_DIR}/include/
	)

        SET(GADGETD_SRCS
                ${GADGETD_SRCS}
                ${CMAKE_CURRENT_SOURCE_DIR}/src/${CODEGEN_PREFIX}.c
	)

	ADD_EXECUTABLE(${PROJECT_NAME} ${GADGETD_SRCS})
	TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

	ADD_DEFINITIONS("-DGD_EXPORT_SYMBOLS")

	ADD_LIBRARY(ffs-daemon SHARED ${FFS-DAEMON-SRC})
	SET_TARGET_PROPERTIES(ffs-daemon PROPERTIES
	        SOVERSION 0
	        VERSION 0.0.0
	)

	INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
	INSTALL(TARGETS ffs-daemon ARCHIVE
	        DESTINATION ${LIBDIR}
	        LIBRARY DESTINATION ${LIBDIR}
	        COMPONENT library)
	INSTALL(FILES include/ffs-daemon.h DESTINATION ${INCLUDEDIR}/gadgetd)
	INSTALL(FILES xml/org.usb.gadgetd.conf DESTINATION /etc/dbus-1/system.d)

	# uninstall target
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
		IMMEDIATE @ONLY)

	add_custom_target(uninstall
		COMMAND ${CMAKE_COMMAND}
		-P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

	IF(BUILD_EXAMPLES)
	        ADD_SUBDIRECTORY(examples)
	ENDIF(BUILD_EXAMPLES)
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
