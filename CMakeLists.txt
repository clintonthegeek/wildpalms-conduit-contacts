find_package(KF6 REQUIRED COMPONENTS Contacts)

add_library(wildpalms_contacts_static STATIC
    contactsbackendplugin.cpp     contactsbackendplugin.h
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
)

target_include_directories(wildpalms_contacts_static
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)
# See calendar plugin's CMakeLists for why this BEFORE include is
# required (Kalburator::Sync ordering vs WildPalmsCore's legacy
# ::Sync include).
target_include_directories(wildpalms_contacts_static BEFORE
    PRIVATE
        $<TARGET_PROPERTY:Kalburator::Sync,INTERFACE_INCLUDE_DIRECTORIES>
)
set_target_properties(wildpalms_contacts_static PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(wildpalms_contacts_static
    PUBLIC
        Kalburator::Sync
    PRIVATE
        # K.8b T6: WildPalmsCore removed to break CMake autogen cycle.
        # Symbols resolved when this static is pulled into WildPalmsCore.so.
        WildPalmsPalmSync
        WildPalmsPalmCalendar           # CategoryMappingStore + parseCategoryAppInfo
        WildPalmsPalmCodecs             # decodeContact / encodeContact / kde_pim_convert
        WildPalmsPalmConflict           # PalmConflictHandler
        KF6::Contacts
        KF6::I18n
        KF6::WidgetsAddons
        Qt::Widgets
)
