  message(STATUS "looking for packages required for MFA and token validation...")
  find_package(Jansson REQUIRED)
  find_package(CURL REQUIRED)
  find_package(CJOSE REQUIRED)