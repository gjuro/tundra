# Define target name and output directory
init_target (SceneWidgetComponents OUTPUT plugins)

MocFolder ()

# Define source files
file (GLOB CPP_FILES *.cpp)
file (GLOB H_FILES *.h)
file (GLOB MOC_FILES ${H_FILES})
set (SOURCE_FILES ${CPP_FILES} ${H_FILES})

# This controls if we link against BrowserUiPlugin
# to get shared data accross all EC_WebView components.
# This is optional and can be turned off with setting this to 0.
set (BROWSER_SHARED_DATA_ENABLED 1)

# Qt4 Wrap
QT4_WRAP_CPP(MOC_SRCS ${MOC_FILES})

use_core_modules (Framework Math Ui Input Scene Asset OgreRenderingModule TundraProtocolModule)

if (BROWSER_SHARED_DATA_ENABLED)
    use_app_modules(BrowserUiPlugin)
endif ()

build_library (${TARGET_NAME} SHARED ${SOURCE_FILES} ${MOC_SRCS})

link_ogre()
link_modules (Framework Math Ui Input Scene Asset OgreRenderingModule TundraProtocolModule)
link_entity_components (EC_Billboard)

if (BROWSER_SHARED_DATA_ENABLED)
    link_modules(BrowserUiPlugin)
    add_definitions (-DSCENEWIDGET_BROWSER_SHARED_DATA)
endif ()

SetupCompileFlags()

final_target ()
