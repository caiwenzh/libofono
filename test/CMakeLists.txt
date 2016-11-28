CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ofono_test C)

SET(ofono_test_src
	main.c
	common.c
	modem.c
	sim.c
	call.c
	sms.c
	network.c
	ss.c
	connman.c
	sat.c
	phonebook.c
	netmon.c
)

ADD_EXECUTABLE(ofono_test ${ofono_test_src})
TARGET_LINK_LIBRARIES(ofono_test ${pkgs_LDFLAGS} "-L${CMAKE_BINARY_DIR} -lofono")
INSTALL(TARGETS ofono_test RUNTIME DESTINATION bin/)
ADD_DEPENDENCIES(ofono_test libofono)
