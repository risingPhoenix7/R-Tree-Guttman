#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <errno.h>

struct BoundDefiner // defines limits for each dimension
{
    int dmin;
    int dmax;
};

int max(int x, int y) // returns greater of x or y
{
    return (x > y) ? x : y;
}

int min(int x, int y) // returns lesser of x or y
{
    return (x < y) ? x : y;
}

typedef struct BoundDefiner *Bounds;
struct Node // represents a node in a tree data structure.
{
    int num_of_children_or_tuples;      // if leaf node, then num of tuples, else num of children.
    struct BoundDefiner *bounddefiners; // a pointer to a BoundDefiner struct that defines the bounds for the node.
    struct Node *parent;                // pointer to the parent node of the current node, If the current node is the root of the tree, this member will be set to NULL.
    struct Node **child_nodes;          // pointer to an array of pointers to child nodes. If the current node is a leaf node, this member will be set to NULL.
    int **list_of_tuples;               // pointer to an array of integer arrays that represent tuples stored in the leaf node. If the current node is not a leaf node, this member will be set to NULL.
};

struct Rtree // struct definition for an R-Tree data structure.
{
    int max_entries;     // determine the maximum number of entries that can be stored in a node of the R-Tree.
    int min_entries;     // determine the minimum number of entries that can be stored in a node of the R-Tree.
    struct Node *root;   // pointer to the root node of the R-Tree.
    int numofdimensions; // specifies the number of dimensions of the spatial data being indexed.
};

struct Node *nodeSplit(struct Rtree *rtree, struct Node *parent_node, struct Node *child_node);   // used to split a non-leaf node of the R-Tree when it becomes too full.
struct Node *adjust_tree(struct Rtree *rtree, struct Node *node1, struct Node *node2);            // used to adjust the R-Tree after a split has occurred.
struct Node *nodeSplit_leaf(struct Rtree *rtree, struct Node *leaf_node, int *tuple);             // similar to nodeSplit, but is used to split a leaf node of the R-Tree.
void addChildNode2Parent(struct Node *parent_node, struct Node *child_node, int numofdimensions); // used to add a child node to a parent node in the R-Tree.

struct Node *new_node(int numofdimensions) // creates a new node for an R-Tree data structure.
{
    struct Node *node = malloc(sizeof(struct Node));                             // allocates memory for a new struct Node object using the malloc function.
    node->num_of_children_or_tuples = 0;                                         // sets the initial number of children (or tuples) in the new node to 0.
    node->bounddefiners = malloc(sizeof(struct BoundDefiner) * numofdimensions); // allocates memory for an array of struct BoundDefiner objects that will be used to define the bounds of the new node in each dimension of the space being indexed.
    for (int i = 0; i < numofdimensions; i++)
    {
        node->bounddefiners[i].dmax = INT_MIN;
        node->bounddefiners[i].dmin = INT_MAX;
    }
    node->child_nodes = NULL;    // sets the initial pointer to the child nodes of the new node to NULL.
    node->parent = NULL;         // sets the initial pointer to the parent node of the new node to NULL.
    node->list_of_tuples = NULL; // sets the initial pointer to the list of tuples in the new node to NULL.
    return node;                 // returns a pointer to the new node.
}

struct Rtree *new_rtree(int max_entries, int min_entries, int numofdimensions) // creates a new R-Tree data structure.
{
    struct Rtree *rtree = malloc(sizeof(struct Rtree)); // allocates memory for a new struct Rtree object using the malloc function.
    rtree->max_entries = max_entries;                   // sets the maximum number of entries that a node in the R-Tree can hold to the value passed as an argument.
    rtree->min_entries = min_entries;                   // sets the minimum number of entries that a node in the R-Tree must have before it can be split to the value passed as an argument.
    rtree->numofdimensions = numofdimensions;           // sets the number of dimensions in the space being indexed by the R-Tree to the value passed as an argument.
    rtree->root = NULL;                                 // sets the initial pointer to the root node of the R-Tree to NULL.
    return rtree;                                       // returns a pointer to the new R-Tree.
}

long int getArea(int numofdimensions, Bounds bounddefiners) // calculates the area of an MBR (minimum bounding rectangle) in a multi-dimensional space.
{
    long int area = 1; // This variable will accumulate the area of the MBR as we calculate it.
    for (int i = 0; i < numofdimensions; i++)
    {
        area *= (bounddefiners[i].dmax - bounddefiners[i].dmin); // multiplies the current value of area by the size of the MBR in the current dimension
    }
    return area; // returns the final calculated area of the MBR.
}

