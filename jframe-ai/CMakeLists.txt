add_library(jframe-ai)
target_use_std_module(jframe-ai)

target_sources(jframe-ai
    PUBLIC FILE_SET CXX_MODULES FILES
        src/jframe.ai.impl.cppm
    PRIVATE
        src/AISystem.cpp
        src/BehaviorTreeManager.cpp
        src/NavigationSystem.cpp
)

target_link_libraries(jframe-ai
    PUBLIC
        jframe-contract
        jframe-assets
        jframe-physics
        RecastNavigation::Recast
        RecastNavigation::Detour
)

# BehaviorTree.CPP may not be available via vcpkg
if(TARGET BehaviorTree::behaviortree_cpp)
    target_link_libraries(jframe-ai PUBLIC BehaviorTree::behaviortree_cpp)
    target_compile_definitions(jframe-ai PRIVATE JFRAME_HAS_BTCPP)
endif()

target_compile_features(jframe-ai PUBLIC cxx_std_23)
