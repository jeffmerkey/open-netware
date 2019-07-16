

/***************************************************************************
*
*   Copyright (c) 1997, 1998 Timpanogas Research Group, Inc.  All Rights
*			    Reserved.
*
*   This program is an unpublished work of TRG, Inc. and contains
*   trade secrets and other proprietary information.  Unauthorized
*   use, copying, disclosure, or distribution of this file without the
*   consent of TRG, Inc. can subject the offender to severe criminal
*   and/or civil penalities.
*
*
*   AUTHOR   :  Jeff V. Merkey
*   FILE     :  TREES.C
*   DESCRIP  :  Binary Tree Library for MANOS v1.0
*   DATE     :  June 26, 1998
*
***************************************************************************/

#include "version.h"
#include "stdarg.h"
#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "string.h"
#include "kernel.h"
#include "keyboard.h"
#include "screen.h"
#include "types.h"
#include "emit.h"
#include "dos.h"
#include "tss.h"
#include "os.h"
#include "mps.h"
#include "hal.h"
#include "timer.h"
#include "peexe.h"
#include "malloc.h"
#include "free.h"

#define TEST


#include "trees.h"

/* A safe malloc() */
static void *tmalloc(int size)
{

    void *p;
    if ((p = malloc(size)) == NULL)
    {
        printf("Out of memory\n");
	return 0;
    }

    return p;
}

/*
 * Create and initialize a node for the user. 'size' both can
 * and should be greater than sizeof(Bnode) to allow for a
 * data area for the user.
 */

Bnode *InitBintreeNode(int size)
{
    Bnode *n;

    n = tmalloc(size);
    n -> link[LEFT] = n -> link[RIGHT]  = NULL;
    RBONLY(n -> red = 0;)

    return n;
}

/* Create an empty tree */
Bintree *NewBintree (Bnode *dummy, CompFunc cf, int dup_ok, int node_size)
{
    Bintree *t;

    t = tmalloc(sizeof(Bintree));
    t -> DummyHead = dummy;
    t -> Compare = cf;
    t -> DuplicatesOk = dup_ok;
    t -> NodeSize = node_size;

    return t;
}

#if defined(SPLAY)

/*
 * During a top-down splay, we build up the future left and right
 * sub-trees in trees whose roots are stored in the array LR[].
 * LRwalk[] retains a current pointer into each of these trees.
 * We are always interested in finding the bottom left node of
 * the right tree or the bottom right node of the left tree.
 * Thus, PUSH(LEFT) starts at LRwalk[LEFT], steps down and to
 * the right until it hits bottom, and then stores the new
 * location in LRwalk[LEFT].
 */

#define PUSH(x) {                                 \
                    Bnode *w;                     \
                    for (w = LRwalk[x];           \
                         w -> link[!(x)];         \
                         w = w -> link[!(x)]);    \
                    LRwalk[x] = w;                \
                }

