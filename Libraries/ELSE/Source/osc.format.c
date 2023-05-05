// stolen from mrpeach's routeOSC

// Copyright (C) 2006-2011 Martin Peach
// slightly modified, renamed and greatly and vastly simplified by Porres 2023

/* oscformat is like sendOSC but outputs a list of floats which are the bytes making up the OSC packet. */
/* This allows for the separation of the protocol and its transport. */
/* oscformat.c makes extensive use of code from OSC-client.c and sendOSC.c */
/* as well as some from OSC-timetag.c. These files have the following header: */
/*
Written by Matt Wright, The Center for New Music and Audio Technologies,
University of California, Berkeley.  Copyright (c) 1996,97,98,99,2000,01,02,03
The Regents of the University of California (Regents).  

Permission to use, copy, modify, distribute, and distribute modified versions
of this software and its documentation without fee and without a signed
licensing agreement, is hereby granted, provided that the above copyright
notice, this paragraph and the following two paragraphs appear in all copies,
modifications, and distributions.

IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

The OSC webpage is http://cnmat.cnmat.berkeley.edu/OpenSoundControl
*/

#include "../../shared/OSC.h"

#define SC_BUFFER_SIZE 64000

typedef struct OSCbuf_struct{
    char        *buffer; /* The buffer to hold the OSC packet */
    size_t      size; /* Size of the buffer */
    char        *bufptr; /* Current position as we fill the buffer */
    int         state; /* State of partially-constructed message */
    uint32_t    *thisMsgSize; /* Pointer to count field before */
                /* currently-being-written message */
    uint32_t    *prevCounts[MAX_BUNDLE_NESTING]; /* Pointers to count */
                /* field before each currently open bundle */
    int         bundleDepth; /* How many sub-sub-bundles are we in now? */
    char        *typeStringPtr; /* This pointer advances through the type */
                /* tag string as you add arguments. */
    int         gettingFirstUntypedArg; /* nonzero if this message doesn't have */
                /*  a type tag and we're waiting for the 1st arg */
} OSCbuf;

typedef struct{
    enum {INT_osc, FLOAT_osc, STRING_osc, BLOB_osc, NOTYPE_osc} type;
    union{
        int   i;
        float f;
        char  *s;
    } datum;
} typedArg;

/* Here are the possible values of the state field: */

#define EMPTY 0 /* Nothing written to packet yet */
#define ONE_MSG_ARGS 1 /* Packet has a single message; gathering arguments */
#define NEED_COUNT 2 /* Just opened a bundle; must write message name or */
                     /* open another bundle */
#define GET_ARGS 3 /* Getting arguments to a message.  If we see a message */
                     /*	name or a bundle open/close then the current message */
                     /*	will end. */
#define DONE 4 /* All open bundles have been closed, so can't write */
                     /*	anything else */

static int OSC_strlen(char *s);
static int OSC_padString(char *dest, char *str);
static int OSC_padStringWithAnExtraStupidComma(char *dest, char *str);
static int OSC_WriteStringPadding(char *dest, int i);
static int OSC_WriteBlobPadding(char *dest, int i);

static int CheckTypeTag(OSCbuf *buf, char expectedType);

/* Initialize the given OSCbuf.  The user of this module must pass in the
   block of memory that this OSCbuf will use for a buffer, and the number of
   bytes in that block.  (It's the user's job to allocate the memory because
   you do it differently in different systems.) */
static void OSC_initBuffer(OSCbuf *buf, size_t size, char *byteArray);

/* Reset the given OSCbuf.  Do this after you send out the contents of
   the buffer and want to start writing new data into it. */
static void OSC_resetBuffer(OSCbuf *buf);

/* Is the buffer empty?  (I.e., would it be stupid to send the buffer
   contents to the synth?) */
static int OSC_isBufferEmpty(OSCbuf *buf);

/* How much space is left in the buffer? */
static size_t OSC_freeSpaceInBuffer(OSCbuf *buf);

/* Does the buffer contain a valid OSC packet?  (Returns nonzero if yes.) */
static int OSC_isBufferDone(OSCbuf *buf);

/* When you're ready to send out the buffer (i.e., when OSC_isBufferDone()
   returns true), call these two procedures to get the OSC packet that's been
   assembled and its size in bytes.  (And then call OSC_resetBuffer() if you
   want to re-use this OSCbuf for the next packet.)  */
static char *OSC_getPacket(OSCbuf *buf);
static int OSC_packetSize(OSCbuf *buf);
static int OSC_CheckOverflow(OSCbuf *buf, size_t bytesNeeded);

/* Here's the basic model for building up OSC messages in an OSCbuf:
    - Make sure the OSCbuf has been initialized with OSC_initBuffer().
    - To open a bundle, call OSC_openBundle().  You can then write
      messages or open new bundles within the bundle you opened.
      Call OSC_closeBundle() to close the bundle.  Note that a packet
      does not have to have a bundle; it can instead consist of just a
      single message.
    - For each message you want to send:
      - Call OSC_writeAddress() with the name of your message.  (In
        addition to writing your message name into the buffer, this
        procedure will also leave space for the size count of this message.)
      - Alternately, call OSC_writeAddressAndTypes() with the name of
        your message and with a type string listing the types of all the
        arguments you will be putting in this message.
      - Now write each of the arguments into the buffer, by calling one of:
        OSC_writeFloatArg()
        OSC_writeIntArg()
        OSC_writeStringArg()
        OSC_writeNullArg()
      - Now your message is complete; you can send out the buffer or you can
        add another message to it.
*/

//static int OSC_openBundle(OSCbuf *buf, OSCTimeTag tt);
//static int OSC_closeBundle(OSCbuf *buf);
static int OSC_writeAddress(OSCbuf *buf, char *name);
static int OSC_writeAddressAndTypes(OSCbuf *buf, char *name, char *types);
static int OSC_writeFloatArg(OSCbuf *buf, float arg);
static int OSC_writeIntArg(OSCbuf *buf, uint32_t arg);
static int OSC_writeBlobArg(OSCbuf *buf, typedArg *arg, size_t nArgs);
static int OSC_writeStringArg(OSCbuf *buf, char *arg);
static int OSC_writeNullArg(OSCbuf *buf, char type);

/* How many bytes will be needed in the OSC format to hold the given
   string?  The length of the string, plus the null char, plus any padding
   needed for 4-byte alignment. */
