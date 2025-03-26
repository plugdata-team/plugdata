/*
 // Copyright (c) 2022 Timothy Schoen and Wasted Audio
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#pragma once

class HeavyCompatibleObjects {
    static inline StringArray const heavyObjects = {
        "!=",
        "%",
        "&",
        "&&",
        "|",
        "||",
        "*",
        "+",
        "-",
        "/",
        "<",
        "<<",
        "<=",
        "==",
        ">",
        ">=",
        ">>",
        "abs",
        "atan",
        "atan2",
        "array",
        "b",
        "bang",
        "bendin",
        "bendout",
        "bng",
        "canvas",
        "change",
        "clip",
        "cnv",
        "cos",
        "ctlin",
        "ctlout",
        "dbtopow",
        "dbtorms",
        "declare",
        "del",
        "delay",
        "div",
        "exp",
        "f",
        "float",
        "ftom",
        "graph",
        "gatom",
        "hradio",
        "hsl",
        "i",
        "inlet",
        "int",
        "line",
        "loadbang",
        "log",
        "makenote",
        "max",
        "metro",
        "min",
        "midiin",
        "midiout",
        "midirealtimein",
        "mod",
        "moses",
        "mtof",
        "nbx",
        "notein",
        "noteout",
        "outlet",
        "pack",
        "pgmin",
        "pgmout",
        "pipe",
        "poly",
        "polytouchin",
        "polytouchout",
        "pow",
        "powtodb",
        "print",
        "r",
        "random",
        "receive",
        "rmstodb",
        "route",
        "s",
        "sel",
        "select",
        "send",
        "sin",
        "spigot",
        "sqrt",
        "stripnote",
        "swap",
        "symbol",
        "t",
        "table",
        "tabread",
        "tabwrite",
        "tan",
        "text",
        "tgl",
        "timer",
        "touchin",
        "touchout",
        "trigger",
        "unpack",
        "until",
        "vradio",
        "vsl",
        "wrap",
        "*~",
        "+~",
        "-~",
        "/~",
        "abs~",
        "adc~",
        "bang~",
        "biquad~",
        "bp~",
        "catch~",
        "clip~",
        "cos~",
        "cpole~",
        "czero_rev~",
        "czero~",
        "dac~",
        "dbtopow~",
        "dbtorms~",
        "delread~",
        "delwrite~",
        "delread4~",
        "env~",
        "exp~",
        "ftom~",
        "hip~",
        "inlet~",
        "line~",
        "lop~",
        "max~",
        "min~",
        "mtof~",
        "noise~",
        "osc~",
        "outlet~",
        "phasor~",
        "powtodb~",
        "pow~",
        "q8_rsqrt~",
        "q8_sqrt~",
        "receive~",
        "rmstodb~",
        "rpole~",
        "rsqrt~",
        "rzero_rev~",
        "rzero~",
        "r~",
        "samphold~",
        "samplerate~",
        "send~",
        "sig~",
        "snapshot~",
        "sqrt~",
        "s~",
        "tabosc4~",
        "tabplay~",
        "tabread4~",
        "tabread~",
        "tabwrite~",
        "throw~",
        "vcf~",
        "vd~",
        "wrap~"
    };

    // Heavylib abstractions:
    // These won't be used for the compatibility testing (it will recognise any abstractions as a canvas)
    // These are only for the suggestions
    static inline StringArray const heavyAbstractions = {
        "hv.comb~",
        "hv.compressor~",
        "hv.compressor2~",
        "hv.dispatch",
        "hv.drunk",
        "hv.envfollow~",
        "hv.eq~",
        "hv.exp~",
        "hv.filter.gain~",
        "hv.filter~",
        "hv.flanger~",
        "hv.flanger2~",
        "hv.freqshift~",
        "hv.gt~",
        "hv.gte~",
        "hv.hip~",
        "hv.lop~",
        "hv.log~",
        "hv.lt~",
        "hv.lfo",
        "hv.lte~",
        "hv.multiplex~",
        "hv.neq~",
        "hv.osc~",
        "hv.pinknoise~",
        "hv.pow~",
        "hv.reverb~",
        "hv.tanh~",
        "hv.vline~"
    };

    static inline StringArray const elseObjects = {
        "knob"
    };

    static inline StringArray const elseAbstractions = {
        "above",
        "add",
        "avg",
        "car2pol",
        "sysrt.in",
        "sysrt.out",
        "trig2bang",
        // "float2sig.unit",
        // "out.mc.hip~",
        // "sendmidi",
    };

    static inline StringArray const pdAbstractions = {
        "complex-mod~",
        "hilbert~",
        "rev1~",
        "rev2~",
        "rev3~",
    };

    static inline StringArray const extra = {
        "comment",
        "floatbox",
        "symbolbox",
        "msg",
        "message",
        "invalid"
    };

    static StringArray initialiseCompatibleObjects()
    {
        StringArray allObjects;
        allObjects.addArray(heavyObjects);
        allObjects.addArray(heavyAbstractions);
        allObjects.addArray(elseObjects);
        allObjects.addArray(elseAbstractions);
        allObjects.addArray(pdAbstractions);
        allObjects.addArray(extra);

        return allObjects;
    }
    
    static inline StringArray allCompatibleObjects = initialiseCompatibleObjects();
    
public:
    
    static bool isCompatible(String const& type)
    {
        return allCompatibleObjects.contains(type.upToFirstOccurrenceOf(" ", false, false));
    }
};
