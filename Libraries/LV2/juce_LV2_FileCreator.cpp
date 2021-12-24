/*
  ==============================================================================

   Juce LV2 Wrapper

  ==============================================================================
*/

#if JucePlugin_Build_LV2

#define wrapperType_LV2 wrapperType_Undefined

#include <cctype>
#include <fstream>

#include "lv2/core/lv2.h"
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/buf-size/buf-size.h"
#include "lv2/instance-access/instance-access.h"
#include "lv2/midi/midi.h"
#include "lv2/options/options.h"
#include "lv2/port-props/port-props.h"
#include "lv2/presets/presets.h"
#include "lv2/state/state.h"
#include "lv2/time/time.h"
#include "lv2/ui/ui.h"
#include "lv2/units/units.h"
#include "lv2/urid/urid.h"
#include "includes/lv2_external_ui.h"
#include "includes/lv2_programs.h"

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_plugin_client/juce_audio_plugin_client.h>

using namespace juce;

/** Plugin requires processing with a fixed/constant block size */
#ifndef JucePlugin_WantsLV2FixedBlockSize
#define JucePlugin_WantsLV2FixedBlockSize 0
#endif

/** Export presets */
#ifndef JucePlugin_WantsLV2Presets
#define JucePlugin_WantsLV2Presets 1
#endif


//==============================================================================
/**
 
 */
class JuceLV2FileCreator
{
    /** Returns plugin type, defined in AppConfig.h or JucePluginCharacteristics.h */
    static String getPluginType(AudioProcessor const* filter)
    {
        String pluginType;
#ifdef JucePlugin_LV2Category
        pluginType  = "lv2:" JucePlugin_LV2Category + ", ";
#endif
        if(filter->acceptsMidi())
        {
            pluginType  += "lv2:InstrumentPlugin, ";
        }
        pluginType += "lv2:Plugin";
        return pluginType;
    }

    
    /** Returns plugin extension */
    static const String& getPluginExtension()
    {
#if JUCE_MAC
        static const String pluginExtension(".dylib");
#elif JUCE_LINUX
        static const String pluginExtension(".so");
#elif JUCE_WINDOWS
        static const String pluginExtension(".dll");
#endif
        return pluginExtension;
    }
    
    /** Returns plugin URI */
    static const String& getPluginURI()
    {
        // JucePlugin_LV2URI might be defined as a function (eg. allowing dynamic URIs based on filename)
        static const String pluginURI(JucePlugin_LV2URI);
        return pluginURI;
    }
    
    /** Queries all available plugin audio ports */
    static void findMaxTotalChannels (AudioProcessor* const filter, int& maxTotalIns, int& maxTotalOuts)
    {
        filter->enableAllBuses();
#ifdef JucePlugin_PreferredChannelConfigurations
        int configs[][2] = { JucePlugin_PreferredChannelConfigurations };
        maxTotalIns = maxTotalOuts = 0;
        
        for (auto& config : configs)
        {
            maxTotalIns =  jmax (maxTotalIns,  config[0]);
            maxTotalOuts = jmax (maxTotalOuts, config[1]);
        }
#else
        auto numInputBuses  = filter->getBusCount (true);
        auto numOutputBuses = filter->getBusCount (false);
        
        if (numInputBuses > 1 || numOutputBuses > 1)
        {
            maxTotalIns = maxTotalOuts = 0;
            
            for (int i = 0; i < numInputBuses; ++i)
                maxTotalIns  += filter->getChannelCountOfBus (true, i);
            
            for (int i = 0; i < numOutputBuses; ++i)
                maxTotalOuts += filter->getChannelCountOfBus (false, i);
        }
        else
        {
            maxTotalIns  = numInputBuses  > 0 ? filter->getBus (true,  0)->getMaxSupportedChannels (64) : 0;
            maxTotalOuts = numOutputBuses > 0 ? filter->getBus (false, 0)->getMaxSupportedChannels (64) : 0;
        }
#endif
    }
    
    static Array<String>& getUsedSymbols()
    {
        static Array<String> usedSymbols;
        return usedSymbols;
    }
    