static int OSC_effectiveStringLength(char *string);

static t_class *oscformat_class;

typedef struct _oscformat{
    t_object    x_obj;
    int         x_typetags; /* typetag flag */
    int         x_timeTagOffset;
    int         x_bundle; /* bundle open flag */
    OSCbuf      x_oscbuf[1]; /* OSCbuffer */
    t_outlet    *x_bdpthout; /* bundle-depth floatoutlet */
    t_outlet    *x_listout; /* OSC packet list ouput */
    size_t      x_buflength; /* number of elements in x_bufferForOSCbuf and x_bufferForOSClist */
    char        *x_bufferForOSCbuf; /*[SC_BUFFER_SIZE];*/
    t_atom      *x_bufferForOSClist; /*[SC_BUFFER_SIZE];*/
    char        *x_prefix;
    int         x_reentry_count;
}t_oscformat;

static void *oscformat_new(void);
//static void oscformat_openbundle(t_oscformat *x);
//static void oscformat_closebundle(t_oscformat *x);
//static void oscformat_settypetags(t_oscformat *x, t_floatarg f);
//static void oscformat_setTimeTagOffset(t_oscformat *x, t_floatarg f);
static void oscformat_sendtyped(t_oscformat *x, t_symbol *s, int argc, t_atom *argv);
//static void oscformat_send_type_forced(t_oscformat *x, t_symbol *s, int argc, t_atom *argv);
static void oscformat_send(t_oscformat *x, t_symbol *s, int argc, t_atom *argv);
static void oscformat_anything(t_oscformat *x, t_symbol *s, int argc, t_atom *argv);
static void oscformat_free(t_oscformat *x);
void oscformat_setup(void);
static typedArg oscformat_parseatom(t_atom *a);
static typedArg oscformat_packMIDI(t_atom *a);
static typedArg oscformat_forceatom(t_atom *a, char ctype);
static typedArg oscformat_blob(t_atom *a);
static int oscformat_writetypedmessage(OSCbuf *buf, char *messageName, int numArgs, typedArg *args, char *typeStr);
static int oscformat_writemessage(t_oscformat *x, OSCbuf *buf, char *messageName, int numArgs, typedArg *args);
static void oscformat_sendbuffer(t_oscformat *x);

/*static void oscformat_openbundle(t_oscformat *x)
{
    int result;
    t_float bundledepth=(t_float)x->x_oscbuf->bundleDepth;
    if (x->x_timeTagOffset == -1)
        result = OSC_openBundle(x->x_oscbuf, OSCTT_Immediately());
    else
        result = OSC_openBundle(x->x_oscbuf, OSCTT_CurrentTimePlusOffset((uint32_t)x->x_timeTagOffset));
    if (result != 0)
    { // reset the buffer
        OSC_initBuffer(x->x_oscbuf, x->x_buflength, x->x_bufferForOSCbuf);
        x->x_bundle = 0;
    }
    else x->x_bundle = 1;
//    outlet_float(x->x_bdpthout, bundledepth);
}*/

/*static void oscformat_closebundle(t_oscformat *x)
{
    t_float bundledepth=(t_float)x->x_oscbuf->bundleDepth;
    if (OSC_closeBundle(x->x_oscbuf))
    {
        pd_error(x, "oscformat: Problem closing bundle.");
        return;
    }
//    outlet_float(x->x_bdpthout, bundledepth);
    // in bundle mode we send when bundle is closed
    if((!OSC_isBufferEmpty(x->x_oscbuf)) && OSC_isBufferDone(x->x_oscbuf))
    {
        x->x_bundle = 0; // call this before _sendbuffer() to be ready for recursive calls
        oscformat_sendbuffer(x);
        return;
    }
}*/

/*static void oscformat_settypetags(t_oscformat *x, t_floatarg f){
    x->x_typetags = (f != 0);
}*/

/*static void oscformat_setbufsize(t_oscformat *x, t_floatarg f)
{
    if (x->x_bufferForOSCbuf != NULL) freebytes((void *)x->x_bufferForOSCbuf, sizeof(char)*x->x_buflength);
    if (x->x_bufferForOSClist != NULL) freebytes((void *)x->x_bufferForOSClist, sizeof(t_atom)*x->x_buflength);
    post("oscformat: bufsize arg is %f (%lu)", f, (long)f);
    x->x_buflength = (long)f;
    x->x_bufferForOSCbuf = (char *)getbytes(sizeof(char)*x->x_buflength);
    if(x->x_bufferForOSCbuf == NULL)
        pd_error(x, "oscformat unable to allocate %lu bytes for x_bufferForOSCbuf", (long)(sizeof(char)*x->x_buflength));
    x->x_bufferForOSClist = (t_atom *)getbytes(sizeof(t_atom)*x->x_buflength);
    if(x->x_bufferForOSClist == NULL)
        pd_error(x, "oscformat unable to allocate %lu bytes for x_bufferForOSClist", (long)(sizeof(t_atom)*x->x_buflength));
    OSC_initBuffer(x->x_oscbuf, x->x_buflength, x->x_bufferForOSCbuf);
    post("oscformat: bufsize is now %d",x->x_buflength);
}*/

/*static void oscformat_setTimeTagOffset(t_oscformat *x, t_floatarg f)
{
    x->x_timeTagOffset = (int)f;
}*/
/* this is the real and only sending routine now, for both typed and */
/* undtyped mode. */

