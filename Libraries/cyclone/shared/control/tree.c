/* Copyright (c) 2003-2004 krzYszcz and others.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

#include "m_pd.h"
#include "control/tree.h"

/* Since there is no sentinel node, the deletion routine has to have
   a few extra checks.  LATER rethink. */

/* LATER freelist */

typedef t_hammernode *(*t_hammertree_inserthook)(t_hammernode *);

#ifdef HAMMERTREE_DEBUG
/* returns black-height or 0 if failed */
static int hammernode_verify(t_hammernode *np)
{
    if (np)
    {
	int bhl, bhr;
	if (((bhl = hammernode_verify(np->n_left)) == 0) ||
	    ((bhr = hammernode_verify(np->n_right)) == 0))
	    return (0);
	if (bhl != bhr)
	{
	    /* failure: two paths rooted in the same node
	       contain different number of black nodes */
	    bug("hammernode_verify: not balanced");
	    return (0);
	}
	if (np->n_black)
	    return (bhl + 1);
	else
	{
	    if ((np->n_left && !np->n_left->n_black) ||
		(np->n_right && !np->n_right->n_black))
	    {
		bug("hammernode_verify: adjacent red nodes");
		return (0);
	    }
	    return (bhl);
	}
    }
    else return (1);
}

/* returns black-height or 0 if failed */
static int hammertree_verify(t_hammertree *tree)
{
    return (hammernode_verify(tree->t_root));
}

static int hammernode_checkmulti(t_hammernode *np1, t_hammernode *np2)
{
    if (np1 && np2 && np1->n_key == np2->n_key)
    {
	if (np1 == np2)
	    bug("hammernode_checkmulti");
	else
	    return (1);
    }
    return (0);
}

static void hammernode_post(t_hammertree *tree, t_hammernode *np,
			    t_hammernode_vshowhook hook, char *message)
{
    startpost("%d ", np->n_key);
    if (tree->t_valuetype == HAMMERTYPE_FLOAT)
	startpost("%g ", HAMMERNODE_GETFLOAT(np));
    else if (tree->t_valuetype == HAMMERTYPE_SYMBOL)
	startpost("%s ", HAMMERNODE_GETSYMBOL(np)->s_name);
    else if (tree->t_valuetype == HAMMERTYPE_ATOM)
    {
	t_atom *ap = HAMMERNODE_GETATOMPTR(np);
	if (ap->a_type == A_FLOAT)
	    startpost("%g ", ap->a_w.w_float);
	else if (ap->a_type == A_SYMBOL)
	    startpost("%s ", ap->a_w.w_symbol->s_name);
    }
    else if (hook)
    {
	char buf[MAXPDSTRING];
	(*hook)(np, buf, MAXPDSTRING);
	startpost("%s ", buf);
    }
    else startpost("0x%08x ", (int)HAMMERNODE_GETSYMBOL(np));
    startpost("%s ", (np->n_black ? "black" : "red"));

    if (hammernode_checkmulti(np, np->n_parent) ||
	hammernode_checkmulti(np, np->n_left) ||
	hammernode_checkmulti(np, np->n_right) ||
	hammernode_checkmulti(np->n_parent, np->n_left) ||
	hammernode_checkmulti(np->n_parent, np->n_right) ||
	hammernode_checkmulti(np->n_left, np->n_right))
	startpost("multi ");

    if (np->n_parent)
	startpost("(%d -> ", np->n_parent->n_key);
    else
	startpost("(nul -> ");
    if (np->n_left)
	startpost("%d, ", np->n_left->n_key);
    else
	startpost("nul, ");
    if (np->n_right)
	startpost("%d)", np->n_right->n_key);
    else
	startpost("nul)");
    if (message)
	post(": %s", message);
    else
	endpost();
}

/* Assert a standard stackless traversal producing the same sequence,
   as the auxiliary list. */
