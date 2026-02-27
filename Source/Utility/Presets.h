/*
 // Copyright (c) 2021-2025 Timothy Schoen
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#pragma once

struct Presets {
    static inline HeapArray<std::pair<String, String>> const presets = {
        { "Default Preset", "AAAAAEAAAAAAAAAAAAAAAMIAAABWQzIhuQAAADw/eG1sIHZlcnNpb249IjEuMCIgZW5jb2Rpbmc9IlVURi04Ij8+IDxwbHVnZGF0YV9zYXZlIFZlcnNpb249IjAuNy4xIiBTcGxpdEluZGV4PSIwIiBPdmVyc2FtcGxpbmc9IjAiIExhdGVuY3k9IjY0IiBUYWlsTGVuZ3RoPSIwLjAiIExlZ2FjeT0iMCIgV2lkdGg9IjEwMDAiIEhlaWdodD0iNjUwIiBQbHVnaW5Nb2RlPSIwIi8+AA==" },
        { "LIRA-8", "AQAAAAAke1BSRVNFVF9ESVJ9LwBAAAAAAAAAAAAAAAA3AQAAVkMyIS4BAAA8P3htbCB2ZXJzaW9uPSIxLjAiIGVuY29kaW5nPSJVVEYtOCI/PiA8cGx1Z2RhdGFfc2F2ZSBWZXJzaW9uPSIwLjcuMSIgU3BsaXRJbmRleD0iMSIgT3ZlcnNhbXBsaW5nPSIwIiBMYXRlbmN5PSI2NCIgVGFpbExlbmd0aD0iMC4wIiBMZWdhY3k9IjAiIFdpZHRoPSIzODUiIEhlaWdodD0iNzE3IiBQbHVnaW5Nb2RlPSJMSVJBLTgucGQiPjxQYXRjaGVzPjxQYXRjaCBDb250ZW50PSIiIExvY2F0aW9uPSIke1BSRVNFVF9ESVJ9L0xJUkEtOC9MSVJBLTgucGQiIFBsdWdpbk1vZGU9IjEiLz48L1BhdGNoZXM+PC9wbHVnZGF0YV9zYXZlPgA=" },
        { "AlmondOrgan",
            "AQAAAAAke1BSRVNFVF9ESVJ9LwBAAAAAAAAAAAAAAABGAQAAVkMyIT0BAAA8P3htbCB2ZXJzaW9uPSIxLjAiIGVuY29kaW5nPSJVVEYtOCI/PiA8cGx1Z2RhdGFfc2F2ZSBWZXJzaW9uPSIwLjcuMSIgU3BsaXRJbmRleD0iMSIgT3ZlcnNhbXBsaW5nPSIwIiBMYXRlbmN5PSI2NCIgVGFpbExlbmd0aD0iMC4wIiBMZWdhY3k9IjAiIFdpZHRoPSI3MjIiIEhlaWdodD0iMzY1IiBQbHVnaW5Nb2RlPSJBbG1vbmRPcmdhbi5wZCI+PFBhdGNoZXM+PFBhdGNoIENvbnRlbnQ9IiIgTG9jYXRpb249IiR7UFJFU0VUX0RJUn0vQWxtb25kT3JnYW4vQWxtb25kT3JnYW4ucGQiIFBsdWdpbk1vZGU9IjEiLz48L1BhdGNoZXM+PC9wbHVnZGF0YV9zYXZlPgA=" },
        { "MiniMock", "AQAAAAAke1BSRVNFVF9ESVJ9LwBAAAAAAAAAAAAAAAA+AQAAVkMyITUBAAA8P3htbCB2ZXJzaW9uPSIxLjAiIGVuY29kaW5nPSJVVEYtOCI/PiA8cGx1Z2RhdGFfc2F2ZSBWZXJzaW9uPSIwLjcuMSIgU3BsaXRJbmRleD0iMSIgT3ZlcnNhbXBsaW5nPSIwIiBMYXRlbmN5PSI2NCIgVGFpbExlbmd0aD0iMC4wIiBMZWdhY3k9IjAiIFdpZHRoPSIxMDU0IiBIZWlnaHQ9IjM3MyIgUGx1Z2luTW9kZT0iTWluaU1vY2sucGQiPjxQYXRjaGVzPjxQYXRjaCBDb250ZW50PSIiIExvY2F0aW9uPSIke1BSRVNFVF9ESVJ9L01pbmlNb2NrL01pbmlNb2NrLnBkIiBQbHVnaW5Nb2RlPSIxIi8+PC9QYXRjaGVzPjwvcGx1Z2RhdGFfc2F2ZT4A" },
        { "Castafiore", "AQAAAAAke1BSRVNFVF9ESVJ9LwBAAAAAAAAAAAAAAABDAQAAVkMyIToBAAA8P3htbCB2ZXJzaW9uPSIxLjAiIGVuY29kaW5nPSJVVEYtOCI/PiA8cGx1Z2RhdGFfc2F2ZSBWZXJzaW9uPSIwLjcuMSIgU3BsaXRJbmRleD0iMSIgT3ZlcnNhbXBsaW5nPSIwIiBMYXRlbmN5PSI2NCIgVGFpbExlbmd0aD0iMC4wIiBMZWdhY3k9IjAiIFdpZHRoPSIzMDIiIEhlaWdodD0iMjQyIiBQbHVnaW5Nb2RlPSJDYXN0YWZpb3JlLnBkIj48UGF0Y2hlcz48UGF0Y2ggQ29udGVudD0iIiBMb2NhdGlvbj0iJHtQUkVTRVRfRElSfS9DYXN0YWZpb3JlL0Nhc3RhZmlvcmUucGQiIFBsdWdpbk1vZGU9IjEiLz48L1BhdGNoZXM+PC9wbHVnZGF0YV9zYXZlPgA=" },
        { "Bulgroz", "AQAAAAAke1BSRVNFVF9ESVJ9LwBAAAAAAAAAAAAAAAA6AQAAVkMyITEBAAA8P3htbCB2ZXJzaW9uPSIxLjAiIGVuY29kaW5nPSJVVEYtOCI/PiA8cGx1Z2RhdGFfc2F2ZSBWZXJzaW9uPSIwLjcuMSIgU3BsaXRJbmRleD0iMSIgT3ZlcnNhbXBsaW5nPSIwIiBMYXRlbmN5PSI2NCIgVGFpbExlbmd0aD0iMC4wIiBMZWdhY3k9IjAiIFdpZHRoPSI0NjEiIEhlaWdodD0iMjAxIiBQbHVnaW5Nb2RlPSJCdWxncm96LnBkIj48UGF0Y2hlcz48UGF0Y2ggQ29udGVudD0iIiBMb2NhdGlvbj0iJHtQUkVTRVRfRElSfS9CdWxncm96L0J1bGdyb3oucGQiIFBsdWdpbk1vZGU9IjEiLz48L1BhdGNoZXM+PC9wbHVnZGF0YV9zYXZlPgA=" },
        { "Pong", "AQAAAAAke1BSRVNFVF9ESVJ9LwBAAAAAAAAAAAAAAAAxAQAAVkMyISgBAAA8P3htbCB2ZXJzaW9uPSIxLjAiIGVuY29kaW5nPSJVVEYtOCI/PiA8cGx1Z2RhdGFfc2F2ZSBWZXJzaW9uPSIwLjcuMSIgU3BsaXRJbmRleD0iMSIgT3ZlcnNhbXBsaW5nPSIwIiBMYXRlbmN5PSI2NCIgVGFpbExlbmd0aD0iMC4wIiBMZWdhY3k9IjAiIFdpZHRoPSI1MDAiIEhlaWdodD0iNTIwIiBQbHVnaW5Nb2RlPSJQb25nLnBkIj48UGF0Y2hlcz48UGF0Y2ggQ29udGVudD0iIiBMb2NhdGlvbj0iJHtQUkVTRVRfRElSfS9Qb25nL1BvbmcucGQiIFBsdWdpbk1vZGU9IjEiLz48L1BhdGNoZXM+PC9wbHVnZGF0YV9zYXZlPgA=" }
    };

    /* Helper function for creating presets
    static void createPreset(AudioProcessor* processor)
    {
        File presetDir = ProjectInfo::appDataDir.getChildFile("Extra").getChildFile("Presets");

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
            SmallArray<XmlElement*> toBeDeleted;

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
        ostream.writeString("${PRESET_DIR}/" + patchFile.fromFirstOccurrenceOf("Extra/Presets/", false, false));

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
    } */
};