static void oscformat_sendtyped(t_oscformat *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    char            messageName[MAXPDSTRING];
    unsigned int	nTypeTags = 0, typeStrTotalSize = 0;
    unsigned int    argsSize = sizeof(typedArg)*argc;
    char*           typeStr = NULL; /* might not be used */
    typedArg*       args = (typedArg*)getbytes(argsSize);
    unsigned int    i, nTagsWithData, nArgs, blobCount;
    unsigned int    m, tagIndex, typedArgIndex, argvIndex;
    char            c;
#ifdef DEBUG
    printf("*** oscformat_sendtyped bundle %d reentry %d\n", x->x_bundle, x->x_reentry_count);
#endif
    x->x_reentry_count++;
    if (args == NULL){
        pd_error(x, "oscformat: unable to allocate %lu bytes for args", (long)argsSize);
        return;
    }
    messageName[0] = '\0'; /* empty */
    if(x->x_prefix){ /* if there is a prefix, prefix it to the path */
        size_t len = strlen(x->x_prefix);
        if(len >= MAXPDSTRING)
        len = MAXPDSTRING-1;
        strncpy(messageName, x->x_prefix, MAXPDSTRING);
        atom_string(&argv[0], messageName+len, (unsigned)(MAXPDSTRING-len));
    }
    else
        atom_string(&argv[0], messageName, MAXPDSTRING); /* the OSC address string */
    if(x->x_typetags & 2){ /* second arg is typestring */
        /* we need to find out how long the type string is before we copy it*/
        nTypeTags = (unsigned int)strlen(atom_getsymbol(&argv[1])->s_name);
        typeStrTotalSize = nTypeTags + 2;
        typeStr = (char*)getzbytes(typeStrTotalSize);
        if (typeStr == NULL)
        {
            pd_error(x, "oscformat: unable to allocate %u bytes for typeStr", nTypeTags);
            return;
        }
        typeStr[0] = ',';
        atom_string(&argv[1], &typeStr[1], typeStrTotalSize);
#ifdef DEBUG
        printf("oscformat_sendtyped typeStr: %s, nTypeTags %u\n", typeStr, nTypeTags);
#endif
        nArgs = argc-2;
        for (m = nTagsWithData = blobCount = 0; m < nTypeTags; ++m)
        {
#ifdef DEBUG
            printf("oscformat_sendtyped typeStr[%d] %c\n", m+1, typeStr[m+1]);
#endif
            if ((c = typeStr[m+1]) == 0) break;
            if (!(c == 'T' || c == 'F' || c == 'N' || c == 'I'))
            {
                ++nTagsWithData; /* anything other than these tags have at least one data byte */
                if (c == 'm') nTagsWithData += 3; // MIDI tag should have four data bytes
/*
    OSC-blob
    An int32 size count, followed by that many 8-bit bytes of arbitrary binary data, 
    followed by 0-3 additional zero bytes to make the total number of bits a multiple of 32.
*/
                if (c == 'b') blobCount++; /* b probably has more than one byte, so set a flag */
            }
        }
        if (((blobCount == 0)&&(nTagsWithData != nArgs)) || ((blobCount != 0)&&(nTagsWithData > nArgs)))
        {
            pd_error(x, "oscformat: Tags count %d doesn't match argument count %d", nTagsWithData, nArgs);
            goto cleanup;
        }
        if (blobCount > 1)
        {
            pd_error(x, "oscformat: Only one blob per packet at the moment...");
            goto cleanup;
        }
        for (tagIndex = typedArgIndex = 0, argvIndex = 2; tagIndex < m; ++tagIndex) /* m is the number of tags */
        {
            c = typeStr[tagIndex+1];
            if (c == 'b')
            { /* A blob has to be the last item, until we get more elaborate. */
                if (tagIndex != m-1)
                {
                    pd_error(x, "oscformat: Since I don't know how big the blob is, Blob must be the last item in the list");
                    goto cleanup;
                }
                /* Pack all the remaining arguments as a blob */
                for (; typedArgIndex < nArgs; ++typedArgIndex, ++argvIndex)
                {
#ifdef DEBUG
                    printf("oscformat_blob %d:\n", nArgs);
#endif
                    args[typedArgIndex] = oscformat_blob(&argv[argvIndex]);
                    /* Make sure it was blobbable */
                    if (args[typedArgIndex].type != BLOB_osc) goto cleanup;
                }
            }
            else if (!(c == 'T' || c == 'F' || c == 'N' || c == 'I')) /* not no data */
            {
                if (c == 'm')
                { // pack the next four arguments into one int
                  args[typedArgIndex++] = oscformat_packMIDI(&argv[argvIndex]);
                  argvIndex += 4;  
                }
                else args[typedArgIndex++] = oscformat_forceatom(&argv[argvIndex++], c);
            }
        }
        //if(oscformat_writetypedmessage(x, x->x_oscbuf, messageName, nArgs, args, typeStr))
        if(oscformat_writetypedmessage(x->x_oscbuf, messageName, typedArgIndex, args, typeStr))
        {
            pd_error(x, "oscformat: usage error, oscformat_writetypedmessage failed.");
            goto cleanup;
        }
    }
    else
    {
        for (i = 0; i < (unsigned)(argc-1); i++)
        {
            args[i] = oscformat_parseatom(&argv[i+1]);
#ifdef DEBUG
            switch (args[i].type)
            {
                case INT_osc:
                    printf("oscformat: cell-cont: %d\n", args[i].datum.i);
                    break;
                case FLOAT_osc:
                    printf("oscformat: cell-cont: %f\n", args[i].datum.f);
                    break;
                case STRING_osc:
                    printf("oscformat: cell-cont: %s\n", args[i].datum.s);
                    break;
                case BLOB_osc:
                    printf("oscformat: blob\n");
                    break;
                case NOTYPE_osc:
                    printf("oscformat: unknown type\n");
                    break;

            }
            printf("oscformat:   type-id: %d\n", args[i].type);
#endif
        }
        if(oscformat_writemessage(x, x->x_oscbuf, messageName, i, args))
        {
            pd_error(x, "oscformat: usage error, oscformat_writemessage failed.");
            goto cleanup;
        }
    }

    if(!x->x_bundle)
    {
        oscformat_sendbuffer(x);
    }

cleanup:
    if (typeStr != NULL) freebytes(typeStr, typeStrTotalSize);
    if (args != NULL) freebytes(args, argsSize);
    x->x_reentry_count--;
}

/*static void oscformat_send_type_forced(t_oscformat *x, t_symbol *s, int argc, t_atom *argv)
{ // typetags are the argument following the OSC path
    x->x_typetags |= 2;// tell oscformat_sendtyped to use the specified typetags...
    oscformat_sendtyped(x, s, argc, argv);
    x->x_typetags &= ~2; // ...this time only
}*/

static void oscformat_send(t_oscformat *x, t_symbol *s, int argc, t_atom *argv){
    if(!argc){
        pd_error(x, "oscformat: not sending empty message.");
        return;
    }
    oscformat_sendtyped(x, s, argc, argv);
}

