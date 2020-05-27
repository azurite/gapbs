#include <set>
#include <algorithm>

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "tc_verifier.h"

using namespace std;

// for each node iterate through it's ordered neighbour pairs and check if they
// have an edge between them i.e. for each (v, w) \in n(u) if {v, w} \in E then
// we have a triangle. All triangles are counted 3 times
size_t NodeIterator(const Graph &g) {
  size_t total = 0;

  for(NodeID u : g.vertices()) {
    for(NodeID v : g.out_neigh(u)) {
      set<NodeID> v_neigh(g.out_neigh(v).begin(), g.out_neigh(v).end());
      for(NodeID w : g.out_neigh(u)) {
        if(v < w && v_neigh.find(w) != v_neigh.end()) {
          total++;
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

  BenchmarkKernel(cli, g, NodeIterator, PrintTriangleStats, TCVerifier);
  return 0;
}
