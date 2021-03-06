% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/arrowhead.R
\name{arrowhead_precision}
\alias{arrowhead_precision}
\alias{arrowhead_recall}
\title{Arrowhead Precision and Recall}
\usage{
arrowhead_precision(x, y)

arrowhead_recall(x, y)
}
\arguments{
\item{x}{A causality PDAG}

\item{y}{A causality PDAG}
}
\value{
Length one numeric between 0 and 1. \code{arrowhead_precision}
    returns \code{NA} if there are no oriented edges in \code{y}.

\code{arrowhead_recall} returns \code{NA} if there are
    no oriented edges (arrows) in \code{x}
}
\description{
\code{arrowhead_precision} calculates the arrowhead precision between two
causality graphs

\code{arrowhead_recall} calculates the arrowhead recall between two
causality graphs
}
\details{
\code{arrowhead_precision} counts the number of directed edges
    (\code{"-->"}) in \code{x} and then counts how many directed edges in
    \code{x} are also in \code{y}. Then, the ratio is returned.

\code{arrowhead_recall} counts the number of directed edges
    \code{x} and then counts how many directed edges in
    \code{y} are in \code{x}. Then, the ratio is returned. 1
    implies that every directed edge in \code{x} are also in \code{y}.
}
\examples{
TODO(arix)
}
\references{
Joseph D. Ramsey: “Scaling up Greedy Causal Search for Continuous Variables”,
 2015; \href{http://arxiv.org/abs/1507.07749}{arxiv:1507.07749[cs.AI]}.

Spirtes et al. “Causation, Prediction, and Search.”, Mit Press,
2001, p. 109.
}
\seealso{
Other graph comparison statistics:
    \code{\link{adjacency_precision}}, \code{\link{adjacency_recall}},
    and \code{\link{shd}}
}
\author{
Alexander Rix
}