Bounds get_bounding_box(int numofdimensions, Bounds bounddefiners, Bounds bounddefiners2) // calculates the bounding box (MBR) that contains two given MBRs in a multi-dimensional space.
{
    Bounds bounding_box = malloc(sizeof(struct BoundDefiner) * numofdimensions); // creates an array bounding_box of BoundDefiner structs to hold the boundaries of the resulting bounding box.
    for (int i = 0; i < numofdimensions; i++)
    {
        bounding_box[i].dmin = min(bounddefiners[i].dmin, bounddefiners2[i].dmin); // sets the minimum boundary of the resulting bounding box for the current dimension to the minimum of the corresponding boundaries of the two input MBRs.
        bounding_box[i].dmax = max(bounddefiners[i].dmax, bounddefiners2[i].dmax); // sets the maximum boundary of the resulting bounding box for the current dimension to the maximum of the corresponding boundaries of the two input MBRs.
    }
    return bounding_box; // returns the final calculated bounding box.
};

int intersects(int numofdimensions, Bounds bounddefiners, Bounds bounddefiners2) //  calculate the area increased from the second MBR if the first MBR intersects with it, which can be used to determine which MBRs to choose for a given search or insertion operation.
{

    long int area = 1; // used to calculate the area of the overlapping region of the two MBRs.
    for (int i = 0; i < numofdimensions; i++)
    {
        if (bounddefiners[i].dmin > bounddefiners2[i].dmax || bounddefiners[i].dmax < bounddefiners2[i].dmin) // checks if the boundaries of the two MBRs overlap in the current dimension
        {
            return -1;
        }
        area *= (max(bounddefiners[i].dmax, bounddefiners2[i].dmax) - min(bounddefiners[i].dmin, bounddefiners2[i].dmin)); // updates the area variable to be the product of the overlapping region of the two MBRs in the current dimension.
    }
    return area - getArea(numofdimensions, bounddefiners2); // return area increased from r2 if r1 intersects
}

bool is_leaf(struct Node *n) // checks whether the node is a leaf node or not.
{
    return (n->list_of_tuples != NULL && n->child_nodes == NULL); // checks if the node has a list of tuples but no child nodes.
}

struct searchResultStruct // used to store the result of a search operation in the R-tree.
{
    int **list_of_tuples; // pointer to an array of integer arrays, where each integer array represents a tuple matching the search criteria.
    int num_of_tuples;    // The number of tuples found in search is stored in num_of_tuples.
};

typedef struct searchResultStruct *searchResult;

searchResult createSearchResult(int **list_of_tuples, int num_of_tuples) // used to create a data structure to store the results of a spatial query.
{
    searchResult result = malloc(sizeof(struct searchResultStruct)); // allocates memory for the searchResult structure
    result->list_of_tuples = list_of_tuples;
    result->num_of_tuples = num_of_tuples;
    return result;
}

int checkIfTupleInBounds(Bounds bounddefiners, int *tuple, int numofdimensions) // checks if a given tuple lies within the bounds defined by a set of BoundDefiner structures.
{
    for (int i = 0; i < numofdimensions; i++)
    {
        if (tuple[i] < bounddefiners[i].dmin || tuple[i] > bounddefiners[i].dmax) // checks If the tuple lies outside of the bounding box in any dimension, then function returns 0,otherwise returns 1.
        {
            return 0;
        }
    }
    return 1;
}

searchResult searchTuplesInGivenBounds(int numofdimensions, Bounds bounddefiners, struct Node *node) // used to search for tuples that fall within a given bounding box.
{
    if (intersects(numofdimensions, bounddefiners, node->bounddefiners) == -1) // checks if the given bounding box intersects with the bounding box of the current node.
    {
        return NULL;
    }
    else
    {
        if (is_leaf(node)) // checks if the current node is a leaf node or an internal node.
        {
            int **list_of_tuples = NULL;
            int numofresults = 0;
            for (int i = 0; i < node->num_of_children_or_tuples; i++) // iterates over each tuple in the node and checks if it falls within the given bounds.
            {
                if (checkIfTupleInBounds(bounddefiners, node->list_of_tuples[i], numofdimensions))
                {
                    list_of_tuples = realloc(list_of_tuples, sizeof(int *) * (numofresults + 1));
                    list_of_tuples[numofresults] = node->list_of_tuples[i]; //  tuple is added to the list_of_tuples array
                    numofresults++;
                }
            }
            return createSearchResult(list_of_tuples, numofresults);
        }
        else
        {
            int **list_of_tuples = NULL;
            int numofresults = 0;
            for (int i = 0; i < node->num_of_children_or_tuples; i++)
            {
                searchResult searchresultfromchild = searchTuplesInGivenBounds(numofdimensions, bounddefiners, node->child_nodes[i]);
                if (searchresultfromchild != NULL)
                {
                    list_of_tuples = realloc(list_of_tuples, sizeof(int *) * (numofresults + searchresultfromchild->num_of_tuples));
                    for (int j = 0; j < searchresultfromchild->num_of_tuples; j++)
                    {
                        list_of_tuples[numofresults] = searchresultfromchild->list_of_tuples[j];
                        numofresults++;
                    }
                }
            }
            return createSearchResult(list_of_tuples, numofresults); // creating a data structure to store the tuples in list_of_tuples.
        }
    }
}