static void oscformat_anything(t_oscformat *x, t_symbol *s, int argc, t_atom *argv){
    // If the message starts with '/', assume it's an OSC path and send it
    t_atom*ap = 0;
    if((*s->s_name) != '/'){
        pd_error(x, "oscformat: bad path: '%s'", s->s_name);
        return;
    }
    ap = (t_atom*)getbytes((argc+1)*sizeof(t_atom));
    SETSYMBOL(ap, s);
    memcpy(ap+1, argv, argc * sizeof(t_atom));
    oscformat_send(x, gensym("send"), argc+1, ap);
    freebytes(ap, (argc+1)*sizeof(t_atom));
}

static void oscformat_free(t_oscformat *x){
    if(x->x_bufferForOSCbuf != NULL)
        freebytes((void *)x->x_bufferForOSCbuf, sizeof(char)*x->x_buflength);
    if(x->x_bufferForOSClist != NULL)
        freebytes((void *)x->x_bufferForOSClist, sizeof(t_atom)*x->x_buflength);
}

static typedArg oscformat_parseatom(t_atom *a){
    typedArg returnVal;
    t_float  f;
    int      i;
    t_symbol s;
    char     buf[MAXPDSTRING];
    atom_string(a, buf, MAXPDSTRING);
#ifdef DEBUG
    printf("oscformat: atom type %d (%s)\n", a->a_type, buf);
#endif
    /* It might be an int, a float, or a string */
    switch (a->a_type){
        case A_FLOAT:
            f = atom_getfloat(a);
            i = atom_getint(a);
            if(f == (t_float)i){ /* assume that if the int and float are the same, it's an int */
                returnVal.type = INT_osc;
                returnVal.datum.i = i;
            }
            else{
                returnVal.type = FLOAT_osc;
                returnVal.datum.f = f;
            }
            return returnVal;
        case A_SYMBOL:
            s = *atom_getsymbol(a);
            returnVal.type = STRING_osc;
            returnVal.datum.s = (char *)s.s_name;
            return returnVal;
        default:
            atom_string(a, buf, MAXPDSTRING);
            post("oscformat: atom type %d not implemented (%s)", a->a_type, buf);
            returnVal.type = NOTYPE_osc;
            returnVal.datum.s = NULL;
            return returnVal;
    }
}

static typedArg oscformat_blob(t_atom *a){ /* ctype is one of i,f,s,T,F,N,I*/
    typedArg    returnVal;
    t_float     f;
    int         i;
    returnVal.type = NOTYPE_osc;
    returnVal.datum.s = NULL;
    /* the atoms must all be bytesl */
    if(a->a_type != A_FLOAT){
        post("oscformat_blob: all values must be floats");
        return returnVal;
    }
    f = atom_getfloat(a);
    i = (int)f;
    if(i != f){
        post("oscformat_blob: all values must be whole numbers");
        return returnVal;
    }
    if ((i < -128) || (i > 255)){
        post("oscformat_blob: all values must be bytes");
        return returnVal;
    }
    returnVal.type = BLOB_osc;
    returnVal.datum.i = i;
    return returnVal;
}

static typedArg oscformat_packMIDI(t_atom *a){ /* pack four bytes at a into one int32 */
    int         i;
    typedArg    returnVal;
    int         m[4];
    for (i = 0; i < 4; ++i, ++a){
#ifdef DEBUG
        atom_string(a, buf, MAXPDSTRING);
        printf("oscformat: atom type %d (%s)\n", a->a_type, buf);
#endif
        if ((a->a_type != A_FLOAT)){
            post("oscformat: MIDI parameters must be floats");
            returnVal.type = NOTYPE_osc;
            returnVal.datum.s = NULL;
            return returnVal;
        }
        m[i] = atom_getint(a);
#ifdef DEBUG
        printf("oscformat_packMIDI: float to integer %d\n", m[i]);
#endif
        if ((i < 2) && (m[i] != (m[i] & 0x0FF))){
            post("oscformat: MIDI parameters must be less than 256");
            returnVal.type = NOTYPE_osc;
            returnVal.datum.s = NULL;
            return returnVal;
        }
        else if ((i > 1) && (m[i] != (m[i] & 0x07F))){
            post("oscformat: MIDI parameters must be less than 128");
            returnVal.type = NOTYPE_osc;
            returnVal.datum.s = NULL;
            return returnVal;
        }
    }
    returnVal.type = INT_osc;
    returnVal.datum.i = (m[0]<<24) + (m[1]<<16) + (m[2]<<8) + m[3];
    return returnVal;
}

static typedArg oscformat_forceatom(t_atom *a, char ctype){ /* ctype is one of i,f,s,T,F,N,I*/
    typedArg    returnVal;
    t_float     f;
    int         i;
    t_symbol    s;
    static char buf[MAXPDSTRING];
#ifdef DEBUG
    atom_string(a, buf, MAXPDSTRING);
    printf("oscformat: atom type %d (%s)\n", a->a_type, buf);
#endif
    /* the atom might be a float, or a symbol */
    switch (a->a_type){
        case A_FLOAT:
            switch (ctype)
            {
                case 'i':
                    returnVal.type = INT_osc;
                    returnVal.datum.i = atom_getint(a);
#ifdef DEBUG
                    printf("oscformat_forceatom: float to integer %d\n", returnVal.datum.i);
#endif
                    break;
                case 'f':
                    returnVal.type = FLOAT_osc;
                    returnVal.datum.f = atom_getfloat(a);
#ifdef DEBUG
                    printf("oscformat_forceatom: float to float %f\n", returnVal.datum.f);
#endif
                    break;
                case 's':
                    f = atom_getfloat(a);
                    sprintf(buf, "%f", f);
                    returnVal.type = STRING_osc;
                    returnVal.datum.s = buf;
#ifdef DEBUG
                    printf("oscformat_forceatom: float to string %s\n", returnVal.datum.s);
#endif
                    break;
                default:
                    post("oscformat: unknown OSC type %c", ctype);
                    returnVal.type = NOTYPE_osc;
                    returnVal.datum.s = NULL;
                    break;
            }
            break;
        case A_SYMBOL:
            s = *atom_getsymbol(a);
            switch (ctype)
            {
                case 'i':
                    i = atoi(s.s_name);
                    returnVal.type = INT_osc;
                    returnVal.datum.i = i;
#ifdef DEBUG
                    printf("oscformat_forceatom: symbol to integer %d\n", returnVal.datum.i);
#endif
                    break;
                case 'f':
                    f = atof(s.s_name);
                    returnVal.type = FLOAT_osc;
                    returnVal.datum.f = f;
#ifdef DEBUG
                    printf("oscformat_forceatom: symbol to float %f\n", returnVal.datum.f);
#endif
                    break;
                case 's':
                    returnVal.type = STRING_osc;
                    returnVal.datum.s = (char *)s.s_name;
#ifdef DEBUG
                    printf("oscformat_forceatom: symbol to string %s\n", returnVal.datum.s);
#endif
                    break;
                default:
                    post("oscformat: unknown OSC type %c", ctype);
                    returnVal.type = NOTYPE_osc;
                    returnVal.datum.s = NULL;
                    break;
            }
            break;
        default:
            atom_string(a, buf, MAXPDSTRING);
            post("oscformat: atom type %d not implemented (%s)", a->a_type, buf);
            returnVal.type = NOTYPE_osc;
            returnVal.datum.s = NULL;
            break;
    }
    return returnVal;
}

