cmake_minimum_required(VERSION 3.20)

project(EndlessMapStudio LANGUAGES C CXX)

if(NOT WIN32)
    message(FATAL_ERROR "Endless Map Studio currently targets Windows.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(EOLIB_BUILD_PROTOCOL OFF CACHE BOOL "Use EOLib data readers without packet generation" FORCE)
set(EOLIB_BUILD_LUA_BINDINGS OFF CACHE BOOL "Do not build EOLib Lua bindings" FORCE)

FetchContent_Declare(
    eolib
    GIT_REPOSITORY https://github.com/sorokya/eolib-c.git
    GIT_TAG 0.6.1
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(eolib)

set(COLLABORATION_SOURCES
    src/collaboration/collaboration_edit.cpp
    src/collaboration/collaboration_protocol.cpp
    src/collaboration/collaboration_session.cpp)

set(PROJECT_INCLUDE_DIRECTORIES
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/collaboration
    ${PROJECT_SOURCE_DIR}/src/editor
    ${PROJECT_SOURCE_DIR}/src/emf
    ${PROJECT_SOURCE_DIR}/src/graphics
    ${PROJECT_SOURCE_DIR}/src/model
    ${PROJECT_SOURCE_DIR}/src/persistence
    ${PROJECT_SOURCE_DIR}/src/ui)

add_executable(EndlessMapStudio WIN32
    src/main.cpp
    src/editor/editor_camera.cpp
    src/editor/editor_state.cpp
    src/editor/edit_history.cpp
    src/editor/map_operations.cpp
    src/emf/emf_io.cpp
    src/graphics/gfx_assets.cpp
    src/ui/classic_ui.cpp
    src/persistence/save_workflow.cpp
    src/collaboration/collaboration_window.cpp
    ${COLLABORATION_SOURCES})
add_executable(editor_camera_tests tests/editor_camera_tests.cpp src/editor/editor_camera.cpp)
add_executable(editor_state_tests tests/editor_state_tests.cpp src/editor/editor_state.cpp src/editor/editor_camera.cpp)
add_executable(collaboration_tests tests/collaboration_tests.cpp ${COLLABORATION_SOURCES})
add_executable(save_workflow_tests tests/save_workflow_tests.cpp src/persistence/save_workflow.cpp)

foreach(project_target IN ITEMS EndlessMapStudio editor_camera_tests editor_state_tests collaboration_tests save_workflow_tests)
    target_include_directories(${project_target} PRIVATE ${PROJECT_INCLUDE_DIRECTORIES})
endforeach()
target_link_libraries(EndlessMapStudio PRIVATE eolib bcrypt comctl32 comdlg32 gdi32 msimg32 shell32 user32 uxtheme ws2_32)
target_link_libraries(collaboration_tests PRIVATE bcrypt ws2_32)
target_compile_definitions(EndlessMapStudio PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_definitions(collaboration_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_definitions(save_workflow_tests PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
add_custom_command(
    TARGET EndlessMapStudio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:eolib>
        $<TARGET_FILE_DIR:EndlessMapStudio>
)

if(MSVC)
    target_compile_options(EndlessMapStudio PRIVATE /W4 /permissive-)
else()
    target_compile_options(EndlessMapStudio PRIVATE -Wall -Wextra -Wpedantic)
endif()

enable_testing()
add_test(
    NAME emf_header_sample
    COMMAND $<TARGET_FILE:EndlessMapStudio> --check-emf "${PROJECT_SOURCE_DIR}/data/maps/00000.emf"
)
add_test(
    NAME emf_roundtrip_sample
    COMMAND $<TARGET_FILE:EndlessMapStudio> --roundtrip-emf
        "${PROJECT_SOURCE_DIR}/data/maps/00000.emf"
        "${CMAKE_CURRENT_BINARY_DIR}/roundtrip-00000.emf"
)
add_test(
    NAME emf_edit_roundtrip_sample
    COMMAND $<TARGET_FILE:EndlessMapStudio> --edit-roundtrip-emf
        "${PROJECT_SOURCE_DIR}/data/maps/00000.emf"
        "${CMAKE_CURRENT_BINARY_DIR}/edited-roundtrip-00000.emf"
)
add_test(
    NAME emf_asset_corpus
    COMMAND $<TARGET_FILE:EndlessMapStudio> --check-emf-directory "${PROJECT_SOURCE_DIR}/data/maps"
)
add_test(
    NAME egf_bitmap_resource_sample
    COMMAND $<TARGET_FILE:EndlessMapStudio> --check-egf 3 100
)
add_test(
    NAME egf_palette_bank_enumeration
    COMMAND $<TARGET_FILE:EndlessMapStudio> --check-egf-banks
)

foreach(model_test IN ITEMS
    flag_block_roundtrip
    flag_warp_roundtrip
    chair_direction_roundtrip
    map_resize_grow
    map_resize_shrink
    map_resize_preserves_in_bounds_data
    base_tile_roundtrip
    map_properties_resize
    base_tile_does_not_destroy_ground_graphics
    clear_specific_graphic_layer
    clear_block_flags_only
    clear_warps_only
    entity_npc_roundtrip
    entity_item_roundtrip
    entity_sign_roundtrip)
    add_test(
        NAME ${model_test}
        COMMAND $<TARGET_FILE:EndlessMapStudio> --model-test ${model_test}
            "${PROJECT_SOURCE_DIR}/data/maps/00000.emf"
            "${CMAKE_CURRENT_BINARY_DIR}/${model_test}.emf"
    )
endforeach()

foreach(b3_test IN ITEMS
    b3_base_tile_serialization b3_base_tile_convergence
    b3_resize_grow_convergence b3_resize_shrink_convergence b3_resize_rejects_oob_pending
    b3_clear_graphic_serialization_convergence b3_clear_flag_serialization_convergence
    b3_clear_all_convergence b3_clear_then_set_ordering
    b3_sign_serialization b3_sign_multiline_convergence b3_sign_delete b3_sign_stale_generation
    b3_npc_collection_serialization b3_npc_multiple_convergence b3_npc_delete b3_npc_clear
    b3_npc_invalid_atomic_rejection b3_npc_stale_generation
    b3_item_collection_serialization b3_item_multiple_convergence b3_item_delete b3_item_clear
    b3_item_invalid_atomic_rejection b3_item_stale_generation
    b3_copy_no_network b3_cancel_no_network b3_paste_save_convergence
    b3_unrelated_scope_generation b3_same_scope_generation b3_remote_dirty_form_preserved
    b3_warp_shared_state b3_warp_shared_generation)
    add_test(NAME collaboration_${b3_test} COMMAND $<TARGET_FILE:collaboration_tests> ${b3_test})
endforeach()

foreach(cd_test IN ITEMS
    cd_presence_serialization cd_presence_no_revision cd_presence_no_dirty cd_viewer_tile_update cd_active_area_update
    cd_invalid_active_area cd_invalid_tile_presence cd_participant_color_assignment cd_display_name_validation
    cd_duplicate_display_name cd_disconnect_clears_presence
    cd_host_role cd_editor_mutation cd_viewer_mutation_rejected cd_viewer_rejection_revision cd_viewer_presence
    cd_editor_to_viewer cd_viewer_to_editor cd_host_cannot_demote cd_disconnect_participant cd_disconnected_cannot_mutate
    cd_no_password cd_correct_password cd_incorrect_password cd_add_password cd_change_password cd_remove_password
    cd_authenticated_survives_password_change cd_password_not_exported)
    add_test(NAME collaboration_${cd_test} COMMAND $<TARGET_FILE:collaboration_tests> ${cd_test})
endforeach()

foreach(camera_test IN ITEMS roundtrip zoom_anchor pan extents held_navigation diagonal_navigation idle_navigation navigation_reset)
    add_test(
        NAME camera_${camera_test}
        COMMAND $<TARGET_FILE:editor_camera_tests> ${camera_test}
    )
endforeach()

foreach(state_test IN ITEMS
    object_cluster_places_objects
    mask_cluster_preserves_category
    switching_brush_does_not_change_graphics_category
    switching_graphics_category_does_not_change_brush_shape
    switching_tool_does_not_change_selected_graphic
    flag_selection_does_not_destroy_graphic_selection
    camera_line_scroll_preserves_editor_state)
    add_test(NAME state_${state_test} COMMAND $<TARGET_FILE:editor_state_tests> ${state_test})
endforeach()

foreach(collaboration_test IN ITEMS protocol_roundtrip password_verification malformed_rejection loopback wrong_password incompatible_protocol
    chat_protocol chat_validation participant_protocol chat_flow participant_lifecycle disconnected_send maximum_participants unauthenticated_chat
    extended_protocol operation_ordering presence_revision save_event_flow edit_serialization model_convergence semantic_network)
    add_test(NAME collaboration_${collaboration_test} COMMAND $<TARGET_FILE:collaboration_tests> ${collaboration_test})
endforeach()

foreach(save_test IN ITEMS
    v1_session_backup_creation v1_session_backup_emf_bytes v1_session_backup_timestamp v1_session_backup_collision
    v1_session_backup_sanitization v1_presave_backup_creation v1_presave_backup_original_bytes v1_presave_backup_before_write
    v1_backup_directory_placement v1_backup_failure_blocks_save
    v1_backup_does_not_clear_dirty v1_unsaved_session_backup
    v1_metadata_creation v1_metadata_version v1_metadata_map_name v1_metadata_exact_comment v1_metadata_blank_comment
    v1_metadata_unicode_comment v1_metadata_multiline_comment v1_metadata_comment_limit v1_metadata_file_size
    v1_metadata_collaborative_author v1_metadata_no_sensitive_fields
    v1_metadata_timestamp v1_metadata_malformed_prior_rewritten
    v1_disconnected_save_allowed v1_host_save_allowed v1_editor_save_denied v1_viewer_save_denied v1_denial_message
    v1_denied_no_dialog v1_denied_no_disk v1_denied_no_event v1_denied_no_revision v1_save_event_after_success
    v1_save_event_no_revision
    v1_client_denial_preserves_dirty v1_save_preserves_semantic_bytes v1_save_event_host_only v1_save_event_no_path
    v1_comment_cancel_aborts v1_map_write_failure_preserves_original v1_replace_failure_preserves_original
    v1_backup_failure_preserves_original v1_metadata_failure_keeps_map v1_map_failure_no_metadata v1_temp_cleanup_on_failure
    v1_map_failure_preserves_dirty v1_map_failure_no_event v1_save_as_failure_preserves_path
    v1_result_filename v1_result_size v1_result_date v1_result_exact_comment v1_result_blank_comment v1_result_multiline_comment)
    add_test(NAME save_${save_test} COMMAND $<TARGET_FILE:save_workflow_tests> ${save_test})
endforeach()
