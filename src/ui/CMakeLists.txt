# UI systems - to be implemented in future phases  
add_library(ui INTERFACE)

target_include_directories(ui 
    INTERFACE 
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(ui 
    INTERFACE
        core
        render
)

target_compile_features(ui 
    INTERFACE 
        cxx_std_23
)