static int hammertree_checktraversal(t_hammertree *tree)
{
    t_hammernode *treewalk = tree->t_root;
    t_hammernode *listwalk = tree->t_first;
    int count = 0;
    while (treewalk)
    {
	t_hammernode *prev = treewalk->n_left;
	if (prev)
	{
	    while (prev->n_right && prev->n_right != treewalk)
		prev = prev->n_right;
	    if (prev->n_right)
	    {
		prev->n_right = 0;
		count++;
		if (treewalk == listwalk)
		    listwalk = listwalk->n_next;
		else
		{
		    bug("hammertree_checktraversal 1");
		    hammernode_post(tree, treewalk, 0, "treewalk");
		    if (listwalk)
			hammernode_post(tree, listwalk, 0, "listwalk");
		    else
			post("empty listwalk pointer");
		    listwalk = treewalk;
		}
		treewalk = treewalk->n_right;
	    }
	    else
	    {
		prev->n_right = treewalk;
		treewalk = treewalk->n_left;
	    }
	}
	else
	{
	    count++;
	    if (treewalk == listwalk)
		listwalk = listwalk->n_next;
	    else
	    {
		bug("hammertree_checktraversal 2");
		hammernode_post(tree, treewalk, 0, "treewalk");
		if (listwalk)
		    hammernode_post(tree, listwalk, 0, "listwalk");
		else
		    post("empty listwalk pointer");
		listwalk = treewalk;
	    }
	    treewalk = treewalk->n_right;
	}
    }
    return (count);
}

static int hammernode_height(t_hammernode *np)
{
    if (np)
    {
	int lh = hammernode_height(np->n_left);
	int rh = hammernode_height(np->n_right);
	return (lh > rh ? lh + 1 : rh + 1);
    }
    else return (0);
}

void hammertree_debug(t_hammertree *tree, int level,
		      t_hammernode_vshowhook hook)
{
    t_hammernode *np;
    int count;
    post("------------------------");
    count = hammertree_checktraversal(tree);
    if (level)
    {
	for (np = tree->t_first; np; np = np->n_next)
	    hammernode_post(tree, np, hook, 0);
	if (level > 1)
	{
	    post("************");
	    for (np = tree->t_last; np; np = np->n_prev)
		startpost("%d ", np->n_key);
	    endpost();
	}
    }
    if (tree->t_root)
    {
	t_hammernode *first = tree->t_root, *last = tree->t_root;
	while (first->n_left && first->n_left != tree->t_root)
	    first = first->n_left;
	while (last->n_right && last->n_right != tree->t_root)
	    last = last->n_right;
	post("count %d, height %d, root %d",
	     count, hammernode_height(tree->t_root), tree->t_root->n_key);
	post("first %d, root->left* %d, last %d, root->right* %d",
	     (tree->t_first ? tree->t_first->n_key : 0), first->n_key,
	     (tree->t_last ? tree->t_last->n_key : 0), last->n_key);
    }
    else post("empty");
    post("...verified (black-height is %d)", hammertree_verify(tree));
    post("------------------------");
}
#endif

/* assuming that target node (np->n_right) exists */
static void hammertree_lrotate(t_hammertree *tree, t_hammernode *np)
{
    t_hammernode *target = np->n_right;
    if (np->n_right = target->n_left)
	np->n_right->n_parent = np;
    if (!(target->n_parent = np->n_parent))
	tree->t_root = target;
    else if (np == np->n_parent->n_left)
	np->n_parent->n_left = target;
    else
	np->n_parent->n_right = target;
    target->n_left = np;
    np->n_parent = target;
}

/* assuming that target node (np->n_left) exists */
static void hammertree_rrotate(t_hammertree *tree, t_hammernode *np)
{
    t_hammernode *target = np->n_left;
    if (np->n_left = target->n_right)
	np->n_left->n_parent = np;
    if (!(target->n_parent = np->n_parent))
	tree->t_root = target;
    else if (np == np->n_parent->n_left)
	np->n_parent->n_left = target;
    else
	np->n_parent->n_right = target;
    target->n_right = np;
    np->n_parent = target;
}

