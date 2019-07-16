

#define TREE_OK    (0)
#define TREE_FAIL (-1)
#define LEFT        1
#define RIGHT       0

#if defined(REDBLACK)
    #define RBONLY(x) x
#else
    #define RBONLY(x)
#endif


/*
 * Basic node structure. The actual size of a node is unknown as
 * the user will have appended data bytes on to the end of
 * this structure. The BINTREE_STUFF macro is a convenient way
 * to summarize the items the tree algorithm requires in the
 * node. Its argument is the tag of the structure being defined.
 */

#define BINTREE_STUFF(x)    struct x *link[2] \
			    RBONLY(;int red)
typedef struct sBnode
{
    BINTREE_STUFF(sBnode);
} Bnode;

/* Control structure for a binary tree */

typedef int (*CompFunc) (void *node1, void *node2);
typedef int (*DoFunc) (void *node, int level);

typedef struct sBintree
{
    Bnode *DummyHead;
    CompFunc Compare;
    int DuplicatesOk;
    int NodeSize;
} Bintree;

/* Prototypes */
Bintree *NewBintree (Bnode *dummy, CompFunc cf, int dup_ok, int node_size);
Bnode *FindBintree(Bintree *t, Bnode *n);
int InsBintree (Bintree *t, Bnode *n);
Bnode *DelBintree (Bintree *t, Bnode *n);
int WalkBintree(Bintree *t, DoFunc df);
Bnode *InitBintreeNode(int size);
