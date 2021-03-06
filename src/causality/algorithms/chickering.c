/* Author: Alexander Rix
 * Date  : 11/30/18
 * Description:
 * chickering.c implements a function to convert directed acyclic graphs into
 * patterns. The algorithm is described in Chickering's paper
 * "A Transformational Characterization of Equivalent Bayesian Network
 * Structures", avaliable on the arxiv: https://arxiv.org/abs/1302.4938
 */

#include <stdlib.h>

#include <causality.h>
#include <cgraph/cgraph.h>
#include <cgraph/edge_list.h>

#define UNKNOWN   -1
#define COMPELLED  DIRECTED
#define REVERSABLE UNDIRECTED

static int order_edges(struct cgraph *cg, int *sort);
static void insertion_sort(struct edge_list *list, int *inv_sort);
static int find_compelled(struct cgraph *cg, int *sort);

int causality_chickering(struct cgraph *cg)
{
    int *sort = causality_sort(cg);
    if (sort == NULL)
        goto ERR;
    if (order_edges(cg, sort))
        goto ERR;
    if (find_compelled(cg, sort))
        goto ERR;
    free(sort);
    return 0;
    ERR:
    CAUSALITY_ERROR("Causality Chickering failure! Exiting...\n");
    if (sort)
        free(sort);
    return 1;
}

/*
 * order_edges orders the parents of cg such that the nodes are in descending
 * order according to the sort.
 */
static int order_edges(struct cgraph *cg, int *sort)
{
    int  n_nodes  = cg->n_nodes;
    int *inv_sort = malloc(n_nodes * sizeof(int));
    if (inv_sort == NULL) {
        CAUSALITY_ERROR("Failed to allocate memory in order edges\n");
        return 1;
    }
    for (int i = 0; i < n_nodes; ++i)
        inv_sort[sort[i]] = i;
    /*
     * Replace the value at each edge with the parent's location in the sort.
     * Then, sort (in place) so that the edges are in descending order.
     * This isn't a problem because in force compelled all the values will be
     * declared unknown.
     */
    struct edge_list **parents = cg->parents;
    for (int i = 0; i < n_nodes; ++i)
        insertion_sort(parents[i], inv_sort);
    free(inv_sort);
    return 0;
}

/*
 * We need a sorting routine so we can order the edges. Typically, we would use
 * a mergesort routine for linked lists, but I suspect insertion sort will be
 * faster because the average degree of causal graphs is 2-5, and insertion sort
 * is faster than merge sort until we hit 10-50 elements.
 */
static void insertion_sort(struct edge_list *list, int *inv_sort)
{
    while (list) {
        struct edge_list *top = list;
        struct edge_list *max = list;
        while (top) {
            if (inv_sort[top->node] > inv_sort[max->node])
                max = top;
            top = top->next;
        }
        int list_node  = list->node;
        list->node     = max->node;
        max->node      = list_node;
        list           = list->next;
    }
}

static int find_compelled(struct cgraph *cg, int *sort)
{
    struct edge_list **parents = cg->parents;
    int          n_nodes = cg->n_nodes;
    for (int i = 0; i < n_nodes; ++i) {
        struct edge_list *p = parents[i];
        while (p) {
            p->edge = UNKNOWN;
            p        = p->next;
        }
    }
    /*
     * we iterate through the sort to satisfy the max min condition
     * necessary to run this part of the algorithm
     */
    for (int i = 0; i < n_nodes; ++i) {
        /* by lemma 5 in Chickering, all the incident edges on y are unknown
         * so we don't need to check to see its unordered */
        int         y  = sort[i];
        struct edge_list *yp = parents[y];
        /* if y has no incident edges, go to the next node in the order */
        if (!yp)
            continue;
        /* Since y has parents, run stepts 5-8 */
        int         x  = yp->node;
        struct edge_list *xp = parents[x];
        /*
         * for each parent of x, w, where w -> x is compelled
         * check to see if w forms a chain (w -> x -> y)
         * or shielded collider (w -> x -> y and w -> x)
         */
        while (xp) { /* STEP 5 */
            if (xp->edge != COMPELLED)
                goto NEXT;
            int w = xp->node;
            /* if true , w --> y , x;  x--> y form a shielded collider */
            if (edge_directed_in_cgraph(cg, w, y)) {
                struct edge_list* p = search_edge_list(parents[y], w);
                p->edge = COMPELLED;
            }
            /* otherwise it is a chain and parents of y are compelled */
            else {
                struct edge_list *p = parents[y];
                while (p) {
                    p->edge = COMPELLED;
                    p        = p->next;
                }
                goto EOFL; /* goto end of for loop */
            }
            NEXT: ;
            xp = xp->next;
        }
        /*
         * now, we need to search for z, where z -> y, x != z, and z is not a
         * parent of x. That is, an unshielded collider.
         */
        int unshielded_collider = 0;
        struct edge_list *p = parents[y];
        while (p) {
            int z = p->node;
            if (z != x && !adjacent_in_cgraph(cg, z, x)) {
                unshielded_collider = 1;
                break;
            }
            p = p->next;
        }
        /* if there is an unshielded collider, label all parents compelled */
        if (unshielded_collider) {
            p = parents[y];
            while (p) {
                p->edge = COMPELLED;
                p        = p->next;
            }
        }
        /* otherwise, label all unknown edges reversable */
        else {
            /*
             * we need to create cpy because unorient_directed_edge operates in
             * place, which would mess the parents[y] pointer
             */
            struct edge_list *cpy = copy_edge_list(parents[y]);
            if (cpy == NULL)
                goto ERR;
            p = cpy;
            while (p) {
                if (p->edge == UNKNOWN)
                    unorient_directed_edge(cg, p->node, y);
                p = p->next;
            }
            /* Error handling. */
            if (0) {
                ERR:
                CAUSALITY_ERROR("Malloc failed in find_compelled. Exiting.\n");
                free_edge_list(cpy);
                return 1;
            }
            free_edge_list(cpy);
        }
        EOFL: ;
    }
    return 0;
}
