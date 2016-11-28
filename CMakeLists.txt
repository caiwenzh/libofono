CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(libofono C)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(EXEC_PREFIX "\${prefix}")
SET(LIBDIR "\${prefix}/lib")
SET(INCLUDEDIR "\${prefix}/include")

# Set required packages
INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	glib-2.0
	gio-2.0
	)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} -Wl,-zdefs -fvisibility=hidden ")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")
ADD_DEFINITIONS(" -DEXPORT_API=\"__attribute__((visibility(\\\"default\\\")))\" ")

### Build ###
SET(SRCS
	src/ofono-common.c
	src/ofono-sim.c
	src/ofono-sms.c
	src/ofono-sms-agent.c
	src/ofono-network.c
	src/ofono-connman.c
	src/ofono-call.c
	src/ofono-ss.c
	src/ofono-modem.c
	src/ofono-sat.c
	src/ofono-phonebook.c
	src/ofono-netmon.c
	src/common.c
   )

ADD_LIBRARY(libofono SHARED ${SRCS})
TARGET_LINK_LIBRARIES(libofono ${pkgs_LDFLAGS})
SET_TARGET_PROPERTIES(libofono PROPERTIES VERSION 0.0.0 SOVERSION 0 OUTPUT_NAME ofono)

# pkgconfig file
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
CONFIGURE_FILE(libofono.pc.in libofono.pc @ONLY)

INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ DESTINATION include/libofono)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libofono.pc DESTINATION lib/pkgconfig)
INSTALL(TARGETS libofono DESTINATION lib COMPONENT Runtime)
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION /usr/share/license RENAME libofono)

ADD_SUBDIRECTORY(test)