int splay(Bintree *t, Bnode *n)
{
    Bnode *s, *ch, *gch;
    Bnode LR[2], *LRwalk[2];
    int s_comp, ch_comp, dir, dir2;

    s = t -> DummyHead -> link[RIGHT];
    if (s == NULL) /* empty tree */
        return 1; /* no match */

    /*
     * Create two empty trees: we place portions of the initial
     * tree onto these two trees as we "splay down" the tree.
     */
    LR[LEFT].link[RIGHT] = NULL;
    LR[RIGHT].link[LEFT] = NULL;

    LRwalk[LEFT]  = &LR[LEFT];
    LRwalk[RIGHT] = &LR[RIGHT];

    /* not really needed */
    LR[LEFT].link[LEFT] = NULL;
    LR[RIGHT].link[RIGHT] = NULL;

    for (;;) {
        /* We are at s. Which way now? First, find s's child */
        s_comp = n ? (t -> Compare)(n, s) : 1;
        dir = s_comp < 0;
        ch = s -> link[dir];
        if (s_comp == 0 || ch == NULL)
            break;

        /* Now, find s's grandchild */
        ch_comp = n ? (t -> Compare)(n, ch) : 1;
        dir2 = ch_comp < 0;
        gch = ch -> link[dir2];

        /*
         * If we've found a match for n (ch_comp==0) or if we've
         * no further to go (gch==NULL), then we're done. ch will
         * be the root of the new tree after reconstruction is
         * complete. This case is the only exit from this loop.
         */
        if (ch_comp == 0 || gch == NULL) {
            s -> link[dir] = NULL; /* break link betw s and ch */
            LRwalk[!dir] -> link[dir] = s; /* hang s on LR */
            PUSH(!dir);  /* and push LRwalk to bottom */

            s = ch; /* advance s to ch */
            s_comp = ch_comp;
            break; /* proceed to tree reconstruction */
        }

        else { /* split up the tree as described in the text */
            if (dir == dir2) { /* zig-zig */
                s -> link[dir] = ch -> link[!dir];
                ch -> link[!dir] = s;
                ch -> link[dir] = NULL;
                LRwalk[!dir] -> link[dir] = ch;
                PUSH(!dir);
            }

            else { /* zig-zag */
                s -> link[dir] = NULL;
                LRwalk[!dir] -> link[dir] = s;
                PUSH(!dir);
                ch -> link[dir2] = NULL;
                LRwalk[!dir2] -> link[dir2] = ch;
                PUSH(!dir2);
            }
            s = gch;
        }
    }

    /* put it all together */
    LRwalk[LEFT]  -> link[RIGHT] = s -> link[LEFT];
    LRwalk[RIGHT] -> link[LEFT]  = s -> link[RIGHT];
    s -> link[LEFT]  = LR[LEFT].link[RIGHT];
    s -> link[RIGHT] = LR[RIGHT].link[LEFT];
    t -> DummyHead -> link[RIGHT] = s;
    return s_comp;
}
#endif

/* Find node n in tree t */

Bnode *FindBintree(Bintree *t, Bnode *n)
{
    #if defined(SPLAY)
    if (splay(t, n))
        return NULL; /* exact match not found */
    else
        return t -> DummyHead -> link[RIGHT];

    #else /* plain or red-black */
    Bnode *s;
    int dir;

    s = t -> DummyHead -> link[RIGHT];
    while (s != NULL) {
        dir = (t -> Compare) (n, s);
        /*
         * If a match, we're done.
         * For Red-Black, must also be a leaf.
         */
        if (dir == 0 RBONLY(&& s -> link[RIGHT] == NULL))
	    return s;
        dir = dir < 0;
        s = s -> link[dir];
    }
    return NULL; /* no match */
    #endif
}

#if defined(REDBLACK)
/*
 * Rotate child and grandchild of r along the path
 * specified by searching for n. For example, if n was
 * equal to 3, gc2, or 4, the following rotation occurs:
 *
 *            r                              r
 *            |                              |
 *            c                             gc2
 *        /       \         ==>            /    \
 *      gc1       gc2                    c        4
 *    /    \     /    \                /   \
 *   1      2   3      4             gc1     3
 *                                 /    \
 *                                1      2
 *
 * As r may connect to c via either its left or right
 * link, there are actually four symmetric variants.
 *
 * A pointer to the top of the new rotated nodes (in the
 * case above, to gc2) is returned.
 *
 * This routine is complicated by the fact that the routine
 * uses the value of the node n to decide which direction
 * to rotate. This may or may not be the direction the caller
 * has is mind. Rather than require the caller to specify
 * the direction of the rotation, it seemed easier to allow
 * the caller to specify whether to go in the direction of n
 * or away from it. This is done by the last argument to the
 * function, flip_mode. The caller can indicate that either
 * or both of the directions to child and grandchild should
 * be reversed during the rotation.
 */

#define NO_FLIP  0
#define FLIP_GCH 1
#define FLIP_CH  2
Bnode *rotate(Bintree *t, Bnode *n, Bnode *r, int flip_mode)
{
    Bnode *ch, *gch;
    int ch_dir, gch_dir;

    /* Identify child and grandchild */
    ch_dir = (t -> Compare) (n, r) < 0;
    if (flip_mode & FLIP_CH)
        ch_dir = !ch_dir;
    if (r == t -> DummyHead) /* special condition */
        ch_dir = RIGHT;
    ch = r -> link[ch_dir];

    gch_dir = (t -> Compare) (n, ch) < 0;
    if (flip_mode) {
	if (flip_mode == FLIP_GCH)
            gch_dir = !gch_dir;
        else
            gch_dir = flip_mode & 1;
    }
    gch = ch -> link[gch_dir];

    /* rotate: now move pointers */
    ch -> link[gch_dir] = gch -> link[!gch_dir];
    gch -> link[!gch_dir] = ch;
    r -> link[ch_dir] = gch;

    return gch;
}