void printTuple(int *tuple, int numofdimensions)
{
    printf("(");                              // Print a left parenthesis
    for (int i = 0; i < numofdimensions; i++) // Loop through each dimension in the tuple
    {
        printf("%d", tuple[i]);       // Print the current element
        if (i != numofdimensions - 1) // Checks if this is not the last dimenstion in the tuple
        {
            printf(", "); // Print a comma and a space to separate the elements
        }
    }
    printf(")"); // Print a right parenthesis
}

void searchRTree(Bounds bounddefiners, struct Rtree *rtree) // searches for tuples within the bounding box by calling the searchTuplesInGivenBounds.
{
    if (rtree->root == NULL)
    {
        printf("Tree empty\n");
        return;
    }
    searchResult result = searchTuplesInGivenBounds(rtree->numofdimensions, bounddefiners, rtree->root);
    if (result == NULL)
    {
        printf("No tuples found in given bounds\n");
    }
    else
    {
        printf("%d Tuple(s) found in given bounds :\n", result->num_of_tuples);
        for (int i = 0; i < result->num_of_tuples; i++)
        {
            printTuple(result->list_of_tuples[i], rtree->numofdimensions);
            printf("\n");
        }
    }
}

void printInternalNodeFromBounds(Bounds bounddefiners, int numofdimensions) // used to print the bounds of internal nodes i.e print all min() and max() values
{
    int *min = malloc(sizeof(int) * numofdimensions); // allocates memory for min array min ,of size numofdimensions, to store the minimum values for each dimension.
    int *max = malloc(sizeof(int) * numofdimensions); // allocates memory for max array max ,of size numofdimensions, to store the maximum values for each dimension.
    for (int i = 0; i < numofdimensions; i++)         // loops through each dimension and assigns the minimum and maximum values of that dimension to the min and max arrays respectively.
    {
        min[i] = bounddefiners[i].dmin;
        max[i] = bounddefiners[i].dmax;
    }
    printTuple(min, numofdimensions); // prints the min array
    printf(" ");
    printTuple(max, numofdimensions); // prints the max array
    printf("\n");
}

void printNode(struct Node *node, int numofdimensions, int depth) // prints the contents of a given node
{
    for (int i = 0; i < depth; i++)
    {
        printf(" ");
    }
    printf("[%d] ", depth);
    if (is_leaf(node))
    {
        printf("Leaf Node: ");
        for (int i = 0; i < node->num_of_children_or_tuples; i++)
        {
            printTuple(node->list_of_tuples[i], numofdimensions);
            printf(" ");
        }
        printf("\n");
    }
    else
    {
        printf("Internal Node with %d children. Bounds: ", node->num_of_children_or_tuples);
        printInternalNodeFromBounds(node->bounddefiners, numofdimensions);
        for (int i = 0; i < node->num_of_children_or_tuples; i++)
        {
            printNode(node->child_nodes[i], numofdimensions, depth + 1);
        }
    }
}

void printRtree(struct Rtree *rtree) // prints the contents of the R-tree
{
    if (rtree->root == NULL)
    {
        printf("Tree empty\n");
        return;
    }
    printNode(rtree->root, rtree->numofdimensions, 0);
}

long int getAreaEnlargedOnInclusion(int numofdimensions, Bounds bounddefiners, int *tuple) // calculates the area enlarged on inclusion of a new tuple within the given bounding box
// new method for area enlarged on inclusion
{
    long int orig_area = getArea(numofdimensions, bounddefiners); // original area of the bounding box is calculated using the getArea function.
    long int new_area = 1;

    for (int i = 0; i < numofdimensions; i++)
    {
        if (tuple[i] < bounddefiners[i].dmin)
            new_area *= (bounddefiners[i].dmax - tuple[i]);
        else if (tuple[i] > bounddefiners[i].dmax)
            new_area *= (tuple[i] - bounddefiners[i].dmin);
        // If the new tuple lies outside the existing bounding box in a dimension, the area added is calculated as the difference between the new tuple's value and the corresponding boundary value.
        else
            new_area *= bounddefiners[i].dmax - bounddefiners[i].dmin;
        //  If the new tuple lies inside the existing bounding box in a dimension, the area added is calculated as the width of the dimension of the bounding box that the new tuple falls within.
    }
    return new_area - orig_area; // returns area enlarged
}

