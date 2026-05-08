find_package(KF6 REQUIRED COMPONENTS Contacts)

kcoreaddons_add_plugin(wildpalms_contacts_v2
    SOURCES
        contactsbackendplugin.cpp     contactsbackendplugin.h
        contactsblobbackend.cpp       contactsblobbackend.h
        contactsconflicthandler.cpp   contactsconflicthandler.h
        contactsvcardtranscoder.cpp   contactsvcardtranscoder.h
        # Phase Ia Task 11 — palm↔vcard4 TransformationStage subclasses.
        # Stages delegate body to encodePalmToVcard / decodeVcardToPalm
        # in contactsvcardtranscoder; registration happens in Task 14.
        palmtovcardtransformation.cpp palmtovcardtransformation.h
    INSTALL_NAMESPACE "wildpalms/plugins"
)
target_include_directories(wildpalms_contacts_v2
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
# See calendar plugin's CMakeLists for why this BEFORE include is
# required (Kalburator::Sync ordering vs WildPalmsCore's legacy
# ::Sync include).
target_include_directories(wildpalms_contacts_v2 BEFORE
    PRIVATE
        $<TARGET_PROPERTY:Kalburator::Sync,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(wildpalms_contacts_v2
    PRIVATE
        WildPalmsCore
        WildPalmsPalmSync
        WildPalmsPalmCalendar           # CategoryMappingStore + parseCategoryAppInfo
        WildPalmsPalmCodecs             # decodeContact / encodeContact / kde_pim_convert
        WildPalmsPalmConflict           # PalmConflictHandler
        KF6::CoreAddons
        KF6::Contacts
        KF6::I18n
        KF6::WidgetsAddons
        Qt::Widgets
        Kalburator::Sync
)