static int oscformat_writetypedmessage(OSCbuf *buf, char *messageName,
int numArgs, typedArg *args, char *typeStr){
    int i, j, returnVal;
#ifdef DEBUG
    printf("oscformat_writetypedmessage: messageName %p (%s) typeStr %p (%s)\n",
        messageName, messageName, typeStr, typeStr);
#endif
    returnVal = OSC_writeAddressAndTypes(buf, messageName, typeStr);
    if (returnVal){
        post("oscformat: Problem writing address. (%d)", returnVal);
        return returnVal;
    }
    for (j = i = 0; (typeStr[i+1]!= 0) || (j < numArgs); j++, i++){
        while (typeStr[i+1] == 'T' || typeStr[i+1] == 'F' || typeStr[i+1] == 'I' || typeStr[i+1] == 'N'){
#ifdef DEBUG
            printf("oscformat_writetypedmessage: NULL [%c]\n", typeStr[i+1]);
#endif
            returnVal = OSC_writeNullArg(buf, typeStr[i+1]);
            ++i;
        }
        if (j < numArgs){
            switch (args[j].type){
                case INT_osc:
#ifdef DEBUG
                    printf("oscformat_writetypedmessage: int [%d]\n", args[j].datum.i);
#endif
                    returnVal = OSC_writeIntArg(buf, args[j].datum.i);
                    break;
                case FLOAT_osc:
#ifdef DEBUG
                    printf("oscformat_writetypedmessage: float [%f]\n", args[j].datum.f);
#endif
                    returnVal = OSC_writeFloatArg(buf, args[j].datum.f);
                    break;
                case STRING_osc:
#ifdef DEBUG
                    printf("oscformat_writetypedmessage: string [%s]\n", args[j].datum.s);
#endif
                    returnVal = OSC_writeStringArg(buf, args[j].datum.s);
                    break;
                case BLOB_osc:
                    /* write all the blob elements at once */
#ifdef DEBUG
                    printf("oscformat_writetypedmessage calling OSC_writeBlobArg\n");
#endif
                    return OSC_writeBlobArg(buf, &args[j], numArgs-j);
                default:

                    break; /* types with no data */
            }
        }
    }
    return returnVal;
}

static int oscformat_writemessage(t_oscformat *x, OSCbuf *buf, char *messageName, int numArgs, typedArg *args){
    int j, returnVal = 0, numTags;
#ifdef DEBUG
    printf("oscformat_writemessage buf %p bufptr %p messageName %s %d args typetags %d\n", buf, buf->bufptr, messageName, numArgs, x->x_typetags);
#endif
    if(!x->x_typetags){
#ifdef DEBUG
        printf("oscformat_writemessage calling OSC_writeAddress with x->x_typetags %d\n", x->x_typetags);
#endif
        returnVal = OSC_writeAddress(buf, messageName);
        if (returnVal)
            post("oscformat: Problem writing address.");
    }
    else{
        char *typeTags;
        /* First figure out the type tags */
        for (numTags = 0; numTags < numArgs; numTags++){
            if (args[numTags].type == BLOB_osc)
                break; /* blob has one type tag and is the last element */
        }
        typeTags=(char*)getbytes(sizeof(char)*(numTags+2)); /* number of args + ',' + '\0' */
        typeTags[0] = ',';
        for (j = 0; j < numTags; ++j){
            switch (args[j].type){
                case INT_osc:
                    typeTags[j+1] = 'i';
                    break;
                case FLOAT_osc:
                    typeTags[j+1] = 'f';
                    break;
                case STRING_osc:
                    typeTags[j+1] = 's';
                    break;
                case BLOB_osc:
                    typeTags[j+1] = 'b';
                    break;
                default:
                    pd_error(x, "[osc.format]: arg %d type is unrecognized(%d)", j, args[j].type);
                    break;
            }
        }
        typeTags[j+1] = '\0';
#ifdef DEBUG
        printf("oscformat_writemessage calling OSC_writeAddressAndTypes with x->x_typetags %d typeTags %p (%s)\n", x->x_typetags, typeTags, typeTags);
#endif
        returnVal = OSC_writeAddressAndTypes(buf, messageName, typeTags);
        if (returnVal)
            pd_error(x, "oscformat: Problem writing address.");
        freebytes(typeTags, sizeof(char)*(numTags+2));
    }
    for (j = 0; j < numArgs; j++){
        switch (args[j].type){
            case INT_osc:
                returnVal = OSC_writeIntArg(buf, args[j].datum.i);
                break;
            case FLOAT_osc:
                returnVal = OSC_writeFloatArg(buf, args[j].datum.f);
                break;
            case STRING_osc:
                returnVal = OSC_writeStringArg(buf, args[j].datum.s);
                break;
            case BLOB_osc:
#ifdef DEBUG
                printf("oscformat_writemessage calling OSC_writeBlobArg\n");
#endif
                return OSC_writeBlobArg(buf, &args[j], numArgs-j); /* All the remaining args are blob */
            default:
                break; /* just skip bad types (which we won't get anyway unless this code is buggy) */
        }
    }
    return returnVal;
}

