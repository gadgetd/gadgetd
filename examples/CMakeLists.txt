MESSAGE("Building sample applications")

SET(FFS_SERVICE_EXAMPLE_SRC
      ffs-service-example.c
      )

SET(FFS_HOST_EXAMPLE_SRC
     ffs-host-example.c
     )

INCLUDE(FindPkgConfig)
pkg_check_modules(LIBUSB REQUIRED
     libusb-1.0
     )

FOREACH(flag ${LIBUSB_CFLAGS})
        SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

ADD_EXECUTABLE(ffs-service-example ${FFS_SERVICE_EXAMPLE_SRC})
TARGET_LINK_LIBRARIES(ffs-service-example "-laio" ffs-daemon)

ADD_EXECUTABLE(ffs-host-example ${FFS_HOST_EXAMPLE_SRC})
TARGET_LINK_LIBRARIES(ffs-host-example ${LIBUSB_LDFLAGS})


INSTALL(TARGETS ffs-host-example DESTINATION ${BINDIR})
INSTALL(TARGETS ffs-service-example DESTINATION ${BINDIR})