struct Node *chooseLeaf(int *tuple, struct Node *node, int numofdimensions) // selects a leaf node in which the given tuple should be inserted.
{
    if (is_leaf(node)) // if the current node is a leaf node, then it is returned as the selected leaf node.
    {
        return node;
    }
    else
    {
        long int minarea = LONG_MAX;
        int minareaindex = -1;
        long int minareaenlargedoninclusion = LONG_MAX;
        for (int i = 0; i < node->num_of_children_or_tuples; i++) // for each child node of the current node, calculates the area enlarged on inclusion if the tuple is added to the child node's bounds.
        {
            long int areaenlargedoninclusion = getAreaEnlargedOnInclusion(numofdimensions, node->child_nodes[i]->bounddefiners, tuple);
            if (areaenlargedoninclusion < minareaenlargedoninclusion) // selects the child node that results in the minimum area enlargement on inclusion.
            {
                minareaenlargedoninclusion = areaenlargedoninclusion;
                minareaindex = i;
            }
            else if (areaenlargedoninclusion == minareaenlargedoninclusion) // If there is a tie, then selects the child node with the minimum area.
            {
                long int area = getArea(numofdimensions, node->child_nodes[i]->bounddefiners);
                if (area < minarea)
                {
                    minarea = area;
                    minareaindex = i;
                }
            }
            // return chooseLeaf(tuple, node->child_nodes[minareaindex], numofdimensions); //changed to outer loop
        }
        return chooseLeaf(tuple, node->child_nodes[minareaindex], numofdimensions); // calls itself recursively with the selected child node as the new current node.
    }
}

struct Node *createLeafNode(int numofdimensions, int *tuple) // creates a new leaf node for an R-tree
{
    struct Node *leaf = malloc(sizeof(struct Node));                // allocates memory for the new leaf node using the malloc.
    leaf->num_of_children_or_tuples = 1;                            // sets the num_of_children_or_tuples field of the node to 1 since it is a leaf node and has only one tuple.
    leaf->list_of_tuples = malloc(sizeof(int *));                   // allocates memory for a list of tuples using malloc.
    leaf->list_of_tuples[0] = tuple;                                // sets the first tuple in the list to tuple.
    leaf->bounddefiners = malloc(sizeof(Bounds) * numofdimensions); // allocates memory for the bounddefiners field, which is an array of Bounds structures that represent the bounding box of the leaf node.
    leaf->parent = NULL;                                            // sets the parent field of the new leaf node to NULL since it has no parent yet,
    // leaf->bounddefiners = malloc(sizeof(BoundDefiner) * numofdimensions); //must be this ?
    for (int i = 0; i < numofdimensions; i++)
    { // For each dimension, sets the dmin and dmax fields of the Bounds structure to the corresponding value in tuple, since this is the only tuple in the leaf node.
        leaf->bounddefiners[i].dmin = tuple[i];
        leaf->bounddefiners[i].dmax = tuple[i];
    }
    return leaf; // returns the new leaf node.
}

struct Node *addTupleToLeafNode(int numofdimensions, int *tuple, struct Node *node) // adds a new tuple to an existing leaf node
{
    node->num_of_children_or_tuples++;
    node->list_of_tuples = realloc(node->list_of_tuples, sizeof(int *) * (node->num_of_children_or_tuples)); // resizes the list_of_tuples array of the node using realloc() to accommodate the new tuple.
    node->list_of_tuples[node->num_of_children_or_tuples - 1] = tuple;                                       // stores the tuple in the last position of the array.
    for (int i = 0; i < numofdimensions; i++)                                                                // updates the bounddefiners of the node by checking each dimension of the new tuple against the current bounds of the node.
    {
        if (tuple[i] < node->bounddefiners[i].dmin)
        {
            node->bounddefiners[i].dmin = tuple[i];
        }
        if (tuple[i] > node->bounddefiners[i].dmax)
        {
            node->bounddefiners[i].dmax = tuple[i];
        }
    }
    return node; // returns a pointer to the modified leaf node.
}

void adjustRootMBR(struct Rtree *rtree) // adjusts the MBR of the root node of an R-tree and typically called after a new level of nodes has been added to the R-tree, to update the MBR of the root to reflect the new structure of the tree.
{
    Bounds bb = rtree->root->child_nodes[0]->bounddefiners;          // initializes the MBR as the bounding box of the first child node of the root
    for (int i = 1; i < rtree->root->num_of_children_or_tuples; i++) // iterates over the remaining child nodes to expand the MBR to include all of them.
    {
        bb = get_bounding_box(rtree->numofdimensions, rtree->root->child_nodes[i]->bounddefiners, bb); // uses the get_bounding_box function to calculate the bounding box of multiple nodes.
    }
    rtree->root->bounddefiners = bb; // sets the root's bounddefiners field to the new MBR.
}