    /** Converts a parameter name to an LV2 compatible symbol. */
    static const String nameToSymbol (const String& name, const uint32 portIndex)
    {
        String symbol, trimmedName = name.trimStart().trimEnd().toLowerCase();
        
        if (trimmedName.isEmpty())
        {
            symbol += "lv2_port_";
            symbol += String(portIndex+1);
        }
        else
        {
            for (int i=0; i < trimmedName.length(); ++i)
            {
                const juce_wchar c = trimmedName[i];
                if (i == 0 && std::isdigit(c))
                    symbol += "_";
                else if (std::isalpha(c) || std::isdigit(c))
                    symbol += c;
                else
                    symbol += "_";
            }
        }
        
        // Do not allow identical symbols
        if (getUsedSymbols().contains(symbol))
        {
            int offset = 2;
            String offsetStr = "_2";
            symbol += offsetStr;
            
            while (getUsedSymbols().contains(symbol))
            {
                offset += 1;
                String newOffsetStr = "_" + String(offset);
                symbol = symbol.replace(offsetStr, newOffsetStr);
                offsetStr = newOffsetStr;
            }
        }
        getUsedSymbols().add(symbol);
        
        return symbol;
    }
    
    /** Prevents NaN or out of 0.0<->1.0 bounds parameter values. */
    static float safeParamValue (float value)
    {
        if (std::isnan(value))
            value = 0.0f;
        else if (value < 0.0f)
            value = 0.0f;
        else if (value > 1.0f)
            value = 1.0f;
        return value;
    }
    
    /** Create the manifest.ttl file contents */
    static const String makeManifestFile (AudioProcessor* const filter, const String& binary)
    {
        const String& pluginURI(getPluginURI());
        String text;
        
        // Header
        text += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
        text += "@prefix pset: <" LV2_PRESETS_PREFIX "> .\n";
        text += "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
        text += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
        text += "\n";
        
        // Plugin
        text += "<" + pluginURI + ">\n";
        text += "    a lv2:Plugin ;\n";
        text += "    lv2:binary <" + binary + getPluginExtension() + "> ;\n";
        text += "    rdfs:seeAlso <" + binary + ".ttl> .\n";
        text += "\n";
        
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
        // UIs
        if (filter->hasEditor())
        {
            text += "<" + pluginURI + "#ExternalUI>\n";
            text += "    a <" LV2_EXTERNAL_UI__Widget "> ;\n";
            text += "    ui:binary <" + binary + getPluginExtension() + "> ;\n";
            text += "    lv2:requiredFeature <" LV2_INSTANCE_ACCESS_URI "> ;\n";
            text += "    lv2:extensionData <" LV2_PROGRAMS__UIInterface "> .\n";
            text += "\n";
            
            text += "<" + pluginURI + "#ParentUI>\n";
#if JUCE_MAC
            text += "    a ui:CocoaUI ;\n";
#elif JUCE_LINUX
            text += "    a ui:X11UI ;\n";
#elif JUCE_WINDOWS
            text += "    a ui:WindowsUI ;\n";
#endif
            text += "    ui:binary <" + binary + getPluginExtension() + "> ;\n";
            text += "    lv2:requiredFeature <" LV2_INSTANCE_ACCESS_URI "> ;\n";
            text += "    lv2:optionalFeature ui:noUserResize ;\n";
            text += "    lv2:extensionData <" LV2_PROGRAMS__UIInterface "> .\n";
            text += "\n";
        }
#endif
        
#if JucePlugin_WantsLV2Presets
        const String presetSeparator(pluginURI.contains("#") ? ":" : "#");
        
        // Presets
        for (int i = 0; i < filter->getNumPrograms(); ++i)
        {
            text += "<" + pluginURI + presetSeparator + "preset" + String::formatted("%03i", i+1) + ">\n";
            text += "    a pset:Preset ;\n";
            text += "    lv2:appliesTo <" + pluginURI + "> ;\n";
            text += "    rdfs:label \"" + filter->getProgramName(i) + "\" ;\n";
            text += "    rdfs:seeAlso <presets.ttl> .\n";
            text += "\n";
        }
#endif
        
        return text;
    }
    
