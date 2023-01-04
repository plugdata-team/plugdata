// stolen from mrpeach's routeOSC

// Copyright (C) 2006-2011 Martin Peach
// slightly modified, renamed and greatly and vastly simplified by Porres 2023

// GNU license

/* oscroute.c 20060424 by Martin Peach, based on OSCroute and OSC-pattern-match.c. for MAX */
/* OSCroute.c header follows: */
/*
Written by Adrian Freed, The Center for New Music and Audio Technologies,
University of California, Berkeley.  Copyright (c) 1992,93,94,95,96,97,98,99,2000,01,02,03,04
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
*/

#include "m_pd.h"

#define MAX_NUM 256 // maximum number of paths (prefixes) we can route

typedef struct _oscroute{
    t_object    x_obj; /* required header */
    int         x_num; /* Number of prefixes we store */
    char        **x_prefixes; /* the OSC addresses to be matched */
    int         *x_prefix_depth; /* the number of slashes in each prefix */
    void        **x_outlets; /* one for each prefix plus one for everything else */
}t_oscroute;

static const char *theWholePattern;  // for warnings
static int MatchBrackets (const char *pattern, const char *test);
static int PatternMatch (const char *  pattern, const char * test);

static t_class *oscroute_class;
t_symbol *ps_list, *ps_complain, *ps_emptySymbol;

static int MyPatternMatch(const char *pattern, const char *test){
    // This allows the special case of "oscroute /* " to be an outlet that
    // matches anything; i.e., it always outputs the input with the first level
    // of the address stripped off.
    if(test[0] == '*' && test[1] == '\0')
        return (1);
    else
        return(PatternMatch(pattern, test));
}

static int MatchList(const char *pattern, const char *test){
    const char *restOfPattern, *tp = test;
    for(restOfPattern = pattern; *restOfPattern != '}'; restOfPattern++){
        if(*restOfPattern == 0){
            post("Unterminated { in pattern \".../%s/...\"", theWholePattern);
            return(0);
        }
    }
    restOfPattern++; /* skip close curly brace */
    pattern++; /* skip open curly brace */
    while(1){
        if(*pattern == ','){
            if (PatternMatch (restOfPattern, tp)) return 1;
            tp = test;
            ++pattern;
        }
        else if(*pattern == '}')
            return(PatternMatch (restOfPattern, tp));
        else if(*pattern == *tp){
            ++pattern;
            ++tp;
        }
        else{
            tp = test;
            while(*pattern != ',' && *pattern != '}')
                pattern++;
            if(*pattern == ',')
                pattern++;
        }
    }
}

static int PatternMatch(const char *  pattern, const char * test){
    theWholePattern = pattern;
    if(pattern == 0 || pattern[0] == 0)
        return(test[0] == 0);
    if(test[0] == 0){
        if(pattern[0] == '*')
            return(PatternMatch (pattern+1, test));
        return(0);
    }
    switch(pattern[0]){
        case 0:
            return test[0] == 0;
        case '?':
            return PatternMatch (pattern+1, test+1);
        case '*':
            if(PatternMatch (pattern+1, test))
                return 1;
            return PatternMatch (pattern, test+1);
        case ']':
        case '}':
            post("oscroute: Spurious %c in pattern \".../%s/...\"",pattern[0], theWholePattern);
            return 0;
        case '[':
            return MatchBrackets (pattern,test);
        case '{':
            return MatchList (pattern,test);
        case '\\':
            if (pattern[1] == 0) return test[0] == 0;
            if (pattern[1] == test[0]) return PatternMatch (pattern+2,test+1);
            return 0;
        default:
            if (pattern[0] == test[0]) return PatternMatch (pattern+1,test+1);
            return 0;
    }
}

