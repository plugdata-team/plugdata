#!/bin/bash

# Function to clean up downloaded fonts and the FontForge script
cleanup() {
    rm -rf ./Inter-Extracted
    rm ./Inter.zip
    rm ./Inter-Regular.ttf
    rm ./GoNotoCurrent-Regular.ttf
    rm ./NotoEmoji-Regular.ttf
    rm ./temp.ttf
    rm ./mergefont.ff
    rm ./GoNotoCurrent-Regular-Scaled.ttf
}

if [ -z "${FF_PATH}" ]; then
    echo "Error: FF_PATH is not set. Please set the FF_PATH environment variable to the FontForge path."
    echo "on macOS, this is usually /Applications/FontForge.app/Contents/Resources/opt/local/bin/fontforge"
    exit 1
fi

# Base64-encoded NotoEmoji font, since there is no easy way to download it from a script

# Download Inter font ZIP file
wget -O ./Inter.zip https://github.com/rsms/inter/releases/download/v4.0/Inter-4.0.zip
wget -O ./GoNotoCurrent-Regular.ttf https://github.com/satbyy/go-noto-universal/releases/download/v7.0/GoNotoCurrent-Regular.ttf

# Extract Inter font
mkdir -p ./Inter-Extracted
unzip ./Inter.zip -d ./Inter-Extracted

# Copy Inter-Regular.ttf from extracted folder
cp ./Inter-Extracted/extras/ttf/Inter-Regular.ttf ./Inter-Regular.ttf

# Decode NotoEmoji font from base64
echo "$NotoEmoji_Base64" | base64 -d > ./NotoEmoji-Regular.ttf

# Write FontForge script to mergefont.ff
cat <<EOT > ./mergefont.ff
#!/usr/local/bin/fontforge

# Scale GoNotoCurrent, because it's default em-size is 1000
Open("GoNotoCurrent-Regular.ttf")
SelectAll()

ScaleToEm(1638, 410)
Generate("GoNotoCurrent-Regular-Scaled.ttf")
Close()

# First merge Inter's latin characters with GoNoto's international ones
# Due to a bug in FontForge, it doesn't work to merge everything all at once
Open("Inter-Regular.ttf")
MergeFonts("GoNotoCurrent-Regular-Scaled.ttf")
Generate("temp.ttf")
Close()

# Then add NotoEmoji to that
Open("temp.ttf")
MergeFonts("NotoEmoji-Regular.ttf")

# Remove arrow glyphs since they cause issues in JUCE (drawing "<-" will make it render a real arrow, which messes up the character counting in JUCE)
Select(0u2190, 0u21FF)  # Selects first Unicode range for arrows
Clear()     
Select(0u27F5, 0u27FE)  # Selects second Unicode range for arrows
Clear()
Select(0uE1D2, 0uE1DF)  # Selects third Unicode range for arrows
Clear()                 # Clears the selected glyphs

Generate("InterUnicode.ttf")
Close()
EOT

# Run FontForge script
"${FF_PATH}" -lang=ff -script ./mergefont.ff

# Clean up downloaded files and FontForge script
cleanup