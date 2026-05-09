find_package(KF6 REQUIRED COMPONENTS Contacts)

kcoreaddons_add_plugin(wildpalms_contacts_v2
    SOURCES
        contactsbackendplugin.cpp     contactsbackendplugin.h
        contactsblobbackend.cpp       contactsblobbackend.h
        palmcontactsbackend.cpp       palmcontactsbackend.h
        contactsconflicthandler.cpp   contactsconflicthandler.h
        contactsvcardtranscoder.cpp   contactsvcardtranscoder.h
        # Phase Ia Task 11 — palm↔vcard4 TransformationStage subclasses.
        # Stages delegate body to encodePalmToVcard / decodeVcardToPalm
        # in contactsvcardtranscoder; registration happens in Task 14.
        palmtovcardtransformation.cpp palmtovcardtransformation.h
        # Phase Ia Task 14 — path A registration helper. Static
        # registerWith() drops the (contacts, palm) peer shape + the
        # two palm<->vcard4 edges into the process-wide
        # TransformationRegistry. Wired in from
        # ContactsBackendPlugin in Task 15.
        contactsdomainextension.cpp   contactsdomainextension.h
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
