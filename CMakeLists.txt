kcoreaddons_add_plugin(wildpalms_contacts
    SOURCES
        contactconduit.cpp
        contactconduit.h
        contactmapper.cpp
        contactmapper.h
        contactview.cpp
        contactview.h
    INSTALL_NAMESPACE "wildpalms/conduits"
)

target_link_libraries(wildpalms_contacts
    WildPalmsCore
    KF6::CoreAddons
    KF6::I18n
    KF6::WidgetsAddons
    Qt::Widgets
)