static void oscformat_sendbuffer(t_oscformat *x){
    int             i;
    int             length;
    unsigned char   *buf;
    int             reentry_count=x->x_reentry_count;      /* must be on stack for recursion */
    size_t          bufsize=sizeof(t_atom)*x->x_buflength; /* must be on stack for recursion */
    t_atom          *atombuffer=x->x_bufferForOSClist;     /* must be on stack in the case of recursion */
    if(reentry_count>0) /* if we are recurse, let's move atombuffer to the stack */
        atombuffer=(t_atom *)getbytes(bufsize);
    if(!atombuffer){
        pd_error(x, "oscformat: unable to allocate %lu bytes for atombuffer", (long)bufsize);
        return;
    }
#ifdef DEBUG
    printf("oscformat_sendbuffer: Sending buffer...\n");
#endif
    if (OSC_isBufferEmpty(x->x_oscbuf)){
        post("oscformat_sendbuffer() called but buffer empty");
        return;
    }
    if (!OSC_isBufferDone(x->x_oscbuf)){
        post("oscformat_sendbuffer() called but buffer not ready!, not exiting");
        return;
    }
    length = OSC_packetSize(x->x_oscbuf);
    buf = (unsigned char *)OSC_getPacket(x->x_oscbuf);
#ifdef DEBUG
    printf("oscformat_sendbuffer: length: %u\n", length);
#endif
    /* convert the bytes in the buffer to floats in a list */
    for (i = 0; i < length; ++i)
        SETFLOAT(&atombuffer[i], buf[i]);
    /* cleanup the OSCbuffer structure (so we are ready for recursion) */
    OSC_initBuffer(x->x_oscbuf, x->x_buflength, x->x_bufferForOSCbuf);
    /* send the list out the outlet */
    outlet_list(x->x_listout, &s_list, length, atombuffer);
    /* cleanup our 'stack'-allocated atombuffer in the case of reentrancy */
    if(reentry_count>0)
        freebytes(atombuffer, bufsize);
}

static void OSC_initBuffer(OSCbuf *buf, size_t size, char *byteArray){
    buf->buffer = byteArray;
    buf->size = size;
    OSC_resetBuffer(buf);
}

static void OSC_resetBuffer(OSCbuf *buf){
    buf->bufptr = buf->buffer;
    buf->state = EMPTY;
    buf->bundleDepth = 0;
    buf->prevCounts[0] = 0;
    buf->gettingFirstUntypedArg = 0;
    buf->typeStringPtr = 0;
}

static int OSC_isBufferEmpty(OSCbuf *buf){
    return buf->bufptr == buf->buffer;
}

static size_t OSC_freeSpaceInBuffer(OSCbuf *buf){
    return buf->size - (buf->bufptr - buf->buffer);
}

static int OSC_isBufferDone(OSCbuf *buf){
    return (buf->state == DONE || buf->state == ONE_MSG_ARGS);
}

static char *OSC_getPacket(OSCbuf *buf){
    return buf->buffer;
}

static int OSC_packetSize(OSCbuf *buf){
    return (buf->bufptr - buf->buffer);
}

static int OSC_CheckOverflow(OSCbuf *buf, size_t bytesNeeded){
    if ((bytesNeeded) > OSC_freeSpaceInBuffer(buf)){
        post("[osc.format]: buffer overflow");
        return 1;
    }
    return 0;
}

static void PatchMessageSize(OSCbuf *buf){
    uint32_t size = buf->bufptr - ((char *) buf->thisMsgSize) - 4;
    *(buf->thisMsgSize) = htonl(size);
}

/*static int OSC_openBundle(OSCbuf *buf, OSCTimeTag tt)
{
    if (buf->state == ONE_MSG_ARGS)
    {
        post("oscformat: Can't open a bundle in a one-message packet");
        return 3;
    }
    if (buf->state == DONE){
        post("oscformat: This packet is finished; can't open a new bundle");
        return 4;
    }
    if (++(buf->bundleDepth) >= MAX_BUNDLE_NESTING){
        post("oscformat: Bundles nested too deeply: maybe change MAX_BUNDLE_NESTING from %d and recompile", MAX_BUNDLE_NESTING);
        return 2;
    }
    if (CheckTypeTag(buf, '\0')) return 9;
    if (buf->state == GET_ARGS){
        PatchMessageSize(buf);
    }
    if (buf->state == EMPTY){
        // Need 16 bytes for "#bundle" and time tag
        if(OSC_CheckOverflow(buf, 16)) return 1;
    }
    else{
        // This bundle is inside another bundle, so we need to leave
        //  a blank size count for the size of this current bundle.
        if(OSC_CheckOverflow(buf, 20))return 1;
        *((uint32_t *)buf->bufptr) = 0xaaaaaaaa;
        buf->prevCounts[buf->bundleDepth] = (uint32_t *)buf->bufptr;
        buf->bufptr += 4;
    }
    buf->bufptr += OSC_padString(buf->bufptr, "#bundle");
    *((OSCTimeTag *) buf->bufptr) = tt;
    if (htonl(1) != 1){
        // Byte swap the 8-byte integer time tag
        uint32_t *intp = (uint32_t *)buf->bufptr;
        intp[0] = htonl(intp[0]);
        intp[1] = htonl(intp[1]);
        // tt  is a struct of two 32-bit words, and even though
        //  each word was wrong-endian, they were in the right order
        //  in the struct.)
    }
    buf->bufptr += sizeof(OSCTimeTag);
    buf->state = NEED_COUNT;
    buf->gettingFirstUntypedArg = 0;
    buf->typeStringPtr = 0;
    return 0;
}*/

/*static int OSC_closeBundle(OSCbuf *buf){
    if (buf->bundleDepth == 0){
        // This handles EMPTY, ONE_MSG, ARGS, and DONE
        post("oscformat: Can't close bundle: no bundle is open!");
        return 5;
    }
    if (CheckTypeTag(buf, '\0')) return 9;
    if (buf->state == GET_ARGS){
        PatchMessageSize(buf);
    }
    if (buf->bundleDepth == 1){
        // Closing the last bundle: No bundle size to patch
        buf->state = DONE;
    }
    else{
        // Closing a sub-bundle: patch bundle size
        int size = buf->bufptr - ((char *) buf->prevCounts[buf->bundleDepth]) - 4;
        *(buf->prevCounts[buf->bundleDepth]) = htonl(size);
        buf->state = NEED_COUNT;
    }
    --buf->bundleDepth;
    buf->gettingFirstUntypedArg = 0;
    buf->typeStringPtr = 0;
    return 0;
}*/