/*
 * Take care of colors and balance. It will color the current
 * location red, the current location's children black, and
 * then look to see if two consecutive red nodes have been
 * created. If so, a single or double rotation will be done
 * to fix the tree.
 */

void split(Bintree *t,  /* tree                  */
           Bnode *n,    /* node being inserted   */
           Bnode **c,   /* current location      */
           Bnode **p,   /* its parent            */
           Bnode *g,    /* its grandparent       */
           Bnode *gg)   /* its great-grandparent */
{
    if (t -> DummyHead -> red)
    {
	printf("dummyhead was red!!\n");
        t -> DummyHead -> red = 0;
    }
    (*c) -> red = 1;
    if ((*c) -> link[LEFT])
        (*c) -> link[LEFT] -> red = 0;
    if ((*c) -> link[RIGHT])
        (*c) -> link[RIGHT] -> red = 0;

    /*
     * Check to make sure we haven't created two red
     * links in a row. If we have, we must rotate.
     */
    if ((*p) -> red) {
        g -> red = 1;
        /*
         * If the red links don't point in the same direction,
         * then will need a double rotation. The lower half
         * is around the grandparent and then the upper half
         * is around the great-grandparent.
         */
        if (((t -> Compare) (n, g) < 0) !=
            ((t -> Compare) (n, *p) < 0))
	    *p = rotate(t, n, g, NO_FLIP);

        /* Same for both single and double rotations. */
        *c = rotate(t, n, gg, NO_FLIP);
        (*c) -> red = 0;
    }

    t -> DummyHead -> link[RIGHT] -> red = 0;
}
#endif

/*
 * Delete node n from tree t. Returns a pointer to the
 * deleted node -- it should then be freed or otherwise
 * destroyed. The versions for the binary tree and the
 * red-black tree are very different, due to the balancing
 * problems that the red-black version must handle.
 */
#if defined(REDBLACK)

