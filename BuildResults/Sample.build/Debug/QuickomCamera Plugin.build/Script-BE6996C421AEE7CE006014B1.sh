#!/bin/sh
# Copy plug-in to path for FCP (which uses embedded frameworks)
echo "Copying plug-in to FCP's 3rd Party Plug-In path"
pushd $SYMROOT/$CONFIGURATIONPREFIX$CONFIGURATION/root/$INSTALL_PATH
mkdir -p ../FCP-DAL
rm -Rf "../FCP-DAL/$PRODUCT_NAME.plugin"
ditto "$PRODUCT_NAME.plugin/" "../FCP-DAL/$PRODUCT_NAME.plugin/"

# Remove the assistant (the "system" one should be used)
echo "Removing Assistant"
cd ../FCP-DAL/
rm -Rf "$PRODUCT_NAME.plugin/Contents/Resources/"*Assistant

# Fix rpaths
echo "Fixing rpaths"
install_name_tool -change /System/Library/Frameworks/CoreMedia.framework/Versions/A/CoreMedia @rpath/CoreMedia.framework/Versions/A/CoreMedia "$PRODUCT_NAME.plugin/Contents/MacOS/$PRODUCT_NAME"
install_name_tool -change /System/Library/Frameworks/CoreMediaIO.framework/Versions/A/CoreMediaIO @rpath/CoreMediaIO.framework/Versions/A/CoreMediaIO "$PRODUCT_NAME.plugin/Contents/MacOS/$PRODUCT_NAME"
echo "otool -L output:"
otool -L "$PRODUCT_NAME.plugin/Contents/MacOS/$PRODUCT_NAME"
popd