static int OSC_writeAddress(OSCbuf *buf, char *name){
    uint32_t paddedLength;
#ifdef DEBUG
    printf("-->OSC_writeAddress buf %p bufptr %p name %s\n", buf, buf->bufptr, name);
#endif
    if (buf->state == ONE_MSG_ARGS){
        post("oscformat: This packet is not a bundle, so you can't write another address");
        return 7;
    }
    if (buf->state == DONE){
        post("oscformat: This packet is finished; can't write another address");
        return 8;
    }
    if (CheckTypeTag(buf, '\0')) return 9;
    paddedLength = OSC_effectiveStringLength(name);
#ifdef DEBUG
    printf("OSC_writeAddress paddedLength %d\n", paddedLength);
#endif
    if (buf->state == EMPTY){
        /* This will be a one-message packet, so no sizes to worry about */
        if(OSC_CheckOverflow(buf, paddedLength))return 1;
        buf->state = ONE_MSG_ARGS;
    }
    else{
        /* GET_ARGS or NEED_COUNT */
        if(OSC_CheckOverflow(buf, 4+paddedLength))return 1;
        if (buf->state == GET_ARGS){
            /* Close the old message */
            PatchMessageSize(buf);
        }
        buf->thisMsgSize = (uint32_t *)buf->bufptr;
        *(buf->thisMsgSize) = 0xbbbbbbbb;
        buf->bufptr += 4;
        buf->state = GET_ARGS;
    }
    /* Now write the name */
    buf->bufptr += OSC_padString(buf->bufptr, name);
    buf->typeStringPtr = 0;
    buf->gettingFirstUntypedArg = 1;
    return 0;
}

static int OSC_writeAddressAndTypes(OSCbuf *buf, char *name, char *types){
    int      result;
    uint32_t paddedLength;
#ifdef DEBUG
    printf("OSC_writeAddressAndTypes buf %p name %s types %s\n", buf, name, types);
#endif
    if (buf == NULL)
        return 10;
    if (CheckTypeTag(buf, '\0'))
        return 9;
    result = OSC_writeAddress(buf, name);
    if (result)
        return result;
    paddedLength = OSC_effectiveStringLength(types);
    if(OSC_CheckOverflow(buf, paddedLength))
        return 1;
    buf->typeStringPtr = buf->bufptr + 1; /* skip comma */
    buf->bufptr += OSC_padString(buf->bufptr, types);
#ifdef DEBUG
    printf("OSC_writeAddressAndTypes buf->typeStringPtr now %p (%s) buf->bufptr now %p (%s)\n",
        buf->typeStringPtr, buf->typeStringPtr, buf->bufptr, buf->bufptr);
#endif
    buf->gettingFirstUntypedArg = 0;
    buf->typeStringPtr = 0;// ready for a new type string
    return 0;
}

static int CheckTypeTag(OSCbuf *buf, char expectedType){
    char c;
    if (buf->typeStringPtr){
#ifdef DEBUG
        printf("CheckTypeTag buf->typeStringPtr %p (%s)\n", buf->typeStringPtr, buf->typeStringPtr);
#endif
        c = *(buf->typeStringPtr);
#ifdef DEBUG
        printf("CheckTypeTag buf %p expectedType %c c is %c\n", buf, expectedType, c);
#endif
        if (c != expectedType){
            if (expectedType == '\0'){
                post("oscformat: According to the type tag (%c) I expected more arguments.", c);
            }
            else if (*(buf->typeStringPtr) == '\0'){
                post("oscformat: According to the type tag I didn't expect any more arguments.");
            }
            else{
                post("oscformat: According to the type tag I expected an argument of a different type.");
                post("* Expected %c, string now %s\n", expectedType, buf->typeStringPtr);
            }
            return 9;
        }
        ++(buf->typeStringPtr);
    }
    return 0;
}

static int OSC_writeFloatArg(OSCbuf *buf, float arg){
    union intfloat32{
        int     i;
        float   f;
    };
    union intfloat32 if32;
    if(OSC_CheckOverflow(buf, 4))return 1;
    if (CheckTypeTag(buf, 'f')) return 9;
    /* Pretend arg is a long int so we can use htonl() */
    if32.f = arg;
    *((uint32_t *) buf->bufptr) = htonl(if32.i);
    buf->bufptr += 4;
    buf->gettingFirstUntypedArg = 0;
    return 0;
}

static int OSC_writeIntArg(OSCbuf *buf, uint32_t arg){
    if(OSC_CheckOverflow(buf, 4))return 1;
    if (CheckTypeTag(buf, 'i')) return 9;
    *((uint32_t *) buf->bufptr) = htonl(arg);
    buf->bufptr += 4;
    buf->gettingFirstUntypedArg = 0;
    return 0;
}

static int OSC_writeBlobArg(OSCbuf *buf, typedArg *arg, size_t nArgs){
    size_t	i;
    unsigned char b;
/* pack all the args as single bytes following a 4-byte length */
    if(OSC_CheckOverflow(buf, nArgs+4))return 1;
    if (CheckTypeTag(buf, 'b')) return 9;
    *((uint32_t *) buf->bufptr) = htonl(nArgs);
#ifdef DEBUG
    printf("OSC_writeBlobArg length : %lu\n", nArgs);
#endif
    buf->bufptr += 4;
    for (i = 0; i < nArgs; i++){
        if (arg[i].type != BLOB_osc){
            post("[osc.format]: blob element %lu not blob type", i);
            return 9;
        }
        b = (unsigned char)((arg[i].datum.i)&0x0FF);/* force int to 8-bit byte */
#ifdef DEBUG
        printf("OSC_writeBlobArg : %d, %d\n", arg[i].datum.i, b);
#endif
        buf->bufptr[i] = b;
    }
    i = OSC_WriteBlobPadding(buf->bufptr, i);
    buf->bufptr += i;
    buf->gettingFirstUntypedArg = 0;
    return 0;
}

static int OSC_writeStringArg(OSCbuf *buf, char *arg){
    int len;
    if (CheckTypeTag(buf, 's')) return 9;
    len = OSC_effectiveStringLength(arg);
    if (buf->gettingFirstUntypedArg && arg[0] == ','){
        /* This un-type-tagged message starts with a string
          that starts with a comma, so we have to escape it
          (with a double comma) so it won't look like a type
          tag string. */
        if(OSC_CheckOverflow(buf, len+4))return 1; /* Too conservative */
        buf->bufptr += 
        OSC_padStringWithAnExtraStupidComma(buf->bufptr, arg);
    }
    else{
        if(OSC_CheckOverflow(buf, len))return 1;
        buf->bufptr += OSC_padString(buf->bufptr, arg);
    }
    buf->gettingFirstUntypedArg = 0;
    return 0;
}