Bnode *DelBintree (Bintree *t, Bnode *n)
{
    /*
     * The goal is to arrive at a leaf with a red parent.
     * Thus, we force this by dragging a red node with us
     * down the tree, re-arranging the tree to keep its
     * balance as we go. All the rearrangements keep the tree
     * balanced, so if we cancel the deletion or don't find
     * the specified node to delete, we can just quit.
     */

    Bnode *s, *p, *g;
    int dir, next_dir;

    g = NULL;
    p = t -> DummyHead;
    s = p -> link[RIGHT];
    dir = RIGHT;

    /*
     * First, check on the root. It must exist, have children,
     * and either it or one of its children must be red. We can
     * just paint the root red, if necessary, as this will
     * affect the black height of the entire tree equally.
     */
    if (s == NULL)
        return NULL;

    /* Check to make sure the root isn't an only child. */
    if (s -> link[LEFT] == NULL) {
        if ((t -> Compare)(n, s) == 0) {
            /* deleting the root */
            p -> link[RIGHT] = NULL;
            return s;
        }
        else
            return NULL;
    }

    /* Now, either the root or one of its kids must be red */
    if (!s -> link[LEFT]  -> red &&
        !s -> link[RIGHT] -> red)
        s -> red = 1; /* Just color the root red */

    /*
     * Now, march down the tree, always working to make sure
     * the current node is red. That way, when we do arrive
     * at a leaf, its parent will be red, making the leaf
     * very easy to delete (just drop the leaf, and replace
     * its (red) parent with its (black) sib.)
     */
    for (;;) {
        /*
         * If we're at a leaf, we're done.
         */
        if (s -> link[LEFT] == NULL)
            break;

        /*
         * Where are we going next?
	 */
        next_dir = (t -> Compare) (n, s) < 0;

        /*
         * If the current node or the next node
         * is red, we can advance.
         */
        if (s -> red || s -> link[next_dir] -> red)
            ;

        /*
         * (If the current node is black)
         * (and the next node is black)
         * but the next node's sib is red ...
         *
         * Then rotate from parent towards the red child. This
         * will lower the current node, and give us a new
         * grandparent (the old parent) and a new
         * parent (the sib that was red). We the paint the
         * current node red and the new parent is painted black.
	 */
        else if (s -> link[!next_dir] -> red) {
            g = p;
            p = rotate(t, s -> link[next_dir], p, FLIP_GCH);
            s -> red = 1;
            p -> red = 0;
        }

        /*
         * (If the current node is black)
         * (and its left child is black)
         * (and its right child is black) ...
         *
         * then (a) the current node's parent must be red (we
         * never advance unless we are leaving a red node),
         * (b) its sib must be black (because the parent is red),
         * and (c) we need to color the current node red. To
         * make this possible, we color the current node red,
         * the parent black and then check for tree imbalances.
         * Two cases exist...
	 */
        else {
            Bnode *sib;
	    if (!p -> red)
		printf("Parent not red in case 2!\n");

	    sib = p -> link[!dir];
            if (sib -> red)
		printf("Sib not black in case 2!\n");
	    if (sib -> link[LEFT] == NULL) {
		printf("Sib has no kids in case 2!\n");
                return NULL;
            }

            s -> red = 1;
            p -> red = 0;

            /*
             * First case: black sib has two black kids. Just
             * color the sib red. In effect, we are reversing
             * a simple color flip.
	     */
            if (!sib -> link[LEFT]  -> red &&
                !sib -> link[RIGHT] -> red)
                sib -> red = 1;

            /*
             * Second case: black sib has at least one red kid.
             * (It makes no difference if both kids are red.)
             * We need to do either a single or double rotation
             * in order to re-balance the tree.
             */
            else {
                int redkid_dir;

                if (sib -> link[LEFT] -> red)
                    redkid_dir = LEFT;
                else
                    redkid_dir = RIGHT;

		if (!dir == redkid_dir)
		{
		    sib -> red = 1;
		    sib -> link[redkid_dir] -> red = 0;
		    g = rotate(t, n, g, FLIP_GCH);
		}
		else
		{
                    rotate(t, n, p, FLIP_CH + redkid_dir);
                    g = rotate(t, n, g, FLIP_GCH);
                }
            }
        }

        /* advance pointers */
        dir = next_dir;
        g = p;
        p = s;
        s = s -> link[dir];
    }

    /* Make the root black */
    t -> DummyHead -> link[RIGHT] -> red = 0;

    /* Delete it, if a match. Parent is red. */
    if ((t -> Compare)(s, n) == 0)
    {
	if (!p -> red && p != t -> DummyHead)
	    printf("Parent not red at delete!\n");

	g -> link[(t -> Compare)(s, g) < 0] =
            p -> link[(t -> Compare)(s, p) >= 0];
        free (p); /* release internal node that we created */
        return s;
    }
    else return NULL;
}

#elif defined(SPLAY) /* Splay tree version */

Bnode *DelBintree (Bintree *t, Bnode *n)
{
    Bnode *save, *t2;

    if (splay(t, n))
	save = NULL; /* match not found */
    else
    {
        save = t -> DummyHead -> link[RIGHT];
        t2 = save -> link[RIGHT];
	if (t -> DummyHead -> link[RIGHT] == save -> link[LEFT])
        {   /* '=' and not '==' is correct on previous line */
            splay(t, NULL);
            t -> DummyHead -> link[RIGHT] -> link[RIGHT] = t2;
        }
        else
            t -> DummyHead -> link[RIGHT] = t2;

    }
    return save;
}

#else /* Binary tree version */

