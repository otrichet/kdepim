
###### Migration tool ######

include_directories(
  ${knode_SOURCE_DIR}
  ${KDE4_INCLUDE_DIR}
)

set( knode_updaters_SRCS
  main.cpp

  updater_base.cpp
  legacy_identity.cpp
)

kde4_add_executable( knode-migrator
  ${knode_updaters_SRCS}
)

target_link_libraries( knode-migrator
  ${KDE4_KDECORE_LIBS}
  ${KDEPIMLIBS_KPIMIDENTITIES_LIBRARY}
  knodecommon
)

install(
  TARGETS knode-migrator
  ${INSTALL_TARGETS_DEFAULT_ARGS}
)