void insert(struct Rtree *rtree, int *tuple) // used to insert a new tuple into the R-tree.
{
    if (rtree->root == NULL) // If the tree is empty, create a new root
    {
        rtree->root = new_node(rtree->numofdimensions);
        addTupleToLeafNode(rtree->numofdimensions, tuple, rtree->root);
    }
    else // If the tree is not empty, traverse it to find the appropriate leaf node to insert the tuple
    {
        struct Node *leaf_node = chooseLeaf(tuple, rtree->root, rtree->numofdimensions);
        struct Node *split_leaf_node = NULL;

        // Add the tuple to the leaf node
        if (leaf_node->num_of_children_or_tuples < rtree->max_entries) // Checks If there is space in the leaf node
        {
            leaf_node = addTupleToLeafNode(rtree->numofdimensions, tuple, leaf_node);
        }
        else // If there is no space in the leaf node, split it
        {
            split_leaf_node = nodeSplit_leaf(rtree, leaf_node, tuple);
        }

        struct Node *split_root = adjust_tree(rtree, leaf_node, split_leaf_node); // After the leaf node is split, adjust_tree is called to adjust the tree and propagate the changes up to the root node.

        if (split_root != NULL) // If the root node is split, a new root node is created and the two split nodes are added as children
        {
            struct Node *new_root = new_node(rtree->numofdimensions);
            addChildNode2Parent(new_root, rtree->root, rtree->numofdimensions);
            addChildNode2Parent(new_root, split_root, rtree->numofdimensions);
            rtree->root = new_root;
        }
        else if (!is_leaf(rtree->root)) // If the root node did not split, but it is not a leaf node, the function adjustRootMBR is called to adjust the MBR of the root node.
        {
            adjustRootMBR(rtree);
        }
    }
}

void adjustNodeMBR(struct Node *node1, int numofdimensions) // adjusts the MBR (minimum bounding rectangle) of a given node.
{
    if (!is_leaf(node1)) // checks whether the given node is a leaf node or not.
    {
        Bounds bb1 = node1->child_nodes[0]->bounddefiners;
        for (int i = 1; i < node1->num_of_children_or_tuples; i++) // iterates through all the child nodes of the given node to compute the bounding box of all the child nodes.
        {
            bb1 = get_bounding_box(numofdimensions, bb1, node1->child_nodes[i]->bounddefiners);
        }
        node1->bounddefiners = bb1;
    }
    else
    {
        struct Node *dummyNode = createLeafNode(numofdimensions, node1->list_of_tuples[0]); // creates a dummy leaf node with the first tuple of the node and computes the bounding box.
        Bounds bb2 = dummyNode->bounddefiners;
        free(dummyNode);
        for (int i = 1; i < node1->num_of_children_or_tuples; i++) // iterates through all the tuples of the node and computes the bounding box of all the tuples.
        {
            struct Node *dummyNode2 = createLeafNode(numofdimensions, node1->list_of_tuples[i]);
            bb2 = get_bounding_box(numofdimensions, dummyNode2->bounddefiners, bb2);
            free(dummyNode2);
        }
        node1->bounddefiners = bb2; // updates the MBR of the given node with the computed bounding box.
    }
}

struct Node *adjust_tree(struct Rtree *rtree, struct Node *node1, struct Node *node2)
{
    // Step 1 : if N == root
    if (node1->parent == NULL)
        return node2;

    // Step 2: Changing MBR of node1 if node not split
    if (node2 == NULL)
        adjustNodeMBR(node1, rtree->numofdimensions);

    struct Node *parent_split = NULL;
    struct Node *parent_node = node1->parent;
    // Step 3: Propogate Node Split
    if (node1->parent->num_of_children_or_tuples < rtree->max_entries && node2 != NULL)
    {
        // to add node to parent node's list of child nodes
        addChildNode2Parent(node1->parent, node2, rtree->numofdimensions);
    }
    else if (node1->parent->num_of_children_or_tuples == rtree->max_entries && node2 != NULL)
    { // to split node
        parent_split = nodeSplit(rtree, node1->parent, node2);
    }

    // Step 4: Move upto next level
    return adjust_tree(rtree, parent_node, parent_split);
}

// NOTES for nodeSplit
// 1. struct Node * nodeSplit(int numofdimensions, struct Node *parent_node, struct Node excess_child_node)
// 2. All nodes after nodeSplit must have MBR adjusted !! (Very Imp)
// 3. Use Quadratic Splitting for PickNext

struct SplitArray // used to represent the result of a node split operation
{
    struct Node **array_of_child_nodes; // pointer to an array of pointers to Node structures, representing the child nodes of the split nodes.
    int numofchildnodes;                // represents the number of child nodes in the array.
};

struct SplitArray *newSplitArray(struct Node *parent_node, struct Node *child_node)
{
    struct SplitArray *splitArray = malloc(sizeof(struct SplitArray));                                               // allocates memory for a new SplitArray struct using malloc().
    splitArray->array_of_child_nodes = malloc(sizeof(struct Node *) * (parent_node->num_of_children_or_tuples + 1)); // allocates memory for an array of Node pointers inside the SplitArray struct, with a size equal to the number of children of the parent node plus one. This is because we're splitting the parent node and adding a new child node, so the number of children will increase by one.
    splitArray->array_of_child_nodes[0] = child_node;                                                                // assigns the child node to the first element of the array of child nodes.
    splitArray->numofchildnodes = parent_node->num_of_children_or_tuples + 1;                                        // updates number of child nodes.
    for (int i = 0; i < parent_node->num_of_children_or_tuples; i++)                                                 // loops through the remaining children of the parent node and copies their pointers to the array_of_child_nodes array of the SplitArray struct, shifting them over by one index to make room for the new child node.
        splitArray->array_of_child_nodes[i + 1] = parent_node->child_nodes[i];