Bnode *DelBintree (Bintree *t, Bnode *n)
{

    Bnode *p, *s, *save;
    int dir, dir_old;

    p = t -> DummyHead;
    s = p -> link[RIGHT];
    dir_old = dir = RIGHT;

    /* Look for a match */
    while (s != NULL && (dir = (t->Compare)(n, s)) != 0) {
        p = s;
        dir = dir < 0;
        dir_old = dir;
        s = p -> link[dir];
    }

    if (s == NULL)
        return NULL; /* no match found */

    save = s;
    /*
     * First case: if s has no right child, then replace s
     * with s's left child.
     */
    if (s -> link[RIGHT] == NULL)
        s = s -> link[LEFT];
    /*
     * Second case: if s has a right child that lacks a left
     * child, then replace s with s's right child and
     * copy s's left child into the right child's left child.
     */
    else if (s -> link[RIGHT] -> link[LEFT] == NULL) {
        s = s -> link[RIGHT];
        s -> link[LEFT] = save -> link[LEFT];
    }
    /*
     * Final case: find leftmost (smallest) node in s's right
     * subtree. By definition, this node has an empty left
     * link. Free this node by copying its right link to
     * its parent's left link and then give it both of s's
     * links (thus replacing s).
     */
    else {
        Bnode *small;

        small = s -> link[RIGHT];
        while (small -> link[LEFT] -> link[LEFT])
            small = small -> link[LEFT];
        s = small -> link[LEFT];
        small -> link[LEFT] = s -> link[RIGHT];
        s -> link[LEFT] = save -> link[LEFT];
        s -> link[RIGHT] = save -> link[RIGHT];
    }

    p -> link[dir_old] = s;

    RBONLY(s -> red = 0;)

    return save;
}
#endif

/* Insert node n into tree t */
int InsBintree (Bintree *t, Bnode *n)
{
    #if defined(REDBLACK)
    int p_dir;
    Bnode *p, *s;
    Bnode *g   = NULL;
    Bnode *gg  = NULL;

    /* Search until we find a leaf. */
    p = t -> DummyHead;
    p_dir = RIGHT; /* direction from p to s */
    s = p -> link[RIGHT];

    if (s) {
        Bnode *temp;
	int dir;

        /* Look for a leaf, splitting nodes on the way down */
        while (s -> link[RIGHT] != NULL) {
            if (s -> link[LEFT]  -> red &&
                s -> link[RIGHT] -> red)
                split(t, n, &s, &p, g, gg);
            gg = g;
            g  = p;
            p = s;
            p_dir = (t -> Compare) (n, s) < 0;
            s = s -> link[p_dir];
        }

        dir = (t -> Compare) (n, s);
        if (t -> DuplicatesOk == 0 && dir == 0)
            return TREE_FAIL; /* duplicate - not allowed */

        /*
         * Must replace s with a new internal node that has as
	 * its children s and n. The new node gets the larger of
         * s and n as its key. The new node gets painted red, its
         * children are black. Coloring is done by split().
         */
        temp = tmalloc(t -> NodeSize);
        dir = dir < 0;

	CopyDataB((LONG *)temp, dir ? (LONG *)s : (LONG *)n, t -> NodeSize);

	temp -> link[dir]  = n;
        temp -> link[!dir] = s;
        n = temp;
    }

    /* Add the new node */
    p -> link[p_dir] = n;

    /* Color this node red and check red-black balance */
    split(t, n, &n, &p, g, gg);
    return TREE_OK;

    #elif defined(SPLAY)
    int dir;
    Bnode *r;

    dir = splay(t, n);
    if (dir == 0 && t -> DuplicatesOk == 0)
	return TREE_FAIL;
    r = t -> DummyHead -> link[RIGHT];

    if (r == NULL) /* first node? */
        t -> DummyHead -> link[RIGHT] = n;
    else {
        dir = dir < 0;
        n -> link[dir] = r -> link[dir];
        r -> link[dir] = NULL;
        n -> link[!dir] = r;
        t -> DummyHead -> link[RIGHT] = n;
    }
    return TREE_OK;

    #else /* plain binary tree */
    int p_dir;
    Bnode *p, *s;

    /* Search until we find an empty arm. */
    p = t -> DummyHead;
    p_dir = RIGHT; /* direction from p to s */
    s = p -> link[RIGHT];

    while (s != NULL) {
        p = s;
        p_dir = (t -> Compare) (n, s);
        if (p_dir == 0 && t -> DuplicatesOk == 0)
            return TREE_FAIL; /* duplicate */
        p_dir = p_dir < 0;
        s = s -> link[p_dir];
    }

    /* Add the new node */
    p -> link[p_dir] = n;
    return TREE_OK;
    #endif
}

/*
 * Recursive tree walk routines. The entry point is
 * WalkBintree. It will do an inorder traversal of the
 * tree, call df() for each node and leaf.
 */
void rWalk(Bnode *n, int level, DoFunc df)
{
    if (n != NULL) {
        rWalk(n -> link[LEFT], level + 1, df);
        df(n, level);
        rWalk(n -> link[RIGHT], level + 1, df);
    }
}