    /** Create the -plugin-.ttl file contents */
    static const String makePluginFile (AudioProcessor* const filter, const int maxNumInputChannels, const int maxNumOutputChannels)
    {
        const String& pluginURI(getPluginURI());
        String text;
        
        // Header
        text += "@prefix atom: <" LV2_ATOM_PREFIX "> .\n";
        text += "@prefix doap: <http://usefulinc.com/ns/doap#> .\n";
        text += "@prefix foaf: <http://xmlns.com/foaf/0.1/> .\n";
        text += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
        text += "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
        text += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
        text += "@prefix unit: <" LV2_UNITS_PREFIX "> .\n";
        text += "\n";
        
        // Plugin
        text += "<" + pluginURI + ">\n";
        text += "    a " + getPluginType(filter) + " ;\n";
        text += "    lv2:requiredFeature <" LV2_BUF_SIZE__boundedBlockLength "> ,\n";
#if JucePlugin_WantsLV2FixedBlockSize
        text += "                        <" LV2_BUF_SIZE__fixedBlockLength "> ,\n";
#endif
        text += "                        <" LV2_URID__map "> ;\n";
        text += "    lv2:extensionData <" LV2_OPTIONS__interface "> ,\n";
#if JucePlugin_WantsLV2State
        text += "                      <" LV2_STATE__interface "> ,\n";
#endif
        text += "                      <" LV2_PROGRAMS__Interface "> ;\n";
        text += "\n";
        
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
        // UIs
        if (filter->hasEditor())
        {
            text += "    ui:ui <" + pluginURI + "#ExternalUI> ,\n";
            text += "          <" + pluginURI + "#ParentUI> ;\n";
            text += "\n";
        }
#endif
        
        uint32 portIndex = 0;
        
        // MIDI input
        text += "    lv2:port [\n";
        text += "        a lv2:InputPort, atom:AtomPort ;\n";
        text += "        atom:bufferType atom:Sequence ;\n";
        text += "        atom:supports <" LV2_MIDI__MidiEvent ">, <" LV2_TIME__Position "> ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_midi_in\" ;\n";
        text += "        lv2:name \"MIDI Input\" ;\n";
        text += "        lv2:designation lv2:control ;\n";
#if ! JucePlugin_IsSynth
        text += "        lv2:portProperty lv2:connectionOptional ;\n";
#endif
        text += "    ] ;\n";
        text += "\n";

        
        // MIDI output
        text += "    lv2:port [\n";
        text += "        a lv2:OutputPort, atom:AtomPort ;\n";
        text += "        atom:bufferType atom:Sequence ;\n";
        text += "        atom:supports <" LV2_MIDI__MidiEvent "> ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_midi_out\" ;\n";
        text += "        lv2:name \"MIDI Output\" ;\n";
        text += "    ] ;\n";
        text += "\n";
        
        // Freewheel port
        text += "    lv2:port [\n";
        text += "        a lv2:InputPort, lv2:ControlPort ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_freewheel\" ;\n";
        text += "        lv2:name \"Freewheel\" ;\n";
        text += "        lv2:default 0.0 ;\n";
        text += "        lv2:minimum 0.0 ;\n";
        text += "        lv2:maximum 1.0 ;\n";
        text += "        lv2:designation <" LV2_CORE__freeWheeling "> ;\n";
        text += "        lv2:portProperty lv2:toggled, <" LV2_PORT_PROPS__notOnGUI "> ;\n";
        text += "    ] ;\n";
        text += "\n";
        
        // Latency port
        text += "    lv2:port [\n";
        text += "        a lv2:OutputPort, lv2:ControlPort ;\n";
        text += "        lv2:index " + String(portIndex++) + " ;\n";
        text += "        lv2:symbol \"lv2_latency\" ;\n";
        text += "        lv2:name \"Latency\" ;\n";
        text += "        lv2:designation <" LV2_CORE__latency "> ;\n";
        text += "        lv2:portProperty lv2:reportsLatency, lv2:integer ;\n";
        text += "    ] ;\n";
        text += "\n";
        
        // Audio inputs
        for (int i=0; i < maxNumInputChannels; ++i)
        {
            if (i == 0)
                text += "    lv2:port [\n";
            else
                text += "    [\n";
            
            text += "        a lv2:InputPort, lv2:AudioPort ;\n";
            text += "        lv2:index " + String(portIndex++) + " ;\n";
            text += "        lv2:symbol \"lv2_audio_in_" + String(i+1) + "\" ;\n";
            text += "        lv2:name \"Audio Input " + String(i+1) + "\" ;\n";
            
            if (i+1 == maxNumInputChannels)
                text += "    ] ;\n\n";
            else
                text += "    ] ,\n";
        }
        
        // Audio outputs
        for (int i=0; i < maxNumOutputChannels; ++i)
        {
            if (i == 0)
                text += "    lv2:port [\n";
            else
                text += "    [\n";
            
            text += "        a lv2:OutputPort, lv2:AudioPort ;\n";
            text += "        lv2:index " + String(portIndex++) + " ;\n";
            text += "        lv2:symbol \"lv2_audio_out_" + String(i+1) + "\" ;\n";
            text += "        lv2:name \"Audio Output " + String(i+1) + "\" ;\n";
            
            if (i+1 == maxNumOutputChannels)
                text += "    ] ;\n\n";
            else
                text += "    ] ,\n";
        }
        
        // Parameters
        auto const& params = filter->getParameters();
        for (int i = 0; i < params.size(); ++i)
        {
            if (i == 0)
                text += "    lv2:port [\n";
            else
                text += "    [\n";
            
            text += "        a lv2:InputPort, lv2:ControlPort ;\n";
            text += "        lv2:index " + String(portIndex++) + " ;\n";
            String const paramName = params[i]->getName(1000);
            text += "        lv2:symbol \"" + nameToSymbol(paramName, i) + "\" ;\n";
            
            if (paramName.isNotEmpty())
                text += "        lv2:name \"" + paramName + "\" ;\n";
            else
                text += "        lv2:name \"Port " + String(i+1) + "\" ;\n";
            
            if(auto* rangeParam = dynamic_cast<RangedAudioParameter*>(params[i]))
            {
                auto const normRange = rangeParam->getNormalisableRange();
                text += "        lv2:default " + String(normRange.convertFrom0to1(rangeParam->getDefaultValue()), 2) + " ;\n";
                text += "        lv2:minimum " +  String(normRange.start, 2) + " ;\n";
                text += "        lv2:maximum " + String(normRange.end, 2) + " ;\n";
            }
            else
            {
                text += "        lv2:default " + String::formatted("%f", safeParamValue(params[i]->getDefaultValue())) + " ;\n";
                text += "        lv2:minimum 0.0 ;\n";
                text += "        lv2:maximum 1.0 ;\n";
            }
            
            if (! params[i]->isAutomatable())
                text += "        lv2:portProperty <" LV2_PORT_PROPS__expensive "> ;\n";
            
            auto const label = params[i]->getLabel();
            if(!label.isEmpty())
            {
                text += "        unit:unit [\n";
                text += "            rdfs:label \"" + label + "\" ;\n";
                text += "            unit:symbol \"" + label + "\" ;\n";
                text += "            unit:render \"%f " + label + "\" ;\n";
                text += "        ] ;\n";
            }
            
            if (i+1 == params.size())
                text += "    ] ;\n\n";
            else
                text += "    ] ,\n";
        }
        
        text += "    doap:name \"" + filter->getName() + "\" ;\n";
        text += "    doap:maintainer [ foaf:name \"" + String(JucePlugin_Manufacturer) + "\" ] .\n";
        
        return text;
    }
    
