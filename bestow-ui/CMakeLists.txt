# bestow-ui - UI System using RmlUi

add_library(bestow-ui)
target_use_std_module(bestow-ui)

target_sources(bestow-ui
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.ui.impl.cppm
)

# Find RmlUi
find_package(RmlUi CONFIG QUIET)

if(RmlUi_FOUND)
    target_link_libraries(bestow-ui
        PUBLIC
            bestow-contract
            RmlUi::RmlUi
            glfw
            glad::glad
    )
    target_compile_definitions(bestow-ui PRIVATE BESTOW_HAS_RMLUI)
    message(STATUS "RmlUi found - UI system enabled")
else()
    # Fallback: stub implementation without RmlUi
    target_link_libraries(bestow-ui
        PUBLIC
            bestow-contract
            glfw
    )
    message(STATUS "RmlUi not found - UI system will use stub implementation")
endif()

target_compile_features(bestow-ui PUBLIC cxx_std_23)