int WalkBintree(Bintree *t, DoFunc df)
{

    if (t -> DummyHead -> link[RIGHT] == NULL)
    {
	printf("Empty tree\n");
	return TREE_FAIL;
    }
    rWalk(t -> DummyHead -> link[RIGHT], 0, df);
    return TREE_OK;
}

#if defined(TEST)
/*
 * Test driver
 */

#define BUFLEN 100

/* Our binary tree is made up of these */
typedef struct sMynode {
    /* A copy of the items in a Bnode */
    BINTREE_STUFF(sMynode);

    /*
     * Now for the user's part of the structure. We could put
     * anything here. For these routines, a simple text area.
     */
    char text[80];
} Mynode;

int LoadString(Bintree *t, char *string)
{
    Mynode *m;

    m = (Mynode *) InitBintreeNode(sizeof(Mynode));
    strncpy(m->text, string, sizeof(m->text));
    m->text[sizeof(m->text) - 1] = 0;

    return InsBintree(t, (Bnode *) m);
}

void FindString(Bintree *t, char *string)
{
    Mynode m, *r;
    strncpy(m.text, string, sizeof(m.text));
    m.text[sizeof(m.text) - 1] = 0;
    if ((r = (Mynode *) FindBintree(t, (Bnode *) &m)) == NULL)
	printf(" Not found.\n");
    else
        printf(" Found '%s'.\n", r -> text);
}

void DeleteString(Bintree *t, char *string)
{
    Mynode m, *n;
    strncpy(m.text, string, sizeof(m.text));
    m.text[sizeof(m.text) - 1] = 0;
    n = (Mynode *) DelBintree(t, (Bnode *) &m);
    if (n)
        free (n);
    else
	printf(" Did not find '%s'.\n", string);
}

void LoadTreeFile(Bintree *t, char *fname)
{
    FILE *infile;
    char buffer[BUFLEN], *s;
    int i = 0, j = 0;

    if ((infile = fopen(fname, "r")) == NULL)
    {
	printf(" Couldn't open the file.\n");
        return;
    }

    while (fgets(buffer, BUFLEN, infile))
    {
	s = buffer + strlen(buffer);

	while(iscntrl(*s))
	    *s-- = 0;

	if (buffer[0] == ';') /* a comment */
            ;
        else if (buffer[0] == '-' && buffer[1] != 0) {
	    DeleteString(t, buffer+1);
            j++;
        }
        else {
            LoadString(t, buffer);
            i++;
        }
    }

    fclose(infile);
    printf("Loaded %d items and deleted %d from %s.\n",
        i, j, fname);
}

void DeleteTreeFile(Bintree *t, char *fname)
{
    FILE *infile;
    char buffer[BUFLEN], *s;
    int j = 0;

    if ((infile = fopen(fname, "r")) == NULL)
    {
	printf(" Couldn't open the file.\n");
        return;
    }

    while (fgets(buffer, BUFLEN, infile))
    {
	s = buffer + strlen(buffer);

	while(iscntrl(*s))
            *s-- = 0;

	if (buffer[0] == ';') /* a comment */
            ;
        else if (buffer[0] == '-' && buffer[1] != 0) {
	    DeleteString(t, buffer+1);
            j++;
        }
        else {
	    DeleteString(t, buffer);
	    j++;
	}
    }

    fclose(infile);
    printf("deleted %d from %s.\n",
	   j, fname);
}

/*
 * A sample action function: it prints out the data
 * at each node along with the node's level in the tree
 */
int ShowFunc(void *m, int level)
{
    RBONLY(if (((Mynode *)m) -> link[LEFT] == NULL))
	printf("%s (%d)\n", ((Mynode *)m) -> text, level);

    return TREE_OK;
}


/*
 * A pair of functions to print the tree as a diagram.
 */

#if !defined(ALTDRAW)
  #define TOP 'Ú'
  #define BOT 'À'
  #define HOR 'Ä'
  #define VRT '³'
#else
  #define TOP '/'
  #define BOT '\\'
  #define HOR '-'
  #define VRT '|'
#endif

