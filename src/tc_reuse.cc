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

struct pair_hash {
  template <class T>
  size_t operator () (const pair<T,T> &p) const {
    // makes sure that h(<a, b>) == h(<b, a>)
    T vertex_min = min(p.first, p.second);
    T vertex_max = max(p.first, p.second);
    string h = to_string(vertex_min) + "|" + to_string(vertex_max);
    return hash<string>{}(h);
  }
};


// returns true iff N(x) is subseteq of N(y)
// note that the neighborhood lists are sorted in increasing order
bool subseteq(const Graph &g, const NodeID x, const NodeID y) {
  NodeID x_min = *(g.out_neigh(x).begin());
  NodeID x_max = *(g.out_neigh(x).end() - 1);
  NodeID y_min = *(g.out_neigh(y).begin());
  NodeID y_max = *(g.out_neigh(y).end() - 1);
  return x_min >= y_min && x_max <= y_max;
}


size_t TCReuse(const Graph &g) {
  unordered_map<pair<NodeID, NodeID>, size_t, pair_hash> intsec;
  size_t total = 0;

  for(NodeID u : g.vertices()) {
    for(NodeID v : g.out_neigh(u)) {
      if(v > u)
        break;
      if(subseteq(g, u, v))
        total += g.out_degree(u);
      for(NodeID w : g.out_neigh(v)) {
        if(subseteq(g, u, w) && intsec.find(make_pair(v, w)) != intsec.end()) {
          total += g.out_degree(u);
        }
        else {
          // intersect N(u) with N(v)
          size_t u_and_v = 0;
          // TODO
        }
      }
    }
  }

  return total / 3;
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

  BenchmarkKernel(cli, g, TCReuse, PrintTriangleStats, TCVerifier);
  return 0;
}
