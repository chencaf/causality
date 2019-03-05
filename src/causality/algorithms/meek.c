/* Author : Alexander Rix
 * Date   : 11/30/18
 * Description:
 * meek.c implements the four meek rules found in meek(1995), as well a
 * causality c function, ccf_meek, that takes a pdag maximially orients it
 * (ie turns it into a PDAG) via the meek rules.
 */

#include <stdlib.h>

#include <causality.h>
#include <cgraph/cgraph.h>
#include <cgraph/edge_list.h>
#include <algorithms/meek.h>

void ccf_meek(struct cgraph *cg);
void apply_rule(struct cgraph *cg, int x, struct edge_list **stack,
                                   meek_rule meek_rule);

/*
 * meek_rules take in a PDAG and maximially orients it by repeatedly applying
 * the four meek rules
 * it currently returns an updated edge matrix
 */
void causality_meek(struct cgraph *cg)
{
    struct edge_list *stack = NULL;
    for (int i = 0; i < cg->n_nodes; ++i) {
        if (cg->spouses[i])
            insert_edge_list(&stack, i, 0, 0);
    }
    while (stack) {
        int node = stack->node;
        apply_rule(cg, node, &stack, meek_rule1);
        apply_rule(cg, node, &stack, meek_rule2);
        apply_rule(cg, node, &stack, meek_rule3);
        apply_rule(cg, node, &stack, meek_rule4);
        struct edge_list *e = stack;
        stack = stack->next;
        free(e);
    }
}

/*
 * apply_rule applied the selected meek rule (passed in by function pointer)
 * it returns 1 if the rule was applied, and 0 if not
 */
void apply_rule(struct cgraph *cg, int x, struct edge_list **stack,
                                   meek_rule meek_rule)
{
    /*
     * we need to create a copy of spouses incase an edge is
     * oriented -- orientation occurs "in place."
     */
    struct edge_list *cpy = copy_edge_list(cg->spouses[x]);
    struct edge_list *p   = cpy;
    if(!p)
        return;
    while(p) {
        int y = p->node;
        if (meek_rule(cg, x, y)) {
            orient_undirected_edge(cg, x, y);
            insert_edge_list(stack, y, 0, 0);
        }
        else if (meek_rule(cg, y, x)) {
            orient_undirected_edge(cg, y, x);
            insert_edge_list(stack, y, 0, 0);
        }
        p = p->next;
    }
    free_edge_list(cpy);
}

/*
 * meek rule one:
 * look for chain z --> x --- y, where !adj(y, z). If so,
 * orient x --> y
 */
 int meek_rule1(struct cgraph *cg, int x, int y)
 {
     struct edge_list *p = cg->parents[x];
     while (p) {
         int z = p->node;
         if (!adjacent_in_cgraph(cg, y, z)) {
             return 1;
         }
         p = p->next;
     }
     return 0;
 }

/*
 * meek rule 2: look for z such that, z --> x, y --> z. If
 * so, orient y --> x to prevent a cycle.
 */
 int meek_rule2(struct cgraph *cg, int x, int y)
 {
     struct edge_list  *c = cg->children[x];
     while (c) {
         int z = c->node;
         if (edge_directed_in_cgraph(cg, z, y))
             return 1;
         c = c->next;
     }
     return 0;
 }

/*
 * meek rule 3: orient node1 --- node2 into node2 --> node1 when there exists
 * chains node2 --- node3 --> node1 and node2 --- node4 --> node1, with
 * !adj(node3, node4)
 */
 int meek_rule3(struct cgraph *cg, int x, int y)
 {
     struct edge_list *yp = cg->parents[y];
     while (yp) {
         int z = yp->node;
         if (!edge_undirected_in_cgraph(cg, z, x))
             goto NEXT_Y_PARENT;
         struct edge_list *yp_cpy = cg->parents[y];
         while (yp_cpy) {
             int w = yp_cpy->node;
             if (w != z && edge_undirected_in_cgraph(cg, w, x)) {
                if (!adjacent_in_cgraph(cg, z, w))
                    return 1;
             }
             yp_cpy = yp_cpy->next;
         }
         NEXT_Y_PARENT: ;
         yp = yp->next;
     }
     return 0;
 }

/*
 * meek4 rule4: orient node1 --> node2 if there is a chain
 * node4 --> node3 --> node2, node1 --- node4, with adj(node3, node1) and
 * !adj(node2, node4)
 */
 int meek_rule4(struct cgraph *cg, int x, int y)
 {
     struct edge_list *yp = cg->parents[y];
     while (yp) {
         int z = yp->node;
         if (adjacent_in_cgraph(cg, x, z)) {
             struct edge_list *zp = cg->parents[z];
             while (zp) {
                 int w = zp->node;
                 if (edge_undirected_in_cgraph(cg, w, x)) {
                    if (!adjacent_in_cgraph(cg, y, w))
                        return 1;
                 }
                 zp = zp->next;
             }
         }
         yp = yp->next;
     }
     return 0;
 }
