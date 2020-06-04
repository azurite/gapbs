#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <cinttypes>
#include <utility>
#include <functional>
#include <unordered_map>
#include <unordered_set>

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "tc_verifier.h"

using namespace std;

// Returns true if neighborhood of y is "dense" and neighborhood of x is
// a subset of neighborhood of y in constant time doing bounds checking.
// Both neighborhoods are sorted in increasing order
bool subseteq(const Graph &g, const NodeID x, const NodeID y, bool *isDense) {
  auto x_neigh = g.out_neigh(x);
  auto y_neigh = g.out_neigh(y);

  NodeID x_first = *x_neigh.begin();
  NodeID x_last = *(x_neigh.end() - 1);
  NodeID y_first = *y_neigh.begin();
  NodeID y_last = *(y_neigh.end() - 1);

  if(g.out_degree(x) <= 1)
    return (x_first == y || (x_first >= y_first && x_last <= y_last)) && isDense[y];

  NodeID x_second = *(x_neigh.begin() + 1);
  NodeID x_second_last = *(x_neigh.end() - 2);
  bool front = (x_first == y && x_second >= y_first) || x_first >= y_first;
  bool back = (x_last == y && x_second_last <= y_last) || x_last <= y_last;

  return front && back && isDense[y];
}

bool isDenseSeq(const Graph &g, const NodeID u) {
  if(g.out_degree(u) <= 1)
    return true;

  auto neigh = g.out_neigh(u);
  NodeID prev = *neigh.begin();

  for(auto it = neigh.begin() + 1; it != neigh.end(); it++) {
    if(!(((prev + 1) == *it) || (((prev + 2) == *it) && (prev + 1 == u)))) {
      return false;
    }

    prev = *it;
  }

  return true;
}


/*
This is a slight optimization to tc.cc and it works as follows:

The idea is to find subset relations of neighborhoods between nodes that belong
to the same edge. If we have found such a subset lets say we have edge {u,v}
and N(v) is subset of N(u) then we can immediatly count an additional
d_v triangles

In order to be able to calculate such a subset relation in constant time some
preprocessing is required. We first check for each node u if its neighborhood
is "dense" that is if the neighborhood has the shape [v, v+1, v+2,...,v+d_u]
for some starting neighbor node v. Once this is established we can find subset
relations in constant time with bounds checking as long as the neighborhoods
are sorted.

Once such a subset relation is found the respective node all it's indicent edges
on the LHS of the subset relation are implicitly deleted by setting isDeleted
and checking for node existence when counting the rest of the triangles with
the usual "compact-forward" algorithm of M. Latapy.
*/
size_t TCReuseV1(const Graph &g) {
  size_t total = 0;
  size_t n = g.num_nodes();

  bool isDense[n];
  bool isDeleted[n];
  size_t smallerThanOffset[n];

  // find all "dense" neighborhoods and calculate offsets
  for(NodeID u : g.vertices()) {
    isDense[u] = isDenseSeq(g, u);

    size_t offset = 0;
    for(NodeID v : g.out_neigh(u)) {
      if(v >= u)
        break;
      offset++;
    }

    smallerThanOffset[u] = offset;
    isDeleted[u] = false; // just initialization at this point
  }

  // start counting triangles with subset neighborhood relation shortcut
  for(NodeID u : g.vertices()) {
    for(NodeID v : g.out_neigh(u)) {
      if(v > u && subseteq(g, v, u, isDense)) {
        total += g.out_degree(v) - smallerThanOffset[v];
        isDeleted[v] = true;
      }
    }
  }

  // count the rest of the triangles on the smaller graph with algorithm
  // "compact-forward"
  for (NodeID u = 0; u < g.num_nodes(); u++) {
    if(isDeleted[u])
      continue;
    for (NodeID v : g.out_neigh(u)) {
      if (v > u)
        break;
      if(isDeleted[v])
        continue;
      auto it = g.out_neigh(u).begin();
      for (NodeID w : g.out_neigh(v)) {
        if(isDeleted[w])
          continue;
        if (w > v)
          break;
        while (*it < w)
          it++;
        if (w == *it)
          total++;
      }
    }
  }

  return total;
}


// heuristic to see if sufficently dense power-law graph
bool WorthRelabelling(const Graph &g) {
  int64_t average_degree = g.num_edges() / g.num_nodes();
  if (average_degree < 10)
    return false;
  SourcePicker<Graph> sp(g);
  int64_t num_samples = min(int64_t(1000), g.num_nodes());
  int64_t sample_total = 0;
  pvector<int64_t> samples(num_samples);
  for (int64_t trial=0; trial < num_samples; trial++) {
    samples[trial] = g.out_degree(sp.PickNext());
    sample_total += samples[trial];
  }
  sort(samples.begin(), samples.end());
  double sample_average = static_cast<double>(sample_total) / num_samples;
  double sample_median = samples[num_samples/2];
  return sample_average / 1.3 > sample_median;
}


// uses heuristic to see if worth relabeling
size_t Hybrid(const Graph &g) {
  if (WorthRelabelling(g)) {
    return TCReuseV1(Builder::RelabelByDegree(g));
  }
  else {
    return TCReuseV1(g);
  }
}


void PrintTriangleStats(const Graph &g, size_t total_triangles) {
  cout << total_triangles << " triangles" << endl;
}


int main(int argc, char* argv[]) {
  CLApp cli(argc, argv, "triangle count");
  if (!cli.ParseArgs())
    return -1;
  Builder b(cli);
  Graph g = b.MakeGraph();
  if (g.directed()) {
    cout << "Input graph is directed but tc requires undirected" << endl;
    return -2;
  }

  g.PrintTopology();

  BenchmarkKernel(cli, g, Hybrid, PrintTriangleStats, TCVerifier);
  return 0;
}
