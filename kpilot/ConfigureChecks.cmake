include(CheckIncludeFiles)
include(CheckFunctionExists)

check_include_files( stdint.h HAVE_STDINT_H )
check_include_files( alloca.h HAVE_ALLOCA_H )
check_include_files( "sys/time.h" HAVE_SYS_TIME_H )
check_include_files( "sys/stat.h" HAVE_SYS_STAT_H )
check_function_exists( cfsetspeed HAVE_CFSETSPEED )
check_function_exists( strdup HAVE_STRDUP )
check_function_exists( setenv HAVE_SETENV )
check_function_exists( unsetenv HAVE_UNSETENV )
check_function_exists( usleep HAVE_USLEEP )
check_function_exists( random HAVE_RANDOM )
check_function_exists( putenv HAVE_PUTENV )
check_function_exists( seteuid HAVE_SETEUID )
check_function_exists( mkstemps HAVE_MKSTEMPS )
check_function_exists( mkstemp HAVE_MKSTEMP )
check_function_exists( mkdtemp HAVE_MKDTEMP )
check_function_exists( revoke HAVE_REVOKE )
check_function_exists( strlcpy HAVE_STRLCPY )
check_function_exists( strlcat HAVE_STRLCAT )
check_function_exists( inet_aton HAVE_INET_ATON )

