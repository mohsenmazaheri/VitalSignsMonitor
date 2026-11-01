# vsm_audit - writes the audit trail to a file.
# C++ standard library only. Kept apart from vsm_core, so the alarm logic itself
# never touches the file system and only knows the IAuditLog interface.
add_library(vsm_audit STATIC
    include/vsm/audit/FileAuditLog.h
    FileAuditLog.cpp
)
add_library(vsm::audit ALIAS vsm_audit)

target_include_directories(vsm_audit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(vsm_audit PUBLIC vsm::core)
vsm_set_warnings(vsm_audit)
