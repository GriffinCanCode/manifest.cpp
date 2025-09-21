# Networking systems - to be implemented in future phases
add_library(net INTERFACE)

target_include_directories(net 
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(net 
    INTERFACE
        core
)

target_compile_features(net 
    INTERFACE 
        cxx_std_23
)