    return splitArray; // returns a pointer to the newly created SplitArray struct.
}

void removeNodeFromSplitArray(struct SplitArray *splitArray, int ind) // removes a node from a SplitArray at the specified index ind
{
    for (int j = ind; j < splitArray->numofchildnodes - 1; j++) // start a loop that goes through all the child nodes in the SplitArray after the specified index ind (including ind). It starts at ind and ends at numofchildnodes - 2 because the last child node will be removed in the next line.
    {
        splitArray->array_of_child_nodes[j] = splitArray->array_of_child_nodes[j + 1]; // shifts each child node one position to the left, effectively overwriting the node at index ind.
    }
    splitArray->array_of_child_nodes[splitArray->numofchildnodes - 1] = NULL; // sets the last child node in the SplitArray to NULL, effectively removing it from the array.
    splitArray->numofchildnodes--;                                            // updating number of child nodes
}

struct Node **pickSeed(int numofdimensions, struct SplitArray *splitArray) // used in the process of selecting two nodes as seeds for splitting a node
{
    long int max_area_inclusion_difference = LONG_MIN;
    int index1, index2;
    for (int i = 0; i < splitArray->numofchildnodes; i++) // Loop through all the child nodes in the split array.
    {
        for (int j = i; j < splitArray->numofchildnodes; j++) //  Loop through all the child nodes in the split array again, but starting from the current value of i.
        {
            // Calculate the difference in area of the bounding box that would be created if the two child nodes at indices i and j were merged and the areas of their original bounding boxes. This difference represents the maximum amount by which the new bounding box would be expanded if these two nodes were chosen as seeds.
            long int temp_area_difference = getArea(numofdimensions, get_bounding_box(numofdimensions, splitArray->array_of_child_nodes[i]->bounddefiners, splitArray->array_of_child_nodes[j]->bounddefiners)) - getArea(numofdimensions, splitArray->array_of_child_nodes[i]->bounddefiners) - getArea(numofdimensions, splitArray->array_of_child_nodes[j]->bounddefiners);
            if (temp_area_difference > max_area_inclusion_difference)
            {
                max_area_inclusion_difference = temp_area_difference;
                index1 = i;
                index2 = j;
            }
        }
    }
    struct Node **ans = malloc(sizeof(struct Node *) * 2);

    ans[0] = splitArray->array_of_child_nodes[index1];         // Stores the child node at index index1 in the ans array at 0th position.
    ans[1] = splitArray->array_of_child_nodes[index2];         // Store the child node at index index2 in the ans array at 1st position.
    removeNodeFromSplitArray(splitArray, max(index1, index2)); // Remove the child node with the higher index among index1 and index2 from the split array.
    removeNodeFromSplitArray(splitArray, min(index1, index2)); // Remove the child node with the lower index among index1 and index2 from the split array.
    return ans;                                                // Return the array ans containing the two selected nodes as seeds.
}

void addChildNode2Parent(struct Node *parent_node, struct Node *child_node, int numofdimensions) // adds a child node to a given parent node
{
    parent_node->num_of_children_or_tuples++; // updates the number of child nodes of the parent node.
    if (parent_node->child_nodes == NULL)
        parent_node->child_nodes = malloc(sizeof(struct Node *) * parent_node->num_of_children_or_tuples); // allocates memory for an array of child nodes with the size of num_of_children_or_tuples
    else
        parent_node->child_nodes = realloc(parent_node->child_nodes, sizeof(struct Node *) * parent_node->num_of_children_or_tuples); // reallocates the memory for the array of child nodes to accommodate the new child node.
    parent_node->child_nodes[parent_node->num_of_children_or_tuples - 1] = child_node;                                                // assigns the child node to the last element of the array of child nodes in the parent node.
    child_node->parent = parent_node;                                                                                                 // sets the parent of the child node to be the parent node.
    if (parent_node->num_of_children_or_tuples == 1)
    {
        // sets the bounding box of the parent node to be the same as the bounding box of the child node.
        for (int i = 0; i < numofdimensions; i++)
        {
            parent_node->bounddefiners[i].dmax = child_node->bounddefiners[i].dmax;
            parent_node->bounddefiners[i].dmin = child_node->bounddefiners[i].dmin;
        }
    }
    else
    {
        // adjusts the bounding box of the parent node to accommodate the new child node by taking the minimum and maximum values of the bounding box dimensions.
        for (int i = 0; i < numofdimensions; i++)
        {
            parent_node->bounddefiners[i].dmax = max(child_node->bounddefiners[i].dmax, parent_node->bounddefiners[i].dmax);
            parent_node->bounddefiners[i].dmin = min(child_node->bounddefiners[i].dmin, parent_node->bounddefiners[i].dmin);
        }
    }
}

