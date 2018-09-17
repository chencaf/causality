#include <causality.h>
#include <cgraph.h>
#include <heap.h>
#include <dataframe.h>
#include <scores.h>
#include <pdx.h>
#include <chickering.h>
#include <edgetypes.h>
#include <stdint.h>


#define UNBLOCKED_PATH    0
#define NO_UNBLOCKED_PATH 1
#define CLIQUE    1
#define NO_CLIQUE 0

typedef struct gesrec {
  int   x;
  int   y;
  int   set_size;
  int   naxy_size;
  int * set;
  int * naxy;
} gesrec; /*  32 bytes */

cgraph  *ccf_fges(dataframe df, score_func score, double * fargs, int * iargs);

SEXP ccf_fges_wrapper(SEXP Df, SEXP ScoreType, SEXP States,
                               SEXP FloatingArgs, SEXP IntegerArgs)
{
    /*
     * calcluate the integer arguments and floating point arguments for the
     * score function.
     */
    int *iargs = NULL;
    if(!isNull(IntegerArgs))
        iargs = INTEGER(IntegerArgs);
    double *fargs = NULL;
    if(!isNull(FloatingArgs))
        fargs = REAL(FloatingArgs);
        dataframe df;
    df.nvar  = length(Df);
    df.nobs  = length(VECTOR_ELT(Df, 0));
    df.states = INTEGER(States);
    df.df     = malloc(df.nvar * sizeof(void *));
    /* populate df with the pointers to the columns of the R dataframe */
    int *states = df.states;
    for(int i = 0; i < df.nvar; ++i) {
        if(states[i])
            df.df[i] = INTEGER(VECTOR_ELT(Df, i));
        else
            df.df[i] = REAL(VECTOR_ELT(Df, i));
    }
    score_func score;
    if(!strcmp(CHAR(STRING_ELT(ScoreType, 0)), BIC_SCORE))
        score = bic_score;
    else if (!strcmp(CHAR(STRING_ELT(ScoreType, 0)), BDEU_SCORE))
        score = bdeu_score;
    else
        error("nope\n");
    /*
     * All the preprocessing work has now been done, so lets instantiate
     * an empty graph and run FGES
     */

    cgraph *cg = ccf_fges(df, score, fargs, iargs);


    /* POST PROCESSING */
    free_cgraph(cg);
    free(df.df);
    return ScalarReal(0);
}

static int is_clique(cgraph *cg, int * nodes, int n_nodes)
{
    for(int i = 0; i < n_nodes; ++i) {
        int inode = nodes[i];
        for(int j = 0; j < i; ++j) {
            if(!adjacent_in_cgraph(cg, inode, nodes[j]))
                return NO_CLIQUE;
        }
    }
    return CLIQUE;
}

static int traverse(ill *p, ill *queue, ill *v, int dst,
                            int seps_size, int *seps) {
    while(p) {
        int neighbor = p->key;
        for(int i = 0; i < seps_size; ++i) {
            if(neighbor == seps[i])
                goto NEXT;
        }
        if(neighbor == dst) {
            return UNBLOCKED_PATH;
        }
        if(!ill_search(v, neighbor)) {
            queue = ill_insert(queue, neighbor, 0);
            v     = ill_insert(v, neighbor, 0);
        }
        NEXT:
        p = p->next;
    }
    return NO_UNBLOCKED_PATH;
}

int no_unblocked_semidirected_path(cgraph *cg, int src, int dst,
                                               int * seps, int seps_size)
{
    int no_unblocked_path = 1;
    ill *queue = ill_insert(NULL, src, 0);
    ill *v     = ill_insert(NULL, src, 0);
    while(queue) {
        /* deque the head of the queue */
        int node = queue->key;
        ill *tmp = queue;
        queue    = queue->next;
        free(tmp);
        ill *p = cg->spouses[node];
        no_unblocked_path = traverse(p, queue, v, dst, seps_size, seps);
        if(!no_unblocked_path)
            goto END;
        p = cg->children[node];
        no_unblocked_path = traverse(p, queue, v, dst, seps_size, seps);
        if(!no_unblocked_path)
            goto END;
        }
    END:
    ill_free(queue);
    ill_free(v);
    return no_unblocked_path;
}