static int OSC_writeNullArg(OSCbuf *buf, char type){ /* Don't write any data, just check the type tag */
    if(OSC_CheckOverflow(buf, 4))return 1;
    if (CheckTypeTag(buf, type)) return 9;
    buf->gettingFirstUntypedArg = 0;
    return 0;
}

static int OSC_strlen(char *s){
    int i;
    for (i = 0; s[i] != '\0'; i++) /* Do nothing */ ;
    return i;
}

#define STRING_ALIGN_PAD 4
static int OSC_effectiveStringLength(char *string){
    int len = OSC_strlen(string) + 1;  /* We need space for the null char. */
    /* Round up len to next multiple of STRING_ALIGN_PAD to account for alignment padding */
    if ((len % STRING_ALIGN_PAD) != 0)
        len += STRING_ALIGN_PAD - (len % STRING_ALIGN_PAD);
    return len;
}

static int OSC_padString(char *dest, char *str){
    int i;
    for(i = 0; str[i] != '\0'; i++)
        dest[i] = str[i];
    return OSC_WriteStringPadding(dest, i);
}

static int OSC_padStringWithAnExtraStupidComma(char *dest, char *str){
    int i;
    dest[0] = ',';
    for (i = 0; str[i] != '\0'; i++) dest[i+1] = str[i];
    return OSC_WriteStringPadding(dest, i+1);
}

static int OSC_WriteStringPadding(char *dest, int i){
    /* pad with at least one zero to fit 4-byte */
    dest[i] = '\0';
    i++;
    for (; (i % STRING_ALIGN_PAD) != 0; i++) dest[i] = '\0';
    return i;
}

static int OSC_WriteBlobPadding(char *dest, int i){
    /* pad if necessary to fit 4-byte */
    for (; (i % STRING_ALIGN_PAD) != 0; i++) dest[i] = '\0';
    return i;
}

// modified from OSC-timetag.c.

#define SECONDS_FROM_1900_to_1970 2208988800LL /* 17 leap years */
#define TWO_TO_THE_32_OVER_ONE_MILLION 4295LL

/*static OSCTimeTag OSCTT_CurrentTimePlusOffset(uint32_t offset)
{ // offset is in microseconds
    OSCTimeTag tt;
    static unsigned int onemillion = 1000000;
#ifdef _WIN32
    static unsigned int onethousand = 1000;
    struct _timeb tb;

    _ftime(&tb);

    // First get the seconds right
    tt.seconds = (unsigned)SECONDS_FROM_1900_to_1970 +
        (unsigned)tb.time+
        (unsigned)offset/onemillion;
    // Now get the fractional part.
    tt.fraction = (unsigned)tb.millitm*onethousand + (unsigned)(offset%onemillion); // in usec
#else
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);

    // First get the seconds right
    tt.seconds = (unsigned) SECONDS_FROM_1900_to_1970 +
        (unsigned) tv.tv_sec +
        (unsigned) offset/onemillion;
    // Now get the fractional part.
    tt.fraction = (unsigned) tv.tv_usec + (unsigned)(offset%onemillion); // in usec
#endif
    if (tt.fraction > onemillion)
    {
        tt.fraction -= onemillion;
        tt.seconds++;
    }
    tt.fraction *= (unsigned) TWO_TO_THE_32_OVER_ONE_MILLION; // convert usec to 32-bit fraction of 1 sec
    return tt;
}*/

static void *oscformat_new(void){
    t_oscformat *x = (t_oscformat *)pd_new(oscformat_class);
    x->x_typetags = 1; /* set typetags to 1 by default */
    x->x_bundle = 0; /* bundle is closed */
    x->x_buflength = SC_BUFFER_SIZE;
    x->x_bufferForOSCbuf = (char *)getbytes(sizeof(char)*x->x_buflength);
    if(x->x_bufferForOSCbuf == NULL){
        pd_error(x, "oscformat: unable to allocate %lu bytes for x_bufferForOSCbuf", (long)(sizeof(char)*x->x_buflength));
        goto fail;
    }
    x->x_bufferForOSClist = (t_atom *)getbytes(sizeof(t_atom)*x->x_buflength);
    if(x->x_bufferForOSClist == NULL){
        pd_error(x, "oscformat: unable to allocate %lu bytes for x_bufferForOSClist", (long)(sizeof(t_atom)*x->x_buflength));
        goto fail;
    }
//    if (x->x_oscbuf != NULL)
        OSC_initBuffer(x->x_oscbuf, x->x_buflength, x->x_bufferForOSCbuf);
    x->x_listout = outlet_new(&x->x_obj, &s_list);
//    x->x_bdpthout = outlet_new(&x->x_obj, &s_float);
    x->x_timeTagOffset = -1; /* immediately */
    x->x_reentry_count = 0;
    return (x);
fail:
    if(x->x_bufferForOSCbuf != NULL) freebytes(x->x_bufferForOSCbuf, (long)(sizeof(char)*x->x_buflength));
    if(x->x_bufferForOSClist != NULL) freebytes(x->x_bufferForOSClist, (long)(sizeof(char)*x->x_buflength));
    return(NULL);
}

void setup_osc0x2eformat(void){
    oscformat_class = class_new(gensym("osc.format"), (t_newmethod)oscformat_new,
        (t_method)oscformat_free, sizeof(t_oscformat), 0, A_DEFFLOAT, 0);
    class_addanything(oscformat_class, (t_method)oscformat_anything);
    class_addlist(oscformat_class, (t_method)oscformat_send);
/*  class_addmethod(oscformat_class, (t_method)oscformat_settypetags,
        gensym("typetags"), A_DEFFLOAT, 0);
    class_addmethod(oscformat_class, (t_method)oscformat_setTimeTagOffset,
        gensym("timetagoffset"), A_DEFFLOAT, 0);
    class_addmethod(oscformat_class, (t_method)oscformat_send_type_forced,
        gensym("sendtyped"), A_GIMME, 0);
    class_addmethod(oscformat_class, (t_method)oscformat_openbundle,
        gensym("["), 0, 0);
    class_addmethod(oscformat_class, (t_method)oscformat_closebundle,
        gensym("]"), 0, 0);*/
}
