#include <set>

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"

using namespace std;

void PrintTriangleStats(const Graph &g, size_t total_triangles) {
  cout << total_triangles << " triangles" << endl;
}

// for each node iterate through it's ordered neighbour pairs and check if they
// have an edge between them i.e. for each (v, w) \in n(u) if {v, w} \in E then
// we have a triangle. All triangles are counted 3 times
size_t NodeIterator(const Graph &g) {
  size_t total = 0;

  for(NodeID u : g.vertices()) {
    for(NodeID v : g.out_neigh(u)) {
      for(NodeID w : g.out_neigh(u)) {
        if(v < w) {
          set<NodeID> v_neigh(g.out_neigh(v).begin(), g.out_neigh(v).end());
          if(v_neigh.find(w) != v_neigh.end()) {
            total++;
          }
        }
      }
    }
  }

  return total / 3;
}

// Compares with simple serial implementation that uses std::set_intersection
bool TCVerifier(const Graph &g, size_t test_total) {
  size_t total = 0;
  vector<NodeID> intersection;
  intersection.reserve(g.num_nodes());
  for (NodeID u : g.vertices()) {
    for (NodeID v : g.out_neigh(u)) {
      auto new_end = set_intersection(g.out_neigh(u).begin(),
                                      g.out_neigh(u).end(),
                                      g.out_neigh(v).begin(),
                                      g.out_neigh(v).end(),
                                      intersection.begin());
      intersection.resize(new_end - intersection.begin());
      total += intersection.size();
    }
  }
  total = total / 6;  // each triangle was counted 6 times
  if (total != test_total)
    cout << total << " != " << test_total << endl;
  return total == test_total;
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