static t_hammernode *hammertree_preinserthook(t_hammernode *np)
{
    while (np->n_prev && np->n_prev->n_key == np->n_key)
	np = np->n_prev;
    if (np->n_left)
    {
	np = np->n_prev;
	if (np->n_right)
	{
	    /* LATER revisit */
	    bug("hammertree_preinserthook");
	    return (0);  /* do nothing */
	}
    }
    return (np);
}

static t_hammernode *hammertree_postinserthook(t_hammernode *np)
{
    while (np->n_next && np->n_next->n_key == np->n_key)
	np = np->n_next;
    if (np->n_right)
    {
	np = np->n_next;
	if (np->n_left)
	{
	    /* LATER revisit */
	    bug("hammertree_postinserthook");
	    return (0);  /* do nothing */
	}
    }
    return (np);
}

/* Returns a newly inserted or already existing node (or 0 if allocation
   failed).  A caller is responsible for assigning a value.  If hook is
   supplied, it is called iff key is found.  In case of key being found
   (which means foundp returns 1), a new node is inserted, unless hook is
   either empty, or returns null.  Hook's nonempty return is the parent
   for the new node.  It is expected to have no more than one child. */
static t_hammernode *hammertree_doinsert(t_hammertree *tree, int key,
					 t_hammertree_inserthook hook,
					 int *foundp)
{
    t_hammernode *np, *parent, *result;
    int leftchild;
    *foundp = 0;
    if (!(np = tree->t_root))
    {
	if (!(np = getbytes(tree->t_nodesize)))
	    return (0);
	np->n_key = key;
	np->n_black = 1;
	tree->t_root = tree->t_first = tree->t_last = np;
	return (np);
    }

    do
    {
	if (np->n_key == key)
	{
	    *foundp = 1;
	    if (hook && (parent = (*hook)(np)))
	    {
		if (parent->n_left && parent->n_right)
		{
		    bug("hammertree_insert, callback return 1");
		    parent = parent->n_next;
		}
		if (leftchild = (key < parent->n_key))
		{
		    if (parent->n_left)
		    {
			bug("hammertree_insert, callback return 2");
			leftchild = 0;
		    }
		}
		else if (parent->n_right)
		    leftchild = 1;
		goto addit;
	    }
	    else return (np);  /* a caller may then keep or replace the value */
	}
	else parent = np;
    }
    while (np = (key < np->n_key ? np->n_left : np->n_right));
    leftchild = (key < parent->n_key);
addit:
    /* parent has no more than one child, new node becomes
       parent's immediate successor or predecessor */
    if (!(np = getbytes(tree->t_nodesize)))
	return (0);
    np->n_key = key;
    np->n_parent = parent;
    if (leftchild)
    {
	parent->n_left = np;
	/* update the auxiliary linked list structure */
	np->n_next = parent;
	if (np->n_prev = parent->n_prev)
	    np->n_prev->n_next = np;
	else
	    tree->t_first = np;
	parent->n_prev = np;
    }
    else
    {
	parent->n_right = np;
	/* update the auxiliary linked list structure */
	np->n_prev = parent;
	if (np->n_next = parent->n_next)
	    np->n_next->n_prev = np;
	else
	    tree->t_last = np;
	parent->n_next = np;
    }
    result = np;

    /* balance the tree -- LATER clean this if possible... */
    np->n_black = 0;
    while (np != tree->t_root && !np->n_parent->n_black)
    {
	t_hammernode *uncle;
	/* np->n_parent->n_parent exists (we always paint root node in black) */
	if (np->n_parent == np->n_parent->n_parent->n_left)
	{
	    uncle = np->n_parent->n_parent->n_right;
	    if (!uncle  /* (sentinel not used) */
		|| uncle->n_black)
	    {
		if (np == np->n_parent->n_right)
		{
		    np = np->n_parent;
		    hammertree_lrotate(tree, np);
		}
		np->n_parent->n_black = 1;
		np->n_parent->n_parent->n_black = 0;
		hammertree_rrotate(tree, np->n_parent->n_parent);
	    }
	    else
	    {
		np->n_parent->n_black = 1;
		uncle->n_black = 1;
		np = np->n_parent->n_parent;
		np->n_black = 0;
	    }
	}
	else
	{
	    uncle = np->n_parent->n_parent->n_left;
	    if (!uncle  /* (sentinel not used) */
		|| uncle->n_black)
	    {
		if (np == np->n_parent->n_left)
		{
		    np = np->n_parent;
		    hammertree_rrotate(tree, np);
		}
		np->n_parent->n_black = 1;
		np->n_parent->n_parent->n_black = 0;
		hammertree_lrotate(tree, np->n_parent->n_parent);
	    }
	    else
	    {
		np->n_parent->n_black = 1;
		uncle->n_black = 1;
		np = np->n_parent->n_parent;
		np->n_black = 0;
	    }
	}
    }
    tree->t_root->n_black = 1;
    return (result);
}