/* we know that pattern[0] == '[' and test[0] != 0 */
static int MatchBrackets (const char *pattern, const char *test){
    int result;
    int negated = 0;
    const char *p = pattern;
    if (pattern[1] == 0){
        post("oscroute: Unterminated [ in pattern \".../%s/...\"", theWholePattern);
        return 0;
    }
    if (pattern[1] == '!'){
        negated = 1;
        p++;
    }
    while (*p != ']'){
        if (*p == 0){
            post("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
            return 0;
        }
        if (p[1] == '-' && p[2] != 0){
            if (test[0] >= p[0] && test[0] <= p[2]){
                result = !negated;
                goto advance;
            }
        }
        if (p[0] == test[0]){
            result = !negated;
            goto advance;
        }
        p++;
    }
    result = negated;
advance:
    if (!result)
        return 0;
    while (*p != ']'){
        if (*p == 0){
            post("Unterminated [ in pattern \".../%s/...\"", theWholePattern);
            return 0;
        }
        p++;
    }
    return PatternMatch (p+1,test+1);
}

static char *NextSlashOrNull(char *p){
    while(*p != '/' && *p != '\0')
        p++;
    return(p);
}

static char *NthSlashOrNull(char *p, int n){
    int i;
    for(i = 0; i < n; ++i){
        while(*p != '/'){
            if(*p == '\0')
                return(p);
            p++;
        }
        if(i < n-1)
            p++; /* skip the slash unless it's the nth slash */
    }
    return(p);
}

static void StrCopyUntilNthSlash(char *target, const char *source, int n){
    /* copy the path up but not including the nth slash */
    int i = n;
    while (i > 0){
        while (*source != '/'){
            *target = *source;
            if (*source == '\0') return;
            ++target;
            ++source;
        }
        i--;
        /* got  a slash. If it's the nth, don't include it */
        if (i == 0){
            *target = 0;
            return;
        }
        *target = *source;
        ++source;
        ++target;
    }
}

static int oscroute_count_slashes(char *prefix){ // find the path depth of the prefix by counting the numberof slashes */
    int i = 0;
    char *p = prefix;
    while (*p != '\0')
        if (*p++ == '/')
            i++;
    return(i);
}

static void oscroute_doanything(t_oscroute *x, t_symbol *s, int argc, t_atom *argv){
    char    *pattern, *nextSlash;
    int     i = 0, pattern_depth = 0, matchedAnything = 0;
    int     noPath = 0; // nonzero if we are dealing with a simple list (as from a previous [oscroute])
    pattern = (char *)s->s_name;
    if (pattern[0] != '/'){
        /* output unmatched data on rightmost outlet */
        outlet_anything(x->x_outlets[x->x_num], s, argc, argv);
        return;
    }
    pattern_depth = oscroute_count_slashes(pattern);
    nextSlash = NextSlashOrNull(pattern+1);
    if (*nextSlash == '\0'){ /* pattern_depth == 1 */
        /* last level of the address, so we'll output the argument list */
        for (i = 0; i < x->x_num; ++i){
            if((x->x_prefix_depth[i] <= pattern_depth) &&
            (MyPatternMatch(pattern+1, x->x_prefixes[i]+1))){
                ++matchedAnything;
                if(noPath){ // just a list starting with a symbol
                  // The special symbol is s
                  outlet_anything(x->x_outlets[i], s, argc, argv);
                }
                else{ // normal OSC path
                    // I hate stupid Max lists with a special first element
                    if(argc == 0)
                        outlet_bang(x->x_outlets[i]);
                    else if(argv[0].a_type == A_SYMBOL)
                        outlet_anything(x->x_outlets[i], argv[0].a_w.w_symbol, argc-1, argv+1);
                    else if (argc > 1){
                        // Multiple arguments starting with a number, so naturally we have
                        // to use a special function to output this "list", since it's what
                        // Max originally meant by "list".
                        outlet_list(x->x_outlets[i], 0L, argc, argv);
                    }
                    else{
                        // There was only one argument, and it was a number, so we output it
                        // not as a list
                        if (argv[0].a_type == A_FLOAT)
                            outlet_float(x->x_outlets[i], argv[0].a_w.w_float);
                        else
                            pd_error(x, "* oscroute: unrecognized atom type!");
                    }
                }
            }
        }
    }
    else{ // There's more address afterwards, so output list begins with next slash.
        t_symbol *restOfPattern = 0; /* avoid the gensym unless we have to output */
        char patternBegin[1000];
        /* Get the incoming pattern to match against all our prefixes */
        for (i = 0; i < x->x_num; ++i){
            restOfPattern = 0;
            if (x->x_prefix_depth[i] <= pattern_depth){
                StrCopyUntilNthSlash(patternBegin, pattern+1, x->x_prefix_depth[i]);
                if (MyPatternMatch(patternBegin, x->x_prefixes[i]+1)){
                    ++matchedAnything;
                    nextSlash = NthSlashOrNull(pattern+1, x->x_prefix_depth[i]);
                    if (*nextSlash != '\0'){
                        restOfPattern = gensym(nextSlash);
                        outlet_anything(x->x_outlets[i], restOfPattern, argc, argv);
                    }
                    else if (argc == 0)
                        outlet_bang(x->x_outlets[i]);
                    else
                    {
                        if (argv[0].a_type == A_SYMBOL) // Promote the symbol that was argv[0] to the special symbol
                            outlet_anything(x->x_outlets[i], argv[0].a_w.w_symbol, argc-1, argv+1);
                        else
                            outlet_anything(x->x_outlets[i], gensym("list"), argc, argv);
                    }
                }
            }
        }
    }
    if (!matchedAnything)
        // output unmatched data on rightmost outlet a la normal 'route' object, jdl 20020908
        outlet_anything(x->x_outlets[x->x_num], s, argc, argv);
}

static void oscroute_bang(t_oscroute *x){
    outlet_bang(x->x_outlets[x->x_num]);
}

static void oscroute_float(t_oscroute *x, t_floatarg f){
    outlet_float(x->x_outlets[x->x_num], f);
}

static void oscroute_symbol(t_oscroute *x, t_symbol *s){
    outlet_symbol(x->x_outlets[x->x_num], s);
}

static void oscroute_list(t_oscroute *x, t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    if (0 == argc) post("oscroute_list (%p) empty list", x);/* this should never happen but catch it just in case... */
    else if (argv[0].a_type == A_SYMBOL) oscroute_doanything(x, argv[0].a_w.w_symbol, argc-1, &argv[1]);
    else if (argv[0].a_type == A_FLOAT){
        outlet_list(x->x_outlets[x->x_num], 0L, argc, argv);
    }
}

static void oscroute_free(t_oscroute *x){
    freebytes(x->x_prefixes, x->x_num*sizeof(char *)); /* the OSC addresses to be matched */
    freebytes(x->x_prefix_depth, x->x_num*sizeof(int));  /* the number of slashes in each prefix */
    freebytes(x->x_outlets, (x->x_num+1)*sizeof(void *)); /* one for each prefix plus one for everything else */
}

static void *oscroute_new(t_symbol *s, int argc, t_atom *argv){
    s = NULL;
    t_oscroute *x = (t_oscroute *)pd_new(oscroute_class);   // get memory for a new object & initialize
    int i;
    if (argc > MAX_NUM){
        pd_error(x, "[osc.route]: too many arguments: %d (max %d)", argc, MAX_NUM);
        return 0;
    }
    x->x_num = 0;
/* first verify that all arguments are symbols whose first character is '/' */
    for (i = 0; i < argc; ++i){
        if (argv[i].a_type == A_SYMBOL){
            if (argv[i].a_w.w_symbol->s_name[0] == '/') // Now that's a nice prefix
                ++(x->x_num);
            else{
                pd_error(x, "[osc.route]: argument %d does not begin with a slash(/).", i);
                return(0);
            }
        }
        else{
            pd_error(x, "[osc.route]: argument %d is not a symbol.", i);
            return 0;
        }
    }
/* now allocate the storage for each path */
    x->x_prefixes = (char **)getzbytes(x->x_num*sizeof(char *)); /* the OSC addresses to be matched */
    x->x_prefix_depth = (int *)getzbytes(x->x_num*sizeof(int));  /* the number of slashes in each prefix */
    x->x_outlets = (void **)getzbytes((x->x_num+1)*sizeof(void *)); /* one for each prefix plus one for everything else */
/* put the pointer to the path in x_prefixes */
/* put the number of levels in x_prefix_depth */
    for (i = 0; i < x->x_num; ++i){
        x->x_prefixes[i] = (char *)argv[i].a_w.w_symbol->s_name;
        x->x_prefix_depth[i] = oscroute_count_slashes(x->x_prefixes[i]);
    }
    /* Have to create the outlets in reverse order */
    /* well, not in pd ? */
    for(i = 0; i <= x->x_num; i++)
        x->x_outlets[i] = outlet_new(&x->x_obj, &s_list);
    return(x);
}

void setup_osc0x2eroute(void){
    oscroute_class = class_new(gensym("osc.route"), (t_newmethod)oscroute_new,
        (t_method)oscroute_free, sizeof(t_oscroute), 0, A_GIMME, 0);
    class_addanything(oscroute_class, oscroute_doanything);
    class_addbang(oscroute_class, oscroute_bang);
    class_addfloat(oscroute_class, oscroute_float);
    class_addsymbol(oscroute_class, oscroute_symbol);
    class_addlist(oscroute_class, oscroute_list);
    ps_emptySymbol = gensym("");
}