    /** Create the presets.ttl file contents */
    static const String makePresetsFile (AudioProcessor* const filter)
    {
        const String& pluginURI(getPluginURI());
        String text;
        
        // Header
        text += "@prefix atom:  <" LV2_ATOM_PREFIX "> .\n";
        text += "@prefix lv2:   <" LV2_CORE_PREFIX "> .\n";
        text += "@prefix pset:  <" LV2_PRESETS_PREFIX "> .\n";
        text += "@prefix rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n";
        text += "@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .\n";
        text += "@prefix state: <" LV2_STATE_PREFIX "> .\n";
        text += "@prefix xsd:   <http://www.w3.org/2001/XMLSchema#> .\n";
        text += "\n";
        
        // Presets
        const int numPrograms = filter->getNumPrograms();
        const String presetSeparator(pluginURI.contains("#") ? ":" : "#");
        
        for (int i = 0; i < numPrograms; ++i)
        {
            std::cout << "\nSaving preset " << i+1 << "/" << numPrograms+1 << "...";
            std::cout.flush();
            
            String preset;
            
            // Label
            filter->setCurrentProgram(i);
            preset += "<" + pluginURI + presetSeparator + "preset" + String::formatted("%03i", i+1) + "> a pset:Preset ;\n";
            
            // State
#if JucePlugin_WantsLV2State
            preset += "    state:state [\n";
#if JucePlugin_WantsLV2StateString
            preset += "        <" JUCE_LV2_STATE_STRING_URI ">\n";
            preset += "\"\"\"\n";
            preset += filter->getStateInformationString().replace("\r\n","\n");
            preset += "\"\"\"\n";
#else
            MemoryBlock chunkMemory;
            filter->getCurrentProgramStateInformation(chunkMemory);
            const String chunkString(Base64::toBase64(chunkMemory.getData(), chunkMemory.getSize()));
            
            preset += "        <" JUCE_LV2_STATE_BINARY_URI "> [\n";
            preset += "            a atom:Chunk ;\n";
            preset += "            rdf:value \"" + chunkString + "\"^^xsd:base64Binary ;\n";
            preset += "        ] ;\n";
#endif
            if (filter->getParameters().size() == 0)
            {
                preset += "    ] .\n\n";
                continue;
            }
            
            preset += "    ] ;\n\n";
#endif
            
            // Port values
            getUsedSymbols().clear();
            auto const& params = filter->getParameters();
            for (int j = 0; j < params.size(); ++j)
            {
                if (j == 0)
                {
                    preset += "    lv2:port [\n";
                }
                else
                {
                    preset += "    [\n";
                }
                
                String const paramName = params[i]->getName(1000);
                preset += "        lv2:symbol \"" + nameToSymbol(paramName, j) + "\" ;\n";
                preset += "        pset:value " + String::formatted("%f", safeParamValue(params[i]->getValue())) + " ;\n";
                
                if (j + 1 == params.size())
                {
                    preset += "    ] ";
                }
                else
                {
                    preset += "    ] ,\n";
                }
            }
            preset += ".\n\n";
            
            text += preset;
        }
        
        return text;
    }
    
public:
    /** Creates manifest.ttl, plugin.ttl and presets.ttl files */
    static void createLv2Files(const char* basename)
    {
        const ScopedJuceInitialiser_GUI juceInitialiser;
        std::unique_ptr<AudioProcessor> filter(createPluginFilterOfType (AudioProcessor::wrapperType_LV2));
        
        int maxNumInputChannels, maxNumOutputChannels;
        findMaxTotalChannels(filter.get(), maxNumInputChannels, maxNumOutputChannels);
        
        String binary(basename);
        String binaryTTL(binary + ".ttl");
        
        std::cout << "Writing manifest.ttl..."; std::cout.flush();
        std::fstream manifest("manifest.ttl", std::ios::out);
        manifest << makeManifestFile(filter.get(), binary) << std::endl;
        manifest.close();
        std::cout << " done!" << std::endl;
        
        std::cout << "Writing " << binary << ".ttl..."; std::cout.flush();
        std::fstream plugin(binaryTTL.toUTF8(), std::ios::out);
        plugin << makePluginFile(filter.get(), maxNumInputChannels, maxNumOutputChannels) << std::endl;
        plugin.close();
        std::cout << " done!" << std::endl;
        
#if JucePlugin_WantsLV2Presets
        std::cout << "Writing presets.ttl..."; std::cout.flush();
        std::fstream presets("presets.ttl", std::ios::out);
        presets << makePresetsFile(filter.get()) << std::endl;
        presets.close();
        std::cout << " done!" << std::endl;
#endif
    }
};


#if JUCE_WINDOWS
 #define JUCE_EXPORTED_FUNCTION extern "C" __declspec (dllexport)
#else
 #define JUCE_EXPORTED_FUNCTION extern "C" __attribute__ ((visibility("default")))
#endif

//==============================================================================
// startup code..

JUCE_EXPORTED_FUNCTION void lv2_generate_ttl (const char* basename);
JUCE_EXPORTED_FUNCTION void lv2_generate_ttl (const char* basename)
{
    JuceLV2FileCreator::createLv2Files (basename);
}

#endif
