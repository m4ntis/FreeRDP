
find_path(CJOSE_INCLUDE_DIR cjose/cjose.h
		  /usr/include
		  /usr/local/include)

find_library(CJOSE_LIBRARY cjose)

find_package_handle_standard_args(CJOSE DEFAULT_MSG CJOSE_INCLUDE_DIR CJOSE_LIBRARY)

if(CJOSE_FOUND)
	set(CJOSE_LIBRARIES ${CJOSE_LIBRARY})
	set(CJOSE_INCLUDE_DIRS ${CJOSE_INCLUDE_DIR})
endif()

mark_as_advanced(CJOSE_INCLUDE_DIR CJOSE_LIBRARY)
