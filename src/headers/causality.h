#include <stdio.h>

#include "cgraph.h"
#include "ges.h"

#ifndef CAUSALITY_H
#define CAUSALITY_H

#ifdef CAUSALITY_R_H
#define CAUSALITY_PRINT(s) Rprintf("%s\n", s);
#define CAUSALITY_ERROR(s) Rprintf("Error: %s\n", s);
#else
#define CAUSALITY_PRINT(s) printf("%s\n", s);
#define CAUSALITY_ERROR(s) fprintf(stderr, "%s\n", s);
#endif


#define DIRECTED      1 /* -->               */
#define UNDIRECTED    2 /* ---               */
#define PLUSPLUSARROW 3 /* ++> aka --> dd nl */
#define SQUIGGLEARROW 4 /* ~~> aka --> pd nl */
#define CIRCLEARROW   5 /* o->               */
#define CIRCLECIRCLE  6 /* o-o               */
#define BIDIRECTED    7 /* <->               */

#define NUM_NL_EDGETYPES  2
#define NUM_LAT_EDGETYPES 7
#define NUM_EDGES_STORED  11

#define IS_DIRECTED(edge) ((edge) == DIRECTED || (edge) == CIRCLEARROW || \
                           (edge) == SQUIGGLEARROW || (edge) == PLUSPLUSARROW)


/* Search algorithms */
double ccf_ges(struct ges_score score, struct cgraph *cg);
/* Graph manipulations */
int           * ccf_sort(struct cgraph *cg);
struct cgraph * ccf_pdx(struct cgraph *cg);
void            ccf_chickering(struct cgraph *cg);
void            ccf_meek(struct cgraph *cg);
/* misc functions */
double ccf_score_graph(struct cgraph *cg, struct dataframe df, score_func score,
                                          struct score_args args);
void ccf_fr_layout(double *positions, int n_nodes, int *edges, int n_edges,
                                      double width, double height,
                                      int iterations);
#endif
