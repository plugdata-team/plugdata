/*
 // Copyright (c) 2021-2022 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

struct Presets
{
    static inline const std::vector<std::pair<String, String>> presets = {
        {"LIRA-8", "AQAAAAAke1BSRVNFVF9ESVJ9TElSQS04L0xJUkEtOC5wZABAAAAAAAAAAAAAAADJAAAAVkMyIcAAAAA8P3htbCB2ZXJzaW9uPSIxLjAiIGVuY29kaW5nPSJVVEYtOCI/PiA8cGx1Z2RhdGFfc2F2ZSBWZXJzaW9uPSIwLjcuMSIgU3BsaXRJbmRleD0iMSIgT3ZlcnNhbXBsaW5nPSIwIiBMYXRlbmN5PSI2NCIgVGFpbExlbmd0aD0iMC4wIiBMZWdhY3k9IjAiIFdpZHRoPSIzODUiIEhlaWdodD0iNzE3IiBQbHVnaW5Nb2RlPSJMSVJBLTgucGQiLz4A"},
        {"AlmondOrgan",
            "AQAAAAAke1BSRVNFVF9ESVJ9QWxtb25kT3JnYW4vQWxtb25kT3JnYW4ucGQAQAAAAAAAAAAAAAAAzgAAAFZDMiHFAAAAPD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4gPHBsdWdkYXRhX3NhdmUgVmVyc2lvbj0iMC43LjEiIFNwbGl0SW5kZXg9IjEiIE92ZXJzYW1wbGluZz0iMCIgTGF0ZW5jeT0iNjQiIFRhaWxMZW5ndGg9IjAuMCIgTGVnYWN5PSIwIiBXaWR0aD0iNzIyIiBIZWlnaHQ9IjM2NSIgUGx1Z2luTW9kZT0iQWxtb25kT3JnYW4ucGQiLz4A"},
        {"MiniMock", "AQAAAAAke1BSRVNFVF9ESVJ9TWluaU1vY2svTWluaU1vY2sucGQAQAAAAAAAAAAAAAAAzAAAAFZDMiHDAAAAPD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiPz4gPHBsdWdkYXRhX3NhdmUgVmVyc2lvbj0iMC43LjEiIFNwbGl0SW5kZXg9IjEiIE92ZXJzYW1wbGluZz0iMCIgTGF0ZW5jeT0iNjQiIFRhaWxMZW5ndGg9IjAuMCIgTGVnYWN5PSIwIiBXaWR0aD0iMTA1NCIgSGVpZ2h0PSIzNzMiIFBsdWdpbk1vZGU9Ik1pbmlNb2NrLnBkIi8+AA=="},
        {"Castafiore", "AQAAAAAke1BSRVNFVF9ESVJ9Q2FzdGFmaW9yZS9DYXN0YWZpb3JlLnBkAEAAAAAAAAAAAAAAAM0AAABWQzIhxAAAADw/eG1sIHZlcnNpb249IjEuMCIgZW5jb2Rpbmc9IlVURi04Ij8+IDxwbHVnZGF0YV9zYXZlIFZlcnNpb249IjAuNy4xIiBTcGxpdEluZGV4PSIxIiBPdmVyc2FtcGxpbmc9IjAiIExhdGVuY3k9IjY0IiBUYWlsTGVuZ3RoPSIwLjAiIExlZ2FjeT0iMCIgV2lkdGg9IjMwMiIgSGVpZ2h0PSIyNDIiIFBsdWdpbk1vZGU9IkNhc3RhZmlvcmUucGQiLz4A"},
        {"Bulgroz", "AQAAAAAke1BSRVNFVF9ESVJ9QnVsZ3Jvei9CdWxncm96LnBkAEAAAAAAAAAAAAAAAMoAAABWQzIhwQAAADw/eG1sIHZlcnNpb249IjEuMCIgZW5jb2Rpbmc9IlVURi04Ij8+IDxwbHVnZGF0YV9zYXZlIFZlcnNpb249IjAuNy4xIiBTcGxpdEluZGV4PSIxIiBPdmVyc2FtcGxpbmc9IjAiIExhdGVuY3k9IjY0IiBUYWlsTGVuZ3RoPSIwLjAiIExlZ2FjeT0iMCIgV2lkdGg9IjQ2MSIgSGVpZ2h0PSIyMDEiIFBsdWdpbk1vZGU9IkJ1bGdyb3oucGQiLz4A"}
    };
    
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
        ostream.writeString("${PRESET_DIR}" + patchFile.fromFirstOccurrenceOf("Extra/Presets/", false, false));

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