int is_valid_insertion(cgraph *cg, gesrec g, int *onion, int onion_size) {
    if(is_clique(cg, onion, onion_size)) {
        if(no_unblocked_semidirected_path(cg, g.y, g.x, onion, onion_size))
            return 1;
    }
    return 0;
}

gesrec score_powerset(cgraph *cg, dataframe data, gesrec g, double *dscore,
                                      score_func score, double *fargs,
                                      int *iargs)
{
    double ds     = DBL_MAX;
    double min_ds = DBL_MAX;
    gesrec min_g = g;
    min_g.naxy = malloc(min_g.naxy_size * sizeof(int));
    memcpy(min_g.naxy, g.naxy, min_g.naxy_size * sizeof(int));
    /* saute in butter for best results */
    int *onion = malloc((g.naxy_size + g.set_size) * sizeof(int));
    for(int i = 0; i < g.naxy_size; ++i)
        onion[i] = g.naxy[i];
    uint64_t n = 1 << g.set_size;
    for(int i = 0; i < n; ++i) {
        int onion_size = 0;
        for(uint32_t j = 0; j <  (uint32_t) g.set_size; ++j) {
            if((i & (1 << j)) == (1 << j)) {
                onion[g.naxy_size + onion_size] = g.set[onion_size];
                onion_size++;
            }
        }
        onion_size += g.naxy_size;
        if(is_valid_insertion(cg, g, onion, onion_size)) {
            ill *parents = cg->parents[g.y];
            int new_npar = onion_size + ill_size(parents) + 1;
            int *xy = malloc((new_npar + 1) * sizeof(int));
            xy[0]        = g.x;
            xy[new_npar] = g.y;
            int j;
            for(j = 0; j < onion_size; ++j)
                xy[1 + j] = onion[j];
            while(parents) {
                xy[j++] = parents->key;
                parents = parents->next;
            }
            ds = score_diff(data, xy, xy + 1, new_npar, new_npar - 1,
                                             fargs, iargs, score);
            if(ds < min_ds) {
                min_ds = ds;
                min_g.set_size = onion_size - g.naxy_size;
                min_g.set = malloc(min_g.set_size * sizeof(int));
                for(int j = 0; j < min_g.set_size; ++j) {
                    min_g.set[j] = onion[j + min_g.naxy_size];
                }
            }
            free(xy);
        }
    }
    free(onion);
    *dscore = min_ds;
    return min_g;
}



static void insert(cgraph *cg, gesrec gesrec)
{
    Rprintf("add %i --> %i\n", gesrec.x, gesrec.y);
    add_edge_to_cgraph(cg, gesrec.x, gesrec.y, DIRECTED);
    for(int i = 0; i < gesrec.set_size; ++i) {
        Rprintf("orient  %i --> %i\n", gesrec.set[i], gesrec.y);
        orient_undirected_edge(cg, gesrec.set[i], gesrec.y);
    }
}

double recalcluate_node(dataframe df, cgraph *cg, gesrec *gesrecp,
                                      score_func score, double *fargs,
                                                        int *iargs)
{
    double     dscore = DBL_MAX;
    double min_dscore = DBL_MAX;
    int y = gesrecp->y;
    for(int x = 0; x < df.nvar; ++x) {
        if((x == y) || adjacent_in_cgraph(cg, x, y))
            continue;
        gesrec g = {0};
        g.x = x;
        g.y = y;
        ill *l = cg->spouses[y];
        while(l) {
            if(adjacent_in_cgraph(cg, x, l->key))
                g.naxy_size += 1;
            else
                g.set_size += 1;
            l = l->next;
        }
        g.naxy = malloc(g.naxy_size * sizeof(int));
        g.set  = malloc(g.set_size * sizeof(int));
        /* we have to reiterate through the list */
        l = cg->spouses[y];
        int j = 0;
        int k = 0;
        while(l) {
            int z = l->key;
            if(adjacent_in_cgraph(cg, x, z))
                g.naxy[j++] = z;
            else
                g.set[k++]  = z;
            l = l->next;
        }
        gesrec min_g = score_powerset(cg, df, g, &dscore, score, fargs, iargs);
        if(dscore < min_dscore) {
            min_dscore = dscore;
            free(gesrecp->set);
            free(gesrecp->naxy);
            *gesrecp = min_g;
        }
        else {
            free(min_g.set);
            free(min_g.naxy);
        }
    free(g.set);
    free(g.naxy);
    }
    //Rprintf("%i --> %i\n naxy %i\n set %i\n dscore %f\n", gesrecp->x, gesrecp->y,
    //gesrecp->naxy_size, gesrecp->set_size, min_dscore);
    return min_dscore;
}


