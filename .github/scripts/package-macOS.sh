#!/bin/bash

# plugdata macOS installer build script, using pkgbuild and productbuild
# based on script for SURGE https://github.com/surge-synthesizer/surge and iPlug2 https://github.com/iPlug2/iPlug2

# Documentation for pkgbuild and productbuild: https://developer.apple.com/library/archive/documentation/DeveloperTools/Reference/DistributionDefinitionRef/Chapters/Distribution_XML_Ref.html

VERSION=${GITHUB_REF#refs/*/}

PRODUCT_NAME=plugdata

LV2="./Plugins/LV2/."
VST3="./Plugins/VST3/."
AU="./Plugins/AU/."
CLAP="./Plugins/CLAP/."
APP="./Plugins/Standalone/."

BINARY_DATA_DYLIB="./Plugins/Standalone/plugdata.app/Contents/MacOS/libBinaryData.dylib"

OUTPUT_BASE_FILENAME="${PRODUCT_NAME}.pkg"

TARGET_DIR="./"
PKG_DIR=${TARGET_DIR}/pkgs

if [[ ! -d ${TARGET_DIR} ]]; then
  mkdir ${TARGET_DIR}
fi

if [[ ! -d ${PKG_DIR} ]]; then
  mkdir ${PKG_DIR}
fi

build_flavor()
{
  TMPDIR=${TARGET_DIR}/tmp
  flavor=$1
  flavorprod=$2
  ident=$3
  loc=$4
  min_os=$5

  echo --- BUILDING ${PRODUCT_NAME}_${flavor}.pkg ---

  mkdir -p $TMPDIR
  cp -a $flavorprod $TMPDIR

  rm -f $TMPDIR/*/Contents/MacOS/libBinaryData.dylib
  rm -f $TMPDIR/*/libBinaryData.dylib

  pkgbuild --analyze --root $TMPDIR ${PKG_DIR}/${PRODUCT_NAME}_${flavor}.plist
  plutil -replace BundleIsRelocatable -bool NO ${PKG_DIR}/${PRODUCT_NAME}_${flavor}.plist
  plutil -replace BundleIsVersionChecked -bool NO ${PKG_DIR}/${PRODUCT_NAME}_${flavor}.plist
  pkgbuild --root $TMPDIR --identifier $ident --version $VERSION  --install-location $loc --min-os-version $min_os --compression latest --component-plist ${PKG_DIR}/${PRODUCT_NAME}_${flavor}.plist ${PKG_DIR}/${PRODUCT_NAME}_${flavor}.pkg

  rm -r $TMPDIR
}

build_shared_dylib()
{
  TMPDIR=${TARGET_DIR}/tmp_shared
  mkdir -p "$TMPDIR"
  cp "$BINARY_DATA_DYLIB" "$TMPDIR/"

  # Create postinstall script that copies dylib into whichever plugin bundles were installed
  SCRIPTS_DIR=${TARGET_DIR}/tmp_scripts
  mkdir -p "$SCRIPTS_DIR"
  cat > "$SCRIPTS_DIR/postinstall" << 'EOF'
#!/bin/bash

DYLIB="/tmp/plugdata_shared/libBinaryData.dylib"

LOCATIONS=(
    "/Library/Audio/Plug-Ins/VST3/plugdata.vst3/Contents/MacOS/"
    "/Library/Audio/Plug-Ins/VST3/plugdata-fx.vst3/Contents/MacOS/"
    "/Library/Audio/Plug-Ins/Components/plugdata.component/Contents/MacOS/"
    "/Library/Audio/Plug-Ins/Components/plugdata-fx.component/Contents/MacOS/"
    "/Library/Audio/Plug-Ins/Components/plugdata-midi.component/Contents/MacOS/"
    "/Library/Audio/Plug-Ins/CLAP/plugdata.clap/Contents/MacOS/"
    "/Library/Audio/Plug-Ins/CLAP/plugdata-fx.clap/Contents/MacOS/"
    "/Library/Audio/Plug-Ins/LV2/plugdata.lv2/"
    "/Library/Audio/Plug-Ins/LV2/plugdata-fx.lv2/"
    "/Applications/plugdata.app/Contents/MacOS/"
)

for loc in "${LOCATIONS[@]}"; do
    if [[ -d "$loc" ]]; then
        cp "$DYLIB" "$loc"
    fi
done

# Clean up temp location
rm -rf /tmp/plugdata_shared

exit 0
EOF
  chmod +x "$SCRIPTS_DIR/postinstall"

  pkgbuild --root "$TMPDIR" \
    --identifier "com.plugdata.sharedlibs.pkg.${PRODUCT_NAME}" \
    --version $VERSION \
    --install-location "/tmp/plugdata_shared" \
    --min-os-version $MIN_OS_VERSION \
    --scripts "$SCRIPTS_DIR" \
    ${PKG_DIR}/${PRODUCT_NAME}_shared.pkg

  rm -r "$TMPDIR"
  rm -r "$SCRIPTS_DIR"
}

if [ -n "$AC_USERNAME" ]; then

# Sign libBinaryData.dylib
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" $BINARY_DATA_DYLIB

# Sign app with hardened runtime and audio entitlement
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" --options runtime --entitlements ./Resources/Installer/Entitlements.plist ./Plugins/Standalone/*.app

# Sign plugins
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/VST3/*.vst3
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/AU/*.component
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/LV2/plugdata.lv2/libplugdata.so
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/LV2/plugdata-fx.lv2/libplugdata-fx.so
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/CLAP/plugdata.clap/Contents/MacOS/plugdata
/usr/bin/codesign --force -s "Developer ID Application: Timothy Schoen (7SV7JPRR2L)" ./Plugins/CLAP/plugdata-fx.clap/Contents/MacOS/plugdata-fx

fi

BUILD_TYPE=$1
if [[ "$BUILD_TYPE" == "Universal" ]]; then
  MIN_OS_VERSION="10.15"
else
  MIN_OS_VERSION="10.11"  # Use default/legacy for 10.11 support
fi

# Build shared dylib package (always required, installed first)
build_shared_dylib

# try to build VST3 package
if [[ -d $VST3 ]]; then
  build_flavor "VST3" $VST3 "com.plugdata.vst3.pkg.${PRODUCT_NAME}" "/Library/Audio/Plug-Ins/VST3" "$MIN_OS_VERSION"
fi

# try to build LV2 package
if [[ -d $LV2 ]]; then
  build_flavor "LV2" $LV2 "com.plugdata.lv2.pkg.${PRODUCT_NAME}" "/Library/Audio/Plug-Ins/LV2" "$MIN_OS_VERSION"
fi

# try to build AU package
if [[ -d $AU ]]; then
  build_flavor "AU" $AU "com.plugdata.au.pkg.${PRODUCT_NAME}" "/Library/Audio/Plug-Ins/Components" "$MIN_OS_VERSION"
fi

# try to build CLAP package
if [[ -d $CLAP ]]; then
  build_flavor "CLAP" $CLAP "com.plugdata.clap.pkg.${PRODUCT_NAME}" "/Library/Audio/Plug-Ins/CLAP" "$MIN_OS_VERSION"
fi

# try to build App package
if [[ -d $APP ]]; then
  build_flavor "APP" $APP "com.plugdata.app.pkg.${PRODUCT_NAME}" "/Applications" "$MIN_OS_VERSION"
fi

# Always include shared libs pkg ref
SHARED_PKG_REF="<pkg-ref id=\"com.plugdata.sharedlibs.pkg.${PRODUCT_NAME}\"/>"
SHARED_CHOICE_DEF="<choice id=\"com.plugdata.sharedlibs.pkg.${PRODUCT_NAME}\" visible=\"false\" title=\"Shared Libraries\"><pkg-ref id=\"com.plugdata.sharedlibs.pkg.${PRODUCT_NAME}\"/></choice><pkg-ref id=\"com.plugdata.sharedlibs.pkg.${PRODUCT_NAME}\" version=\"${VERSION}\" onConclusion=\"none\">${PRODUCT_NAME}_shared.pkg</pkg-ref>"

if [[ -d $VST3 ]]; then
	VST3_PKG_REF="<pkg-ref id=\"com.plugdata.vst3.pkg.${PRODUCT_NAME}\"/>"
	VST3_CHOICE="<line choice=\"com.plugdata.vst3.pkg.${PRODUCT_NAME}\"/>"
	VST3_CHOICE_DEF="<choice id=\"com.plugdata.vst3.pkg.${PRODUCT_NAME}\" visible=\"true\" start_selected=\"true\" title=\"VST3 Plug-in\"><pkg-ref id=\"com.plugdata.vst3.pkg.${PRODUCT_NAME}\"/></choice><pkg-ref id=\"com.plugdata.vst3.pkg.${PRODUCT_NAME}\" version=\"${VERSION}\" onConclusion=\"none\">${PRODUCT_NAME}_VST3.pkg</pkg-ref>"
fi
if [[ -d $LV2 ]]; then
	LV2_PKG_REF="<pkg-ref id=\"com.plugdata.lv2.pkg.${PRODUCT_NAME}\"/>"
	LV2_CHOICE="<line choice=\"com.plugdata.lv2.pkg.${PRODUCT_NAME}\"/>"
	LV2_CHOICE_DEF="<choice id=\"com.plugdata.lv2.pkg.${PRODUCT_NAME}\" visible=\"true\" start_selected=\"true\" title=\"LV2 Plug-in\"><pkg-ref id=\"com.plugdata.lv2.pkg.${PRODUCT_NAME}\"/></choice><pkg-ref id=\"com.plugdata.lv2.pkg.${PRODUCT_NAME}\" version=\"${VERSION}\" onConclusion=\"none\">${PRODUCT_NAME}_LV2.pkg</pkg-ref>"
fi
if [[ -d $AU ]]; then
	AU_PKG_REF="<pkg-ref id=\"com.plugdata.au.pkg.${PRODUCT_NAME}\"/>"
	AU_CHOICE="<line choice=\"com.plugdata.au.pkg.${PRODUCT_NAME}\"/>"
	AU_CHOICE_DEF="<choice id=\"com.plugdata.au.pkg.${PRODUCT_NAME}\" visible=\"true\" start_selected=\"true\" title=\"Audio Unit Plug-in\"><pkg-ref id=\"com.plugdata.au.pkg.${PRODUCT_NAME}\"/></choice><pkg-ref id=\"com.plugdata.au.pkg.${PRODUCT_NAME}\" version=\"${VERSION}\" onConclusion=\"none\">${PRODUCT_NAME}_AU.pkg</pkg-ref>"
fi
if [[ -d $CLAP ]]; then
	CLAP_PKG_REF="<pkg-ref id=\"com.plugdata.clap.pkg.${PRODUCT_NAME}\"/>"
	CLAP_CHOICE="<line choice=\"com.plugdata.clap.pkg.${PRODUCT_NAME}\"/>"
	CLAP_CHOICE_DEF="<choice id=\"com.plugdata.clap.pkg.${PRODUCT_NAME}\" visible=\"true\" start_selected=\"true\" title=\"CLAP Plug-in\"><pkg-ref id=\"com.plugdata.clap.pkg.${PRODUCT_NAME}\"/></choice><pkg-ref id=\"com.plugdata.clap.pkg.${PRODUCT_NAME}\" version=\"${VERSION}\" onConclusion=\"none\">${PRODUCT_NAME}_CLAP.pkg</pkg-ref>"
fi
if [[ -d $APP ]]; then
	APP_PKG_REF="<pkg-ref id=\"com.plugdata.app.pkg.${PRODUCT_NAME}\"/>"
	APP_CHOICE="<line choice=\"com.plugdata.app.pkg.${PRODUCT_NAME}\"/>"
	APP_CHOICE_DEF="<choice id=\"com.plugdata.app.pkg.${PRODUCT_NAME}\" visible=\"true\" start_selected=\"true\" title=\"Standalone App\"><pkg-ref id=\"com.plugdata.app.pkg.${PRODUCT_NAME}\"/></choice><pkg-ref id=\"com.plugdata.app.pkg.${PRODUCT_NAME}\" version=\"${VERSION}\" onConclusion=\"none\">${PRODUCT_NAME}_APP.pkg</pkg-ref>"
fi


touch ${TARGET_DIR}/distribution.xml
cat > ${TARGET_DIR}/distribution.xml << XMLEND
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1">
    <title>plugdata Installer</title>
    <license file="Resources/Installer/LICENSE.rtf" mime-type="application/rtf"/>
    ${SHARED_PKG_REF}
    ${VST3_PKG_REF}
    ${AU_PKG_REF}
    ${LV2_PKG_REF}
    ${CLAP_PKG_REF}
    ${APP_PKG_REF}
    <options require-scripts="false" customize="always" />
    <options hostArchitectures="arm64,x86_64" />
    <choices-outline>
        <line choice="com.plugdata.sharedlibs.pkg.${PRODUCT_NAME}"/>
        ${VST3_CHOICE}
        ${AU_CHOICE}
        ${LV2_CHOICE}
        ${CLAP_CHOICE}
        ${APP_CHOICE}
    </choices-outline>
    ${SHARED_CHOICE_DEF}
    ${VST3_CHOICE_DEF}
    ${AU_CHOICE_DEF}
    ${LV2_CHOICE_DEF}
    ${CLAP_CHOICE_DEF}
    ${APP_CHOICE_DEF}
</installer-gui-script>
XMLEND

# Build installer
productbuild --resources ./ --distribution ${TARGET_DIR}/distribution.xml --package-path ${PKG_DIR} "${TARGET_DIR}/$OUTPUT_BASE_FILENAME"

rm ${TARGET_DIR}/distribution.xml
rm -r $PKG_DIR

if [ -z "$AC_USERNAME" ]; then
    echo "No user name, skipping sign/notarize"
    # pretend that we signed the package and bail out
    mv ${PRODUCT_NAME}.pkg $1
    exit 0
fi

# Sign installer
productsign -s "Developer ID Installer: Timothy Schoen (7SV7JPRR2L)" ${PRODUCT_NAME}.pkg $1

# Notarize installer
xcrun notarytool store-credentials "notary_login" --apple-id ${AC_USERNAME} --password ${AC_PASSWORD} --team-id "7SV7JPRR2L"
xcrun notarytool submit $1 --keychain-profile "notary_login" --wait
xcrun stapler staple "plugdata-MacOS-$1.pkg"

.github/scripts/generate-upload-info.sh $1
