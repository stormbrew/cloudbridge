# TomCrypt
# Once done, this will define
#
#  TomCrypt_FOUND - system has Magick++
#  TomCrypt_INCLUDE_DIRS - the Magick++ include directories
#  TomCrypt_LIBRARIES - link these to use Magick++

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(TomCrypt_PKGCONF TomCrypt)

# Include dir
find_path(TomCrypt_INCLUDE_DIR
  NAMES tomcrypt.h
  PATHS ${TomCrypt_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(TomCrypt_LIBRARY
  NAMES tomcrypt
  PATHS ${TomCrypt_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(TomCrypt_PROCESS_INCLUDES TomCrypt_INCLUDE_DIR)
set(TomCrypt_PROCESS_LIBS TomCrypt_LIBRARY)
libfind_process(TomCrypt)
