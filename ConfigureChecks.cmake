include(CheckIncludeFile)
include(CheckIncludeFiles)
include(CheckSymbolExists)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckPrototypeExists)
include(CheckTypeSize)
include(MacroBoolTo01)
# The FindKDE4.cmake module sets _KDE4_PLATFORM_DEFINITIONS with
# definitions like _GNU_SOURCE that are needed on each platform.
set(CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS})

OPTION(KDE4_KDEPIM_NEW_DISTRLISTS   "Whether to use new distribution lists, to store them like normal contacts; useful for Kolab") 

macro_bool_to_01(KDE4_KDEPIM_NEW_DISTRLISTS KDEPIM_NEW_DISTRLISTS)
macro_bool_to_01(SASL2_FOUND HAVE_LIBSASL2)
macro_bool_to_01(BOOST_FOUND HAVE_BOOST)
macro_bool_to_01(GNOKII_FOUND HAVE_GNOKII_H) 

OPTION(WITH_INDEXLIB "Enable full-text indexing in KMail." OFF)
macro_bool_to_01(WITH_INDEXLIB HAVE_INDEXLIB)

#now check for dlfcn.h using the cmake supplied CHECK_include_FILE() macro
# If definitions like -D_GNU_SOURCE are needed for these checks they
# should be added to _KDE4_PLATFORM_DEFINITIONS when it is originally
# defined outside this file.  Here we include these definitions in
# CMAKE_REQUIRED_DEFINITIONS so they will be included in the build of
# checks below.
set(CMAKE_REQUIRED_DEFINITIONS ${_KDE4_PLATFORM_DEFINITIONS})
if (WIN32)
   set(CMAKE_REQUIRED_LIBRARIES ${KDEWIN32_LIBRARIES} )
   set(CMAKE_REQUIRED_INCLUDES  ${KDEWIN32_INCLUDES} )
endif (WIN32)
check_include_files(byteswap.h HAVE_BYTESWAP_H)
check_include_files(err.h HAVE_ERR_H)
check_include_files(execinfo.h HAVE_EXECINFO_H)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(inttypes.h HAVE_INTTYPES_H)
check_include_files(malloc.h HAVE_MALLOC_H)
check_include_files(paths.h HAVE_PATHS_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(sys/bitypes.h HAVE_SYS_BITYPES_H)
check_include_files(sys/cdefs.h HAVE_SYS_CDEFS_H)
check_include_files(sys/dir.h HAVE_SYS_DIR_H)
check_include_files(sys/file.h HAVE_SYS_FILE_H)
check_include_files(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_files(sys/limits.h HAVE_SYS_LIMITS_H)
check_include_files(sys/ndir.h HAVE_SYS_NDIR_H)
check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(sys/select.h HAVE_SYS_SELECT_H)
check_include_files(sys/stat.h HAVE_SYS_STAT_H)
check_include_files(sys/sysctl.h HAVE_SYS_SYSCTL_H)
check_include_files(sys/time.h HAVE_SYS_TIME_H)
check_include_files(sys/types.h HAVE_SYS_TYPES_H)
check_include_files(time.h HAVE_TIME_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(values.h HAVE_VALUES_H)
check_include_files(sys/time.h TM_IN_SYS_TIME)
check_include_files("sys/time.h;time.h" TIME_WITH_SYS_TIME)

check_function_exists(unsetenv   HAVE_UNSETENV)
check_function_exists(setenv     HAVE_SETENV)
check_function_exists(usleep     HAVE_USLEEP)

check_symbol_exists(snprintf        "stdio.h"                  HAVE_SNPRINTF)
check_symbol_exists(vsnprintf       "stdio.h"                  HAVE_VSNPRINTF)


check_prototype_exists(gethostname "stdlib.h;unistd.h" HAVE_GETHOSTNAME_PROTO)
check_prototype_exists(getdomainname "stdlib.h;unistd.h;netdb.h" HAVE_GETDOMAINNAME_PROTO)
check_prototype_exists(unsetenv "stdlib.h" HAVE_UNSETENV_PROTO)
check_prototype_exists(setenv "stdlib.h" HAVE_SETENV_PROTO)
check_prototype_exists(usleep unistd.h HAVE_USLEEP_PROTO)

check_type_size("int" SIZEOF_INT)
check_type_size("char *"  SIZEOF_CHAR_P)
check_type_size("long" SIZEOF_LONG)
check_type_size("short" SIZEOF_SHORT)
check_type_size("size_t" SIZEOF_SIZE_T)
check_type_size("unsigned long" SIZEOF_UNSIGNED_LONG)
check_type_size("uint64_t" SIZEOF_UINT64_T)
check_type_size("unsigned long long" SIZEOF_UNSIGNED_LONG_LONG)

include(CheckTimezone)

