# Try to find kcgi
# Once done this will define
#  KCGI_FOUND
#  KCGI_INCLUDE_DIRS
#  KCGI_LIBRARIES
#  KCGI_DEFINITIONS


find_package(PkgConfig)
pkg_check_modules(PC_KCGI QUIET libkcgi)
set(KCGI_DEFINITIONS ${PC_KCGI_CFLAGS_OTHER})

find_path(KCGI_INCLUDE_DIR kcgi.h HINTS ${PC_KCGI_INCLUDEDIR} ${PC_KCGI_INCLUDE_DIRS} PATH_SUFFIXES kcgi)

find_library(KCGI_LIBRARY NAMES kcgi libkcgi HINTS ${PC_KCGI_LIBDIR} ${PC_KCGI_LIBRARY_DIRS})

set(KCGI_LIBRARIES ${KCGI_LIBRARY})
set(KCGI_INCLUDE_DIRS ${KCGI_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libkcgi DEFAULT_MSG KCGI_LIBRARY KCGI_INCLUDE_DIR)

mark_as_advanced(KCGI_INCLUDE_DIR KCGI_LIBRARY)
