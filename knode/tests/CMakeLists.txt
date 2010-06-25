
set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )


#### Standard tests ##########################################

macro( knode_add_test _test_name )

  set( _knodetest_SRCS ${ARGN} )
  set( _name "knode-${_test_name}" )

  kde4_add_unit_test( ${_name}
    ${_knodetest_SRCS}
  )

  target_link_libraries( ${_name}
    knodecommon
    ${QT_QTCORE_LIBRARY}
    ${QT_QTTEST_LIBRARY}
  )

endmacro( knode_add_test )


knode_add_test( utilities-locale knodetest.cpp )



#### Akonadi tests ##########################################

# based on kdepimlibs/akonadi/tests
macro( knode_add_akonadi_isolated_test _test_name )

  set( _knodetest_SRCS ${ARGN} )
  set( _name "knode-${_test_name}" )

  kde4_add_executable( ${_name}
    TEST
    ${_knodetest_SRCS}
  )
  target_link_libraries( ${_name}
    knodecommon
    ${QT_QTCORE_LIBRARY}
    ${QT_QTTEST_LIBRARY}
    ${KDEPIMLIBS_AKONADI_KDE_LIBS}
  )

  # based on kde4_add_unit_test
  if (WIN32)
    get_target_property( _loc ${_name} LOCATION )
    set( _executable ${_loc}.bat )
  else (WIN32)
    set( _executable ${EXECUTABLE_OUTPUT_PATH}/${_name} )
  endif (WIN32)
  if (UNIX)
    if (APPLE)
      set( _executable ${_executable}.app/Contents/MacOS/${_name} )
    else (APPLE)
      set( _executable ${_executable}.shell )
    endif (APPLE)
  endif (UNIX)

  add_test(
    NAME ${_name}
    COMMAND akonaditest -c ${CMAKE_CURRENT_SOURCE_DIR}/data/unittestenv/config.xml ${_executable}
  )

endmacro( knode_add_akonadi_isolated_test )

macro( knode_add_akonadi_migration_test _test_name _test_class )
  knode_add_akonadi_isolated_test( ${_test_name}
    ${_test_class}
    ../update/akonadi_migrator.cpp
    ../update/updater_base.cpp
    ../update/legacy/knarticle.cpp
    ../update/legacy/knarticlecollection.cpp
    ../update/legacy/kncollection.cpp
    ../update/legacy/knfolder.cpp
    ../update/legacy/knfoldermanager.cpp
  )
endmacro( knode_add_akonadi_migration_test )

knode_add_akonadi_migration_test( update-akonadimigrator-folders akonadi_migrator_test_folders.cpp )