void pickNext(struct Node *node1, struct Node *node2, struct SplitArray *splitArray, struct Rtree *rtree) // helper function used in the process of splitting a node as it helps to select which of the two nodes the next child node from the split array should be added to.
{
    long int max_area_difference = LONG_MIN;
    struct Node *node_temp;                               // pointer to the node that will be selected as the parent for the next child node to be added to the split array.
    int index;                                            // index of the next child node to be added to the parent node.
    for (int i = 0; i < splitArray->numofchildnodes; i++) // iterates over each child node in the split array.
    {
        // calculates the difference between the areas of the bounding boxes that would result if the child node were added to node1 or node2.
        long int area4node1 = getArea(rtree->numofdimensions, get_bounding_box(rtree->numofdimensions, splitArray->array_of_child_nodes[i]->bounddefiners, node1->bounddefiners)) - getArea(rtree->numofdimensions, node1->bounddefiners);
        long int area4node2 = getArea(rtree->numofdimensions, get_bounding_box(rtree->numofdimensions, splitArray->array_of_child_nodes[i]->bounddefiners, node2->bounddefiners)) - getArea(rtree->numofdimensions, node2->bounddefiners);
        // selects the child node that results in the largest difference in area between node1 and node2.
        if (labs(area4node1 - area4node2) > max_area_difference)
        {
            max_area_difference = labs(area4node1 - area4node2);
            node_temp = (area4node1 > area4node2) ? node2 : node1;
            index = i;
        }
    }
    // adds the selected child node to the parent node, and removes it from the split array.
    addChildNode2Parent(node_temp, splitArray->array_of_child_nodes[index], rtree->numofdimensions);
    removeNodeFromSplitArray(splitArray, index);
}

void emptyNode(struct Node *node, int numofdimensions) // used to clear out the contents of a node, making it empty and ready to be reused. When a node becomes empty, it can be reused by splitting the contents of another node or by moving its contents to a parent node.
{
    if (node->child_nodes)
    {
        free(node->child_nodes);
        node->child_nodes = NULL;
        node->child_nodes = malloc(sizeof(struct Node *));
    }

    for (int i = 0; i < numofdimensions; i++)
    {
        node->bounddefiners[i].dmax = INT_MIN;
        node->bounddefiners[i].dmin = INT_MAX;
    }
    node->num_of_children_or_tuples = 0;
}

struct Node *nodeSplit(struct Rtree *rtree, struct Node *parent_node, struct Node *child_node) // Splits a node in an R tree
{
    struct SplitArray *splitArray = newSplitArray(parent_node, child_node); // will be used to store the child nodes that need to be split between the parent and the new split node.
    struct Node *grandparent_node = parent_node->parent;                    // creates a pointer to the grandparent node of the child node by getting the parent of the parent node.

    struct Node *split_node = new_node(rtree->numofdimensions); // This node will be used as the new split node.
    emptyNode(parent_node, rtree->numofdimensions);             // empties the parent node by calling the emptyNode function, necessary because the parent node will be used to store some of the child nodes after the split.

    // Step 1: Call pickSeed
    struct Node **pick_seed_node = pickSeed(rtree->numofdimensions, splitArray); // pick two seed nodes from the child nodes and assign them to the parent and split nodes. The pickSeed function is called and it returns an array of two pointers to nodes.
    addChildNode2Parent(parent_node, pick_seed_node[0], rtree->numofdimensions); // These two seed nodes are added to the parent and split nodes using the addChildNode2Parent function.
    addChildNode2Parent(split_node, pick_seed_node[1], rtree->numofdimensions);

    // Step 2: Call pickNext if min_enties not fulfilled
    for (int i = 0; i < rtree->max_entries - 1; i++)
    {
        if (parent_node->num_of_children_or_tuples + splitArray->numofchildnodes == rtree->min_entries && split_node->num_of_children_or_tuples >= rtree->min_entries)
        {
            // If adding a child node to parent_node would cause it to have fewer than the minimum number of entries and adding a child node to split_node would leave it with at least the minimum number of entries, then add the child node to parent_node.
            addChildNode2Parent(parent_node, splitArray->array_of_child_nodes[0], rtree->numofdimensions);
            removeNodeFromSplitArray(splitArray, 0);
        }
        else if (split_node->num_of_children_or_tuples + splitArray->numofchildnodes == rtree->min_entries && parent_node->num_of_children_or_tuples >= rtree->min_entries)
        {
            // If adding a child node to split_node would cause it to have fewer than the minimum number of entries and adding a child node to parent_node would leave it with at least the minimum number of entries, then add the child node to split_node.
            addChildNode2Parent(split_node, splitArray->array_of_child_nodes[0], rtree->numofdimensions);
            removeNodeFromSplitArray(splitArray, 0);
        }
        // If neither of the above conditions are true, call pickNext to determine which node to add the child node to.
        else
        {
            pickNext(parent_node, split_node, splitArray, rtree);
        }
    }

    free(splitArray->array_of_child_nodes);
    free(splitArray);
    parent_node->parent = grandparent_node; // sets the parent of parent_node to grandparent_node.
    split_node->parent = NULL;              // sets the parent of split_node to NULL.

