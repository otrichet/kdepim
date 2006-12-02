# This is a modified copy of playground/base/strigiapplet/cmake/FindStrigi.cmake
#
#  STRIGI_FOUND - system has Strigi
#  STRIGI_INCLUDE_DIR - the Strigi include directory
#  STRIGIHTMLGUI_LIBRARY - Link these to use Strigi html gui
#  STRIGICLIENT_LIBRARY - Link to use the Strigi C++ client
#

FIND_PATH(STRIGI_INCLUDE_DIR strigihtmlgui.h
  PATHS
  /usr/include
  /usr/local/include
  ${INCLUDE_INSTALL_DIR}
)

FIND_LIBRARY(STRIGIHTMLGUI_LIBRARY NAMES strigihtmlgui
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
)
FIND_LIBRARY(STRIGICLIENT_LIBRARY NAMES searchclient
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
)
FIND_LIBRARY(STRIGIQTDBUSCLIENT_LIBRARY NAMES strigiqtdbusclient 
  PATHS
  /usr/lib
  /usr/local/lib
  ${LIB_INSTALL_DIR}
)

IF(STRIGI_INCLUDE_DIR AND STRIGIHTMLGUI_LIBRARY AND STRIGICLIENT_LIBRARY AND STRIGIQTDBUSCLIENT_LIBRARY)
   SET(STRIGI_FOUND TRUE)
ENDIF(STRIGI_INCLUDE_DIR AND STRIGIHTMLGUI_LIBRARY AND STRIGICLIENT_LIBRARY AND STRIGIQTDBUSCLIENT_LIBRARY)

IF(STRIGI_FOUND)
  IF(NOT Strigi_FIND_QUIETLY)
    MESSAGE(STATUS "Found Strigi: ${STRIGIHTMLGUI_LIBRARY}")
  ENDIF(NOT Strigi_FIND_QUIETLY)
ELSE(STRIGI_FOUND)
  IF(Strigi_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find Strigi")
  ENDIF(Strigi_FIND_REQUIRED)
ENDIF(STRIGI_FOUND)