/* assuming that requested node exists */
void hammertree_delete(t_hammertree *tree, t_hammernode *gone)
{
    t_hammernode *parent;  /* parent of gone, after relinking */
    t_hammernode *child;   /* gone's only child (or null), after relinking */
    /* gone has to be the parent of no more than one child */
    if (gone->n_left && gone->n_right)
    {
	/* Successor is the new parent of gone's children, and a new child
	   of gone's parent (if any).  Successor always exists in this context,
	   and it has no left child.  The simplistic scheme is to replace
	   gone's fields with successor's fields, and delete the successor.
	   We cannot do so, however, because successor may be pointed at... */
	t_hammernode *successor = gone->n_next;
	child = successor->n_right;
	successor->n_left = gone->n_left;
	successor->n_left->n_parent = successor;
	if (successor == gone->n_right)
	    parent = successor;
	else
	{
	    /* successor's parent always exists in this context,
	       successor is the left child of its parent */
	    parent = successor->n_parent;
	    parent->n_left = child;
	    if (child)  /* (sentinel not used) */
		child->n_parent = parent;
	    successor->n_right = gone->n_right;
	    successor->n_right->n_parent = successor;
	}
	if (gone->n_parent)
	{
	    int swp;
	    if (gone == gone->n_parent->n_left)
		gone->n_parent->n_left = successor;
	    else
		gone->n_parent->n_right = successor;
	    successor->n_parent = gone->n_parent;
	    swp = gone->n_black;
	    gone->n_black = successor->n_black;
	    successor->n_black = swp;
	}
	else
	{
	    tree->t_root = successor;
	    successor->n_parent = 0;
	    gone->n_black = successor->n_black;
	    successor->n_black = 1;  /* LATER rethink */
	}

	/* update the auxiliary linked list structure */
	if (successor->n_prev = gone->n_prev)
	    gone->n_prev->n_next = successor;
	else
	    tree->t_first = successor;
    }
    else
    {
	/* update the auxiliary linked list structure */
	if (gone->n_prev)
	    gone->n_prev->n_next = gone->n_next;
	else
	    tree->t_first = gone->n_next;
	if (gone->n_next)
	    gone->n_next->n_prev = gone->n_prev;
	else
	    tree->t_last = gone->n_prev;

	/* connect gone's child with gone's parent */
	if (gone->n_left)
	    child = gone->n_left;
	else
	    child = gone->n_right;
	if (parent = gone->n_parent)
	{
	    if (child)  /* (sentinel not used) */
		child->n_parent = parent;
	    if (gone == parent->n_left)
		parent->n_left = child;
	    else
		parent->n_right = child;
	}
	else
	{
	    if (tree->t_root = child)
	    {
		child->n_parent = 0;
		child->n_black = 1;  /* LATER rethink */
	    }
	    goto done;
	}
    }

    if (gone->n_black)
    {
	/* balance the tree -- LATER clean this if possible... */
	/* on entry:  tree is not empty, parent always exists, child
	   not necessarily... */
	while (child != tree->t_root &&
	       (!child ||  /* (sentinel not used) */
		child->n_black))
	{
	    t_hammernode *other;  /* another child of the same parent */
	    if (child == parent->n_left)
	    {
		other = parent->n_right;
		if (other &&  /* (sentinel not used) */
		    !other->n_black)
		{
		    other->n_black = 1;
		    parent->n_black = 0;
		    hammertree_lrotate(tree, parent);
		    other = parent->n_right;
		}
		if (!other ||  /* (sentinel not used) */
		    (!other->n_left || other->n_left->n_black) &&
		    (!other->n_right || other->n_right->n_black))
		{
		    if (other)  /* (sentinel not used) */
			other->n_black = 0;
		    child = parent;
		    parent = parent->n_parent;
		}
		else
		{
		    if (!other ||  /* (sentinel not used) */
			!other->n_right || other->n_right->n_black)
		    {
			if (other)  /* (sentinel not used) */
			{
			    if (other->n_left) other->n_left->n_black = 1;
			    other->n_black = 0;
			    hammertree_rrotate(tree, other);
			    other = parent->n_right;
			}
		    }
		    if (other)  /* (sentinel not used) */
		    {
			if (other->n_right) other->n_right->n_black = 1;
			other->n_black = parent->n_black;
		    }
		    parent->n_black = 1;
		    hammertree_lrotate(tree, parent);
		    tree->t_root->n_black = 1;  /* LATER rethink */
		    goto done;
		}
	    }
	    else  /* right child */
	    {
		other = parent->n_left;
		if (other &&  /* (sentinel not used) */
		    !other->n_black)
		{
		    other->n_black = 1;
		    parent->n_black = 0;
		    hammertree_rrotate(tree, parent);
		    other = parent->n_left;
		}
		if (!other ||  /* (sentinel not used) */
		    (!other->n_left || other->n_left->n_black) &&
		    (!other->n_right || other->n_right->n_black))
		{
		    if (other)  /* (sentinel not used) */
			other->n_black = 0;
		    child = parent;
		    parent = parent->n_parent;
		}
		else
		{
		    if (!other ||  /* (sentinel not used) */
			!other->n_left || other->n_left->n_black)
		    {
			if (other)  /* (sentinel not used) */
			{
			    if (other->n_right) other->n_right->n_black = 1;
			    other->n_black = 0;
			    hammertree_lrotate(tree, other);
			    other = parent->n_left;
			}
		    }
		    if (other)  /* (sentinel not used) */
		    {
			if (other->n_left) other->n_left->n_black = 1;
			other->n_black = parent->n_black;
		    }
		    parent->n_black = 1;
		    hammertree_rrotate(tree, parent);
		    tree->t_root->n_black = 1;  /* LATER rethink */
		    goto done;
		}
	    }
	}
	if (child)  /* (sentinel not used) */
	    child->n_black = 1;
    }
done:
    freebytes(gone, tree->t_nodesize);
#ifdef HAMMERTREE_DEBUG
    hammertree_verify(tree);
#endif
}

