#!/bin/sh

if [ $# -lt 1 ] ; then
  echo "Usage: mk_vtp_src_zips.sh date"
  echo
  echo "Example for October 29 2007: mk_vtp_src_zips.sh 071029"
  exit
fi

SOURCEDIR=/cygdrive/c
TARGETDIR=/cygdrive/c/Distrib
DATE=$1
DIST_FILE1=${TARGETDIR}/vtp-src-${DATE}.zip
DIST_FILE2=${TARGETDIR}/vtp-srcdocs-${DATE}.zip

cd $SOURCEDIR

# Create the archive containing the Source
rm -f $DIST_FILE1

zip $DIST_FILE1 VTP/Make.defs
zip $DIST_FILE1 VTP/Makefile

# zip $DIST_FILE1 VTP/TerrainSDK/configure
# zip $DIST_FILE1 VTP/TerrainSDK/configure.in
zip $DIST_FILE1 VTP/TerrainSDK/install-sh
zip $DIST_FILE1 VTP/TerrainSDK/Makefile

zip $DIST_FILE1 VTP/TerrainSDK/minidata/*

zip $DIST_FILE1 VTP/TerrainSDK/unzip/*

zip $DIST_FILE1 VTP/TerrainSDK/vtdata/*.h
zip $DIST_FILE1 VTP/TerrainSDK/vtdata/*.inl
zip $DIST_FILE1 VTP/TerrainSDK/vtdata/*.cpp
zip $DIST_FILE1 VTP/TerrainSDK/vtdata/*.cfg
zip $DIST_FILE1 VTP/TerrainSDK/vtdata/Makefile
zip $DIST_FILE1 VTP/TerrainSDK/vtdata/*.vcproj

zip $DIST_FILE1 VTP/TerrainSDK/vtdata/shapelib/*
zip $DIST_FILE1 VTP/TerrainSDK/vtdata/triangle/*

zip $DIST_FILE1 VTP/TerrainSDK/vtlib/*
zip $DIST_FILE1 VTP/TerrainSDK/vtlib/core/*
zip $DIST_FILE1 VTP/TerrainSDK/vtlib/vtosg/*
zip $DIST_FILE1 VTP/TerrainSDK/vtlib/vtopensg/*
zip $DIST_FILE1 VTP/TerrainSDK/vtlib/vtsgl/*

zip $DIST_FILE1 VTP/TerrainSDK/xmlhelper/*
zip $DIST_FILE1 VTP/TerrainSDK/xmlhelper/include/*

zip $DIST_FILE1 VTP/TerrainSDK/vtui/*

# zip $DIST_FILE1 VTP/TerrainApps/configure
# zip $DIST_FILE1 VTP/TerrainApps/configure.in
zip $DIST_FILE1 VTP/TerrainApps/install-sh
zip $DIST_FILE1 VTP/TerrainApps/Makefile
zip $DIST_FILE1 VTP/TerrainApps/README.sgi
zip $DIST_FILE1 VTP/TerrainApps/BExtractor/*.cpp
zip $DIST_FILE1 VTP/TerrainApps/BExtractor/*.h
zip $DIST_FILE1 VTP/TerrainApps/BExtractor/BExtractor*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/BExtractor/BExtractor*.sln
zip $DIST_FILE1 VTP/TerrainApps/BExtractor/BExtractor.rc
zip $DIST_FILE1 VTP/TerrainApps/BExtractor/res/*

zip $DIST_FILE1 VTP/TerrainApps/CManager/*.cpp
zip $DIST_FILE1 VTP/TerrainApps/CManager/*.h
zip $DIST_FILE1 VTP/TerrainApps/CManager/CManager*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/CManager/CManager*.sln
zip $DIST_FILE1 VTP/TerrainApps/CManager/CManager.rc
zip $DIST_FILE1 VTP/TerrainApps/CManager/itemtypes.txt
zip $DIST_FILE1 VTP/TerrainApps/CManager/cmanager.wdr
zip $DIST_FILE1 VTP/TerrainApps/CManager/icons/*

# deal specially with Enviro.xml
mv VTP/TerrainApps/Enviro/Enviro.xml VTP/TerrainApps/Enviro/Enviro_dev.xml
cp VTP/TerrainApps/Enviro/Ship/Enviro.xml VTP/TerrainApps/Enviro
zip $DIST_FILE1 VTP/TerrainApps/Enviro/Enviro.xml
rm VTP/TerrainApps/Enviro/Enviro.xml
mv VTP/TerrainApps/Enviro/Enviro_dev.xml VTP/TerrainApps/Enviro/Enviro.xml

zip $DIST_FILE1 VTP/TerrainApps/Enviro/*.cpp
zip $DIST_FILE1 VTP/TerrainApps/Enviro/*.h
zip $DIST_FILE1 VTP/TerrainApps/Enviro/Makefile
zip $DIST_FILE1 VTP/TerrainApps/Enviro/wxEnviro*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/Enviro/wxEnviro*.sln
zip $DIST_FILE1 VTP/TerrainApps/Enviro/mfcEnviro*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/Enviro/mfcEnviro*.sln
zip $DIST_FILE1 VTP/TerrainApps/Enviro/wx/*.cpp
zip $DIST_FILE1 VTP/TerrainApps/Enviro/wx/*.h
zip $DIST_FILE1 VTP/TerrainApps/Enviro/wx/*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/Enviro/wx/enviro-wx.rc
zip $DIST_FILE1 VTP/TerrainApps/Enviro/wx/enviro.wdr
zip $DIST_FILE1 VTP/TerrainApps/Enviro/wx/bitmap/*
zip $DIST_FILE1 VTP/TerrainApps/Enviro/wx/icons/*
zip $DIST_FILE1 VTP/TerrainApps/Enviro/mfc/*
zip $DIST_FILE1 VTP/TerrainApps/Enviro/mfc/res/*

zip $DIST_FILE1 VTP/TerrainApps/fltkSimple/appfltk.cpp
zip $DIST_FILE1 VTP/TerrainApps/fltkSimple/fltkSimple*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/fltkSimple/fltkSimple*.sln
zip $DIST_FILE1 VTP/TerrainApps/fltkSimple/Data/Simple.xml

zip $DIST_FILE1 VTP/TerrainApps/glutSimple/app.cpp
zip $DIST_FILE1 VTP/TerrainApps/glutSimple/glutSimple*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/glutSimple/glutSimple*.sln
zip $DIST_FILE1 VTP/TerrainApps/glutSimple/license.txt
zip $DIST_FILE1 VTP/TerrainApps/glutSimple/Makefile
zip $DIST_FILE1 VTP/TerrainApps/glutSimple/README.txt
zip $DIST_FILE1 VTP/TerrainApps/glutSimple/Data/Simple.xml
zip $DIST_FILE1 VTP/TerrainApps/glutSimple/Data/Elevation/README.txt

zip $DIST_FILE1 VTP/TerrainApps/mfcSimple/*.cpp
zip $DIST_FILE1 VTP/TerrainApps/mfcSimple/*.h
zip $DIST_FILE1 VTP/TerrainApps/mfcSimple/mfcSimple*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/mfcSimple/mfcSimple*.sln
zip $DIST_FILE1 VTP/TerrainApps/mfcSimple/mfcSimple.rc
zip $DIST_FILE1 VTP/TerrainApps/mfcSimple/Data/Simple.xml
zip $DIST_FILE1 VTP/TerrainApps/mfcSimple/Data/Elevation/README.txt
zip $DIST_FILE1 VTP/TerrainApps/mfcSimple/res/*

zip $DIST_FILE1 VTP/TerrainApps/prSimple/prSimple.cpp
zip $DIST_FILE1 VTP/TerrainApps/prSimple/prSimple*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/prSimple/prSimple*.sln
zip $DIST_FILE1 VTP/TerrainApps/prSimple/Makefile
zip $DIST_FILE1 VTP/TerrainApps/prSimple/Data/Simple.xml
zip $DIST_FILE1 VTP/TerrainApps/prSimple/Data/Elevation/README.txt

zip $DIST_FILE1 VTP/TerrainApps/sdlSimple/app.cpp
zip $DIST_FILE1 VTP/TerrainApps/sdlSimple/sdlSimple*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/sdlSimple/sdlSimple*.sln
zip $DIST_FILE1 VTP/TerrainApps/sdlSimple/Makefile
zip $DIST_FILE1 VTP/TerrainApps/sdlSimple/Data/Simple.xml
zip $DIST_FILE1 VTP/TerrainApps/sdlSimple/Data/Elevation/README.txt

zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/*.cpp
zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/*.h
zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/Makefile
zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/VTBuilder.rc
zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/VTBuilder*.sln
zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/VTBuilder.wdr
zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/bitmaps/*
zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/icons/*
zip $DIST_FILE1 VTP/TerrainApps/VTBuilder/cursors/*

zip $DIST_FILE1 VTP/TerrainApps/wxSimple/*.cpp
zip $DIST_FILE1 VTP/TerrainApps/wxSimple/*.h
zip $DIST_FILE1 VTP/TerrainApps/wxSimple/Makefile
zip $DIST_FILE1 VTP/TerrainApps/wxSimple/README.txt
zip $DIST_FILE1 VTP/TerrainApps/wxSimple/wxSimple*.vcproj
zip $DIST_FILE1 VTP/TerrainApps/wxSimple/wxSimple*.sln
zip $DIST_FILE1 VTP/TerrainApps/wxSimple/Data/Simple.xml
zip $DIST_FILE1 VTP/TerrainApps/wxSimple/Data/Elevation/README.txt

zip $DIST_FILE1 VTP/TerrainApps/VTConvert/VTConvert.cpp
zip $DIST_FILE1 VTP/TerrainApps/VTConvert/VTConvert*.sln
zip $DIST_FILE1 VTP/TerrainApps/VTConvert/VTConvert*.vcproj

zip $DIST_FILE1 VTP/TerrainApps/vtocx/*.cpp
zip $DIST_FILE1 VTP/TerrainApps/vtocx/*.h
zip $DIST_FILE1 VTP/TerrainApps/vtocx/*.def
zip $DIST_FILE1 VTP/TerrainApps/vtocx/*.ico
zip $DIST_FILE1 VTP/TerrainApps/vtocx/*.idl
zip $DIST_FILE1 VTP/TerrainApps/vtocx/*.rc
zip $DIST_FILE1 VTP/TerrainApps/vtocx/vtocx*.sln
zip $DIST_FILE1 VTP/TerrainApps/vtocx/vtocx*.vcproj


# Create the archive containing the Source Help
rm -f $DIST_FILE2

zip $DIST_FILE2 VTP/TerrainSDK/Doc/vtdata/*
zip $DIST_FILE2 VTP/TerrainSDK/Doc/vtlib/*

echo $DIST_FILE1 ready.
echo $DIST_FILE2 ready.