    return split_node; // Returns the newly created split_node.
}

void convertChild2Tuple(struct Node *parent_node, int numofdimensions) // converts the child nodes of a given parent node into tuples.
{
    if (parent_node == NULL)
        return;
    // reallocates memory for the parent node's list of tuples. It resizes the memory block to hold the number of child nodes that the parent node has.
    parent_node->list_of_tuples = realloc(parent_node->list_of_tuples, (sizeof(int *) * parent_node->num_of_children_or_tuples));
    for (int i = 0; i < parent_node->num_of_children_or_tuples; i++)
    {
        parent_node->list_of_tuples[i] = malloc(sizeof(int) * numofdimensions); //  allocates memory for the i-th tuple. It resizes the memory block to hold the number of dimensions in the R tree.
        for (int j = 0; j < numofdimensions; j++)
        {
            parent_node->list_of_tuples[i][j] = parent_node->child_nodes[i]->bounddefiners[j].dmax; //  assigns the maximum value of the j-th dimension of the i-th child node's bounding box to the corresponding element of the i-th tuple.
        }
    }
    free(parent_node->child_nodes); // frees the memory allocated for the parent node's child nodes.
    parent_node->child_nodes = NULL;
}

struct Node *nodeSplit_leaf(struct Rtree *rtree, struct Node *leaf_node, int *tuple) // used to split a leaf node
{
    // converting tuples to child nodes
    struct Node *tuple_node = createLeafNode(rtree->numofdimensions, tuple);
    int **list_of_tuples = leaf_node->list_of_tuples; // Gets the list of tuples from the input leaf node.
    emptyNode(leaf_node, rtree->numofdimensions);     // Clear the contents of leaf node.

    for (int i = 0; i < rtree->max_entries; i++)
    {
        addChildNode2Parent(leaf_node, createLeafNode(rtree->numofdimensions, list_of_tuples[i]), rtree->numofdimensions); //  Adds a child node to the leaf node
    }

    struct Node *split_leaf_node = nodeSplit(rtree, leaf_node, tuple_node); // Call the nodeSplit function to split the leaf node.

    // converting child nodes to tuple
    convertChild2Tuple(leaf_node, rtree->numofdimensions);
    convertChild2Tuple(split_leaf_node, rtree->numofdimensions);

    return split_leaf_node; // Returns the split leaf node.
}

int read_tuples_and_insert(struct Rtree *rtree, const char *filename) //  reads tuples from a file and inserts them into an R-tree data structure
{
    FILE *file = fopen(filename, "r"); // opens the specified file in read mode and assigns a file pointer to file.
    if (file == NULL)
    { // file failed to open
        printf("Error opening file: %s\n", strerror(errno));
        return 1;
    }

    int count, value;
    while (fscanf(file, "%d", &value) == 1) // reads the tuples from the file line by line.
    {
        count = 0;
        int *tuple = malloc(sizeof(int) * rtree->numofdimensions);
        if (tuple == NULL)
        {
            printf("Error allocating memory");
            fclose(file);
            return 1;
        }

        tuple[count++] = value;

        for (int i = 1; i < rtree->numofdimensions; ++i)
        {
            if (fscanf(file, "%d", &value) == 1)
            {
                tuple[count++] = value;
            }
            else
            {
                break;
            }
        }
        // If a complete tuple with the correct number of dimensions is read, the insert() function is called to insert the tuple into the R-tree.
        if (count == rtree->numofdimensions)
        {
            insert(rtree, tuple);
        }
        // If the tuple is incomplete, an error message is printed, and the allocated memory for the tuple is freed.
        else
        {
            printf("Error: Tuple count is less than the number of dimensions (%d < %d)\n", count, rtree->numofdimensions);
            free(tuple);
        }
    }

    fclose(file);
    return 0;
}

void free_node(struct Node *node, int numofdimensions)
{
    if (node == NULL)
        return;

    if (is_leaf(node))
    {
        for (int i = 0; i < node->num_of_children_or_tuples; i++)
        {
            free(node->list_of_tuples[i]);
        }
    }
    else
    {
        for (int i = 0; i < node->num_of_children_or_tuples; i++)
        {
            free_node(node->child_nodes[i], numofdimensions);
        }
    }

    free(node->bounddefiners);
    free(node->child_nodes);
    free(node->list_of_tuples);
    free(node);
}

void free_rtree(struct Rtree *rtree)
{
    if (rtree == NULL)
        return;

    free_node(rtree->root, rtree->numofdimensions);
    free(rtree);
}

int main(int argc, char *argv[])
{
    struct Rtree *rtree = new_rtree(4, 2, 2);
    printRtree(rtree);
    if (argc != 2)
    {
        printf("Please provide a filename\n");
        return 1;
    }
    read_tuples_and_insert(rtree, argv[1]);
    printRtree(rtree);
    free_rtree(rtree);

    // run the script file

    return 0;
}