#if defined(REDBLACK)
  #if !defined(ALTDRAW)
    #define RTOP 'É'
    #define RBOT 'È'
    #define RHOR 'Í'
    #define RVRT 'º'
  #else
    #define RTOP '*'
    #define RBOT '*'
    #define RHOR '#'
    #define RVRT '#'
  #endif
#endif

#define DRAWBUF 100
char draw[DRAWBUF];
char work[DRAWBUF * 2];
int maxdepth;
RBONLY(int blackheight;)
RBONLY(int maxblack;)
FILE *outfile;

void xrWalk(Bnode *n, int level)
{
    int i;

    if (n != NULL) {
        /* Monitor */
        if (level > maxdepth)
            maxdepth = level;
        RBONLY(if (!n -> red) blackheight++;)

        /*
         * Go right
         */
        draw[level * 2] = TOP;
        #if defined(REDBLACK)
        if (n -> link[RIGHT] && n -> link[RIGHT] -> red)
            draw[level * 2] = RTOP;
        #endif
        draw[level * 2 + 1] = ' ';
        xrWalk(n -> link[RIGHT], level + 1);

        /*
         * Show current node
         */
        strncpy(work, draw, level * 2);
        if (level > 0) {
            int c;

            c = work[0];
	    for (i = 2; i < level * 2; i += 2)
                #if !defined(REDBLACK)
                if (work[i] == c)
                #else
                if (((c == TOP || c == RTOP) &&
                    (work[i] == TOP || work[i] == RTOP)) ||
                    ((c == BOT || c == RBOT) &&
                    (work[i] == BOT || work[i] == RBOT)))
                #endif
                    work[i - 2] = ' ';
                else
                    c = work[i];

            work[level * 2 - 1] =
                RBONLY(((Mynode *)n) -> red ? RHOR :)
                HOR;

            for (i = 0; i < level * 2 - 2; i += 2)
                if (work[i] != ' ') {
                    #if !defined(REDBLACK)
		    work[i] = VRT;
                    #else
                    if (work[i] == TOP || work[i] == BOT)
                        work[i] = VRT;
                    else
                        work[i] = RVRT;
                    #endif
                }
        }

        sprintf(work + level * 2, "%s (%d)",
                            ((Mynode *)n)->text, level);
	printf(work, outfile);

        #if defined(REDBLACK)
        if (n -> link[LEFT] == NULL) { /* leaf */
            if (maxblack < 0)
                maxblack = blackheight;
            else if (maxblack != blackheight)

		printf("  Leaf has black height %d!",
                blackheight - 1);
        }
        #endif
	printf("\n");

        /*
         * Go left
         */
        draw[level * 2] = BOT;
        #if defined(REDBLACK)
        if (n -> link[LEFT] && n -> link[LEFT] -> red)
            draw[level * 2] = RBOT;
        #endif
        draw[level * 2 + 1] = ' ';
        xrWalk(n -> link[LEFT], level + 1);

        RBONLY(if (!n -> red) blackheight--;)
    }
}

int xWalkBintree(Bintree *t, char *name, char *mode)
{
    if (t -> DummyHead -> link[RIGHT] == NULL)
    {
	printf("Empty tree\n");
        return TREE_FAIL;
    }

    maxdepth = -1;
    RBONLY(blackheight = 0;)
    RBONLY(maxblack = -1;)

    if (name)
    {
	outfile = fopen(name, mode);
	if (outfile == NULL)
	{
	    printf("Can't open %s.\n", name);
	    name = NULL;
	}
    }

    xrWalk(t -> DummyHead -> link[RIGHT], 0);
    #if defined(REDBLACK)
    printf("Max depth %d, black height %d.\n",
        maxdepth, maxblack - 1);
    #else
    printf("Max depth %d.\n", maxdepth);
    #endif

    if (name)
        fclose(outfile); /* a real file */

    return TREE_OK;
}

int compare_length = 0;
int CompareFunc(void *n1, void *n2)
{
    if (compare_length)
	return strncmp(((Mynode *)n1)->text,((Mynode *)n2)->text, compare_length);
    else
	return strcmp(((Mynode *)n1)->text,((Mynode *)n2)->text);
}

