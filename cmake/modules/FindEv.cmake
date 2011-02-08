# Ev
# Once done, this will define
#
#  Ev_FOUND - system has Ev
#  Ev_INCLUDE_DIRS - the Ev include directories
#  Ev_LIBRARIES - link these to use Ev

include(LibFindMacros)

# Use pkg-config to get hints about paths
libfind_pkg_check_modules(Ev_PKGCONF Ev)

# Include dir
find_path(Ev_INCLUDE_DIR
  NAMES ev.h
  PATHS ${Ev_PKGCONF_INCLUDE_DIRS}
)

# Finally the library itself
find_library(Ev_LIBRARY
  NAMES ev
  PATHS ${Ev_PKGCONF_LIBRARY_DIRS}
)

# Set the include dir variables and the libraries and let libfind_process do the rest.
# NOTE: Singular variables for this library, plural for libraries this this lib depends on.
set(Ev_PROCESS_INCLUDES Ev_INCLUDE_DIR)
set(Ev_PROCESS_LIBS Ev_LIBRARY)
libfind_process(Ev)
