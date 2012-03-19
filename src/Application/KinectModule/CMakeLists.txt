
macro (configure_kinect)
    if (WIN32 AND VC100)
        message ("** Configuring KINECT")
        message (STATUS "Looking for Microsoft Kinect for Windows SDK 1.0")

        # Setup the find paths for Microsoft Kinect for Windows 1.0.
        # These are the default paths for the installer, user cannot change them.
        set (_KINECT_FIND_PATHS 
            "C:/Program Files/Microsoft SDKs/Kinect/v1.0/inc"
            "C:/Program Files/Microsoft SDKs/Kinect/v1.0/lib/x86"
            "C:/Program Files/Microsoft SDKs/Kinect/v1.0/lib/amd64"
            "C:/Program Files (x86)/Microsoft SDKs/Kinect/v1.0/inc"
            "C:/Program Files (x86)/Microsoft SDKs/Kinect/v1.0/lib/x86"
            "C:/Program Files (x86)/Microsoft SDKs/Kinect/v1.0/lib/amd64"
        )
        
        find_path (KINECT_INCLUDE_DIRS NuiApi.h PATHS ${_KINECT_FIND_PATHS})   
        find_path (KINECT_LIBRARY_DIRS Kinect10.lib PATHS ${_KINECT_FIND_PATHS})
        find_library (KINECT_LIBRARIES Kinect10 PATHS ${_KINECT_FIND_PATHS})

        if (${KINECT_INCLUDE_DIRS} STREQUAL "KINECT_INCLUDE_DIRS-NOTFOUND" OR
            ${KINECT_LIBRARY_DIRS} STREQUAL "KINECT_LIBRARY_DIRS-NOTFOUND" OR
            ${KINECT_LIBRARIES} STREQUAL "KINECT_LIBRARIES-NOTFOUND")
            message (FATAL_ERROR "-- Could not automatically find Microsoft Kinect for Windows SDK.")
        endif ()

        # Report findings
        message (STATUS "-- Include Directories:")
        message (STATUS "       " ${KINECT_INCLUDE_DIRS})
        message (STATUS "-- Library Directories:")
        message (STATUS "       " ${KINECT_LIBRARY_DIRS})
        message (STATUS "-- Libraries:")
        message (STATUS "       " ${KINECT_LIBRARIES})
        message ("")
    endif ()    
endmacro ()

if (WIN32 AND VC100)
    configure_kinect ()

    # Define target name and output directory
    init_target (KinectModule OUTPUT plugins)

    # Define source files
    file (GLOB CPP_FILES *.cpp)
    file (GLOB H_FILES *.h )
    file (GLOB UI_FILES ui/*.ui)

    # Qt4 Moc files to subgroup "CMake Moc"
    # and ui_*.h generated .h files to "Generated UI" subgroup
    MocFolder ()
    UiFolder ()

    # Qt4 Wrap 
    QT4_WRAP_CPP(MOC_SRCS ${H_FILES})
    QT4_WRAP_UI(UI_SRCS ${UI_FILES})

    set (SOURCE_FILES ${SOURCE_FILES} ${CPP_FILES} ${H_FILES} ${MOC_SRCS} ${UI_SRCS})

    add_definitions (-DKINECT_MODULE_EXPORTS)

    use_core_modules (Framework Math Ui OgreRenderingModule)
    use_app_modules (JavascriptModule)
    use_package (KINECT)
    
    build_library (${TARGET_NAME} SHARED ${SOURCE_FILES})

    link_modules (Framework Math Ui OgreRenderingModule JavascriptModule)
    link_package (KINECT)
    
    SetupCompileFlags()

    final_target ()
endif ()

if (APPLE OR UNIX OR VC90)
    message (FATAL_ERROR "KinectModule can only be included in Windows with a VC100 compiler build at the moment. Try cmake_vs2010.bat with Visual Studio 2010 installed to enable kinect functionality.")
endif ()
