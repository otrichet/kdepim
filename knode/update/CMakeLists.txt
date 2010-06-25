
###### Migration tool ######

set( knode_updaters_SRCS
  main.cpp

  updater_base.cpp
  akonadi_migrator.cpp
  legacy_identity.cpp

  legacy/knarticle.cpp
  legacy/knarticlecollection.cpp
  legacy/kncollection.cpp
  legacy/knfolder.cpp
  legacy/knfoldermanager.cpp
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