t_hammernode *hammertree_search(t_hammertree *tree, int key)
{
    t_hammernode *np = tree->t_root;
    while (np && np->n_key != key)
	np = (key < np->n_key ? np->n_left : np->n_right);
    return (np);
}

t_hammernode *hammertree_closest(t_hammertree *tree, int key, int geqflag)
{
    t_hammernode *np, *parent;
    if (!(np = tree->t_root))
	return (0);
    do
	if (np->n_key == key)
	    return (np);
	else
	    parent = np;
    while (np = (key < np->n_key ? np->n_left : np->n_right));
    if (geqflag)
	return (key > parent->n_key ? parent->n_next : parent);
    else
	return (key < parent->n_key ? parent->n_prev : parent);
}

t_hammernode *hammertree_insert(t_hammertree *tree, int key, int *foundp)
{
    return (hammertree_doinsert(tree, key, 0, foundp));
}

t_hammernode *hammertree_multiinsert(t_hammertree *tree, int key, int fifoflag)
{
    int found;
    return (hammertree_doinsert(tree, key, (fifoflag ?
					    hammertree_postinserthook :
					    hammertree_preinserthook), &found));
}

t_hammernode *hammertree_insertfloat(t_hammertree *tree, int key, t_float f,
				     int replaceflag)
{
    int found;
    t_hammernode *np = hammertree_doinsert(tree, key, 0, &found);
    if (np && (!found || replaceflag))
    {
	if (tree->t_valuetype == HAMMERTYPE_FLOAT)
	{
	    t_hammernode_float *npf = (t_hammernode_float *)np;
	    npf->nf_value = f;
	}
	else if (tree->t_valuetype == HAMMERTYPE_ATOM)
	{
	    t_hammernode_atom *npa = (t_hammernode_atom *)np;
	    t_atom *ap = &npa->na_value;
	    SETFLOAT(ap, f);
	}
	else bug("hammertree_insertfloat");
    }
    return (np);
}

