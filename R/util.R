# get the skeleton from the edge representation
# loop over the nodes
.calculate_adjacencies_from_edges <- function(edges, nodes) {
  n_edges <- nrow(edges)
  adjacency <- lapply(nodes, function(node) {
    neighborhood <- c()
    # loop over the edges
    for (i in 1:n_edges) {
      # if a node is in an edge, find its partner (neighbor)
      if( node %in% edges[i, 1:2]) {
        neighbor <- edges[i, c(node != edges[i,1:2], F)]
        if (!(neighbor %in% neighborhood))
          neighborhood <- c(neighborhood, neighbor)
      }
    }
    return(neighborhood)
  }
  )
  names(adjacency) <- nodes
  return(adjacency)
}