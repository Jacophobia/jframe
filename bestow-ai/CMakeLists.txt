add_library(bestow-ai)
target_use_std_module(bestow-ai)

target_sources(bestow-ai
    PUBLIC FILE_SET CXX_MODULES FILES
        src/bestow.ai.impl.cppm
    PRIVATE
        src/AISystem.cpp
        src/BehaviorTreeManager.cpp
        src/NavigationSystem.cpp
)

target_link_libraries(bestow-ai
    PUBLIC
        bestow-contract
        bestow-assets
        bestow-physics
        kangaru::kangaru
        RecastNavigation::Recast
        RecastNavigation::Detour
)

# BehaviorTree.CPP may not be available via vcpkg
if(TARGET BehaviorTree::behaviortree_cpp)
    target_link_libraries(bestow-ai PUBLIC BehaviorTree::behaviortree_cpp)
    target_compile_definitions(bestow-ai PRIVATE BESTOW_HAS_BTCPP)
endif()

target_compile_features(bestow-ai PUBLIC cxx_std_23)