void bintree(SCREEN *screen)
{

    char inbuf[BUFLEN], *s;
    Bintree *tree;
    Mynode *dummy;
    FILE *logfile = NULL;


    #if defined(REDBLACK)
    printf("\nRed-black binary tree test driver.\n"
        "  Lines to red nodes are drawn with %c%c%c lines and\n"
        "  lines to black nodes are drawn with %c%c%c lines.\n",
	   RHOR, RHOR, RHOR, HOR, HOR, HOR);
    #elif defined(SPLAY)
    printf("\nSplay tree test driver.\n");
    #else
    printf("\nPlain binary tree test driver.\n");
    #endif

    /* create a dummy node for the tree algorithms */
    dummy = (Mynode *) InitBintreeNode(sizeof(Mynode));
    dummy->text[0] = 0; /* must contain valid data */

    /* create a tree */
    tree = NewBintree((Bnode *) dummy,
                        CompareFunc, 1, sizeof(Mynode));

    for (;;)
    {
	printf("*  ");
	ScreenInputFromKeyboard(screen, inbuf, 0, 80, 7);
	printf("\n");

	s = inbuf + strlen(inbuf);
	while(*s && iscntrl(*s))
	    *s-- = 0;

	if (logfile)
	    printf("%s\n", inbuf);

	switch (inbuf[0])
	{
	    case '?':
		printf(
		    "\n@file     - Load strings in file to tree\n"
                    "a string  - Add string to tree\n"
                    "c nn      - Compare only first nn chars\n"
                    "d string  - Delete string from tree\n"
                    "dup [0|1] - Disallow/allow duplicates\n"
                    "f string  - Find string in tree\n"
                    "l file    - Log actions to file\n"
                    "l         - Turn off action logging\n"
                    "s [file]  - Display tree (overwrite file)\n"
                    "S [file]  - Display tree (append to file)\n"
                    "w         - Walk tree, running ShowFunc()\n"
                    "q         - Quit\n"
		    );
		break;

            case '@':
		LoadTreeFile(tree, inbuf + 1);
                break;

	    case '#':
		DeleteTreeFile(tree, inbuf + 1);
                break;

	    case 'a':
                if (inbuf[1] != ' ' || inbuf[2] == 0)
		    printf("Not a valid command\n");
                else
                    if (LoadString(tree, inbuf + 2)==TREE_FAIL)
			printf(" ** Insertion failed\n");
		break;

	    case 'c':
		if (inbuf[1] == ' ' && inbuf[2] != 0)
		{
		    compare_length = atol(inbuf+2);
		    if (compare_length < 0)
			compare_length  = 0;
		}
		if (compare_length)
		    printf("Comparing first %d chars.\n",
		    compare_length);
		else
		    printf("Comparing entire text.\n");
		break;

	    case 'd':
                if (inbuf[1] == 'u' && inbuf[2] == 'p') {
                    if (inbuf[3] == ' ' &&
                        (inbuf[4] == '0' || inbuf[4] == '1'))
                        tree -> DuplicatesOk =
                        inbuf[4] == '0' ? 0 : 1;
		    printf("Duplicates are ");
		    if (tree -> DuplicatesOk == 0)
			printf("not ");
		    printf("allowed.\n");
		    break;
		}

		if (inbuf[1] != ' ' || inbuf[2] == 0)
		    printf("Not a valid command\n");

		else
                    DeleteString(tree, inbuf + 2);
                break;

            case 'f':
                if (inbuf[1] != ' ' || inbuf[2] == 0)
		    printf("Not a valid command\n");
                else
                    FindString(tree, inbuf + 2);
                break;

            case 'l':
                if (inbuf[1] != ' ' || inbuf[2] == 0) {
                    if (logfile) {
                        fclose(logfile);
                        logfile = NULL;
                    }
                    else
			printf(" Logfile not open\n");
                }
                else {
                    logfile = fopen(inbuf + 2, "w");
                    if (logfile == NULL)
                        printf("Can't open %s\n", inbuf + 2);
                }
                break;

            case 's': case 'S':
                if (inbuf[1] == ' ' && inbuf[2] != 0)
                    xWalkBintree(tree, inbuf + 2,
                    inbuf[0] == 's' ? "w" : "a");
                else
                    xWalkBintree(tree, NULL, NULL);
                break;

            case 'w':
                WalkBintree(tree, ShowFunc);
                break;

            case 'q':
                if (logfile)
                    fclose(logfile);
                return;

            case ';':
                break;  /* comment */

            default:
		break;
        }
    }
}
#endif
