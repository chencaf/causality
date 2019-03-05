#ifndef EDGE_LIST_H
#define EDGE_LIST_H

/* definition of each node in a linked list */
struct edge_list {
    int   node;
    short edge;
    short tag;
    struct edge_list *next;
};

void insert_edge_list(struct edge_list **root, int node, short edge, short tag);
struct edge_list * copy_edge_list(struct edge_list *root);
void free_edge_list(struct edge_list *root);
struct edge_list * search_edge_list(struct edge_list *root, int node);
void print_edge_list(struct edge_list *root);
int size_edge_list(struct edge_list *root);
void delete_edge_list(struct edge_list **root, int node);
int identical_edge_lists(struct edge_list *e1, struct edge_list *e2);
#endif