static void delete(cgraph *cg, gesrec gesrec)
{
    delete_edge_from_cgraph(cg, gesrec.x, gesrec.y, DIRECTED);
    for(int i = 0; i < gesrec.set_size; ++i) {
        orient_undirected_edge(cg, gesrec.x, gesrec.set[i]);
        orient_undirected_edge(cg, gesrec.y, gesrec.set[i]);
    }
}


cgraph *ccf_fges(dataframe df, score_func score,
                                        double *fargs, int *iargs)
{
    cgraph *cg         = create_cgraph(df.nvar);
    double graph_score = 0.0f;
    double dscore      = 0.0f;
    /*
    * We need to set up the priority queue so we know which edge to add
    * (and the other relevant information) at each stage of fges. Roughly how
    * this works is that each the highest scoring edge incident in each node is
    * recorded and then we find the highest scoring edge of all of those by
    * using the heap data structure we have
    */
    gesrec *gesrecords = calloc(df.nvar, sizeof(gesrec));
    heap       *queue      = create_heap(df.nvar);
    double     *dscores    = queue->dscores;
    void      **records    = queue->records;
    int        *indices    = queue->indices;
    for(int i = 0; i < df.nvar; ++i) {
        records[i] = gesrecords + i;
        indices[i] = i;
    }
    /* STEP 0: score x --> y */
    for(int y = 0; y < df.nvar; ++y) {
        double min     = DBL_MAX;
        int    arg_min = -1;
        for(int x = 0; x < y; ++x) {
            int    xy[2] = {x, y};
            double ds = score_diff(df, xy, NULL, 1, 0, fargs, iargs, score);
            if(ds < min) {
                min     = ds;
                arg_min = x;
            }
        }
        dscores[y]      = min;
        gesrecords[y].x = arg_min;
        gesrecords[y].y = y;
    }
    build_heap(queue);
    /* FORWARD EQUIVALENCE SEARCH (FES) */
    gesrec * gesrecp;
    while((gesrecp = extract_heap(queue, &dscore)) && dscore <= 0) {
        graph_score += dscore;
        insert(cg, *gesrecp);
        ccf_chickering(cg = ccf_pdx(cg));
        for(int i = 0; i < df.nvar; ++i) {
            gesrecp = gesrecords + i;
            gesrecp->y = i;
            dscore = recalcluate_node(df, cg, gesrecp, score, fargs, iargs);
            queue->dscores[i] = dscore;
            queue->records[i] = gesrecp;
            queue->indices[i] = i;
        }
        build_heap(queue);

        /*
        insert_heap(queue, dscore, gesrecp, gesrecp->y);
        remove_heap(queue, gesrecp->x);
        gesrecp = gesrecords + gesrecp->x;
        dscore = recalcluate_node(df, cg, gesrecp, score, fargs, iargs);
        insert_heap(queue, dscore, gesrecp, gesrecp->y);
        */

    }
    /* FORWARD EQUIVALENCE SEARCH (FES) */
    while(0) { /* TODO */}
    if(0) {
        delete(cg, *gesrecp);
    }

    for(int i = 0; i < df.nvar; ++i) {
        free(gesrecords[i].set);
        free(gesrecords[i].naxy);
    }
    free(gesrecords);
    print_cgraph(cg);
    free_heap(queue);
    Rprintf("fges complete\n");
    return cg;
}
