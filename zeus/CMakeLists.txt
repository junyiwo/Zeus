set(SOURCE_FILES 
    app.cpp
    stream.cpp
    main.cpp)

set(HEADER_FILES 
    app.h 
    stream.h)

# Set Visual Studio source filters
source_group("Source Files" FILES ${SOURCE_FILES})
source_group("Header Files" FILES ${HEADER_FILES})

add_executable(nvss_sample ${SOURCE_FILES} ${HEADER_FILES})
target_include_directories(nvss_sample PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(nvss_sample PRIVATE nvss_video common_sample common_util OpenCV CUDA)

set_target_properties(nvss_sample PROPERTIES FOLDER SampleApps)

# Copy executables into package folder
if(INSTALL_SDK)
    install(TARGETS nvss_sample RUNTIME DESTINATION ./samples/nvss_sample)

elseif(INSTALL_FLAT)
    install(TARGETS nvss_sample RUNTIME DESTINATION .)
if(WIN32)
    install(FILES $<TARGET_PDB_FILE:nvss_sample> DESTINATION . CONFIGURATIONS Debug)
endif()
endif()
