set(OPTION_DEFAULT OFF)
set(OPTION_CLIENT_DEFAULT OFF)
if ( WITH_MFA )
	set(OPTION_SERVER_DEFAULT ON)
else()
	set(OPTION_SERVER_DEFAULT OFF)
endif()

define_channel_options(NAME "mfa" TYPE "static"
	DESCRIPTION "MFA Virtual Channel Extension"
	SPECIFICATIONS "[MY-MFAAUTH]"
	DEFAULT ${OPTION_DEFAULT})

define_channel_client_options(${OPTION_CLIENT_DEFAULT})
define_channel_server_options(${OPTION_SERVER_DEFAULT})