t_hammernode *hammertree_insertsymbol(t_hammertree *tree, int key, t_symbol *s,
				      int replaceflag)
{
    int found;
    t_hammernode *np = hammertree_doinsert(tree, key, 0, &found);
    if (np && (!found || replaceflag))
    {
	if (tree->t_valuetype == HAMMERTYPE_SYMBOL)
	{
	    t_hammernode_symbol *nps = (t_hammernode_symbol *)np;
	    nps->ns_value = s;
	}
	else if (tree->t_valuetype == HAMMERTYPE_ATOM)
	{
	    t_hammernode_atom *npa = (t_hammernode_atom *)np;
	    t_atom *ap = &npa->na_value;
	    SETSYMBOL(ap, s);
	}
	else bug("hammertree_insertsymbol");
    }
    return (np);
}

t_hammernode *hammertree_insertatom(t_hammertree *tree, int key, t_atom *ap,
				    int replaceflag)
{
    int found;
    t_hammernode *np = hammertree_doinsert(tree, key, 0, &found);
    if (np && (!found || replaceflag))
    {
	if (tree->t_valuetype == HAMMERTYPE_ATOM)
	{
	    t_hammernode_atom *npa = (t_hammernode_atom *)np;
	    npa->na_value = *ap;
	}
	else bug("hammertree_insertatom");
    }
    return (np);
}

/* LATER preallocate 'freecount' nodes */
static void hammertree_doinit(t_hammertree *tree, t_hammertype vtype,
			      size_t nodesize, int freecount)
{
    tree->t_root = tree->t_first = tree->t_last = 0;
    tree->t_valuetype = vtype;
    tree->t_nodesize = nodesize;
}

void hammertree_inittyped(t_hammertree *tree,
			  t_hammertype vtype, int freecount)
{
    size_t nsize;
    switch (vtype)
    {
    case HAMMERTYPE_FLOAT:
	nsize = sizeof(t_hammernode_float);
	break;
    case HAMMERTYPE_SYMBOL:
	nsize = sizeof(t_hammernode_symbol);
	break;
    case HAMMERTYPE_ATOM:
	nsize = sizeof(t_hammernode_atom);
	break;
    default:
	bug("hammertree_inittyped");
	vtype = HAMMERTYPE_ILLEGAL;
	nsize = sizeof(t_hammernode);
    }
    hammertree_doinit(tree, vtype, nsize, freecount);
}

void hammertree_initcustom(t_hammertree *tree,
			   size_t nodesize, int freecount)
{
    hammertree_doinit(tree, HAMMERTYPE_CUSTOM, nodesize, freecount);
}

/* LATER keep and/or preallocate 'freecount' nodes (if negative, keep all) */
void hammertree_clear(t_hammertree *tree, int freecount)
{
    t_hammernode *np, *next = tree->t_first;
    while (next)
    {
	np = next;
	next = next->n_next;
	freebytes(np, tree->t_nodesize);
    }
    hammertree_doinit(tree, tree->t_valuetype, tree->t_nodesize, 0);
}
