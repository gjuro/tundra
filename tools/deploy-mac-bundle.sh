#!/bin/sh

echo "####### DEPLOYING TUNDRA 2 BUNDLE APPLICATION #######"

TUNDRA_PWD= #Specify your tundra working directory
TUNDRA_DEPS= #Specify your dependencies directory
LPATH=$TUNDRA_DEPS/lib
QT_LPATH=/usr/local/Trolltech/Qt-4.7.1/lib
OGRE_LPATH=$TUNDRA_DEPS/OgreSDK/lib
QTPROPERTYBROWSER_LPATH=$TUNDRA_DEPS/build/qt-solutions/qtpropertybrowser/lib

echo "### Prepare bundle application ###"

deploy=$TUNDRA_PWD/../deploy2
tundrabundle=$deploy/Tundra.app
mkdir -p $deploy $tundrabundle $tundrabundle/Contents/{Frameworks,lib,Plugins,Components,Resources,MacOS} $tundrabundle/Contents/Resources/Scripts

echo "[Paths]\n
Plugins = Plugins" > $tundrabundle/Contents/Resources/qt.conf

frameworksPWD=$tundrabundle/Contents/Frameworks

echo "### Prepare Qt frameworks ###"

cp -R $QT_LPATH/QtCore.framework $frameworksPWD
cp -R $QT_LPATH/QtGui.framework $frameworksPWD
cp -R $QT_LPATH/QtNetwork.framework $frameworksPWD
cp -R $QT_LPATH/QtXml.framework $frameworksPWD
cp -R $QT_LPATH/QtXmlPatterns.framework $frameworksPWD
cp -R $QT_LPATH/QtScript.framework $frameworksPWD
cp -R $QT_LPATH/QtScriptTools.framework $frameworksPWD
cp -R $QT_LPATH/QtWebKit.framework $frameworksPWD
cp -R $QT_LPATH/QtSvg.framework $frameworksPWD
cp -R $QT_LPATH/QtSql.framework $frameworksPWD
cp -R $QT_LPATH/QtOpenGL.framework $frameworksPWD
cp -R $QT_LPATH/QtDeclarative.framework $frameworksPWD
cp -R $QT_LPATH/phonon.framework $frameworksPWD

echo "### Qt: Prepare Qt Plugins ###"

