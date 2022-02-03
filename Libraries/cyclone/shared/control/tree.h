/* Copyright (c) 2003-2004 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#ifndef __HAMMERTREE_H__
#define __HAMMERTREE_H__

#ifdef KRZYSZCZ
#define HAMMERTREE_DEBUG
#endif

typedef enum
{
    HAMMERTYPE_FLOAT, HAMMERTYPE_SYMBOL, HAMMERTYPE_ATOM,
    HAMMERTYPE_CUSTOM, HAMMERTYPE_ILLEGAL
} t_hammertype;

typedef struct _hammernode
{
    int     n_key;
    int     n_black;
    struct _hammernode  *n_left;
    struct _hammernode  *n_right;
    struct _hammernode  *n_parent;
    struct _hammernode  *n_prev;
    struct _hammernode  *n_next;
} t_hammernode;

typedef struct _hammernode_float
{
    t_hammernode  nf_node;
    t_float       nf_value;
} t_hammernode_float;

typedef struct _hammernode_symbol
{
    t_hammernode  ns_node;
    t_symbol     *ns_value;
} t_hammernode_symbol;

typedef struct _hammernode_atom
{
    t_hammernode  na_node;
    t_atom        na_value;
} t_hammernode_atom;

typedef struct _hammertree
{
    t_hammernode  *t_root;
    t_hammernode  *t_first;
    t_hammernode  *t_last;
    t_hammertype   t_valuetype;
    size_t         t_nodesize;
} t_hammertree;

#define HAMMERNODE_GETFLOAT(np)  (((t_hammernode_float *)(np))->nf_value)
#define HAMMERNODE_GETSYMBOL(np)  (((t_hammernode_symbol *)(np))->ns_value)
#define HAMMERNODE_GETATOMPTR(np)  (&((t_hammernode_atom *)(np))->na_value)

typedef void (*t_hammernode_vshowhook)(t_hammernode *, char *, unsigned);

t_hammernode *hammertree_search(t_hammertree *tree, int key);
t_hammernode *hammertree_closest(t_hammertree *tree, int key, int geqflag);

t_hammernode *hammertree_insert(t_hammertree *tree, int key, int *foundp);
t_hammernode *hammertree_multiinsert(t_hammertree *tree, int key, int fifoflag);
t_hammernode *hammertree_insertfloat(t_hammertree *tree, int key, t_float f,
				     int replaceflag);
t_hammernode *hammertree_insertsymbol(t_hammertree *tree, int key, t_symbol *s,
				      int replaceflag);
t_hammernode *hammertree_insertatom(t_hammertree *tree, int key, t_atom *ap,
				    int replaceflag);
void hammertree_delete(t_hammertree *tree, t_hammernode *np);

void hammertree_inittyped(t_hammertree *tree,
			  t_hammertype vtype, int freecount);
void hammertree_initcustom(t_hammertree *tree,
			   size_t nodesize, int freecount);
void hammertree_clear(t_hammertree *tree, int freecount);

#ifdef HAMMERTREE_DEBUG
void hammertree_debug(t_hammertree *tree, int level,
		      t_hammernode_vshowhook hook);
#endif

#endif
