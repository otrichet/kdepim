project(knode)

add_definitions( -DTRANSLATION_DOMAIN=\"knode\" )
# add_definitions( -DQT_NO_CAST_FROM_ASCII )
# add_definitions( -DQT_NO_CAST_TO_ASCII )

if(BUILD_TESTING)
    add_subdirectory( tests )
endif()
add_subdirectory( pics )
add_subdirectory( filters )
add_subdirectory( kconf_update )

include_directories(
  ${Boost_INCLUDE_DIR}
)


########### Private library ###############

set(knodecommon_LIB_SRCS
   knconfigmanager.cpp
   knconfig.cpp
   knconfigwidgets.cpp
   settings.cpp
   scheduler.cpp
   knjobdata.cpp
   mailsendjob.cpp
   nntpjobs.cpp
   knaccountmanager.cpp
   kncollection.cpp
   kncollectionviewitem.cpp
   knserverinfo.cpp
   knnntpaccount.cpp
   kngroupmanager.cpp
   knarticlecollection.cpp
   kngroup.cpp
   kngrouppropdlg.cpp
   knfoldermanager.cpp
   knfolder.cpp
   knmemorymanager.cpp
   kncleanup.cpp
   knarticlemanager.cpp
   knarticle.cpp
   kndisplayedheader.cpp
   knsourceviewwindow.cpp
   knarticlewindow.cpp
#    kncomposer.cpp
#    kncomposerview.cpp
#    kncomposereditor.cpp
   knarticlefactory.cpp
   knfiltermanager.cpp
   knstatusfilter.cpp
   knstringfilter.cpp
   knrangefilter.cpp
   knarticlefilter.cpp
   knfilterconfigwidget.cpp
   knfilterdialog.cpp
   knsearchdialog.cpp
   utilities.cpp
   knmainwidget.cpp
   aboutdata.cpp
   knglobals.cpp
   kncollectionview.cpp
   articlewidget.cpp
#    csshelper.cpp
   composer/attachment_view.cpp
   composer/followupto_combobox.cpp
   configuration/identity_widget.cpp
   configuration/identity_edition_dialog.cpp
   groupselection/dialog_base.cpp
   groupselection/group_subscription_dialog.cpp
   groupselection/group_selection_dialog.cpp
   groupselection/model/checked_state_proxy_model.cpp
   groupselection/model/filter_group_proxy_model.cpp
   groupselection/model/subscription_state_proxy_model.cpp
   groupselection/model/base_grouping_proxy_model.cpp
   groupselection/model/subscription_grouping_proxy_model.cpp
   groupselection/model/selection_grouping_proxy_model.cpp
   groupselection/model/group_model.cpp
   groupselection/helper/group_list_date_picker.cpp
   utils/locale.cpp
   utils/startup.cpp
   messagelist/headers_widget.cpp
   messagelist/headers_model.cpp
   messagelist/headers_view.cpp
   treewidget.cpp
   foldertreewidget.cpp
)

qt5_add_dbus_adaptor( knodecommon_LIB_SRCS org.kde.knode.xml knmainwidget.h KNMainWidget)


ki18n_wrap_ui(knodecommon_LIB_SRCS
   nntpaccountdialog_base.ui
   nntpaccountlistwidget_base.ui
   readnewsgeneralwidget_base.ui
   readnewsnavigationwidget_base.ui
   readnewsviewerwidget_base.ui
   postnewstechnicalwidget_base.ui
   postnewscomposerwidget_base.ui
   composer/composer_view.ui
   configuration/identity_widget.ui
   configuration/identity_edition_dialog.ui
   groupselection/dialog_base.ui
   groupselection/helper/group_list_date_picker.ui
)

kconfig_add_kcfg_files(knodecommon_LIB_SRCS settings_base.kcfgc )

add_library(knodecommon ${knodecommon_LIB_SRCS})
# generate_export_header(knodeprivate BASE_NAME knode)

target_link_libraries(knodecommon
    PRIVATE
#         KF5::Contacts
        KF5::IdentityManagement
        KF5::I18n
        KF5::KCMUtils
        KF5::KDELibs4Support
        KF5::KHtml
        KF5::KIOCore
        KF5::Libkdepim
        KF5::Libkleo
        KF5::MailTransport
        KF5::MessageComposer
        KF5::Mime
        KF5::SonnetUi
        KF5::XmlGui
)


set_target_properties(knodecommon
   PROPERTIES VERSION ${KDEPIM_LIB_VERSION}
   SOVERSION ${KDEPIM_LIB_SOVERSION}
)
install(TARGETS knodecommon ${INSTALL_TARGETS_DEFAULT_ARGS})


########### KCM ###############

set(kcm_knode_PART_SRCS knconfigpages.cpp )
add_library(kcm_knode MODULE ${kcm_knode_PART_SRCS})
target_link_libraries(kcm_knode
    PRIVATE
        knodecommon
        KF5::ConfigWidgets
        KF5::IdentityManagement
        KF5::KCMUtils
        KF5::KDELibs4Support
        KF5::Mime
)
install(TARGETS kcm_knode  DESTINATION ${KDE_INSTALL_PLUGINDIR})

########### Main binary ###############

set(knode_SRCS knode.cpp knapplication.cpp main.cpp )
file(GLOB ICONS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/icons/*-apps-knode.png")
ecm_add_app_icon(knode_SRCS ICONS ${ICONS_SRCS})
add_executable(knode ${knode_SRCS})
target_link_libraries(knode
    PRIVATE
        knodecommon
        KF5::CoreAddons
        KF5::DBusAddons
        KF5::I18n
        KF5::KDELibs4Support
        KF5::Libkdepim
        KF5::Mime
        KF5::XmlGui
)
install(TARGETS knode  ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


########### Install other files ###############

install( PROGRAMS org.kde.knode.desktop  DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES knode_config_identity.desktop
              knode_config_accounts.desktop
              knode_config_appearance.desktop
              knode_config_read_news.desktop
              knode_config_post_news.desktop
              knode_config_privacy.desktop
              knode_config_cleanup.desktop
        DESTINATION ${KDE_INSTALL_KSERVICES5DIR})
install(FILES knodeui.rc
              knreaderui.rc
              kncomposerui.rc
        DESTINATION ${KDE_INSTALL_KXMLGUI5DIR}/knode)
install(FILES headers.rc
        DESTINATION ${KDE_INSTALL_DATADIR}/knode)
install(FILES org.kde.knode.xml DESTINATION ${KDE_INSTALL_DBUSINTERFACEDIR} )

add_subdirectory(icons)