cp -R $QT_LPATH/../plugins/* $tundrabundle/Contents/Plugins

echo "### Qt: Remove debug files ###"

debuginfo=`find $frameworksPWD -name "*_debug*"`
rm $debuginfo

debuginfo=`find $tundrabundle/Contents/Plugins -name "*_debug*"`
rm $debuginfo

echo "### Qt: Change install information ###"

install_name_tool -id @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtCore.framework/Versions/4/QtCore
install_name_tool -id @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $frameworksPWD/QtGui.framework/Versions/4/QtGui
install_name_tool -id @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $frameworksPWD/QtNetwork.framework/Versions/4/QtNetwork
install_name_tool -id @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $frameworksPWD/QtXml.framework/Versions/4/QtXml
install_name_tool -id @executable_path/../Frameworks/QtXmlPatterns.framework/Versions/4/QtXmlPatterns $frameworksPWD/QtXmlPatterns.framework/Versions/4/QtXmlPatterns
install_name_tool -id @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $frameworksPWD/QtScript.framework/Versions/4/QtScript
install_name_tool -id @executable_path/../Frameworks/QtScriptTools.framework/Versions/4/QtScriptTools $frameworksPWD/QtScriptTools.framework/Versions/4/QtScriptTools 
install_name_tool -id @executable_path/../Frameworks/QtWebKit.framework/Versions/4/QtWebKit $frameworksPWD/QtWebKit.framework/Versions/4/QtWebKit
install_name_tool -id @executable_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL $frameworksPWD/QtOpenGL.framework/Versions/4/QtOpenGL
install_name_tool -id @executable_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg $frameworksPWD/QtSvg.framework/Versions/4/QtSvg
install_name_tool -id @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $frameworksPWD/QtSql.framework/Versions/4/QtSql
install_name_tool -id @executable_path/../Frameworks/QtDeclarative.framework/Versions/4/QtDeclarative $frameworksPWD/QtDeclarative.framework/Versions/4/QtDeclarative
install_name_tool -id @executable_path/../Frameworks/phonon.framework/Versions/4/phonon $frameworksPWD/phonon.framework/Versions/4/phonon

echo "### Qt: Change dependencies dyld paths ###"
echo "# QtGui depends on QtCore"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtGui.framework/Versions/4/QtGui

echo "# QtNetwork depends on QtCore"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtNetwork.framework/Versions/4/QtNetwork

echo "# QtXml depends on QtCore"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtXml.framework/Versions/4/QtXml

echo "# QtXmlPatterns depends on QtCore, QtNetwork"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtXmlPatterns.framework/Versions/4/QtXmlPatterns
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $frameworksPWD/QtXmlPatterns.framework/Versions/4/QtXmlPatterns

echo "# QtScript depends on QtCore"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtScript.framework/Versions/4/QtScript

echo "# QtScriptTools depends on QtCore, QtGui, QtScript"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtScriptTools.framework/Versions/4/QtScriptTools
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $frameworksPWD/QtScriptTools.framework/Versions/4/QtScriptTools
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $frameworksPWD/QtScriptTools.framework/Versions/4/QtScriptTools

echo "# QtWebKit depends on QtCore, QtGui, QtNetwork, Phonon"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtWebKit.framework/Versions/4/QtWebKit
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $frameworksPWD/QtWebKit.framework/Versions/4/QtWebKit
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $frameworksPWD/QtWebKit.framework/Versions/4/QtWebKit
install_name_tool -change $QT_LPATH/phonon.framework/Versions/4/phonon @executable_path/../Frameworks/phonon.framework/Versions/4/phonon $frameworksPWD/QtWebKit.framework/Versions/4/QtWebKit

echo "# QtOpenGL depends on QtCore, QtGui"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtOpenGL.framework/Versions/4/QtOpenGL
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $frameworksPWD/QtOpenGL.framework/Versions/4/QtOpenGL

echo "# QtSvg depens on QtCore, QtGui"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtSvg.framework/Versions/4/QtSvg
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $frameworksPWD/QtSvg.framework/Versions/4/QtSvg

echo "# QtSql depends on QtCore"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtSql.framework/Versions/4/QtSql

echo "# QtDeclarative depends on QtCore, QtGui, QtNetwork, QtOpenGL, QtXmlPatterns, QtSql, QtSvg, QtScript"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/QtDeclarative.framework/Versions/4/QtDeclarative
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $frameworksPWD/QtDeclarative.framework/Versions/4/QtDeclarative
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $frameworksPWD/QtDeclarative.framework/Versions/4/QtDeclarative
install_name_tool -change $QT_LPATH/QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL $frameworksPWD/QtDeclarative.framework/Versions/4/QtDeclarative
install_name_tool -change $QT_LPATH/QtXmlPatterns.framework/Versions/4/QtXmlPatterns @executable_path/../Frameworks/QtXmlPatterns.framework/Versions/4/QtXmlPatterns $frameworksPWD/QtDeclarative.framework/Versions/4/QtDeclarative
install_name_tool -change $QT_LPATH/QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $frameworksPWD/QtDeclarative.framework/Versions/4/QtDeclarative
install_name_tool -change $QT_LPATH/QtSvg.framework/Versions/4/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg $frameworksPWD/QtDeclarative.framework/Versions/4/QtDeclarative
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $frameworksPWD/QtDeclarative.framework/Versions/4/QtDeclarative

echo "# Phonon depends on QtCore, QtGui"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $frameworksPWD/phonon.framework/Versions/4/phonon
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $frameworksPWD/phonon.framework/Versions/4/phonon

echo "### Change Qt plugins install information ###"
echo "# accessible"
install_name_tool -change $QT_LPATH/Qt3Support.framework/Versions/4/Qt3Support @executable_path/../Frameworks/Qt3Support.framework/Versions/4/Qt3Support $tundrabundle/Contents/Plugins/accessible/libqtaccessiblecompatwidgets.dylib
install_name_tool -change $QT_LPATH/QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $tundrabundle/Contents/Plugins/accessible/libqtaccessiblecompatwidgets.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/accessible/libqtaccessiblecompatwidgets.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/Plugins/accessible/libqtaccessiblecompatwidgets.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/accessible/libqtaccessiblecompatwidgets.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/accessible/libqtaccessiblecompatwidgets.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/accessible/libqtaccessiblewidgets.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/accessible/libqtaccessiblewidgets.dylib

echo "# bearer"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/bearer/libqgenericbearer.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/Plugins/bearer/libqgenericbearer.dylib

install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/bearer/libqcorewlanbearer.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/bearer/libqcorewlanbearer.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/Plugins/bearer/libqcorewlanbearer.dylib

echo "# codecs"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/codecs/libqcncodecs.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/codecs/libqjpcodecs.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/codecs/libqkrcodecs.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/codecs/libqtwcodecs.dylib

echo "# designer"
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/designer/libarthurplugin.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/designer/libarthurplugin.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/designer/libarthurplugin.dylib
install_name_tool -change $QT_LPATH/QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL $tundrabundle/Contents/Plugins/designer/libarthurplugin.dylib
install_name_tool -change $QT_LPATH/QtDesigner.framework/Versions/4/QtDesigner @executable_path/../Frameworks/QtDesigner.framework/Versions/4/QtDesigner $tundrabundle/Contents/Plugins/designer/libarthurplugin.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/Plugins/designer/libarthurplugin.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/designer/libcustomwidgetplugin.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/designer/libcustomwidgetplugin.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/designer/libcustomwidgetplugin.dylib
install_name_tool -change $QT_LPATH/QtDesigner.framework/Versions/4/QtDesigner @executable_path/../Frameworks/QtDesigner.framework/Versions/4/QtDesigner $tundrabundle/Contents/Plugins/designer/libcustomwidgetplugin.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/Plugins/designer/libcustomwidgetplugin.dylib

install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtDeclarative/Versions/4/QtDeclarative @executable_path/../Frameworks/QtDeclarative.framework/Versions/4/QtDeclarative $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtSvg.framework/Versions/4/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtXmlPatterns.framework/Versions/4/QtXmlPatterns @executable_path/../Frameworks/QtXmlPatterns.framework/Versions/4/QtXmlPatterns $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtDesigner.framework/Versions/4/QtDesigner @executable_path/../Frameworks/QtDesigner.framework/Versions/4/QtDesigner $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/Plugins/designer/libqdeclarativeview.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/designer/libqwebview.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/designer/libqwebview.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/designer/libqwebview.dylib
install_name_tool -change $QT_LPATH/QtDesigner.framework/Versions/4/QtDesigner @executable_path/../Frameworks/QtDesigner.framework/Versions/4/QtDesigner $tundrabundle/Contents/Plugins/designer/libqwebview.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/Plugins/designer/libqwebview.dylib
install_name_tool -change $QT_LPATH/QtWebKit.framework/Versions/4/QtWebKit @executable_path/../Frameworks/QtWebKit.framework/Versions/4/QtWebKit $tundrabundle/Contents/Plugins/designer/libqwebview.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/designer/libworldtimeclockplugin.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/designer/libworldtimeclockplugin.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/designer/libworldtimeclockplugin.dylib
install_name_tool -change $QT_LPATH/QtDesigner.framework/Versions/4/QtDesigner @executable_path/../Frameworks/QtDesigner.framework/Versions/4/QtDesigner $tundrabundle/Contents/Plugins/designer/libworldtimeclockplugin.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/Plugins/designer/libworldtimeclockplugin.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/designer/libphononwidgets.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/designer/libphononwidgets.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/designer/libphononwidgets.dylib
install_name_tool -change $QT_LPATH/QtDesigner.framework/Versions/4/QtDesigner @executable_path/../Frameworks/QtDesigner.framework/Versions/4/QtDesigner $tundrabundle/Contents/Plugins/designer/libphononwidgets.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/Plugins/designer/libphononwidgets.dylib
install_name_tool -change $QT_LPATH/phonon.framework/Versions/4/phonon @executable_path/../Frameworks/phonon.framework/Versions/4/phonon $tundrabundle/Contents/Plugins/designer/libphononwidgets.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/designer/libqt3supportwidgets.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/designer/libqt3supportwidgets.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/designer/libqt3supportwidgets.dylib
install_name_tool -change $QT_LPATH/QtDesigner.framework/Versions/4/QtDesigner @executable_path/../Frameworks/QtDesigner.framework/Versions/4/QtDesigner $tundrabundle/Contents/Plugins/designer/libqt3supportwidgets.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/Plugins/designer/libqt3supportwidgets.dylib
install_name_tool -change $QT_LPATH/QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $tundrabundle/Contents/Plugins/designer/libqt3supportwidgets.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/Plugins/designer/libqt3supportwidgets.dylib
install_name_tool -change $QT_LPATH/Qt3Support.framework/Versions/4/Qt3Support @executable_path/../Frameworks/Qt3Support.framework/Versions/4/Qt3Support $tundrabundle/Contents/Plugins/designer/libqt3supportwidgets.dylib

echo "# graphicssystems"
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/graphicssystems/libqglgraphicssystem.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/graphicssystems/libqglgraphicssystem.dylib
install_name_tool -change $QT_LPATH/QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL $tundrabundle/Contents/Plugins/graphicssystems/libqglgraphicssystem.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/graphicssystems/libqtracegraphicssystem.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/graphicssystems/libqtracegraphicssystem.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/Plugins/graphicssystems/libqtracegraphicssystem.dylib

echo "# iconengines"
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/iconengines/libqsvgicon.dylib
install_name_tool -change $QT_LPATH/QtSvg.framework/Versions/4/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg $tundrabundle/Contents/Plugins/iconengines/libqsvgicon.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/iconengines/libqsvgicon.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/iconengines/libqsvgicon.dylib

echo "# imageformats"
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/imageformats/libqgif.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/imageformats/libqgif.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/imageformats/libqico.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/imageformats/libqico.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/imageformats/libqjpeg.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/imageformats/libqjpeg.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/imageformats/libqmng.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/imageformats/libqmng.dylib

install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/Plugins/imageformats/libqsvg.dylib
install_name_tool -change $QT_LPATH/QtSvg.framework/Versions/4/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg $tundrabundle/Contents/Plugins/imageformats/libqsvg.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/imageformats/libqsvg.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/imageformats/libqsvg.dylib

install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/imageformats/libqtiff.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/imageformats/libqtiff.dylib

echo "# phonon_backend"
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/Plugins/phonon_backend/libphonon_qt7.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/phonon_backend/libphonon_qt7.dylib
install_name_tool -change $QT_LPATH/QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL $tundrabundle/Contents/Plugins/phonon_backend/libphonon_qt7.dylib
install_name_tool -change $QT_LPATH/phonon.framework/Versions/4/phonon @executable_path/../Frameworks/phonon.framework/Versions/4/phonon $tundrabundle/Contents/Plugins/phonon_backend/libphonon_qt7.dylib

echo "# sqldrivers"
install_name_tool -change $QT_LPATH/QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $tundrabundle/Contents/Plugins/sqldrivers/libqsqlite.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/sqldrivers/libqsqlite.dylib

install_name_tool -change $QT_LPATH/QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $tundrabundle/Contents/Plugins/sqldrivers/libqsqlodbc.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/Plugins/sqldrivers/libqsqlodbc.dylib

echo "### Prepare Ogre ###"

cp $OGRE_LPATH/Plugin_* $tundrabundle/Contents/Plugins
cp $OGRE_LPATH/RenderSystem_GL.dylib $tundrabundle/Contents/Plugins
cp $OGRE_LPATH/libOgre* $tundrabundle/Contents/Components
cp -R $OGRE_LPATH/release/Ogre.framework $frameworksPWD
cp -R /Library/Frameworks/Cg.framework $frameworksPWD
chmod -R u+w $frameworksPWD/Cg.framework

install_name_tool -change @executable_path/../Library/Frameworks/Cg.framework/Cg @executable_path/../Frameworks/Cg.framework/Cg $tundrabundle/Contents/Plugins/Plugin_CgProgramManager.dylib

echo "### Prepare Sparkle ###"
cp -R $HOME/Library/Frameworks/Sparkle.framework $frameworksPWD
install_name_tool -id @executable_path/../Frameworks/Sparkle.framework/Versions/A/Sparkle $frameworksPWD/Sparkle.framework/Versions/A/Sparkle

echo "### Prepare Tundra ###"

cp -R $TUNDRA_PWD/bin/* $tundrabundle/Contents/MacOS
cp $LPATH/libogg.0.dylib $LPATH/libcelt0.2.dylib $LPATH/libmumbleclient.dylib $LPATH/libprotobuf.7.dylib $tundrabundle/Contents/lib
cp $QTPROPERTYBROWSER_LPATH/libQtSolutions_PropertyBrowser-head.1.0.0.dylib $tundrabundle/Contents/lib

echo "### Cleaning up ###"

rm $tundrabundle/Contents/MacOS/RenderSystem_GL.dylib
rm $tundrabundle/Contents/MacOS/libOgre*
rm $tundrabundle/Contents/MacOS/Plugin_*
mv $tundrabundle/Contents/MacOS/libboost_regex-xgcc42-mt-1_46_1.dylib $tundrabundle/Contents/MacOS/libboost_thread-xgcc42-mt-1_46_1.dylib $tundrabundle/Contents/lib/
mv $tundrabundle/Contents/MacOS/lib* $tundrabundle/Contents/lib/
mv $tundrabundle/Contents/lib/lib $tundrabundle/Contents/MacOS

echo "### Changing dependencies install information ###"

echo "# PythonQt"
install_name_tool -id @executable_path/../lib/libPythonQt.1.dylib $tundrabundle/Contents/lib/libPythonQt.1.0.0.dylib
install_name_tool -id @executable_path/../lib/libPythonQt_QtAll.1.dylib $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib

echo "--- libPythonQt.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/lib/libPythonQt.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/lib/libPythonQt.1.0.0.dylib


echo "--- libPythonQt_QtAll.1.0.0.dylib ---"
install_name_tool -change libPythonQt.1.dylib @executable_path/../lib/libPythonQt.1.dylib $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtXmlPatterns.framework/Versions/4/QtXmlPatterns @executable_path/../Frameworks/QtXmlPatterns.framework/Versions/4/QtXmlPatterns $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtWebKit.framework/Versions/4/QtWebKit @executable_path/../Frameworks/QtWebKit.framework/Versions/4/QtWebKit $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtSvg.framework/Versions/4/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/lib/libPythonQt_QtAll.1.0.0.dylib

echo "# SkyX / Hydrax"
install_name_tool -id @executable_path/../lib/libhydrax.0.5.3.dylib $tundrabundle/Contents/lib/libhydrax.0.5.3.dylib
install_name_tool -id @executable_path/../lib/libskyx.0.1.1.dylib $tundrabundle/Contents/lib/libskyx.0.1.1.dylib

#echo "# Theora"
#install_name_tool -id @executable_path/../lib/libtheora.0.dylib $tundrabundle/Contents/lib/libtheora.0.dylib
#install_name_tool -id @executable_path/../lib/libtheoradec.1.dylib $tundrabundle/Contents/lib/libtheoradec.1.dylib
#install_name_tool -id @executable_path/../lib/libtheoraenc.1.dylib $tundrabundle/Contents/lib/libtheoraenc.1.dylib
#install_name_tool -change $LPATH/libogg.0.dylib @executable_path/../lib/libogg.0.dylib $tundrabundle/Contents/lib/libtheora.0.dylib
#install_name_tool -change $LPATH/libogg.0.dylib @executable_path/../lib/libogg.0.dylib $tundrabundle/Contents/lib/libtheoraenc.1.dylib

echo "# CELT"
install_name_tool -id @executable_path/../lib/libcelt0.2.dylib $tundrabundle/Contents/lib/libcelt0.2.dylib

echo "# Protobuf"
install_name_tool -id @executable_path/../lib/libprotobuf.7.dylib $tundrabundle/Contents/lib/libprotobuf.7.dylib

echo "# Mumbleclient"
install_name_tool -id @executable_path/../lib/libmumbleclient.dylib $tundrabundle/Contents/lib/libmumbleclient.dylib
install_name_tool -change $LPATH/libprotobuf.7.dylib @executable_path/../lib/libprotobuf.7.dylib $tundrabundle/Contents/lib/libmumbleclient.dylib

echo "# QtPropertyBrowser"
install_name_tool -id @executable_path/../lib/libQtSolutions_PropertyBrowser-head.1.0.0.dylib $tundrabundle/Contents/lib/libQtSolutions_PropertyBrowser-head.1.0.0.dylib

echo "--- libQtSolutions_PropertyBrowser-head.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/lib/libQtSolutions_PropertyBrowser-head.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/lib/libQtSolutions_PropertyBrowser-head.1.0.0.dylib

echo "# OGG"
install_name_tool -id @executable_path/../lib/libogg.0.dylib $tundrabundle/Contents/lib/libogg.0.dylib
#install_name_tool -id @executable_path/../lib/libvorbisfile.3.dylib $tundrabundle/Contents/lib/libvorbisfile.3.dylib
#install_name_tool -id @executable_path/../lib/libvorbis.0.dylib $tundrabundle/Contents/lib/libvorbis.0.dylib

#install_name_tool -change $LPATH/libogg.0.dylib @executable_path/../lib/libogg.0.dylib $tundrabundle/Contents/lib/libvorbisfile.3.dylib
#install_name_tool -change $LPATH/libvorbis.0.dylib @executable_path/../lib/libvorbis.0.dylib $tundrabundle/Contents/lib/libvorbisfile.3.dylib
#install_name_tool -change $LPATH/libogg.0.dylib @executable_path/../lib/libogg.0.dylib $tundrabundle/Contents/lib/libvorbis.0.dylib

echo "### Changing Tundra modules install information ###"

echo "--- AssetInterestPlugin.dylib ---"
install_name_tool -id @executable_path/plugins/AssetInterestPlugin.dylib $tundrabundle/Contents/MacOS/plugins/AssetInterestPlugin.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/AssetInterestPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/AssetInterestPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/AssetInterestPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/AssetInterestPlugin.dylib

echo "--- AssetModule.dylib ---"
install_name_tool -id @executable_path/plugins/AssetModule.dylib $tundrabundle/Contents/MacOS/plugins/AssetModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/AssetModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/AssetModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/AssetModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/AssetModule.dylib

echo "--- AvatarModule.dylib ---"
install_name_tool -id @executable_path/plugins/AvatarModule.dylib $tundrabundle/Contents/MacOS/plugins/AvatarModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/AvatarModule.dylib

echo "--- BrowserUiPlugin.dylib ---"
install_name_tool -id @executable_path/plugins/BrowserUiPlugin.dylib $tundrabundle/Contents/MacOS/plugins/BrowserUiPlugin.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/JavascriptModule.dylib @executable_path/plugins/JavascriptModule.dylib $tundrabundle/Contents/MacOS/plugins/BrowserUiPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/BrowserUiPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/BrowserUiPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/BrowserUiPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/BrowserUiPlugin.dylib

echo "--- CAVEStereoModule.dylib ---"
install_name_tool -id @executable_path/plugins/CAVEStereoModule.dylib $tundrabundle/Contents/MacOS/plugins/CAVEStereoModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/CAVEStereoModule.dylib

echo "--- DebugStatsModule.dylib ---"
install_name_tool -id @executable_path/plugins/DebugStatsModule.dylib $tundrabundle/Contents/MacOS/plugins/DebugStatsModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/DebugStatsModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/DebugStatsModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/AssetModule.dylib @executable_path/plugins/AssetModule.dylib $tundrabundle/Contents/MacOS/plugins/DebugStatsModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/DebugStatsModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreAssetEditorModule.dylib @executable_path/plugins/OgreAssetEditorModule.dylib $tundrabundle/Contents/MacOS/plugins/DebugStatsModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/DebugStatsModule.dylib

echo "--- ECEditorModule.dylib ---"
install_name_tool -id @executable_path/plugins/ECEditorModule.dylib $tundrabundle/Contents/MacOS/plugins/ECEditorModule.dylib

install_name_tool -change $QTPROPERTYBROWSER_LPATH/libQtSolutions_PropertyBrowser-head.1.dylib @executable_path/../lib/libQtSolutions_PropertyBrowser-head.1.dylib $tundrabundle/Contents/MacOS/plugins/ECEditorModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/ECEditorModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/ECEditorModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/ECEditorModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/ECEditorModule.dylib

echo "--- EnvironmentModule.dylib ---"
install_name_tool -id @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/EnvironmentModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/EnvironmentModule.dylib

echo "--- JavascriptModule.dylib ---"
install_name_tool -id @executable_path/plugins/JavascriptModule.dylib $tundrabundle/Contents/MacOS/plugins/JavascriptModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/JavascriptModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/JavascriptModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/JavascriptModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/JavascriptModule.dylib

echo "--- MumbleVoipModule.dylib ---"
install_name_tool -id @executable_path/plugins/MumbleVoipModule.dylib $tundrabundle/Contents/MacOS/plugins/MumbleVoipModule.dylib

install_name_tool -change /Users/cvetan/workspace/COCOA_BUILD/deps/build/mumbleclient/libmumbleclient.dylib @executable_path/../lib/libmumbleclient.dylib $tundrabundle/Contents/MacOS/plugins/MumbleVoipModule.dylib
install_name_tool -change $LPATH/libcelt0.2.dylib @executable_path/../lib/libcelt0.2.dylib $tundrabundle/Contents/MacOS/plugins/MumbleVoipModule.dylib
install_name_tool -change $LPATH/libprotobuf.7.dylib @executable_path/../lib/libprotobuf.7.dylib $tundrabundle/Contents/MacOS/plugins/MumbleVoipModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/MumbleVoipModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/MumbleVoipModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/JavascriptModule.dylib @executable_path/plugins/JavascriptModule.dylib $tundrabundle/Contents/MacOS/plugins/MumbleVoipModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/MumbleVoipModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/MumbleVoipModule.dylib

echo "--- OgreAssetEditorModule.dylib ---"
install_name_tool -id @executable_path/plugins/OgreAssetEditorModule.dylib $tundrabundle/Contents/MacOS/plugins/OgreAssetEditorModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/OgreAssetEditorModule.dylib

echo "--- OgreRenderingModule.dylib ---"
install_name_tool -id @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/OgreRenderingModule.dylib

echo "--- PhysicsModule.dylib ---"
install_name_tool -id @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/PhysicsModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/PhysicsModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/PhysicsModule.dylib

echo "--- PythonScriptModule.dylib ---"
install_name_tool -id @executable_path/plugins/PythonScriptModule.dylib $tundrabundle/Contents/MacOS/plugins/PythonScriptModule.dylib

install_name_tool -change libPythonQt.1.dylib @executable_path/../lib/libPythonQt.1.dylib $tundrabundle/Contents/MacOS/plugins/PythonScriptModule.dylib
install_name_tool -change libPythonQt_QtAll.1.dylib @executable_path/../lib/libPythonQt_QtAll.1.dylib $tundrabundle/Contents/MacOS/plugins/PythonScriptModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/PythonScriptModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/PythonScriptModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/PythonScriptModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/PythonScriptModule.dylib

echo "--- SceneWidgetComponents.dylib ---"
install_name_tool -id @executable_path/plugins/SceneWidgetComponents.dylib $tundrabundle/Contents/MacOS/plugins/SceneWidgetComponents.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/SceneWidgetComponents.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/SceneWidgetComponents.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/SceneWidgetComponents.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/SceneWidgetComponents.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/BrowserUiPlugin.dylib @executable_path/plugins/BrowserUiPlugin.dylib $tundrabundle/Contents/MacOS/plugins/SceneWidgetComponents.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/JavascriptModule.dylib @executable_path/plugins/JavascriptModule.dylib $tundrabundle/Contents/MacOS/plugins/SceneWidgetComponents.dylib

echo "--- SkyXHydrax.dylib ---"
install_name_tool -id @executable_path/plugins/SkyXHydrax.dylib $tundrabundle/Contents/MacOS/plugins/SkyXHydrax.dylib

install_name_tool -change libskyx.0.1.1.dylib @executable_path/../lib/libskyx.0.1.1.dylib $tundrabundle/Contents/MacOS/plugins/SkyXHydrax.dylib
install_name_tool -change libhydrax.0.5.3.dylib @executable_path/../lib/libhydrax.0.5.3.dylib $tundrabundle/Contents/MacOS/plugins/SkyXHydrax.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/SkyXHydrax.dylib


echo "--- TundraProtocolModule.dylib ---"
install_name_tool -id @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/TundraProtocolModule.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/TundraProtocolModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/TundraProtocolModule.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/TundraProtocolModule.dylib

echo "--- VlcPlugin.dylib ---"
install_name_tool -id @executable_path/plugins/VlcPlugin.dylib $tundrabundle/Contents/MacOS/plugins/VlcPlugin.dylib

install_name_tool -change @loader_path/lib/libvlc.5.dylib @executable_path/lib/libvlc.5.dylib $tundrabundle/Contents/MacOS/plugins/VlcPlugin.dylib

install_name_tool -change $TUNDRA_PWD/bin/plugins/EnvironmentModule.dylib @executable_path/plugins/EnvironmentModule.dylib $tundrabundle/Contents/MacOS/plugins/VlcPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/OgreRenderingModule.dylib @executable_path/plugins/OgreRenderingModule.dylib $tundrabundle/Contents/MacOS/plugins/VlcPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/TundraProtocolModule.dylib @executable_path/plugins/TundraProtocolModule.dylib $tundrabundle/Contents/MacOS/plugins/VlcPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/PhysicsModule.dylib @executable_path/plugins/PhysicsModule.dylib $tundrabundle/Contents/MacOS/plugins/VlcPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/SceneWidgetComponents.dylib @executable_path/plugins/SceneWidgetComponents.dylib $tundrabundle/Contents/MacOS/plugins/VlcPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/BrowserUiPlugin.dylib @executable_path/plugins/BrowserUiPlugin.dylib $tundrabundle/Contents/MacOS/plugins/VlcPlugin.dylib
install_name_tool -change $TUNDRA_PWD/bin/plugins/JavascriptModule.dylib @executable_path/plugins/JavascriptModule.dylib $tundrabundle/Contents/MacOS/plugins/VlcPlugin.dylib

echo "--- UpdateModule.dylib ---"
install_name_tool -id @executable_path/plugins/UpdateModule.dylib $tundrabundle/Contents/MacOS/plugins/UpdateModule.dylib

install_name_tool -change @loader_path/../Frameworks/Sparkle.framework/Versions/A/Sparkle @executable_path/../Frameworks/Sparkle.framework/Versions/A/Sparkle $tundrabundle/Contents/MacOS/plugins/UpdateModule.dylib
echo "### Performing final settings ###"

ALLMODULES=$tundrabundle/Contents/MacOS/plugins/*.dylib
ALLMODULES="$ALLMODULES $tundrabundle/Contents/MacOS/Tundra"
for module in $ALLMODULES
do
    install_name_tool -change libboost_thread-xgcc42-mt-1_46_1.dylib @executable_path/../lib/libboost_thread-xgcc42-mt-1_46_1.dylib $module
    install_name_tool -change libboost_regex-xgcc42-mt-1_46_1.dylib @executable_path/../lib/libboost_regex-xgcc42-mt-1_46_1.dylib $module
    
    install_name_tool -change $LPATH/libogg.0.dylib @executable_path/../lib/libogg.0.dylib $module
    #install_name_tool -change $LPATH/libvorbisfile.3.dylib @executable_path/../lib/libvorbisfile.3.dylib $module
    #install_name_tool -change $LPATH/libvorbis.0.dylib @executable_path/../lib/libvorbis.0.dylib $module

    install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $module
    install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $module
    install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $module
    install_name_tool -change $QT_LPATH/QtXmlPatterns.framework/Versions/4/QtXmlPatterns @executable_path/../Frameworks/QtXmlPatterns.framework/Versions/4/QtXmlPatterns $module
    install_name_tool -change $QT_LPATH/QtWebKit.framework/Versions/4/QtWebKit @executable_path/../Frameworks/QtWebKit.framework/Versions/4/QtWebKit $module
    install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $module
    install_name_tool -change $QT_LPATH/QtScriptTools.framework/Versions/4/QtScriptTools @executable_path/../Frameworks/QtScriptTools.framework/Versions/4/QtScriptTools $module
    install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $module
    install_name_tool -change $QT_LPATH/QtSvg.framework/Versions/4/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg $module
    install_name_tool -change $QT_LPATH/QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $module
    install_name_tool -change $QT_LPATH/QtDeclarative.framework/Versions/4/QtDeclarative @executable_path/../Frameworks/QtDeclarative.framework/Versions/4/QtDeclarative $module
done

echo "### Changing QtScript plugins install settings ###"

echo "--- libqtscript_core.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_core.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_core.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_core.1.0.0.dylib

echo "--- libqtscript_gui.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_gui.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_gui.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_gui.1.0.0.dylib

echo "--- libqtscript_network.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_network.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_network.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_network.1.0.0.dylib

echo "--- libqtscript_opengl.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_opengl.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_opengl.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_opengl.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtOpenGL.framework/Versions/4/QtOpenGL @executable_path/../Frameworks/QtOpenGL.framework/Versions/4/QtOpenGL $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_opengl.1.0.0.dylib

echo "--- libqtscript_phonon.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_phonon.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_phonon.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_phonon.1.0.0.dylib
install_name_tool -change $QT_LPATH/phonon.framework/Versions/4/phonon @executable_path/../Frameworks/phonon.framework/Versions/4/phonon $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_phonon.1.0.0.dylib

echo "--- libqtscript_sql.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_sql.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtSql.framework/Versions/4/QtSql @executable_path/../Frameworks/QtSql.framework/Versions/4/QtSql $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_sql.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_sql.1.0.0.dylib

echo "--- libqtscript_svg.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_svg.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_svg.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_svg.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtSvg.framework/Versions/4/QtSvg @executable_path/../Frameworks/QtSvg.framework/Versions/4/QtSvg $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_svg.1.0.0.dylib

echo "--- libqtscript_uitools.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_uitools.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_uitools.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_uitools.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_uitools.1.0.0.dylib

echo "--- libqtscript_webkit.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_webkit.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_webkit.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_webkit.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_webkit.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtWebKit.framework/Versions/4/QtWebKit @executable_path/../Frameworks/QtWebKit.framework/Versions/4/QtWebKit $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_webkit.1.0.0.dylib

echo "--- libqtscript_xml.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_xml.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_xml.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtXml.framework/Versions/4/QtXml @executable_path/../Frameworks/QtXml.framework/Versions/4/QtXml $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_xml.1.0.0.dylib

echo "--- libqtscript_xmlpatterns.1.0.0.dylib ---"
install_name_tool -change $QT_LPATH/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_xmlpatterns.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtNetwork.framework/Versions/4/QtNetwork @executable_path/../Frameworks/QtNetwork.framework/Versions/4/QtNetwork $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_xmlpatterns.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtScript.framework/Versions/4/QtScript @executable_path/../Frameworks/QtScript.framework/Versions/4/QtScript $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_xmlpatterns.1.0.0.dylib
install_name_tool -change $QT_LPATH/QtXmlPatterns.framework/Versions/4/QtXmlPatterns @executable_path/../Frameworks/QtXmlPatterns.framework/Versions/4/QtXmlPatterns $tundrabundle/Contents/MacOS/qtplugins/script/libqtscript_xmlpatterns.1.0.0.dylib

cp $TUNDRA_PWD/tools/installers/mac/Icon.icns $tundrabundle/Contents/Resources
cp $TUNDRA_PWD/tools/installers/mac/Info.plist $tundrabundle/Contents
cp $TUNDRA_PWD/tools/installers/mac/applet.rsrc $tundrabundle/Contents/Resources
cp $TUNDRA_PWD/tools/installers/mac/main.scpt $tundrabundle/Contents/Resources/Scripts
cp $TUNDRA_PWD/tools/installers/mac/dsa_pub.pem $tundrabundle/Contents/Resources
echo "### END DEPLOYING ###"
