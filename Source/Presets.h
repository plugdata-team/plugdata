/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

struct Presets
{
    static inline const std::vector<std::pair<String, String>> presets = {
        {"LIRA-8", "AQAAAAAvVXNlcnMvdGltc2Nob2VuL0xpYnJhcnkvcGx1Z2RhdGEvMC43LjEtdGVzdC9FeHRyYS9QcmVzZXRzL0xJUkEtOC9MSVJBLTgucGQAQAAAAAAAAAAAAAAAwQAAAFZDMiG4AAAAPD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4gPHBsdWdkYXRhX3NhdmUgVmVyc2lvbj0iMC43LjEiIFNwbGl0SW5kZXg9IjEiIE92ZXJzYW1wbGluZz0iMCIgTGF0ZW5jeT0iNjQiIFRhaWxMZW5ndGg9IjAuMCIgTGVnYWN5PSIwIiBXaWR0aD0iMzg1IiBIZWlnaHQ9IjcxNyIgUGx1Z2luTW9kZT0iMSIvPgA="}};
    
    static void createPreset(AudioProcessor* processor)
    {
        File presetDir = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getChildFile("plugdata").getChildFile("Library").getChildFile("Extra").getChildFile("Presets");

        MemoryBlock data;
        processor->getStateInformation(data);
        
        MemoryInputStream istream(data, false);
        
        jassert(istream.readInt() == 1);
        
        int numPatches = 1;
        
        istream.readString();
        auto patchFile = istream.readString();
        
        auto latency = istream.readInt();
        auto oversampling = istream.readInt();
        auto tailLength = istream.readFloat();
        
        auto xmlSize = istream.readInt();
        
        auto* xmlData = new char[xmlSize];
        istream.read(xmlData, xmlSize);
        
        std::unique_ptr<XmlElement> xmlState(AudioProcessor::getXmlFromBinary(xmlData, xmlSize));
        
        jassert(xmlState);
        
        if (xmlState) {
            
            // Remove all children to make preset more compact
            // Don't do this if we end up adding actual params!
            
            Array<XmlElement*> toBeDeleted;

            forEachXmlChildElement(*xmlState, child)
            {
                toBeDeleted.add(child);
            }

            for (auto* child : toBeDeleted)
                xmlState->removeChildElement(child, true);
        }
        
        MemoryBlock outBlock;
        
        // Now reconstruct the modified save file
        MemoryOutputStream ostream(outBlock, false);
        
        ostream.writeInt(1);

        ostream.writeString("");
        ostream.writeString(patchFile.replace(presetDir.getFullPathName(), "${PRESET_DIR}"));

        ostream.writeInt(latency);
        ostream.writeInt(oversampling);
        ostream.writeFloat(tailLength);
        
        MemoryBlock xmlBlock;
        AudioProcessor::copyXmlToBinary(*xmlState, xmlBlock);
        ostream.writeInt(static_cast<int>(xmlBlock.getSize()));
        ostream.write(xmlBlock.getData(), xmlBlock.getSize());
        
        
        auto block = ostream.getMemoryBlock();
        
        MemoryOutputStream base64_ostream;
        Base64::convertToBase64(base64_ostream, block.getData(), block.getSize());
        auto result = base64_ostream.toString();
        std::cout << result << std::endl;
        SystemClipboard::copyTextToClipboard(result);
    }
    
};
