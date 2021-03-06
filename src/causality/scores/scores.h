#ifndef SCORES_H
#define SCORES_H

#include <dataframe.h>

#define BIC_SCORE "bic"
#define BDEU_SCORE "bdeu"
#define DISCRETE_BIC_SCORE "discrete-bic"

struct score_args {
    double *fargs;
    int    *iargs;
};

typedef double (*score_func)(struct dataframe *df, int *xy, int npar,
                                 struct score_args *args);

double bdeu_score(struct dataframe *df, int *xy, int npar,
                      struct score_args *args);

double discrete_bic_score(struct dataframe *df, int *xy, int npar,
                              struct score_args *args);

double bic_score(struct dataframe *df, int *xy, int npar,
                     struct score_args *args);

double calculate_rss(double *cov, int m);
#endif
