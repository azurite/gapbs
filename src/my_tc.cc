#include <set>
#include <algorithm>

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

// for each edge (u, v) intersect the neighbours of u with the neighbours of v
// the size of the intersection determines the number of newly found triangles
// we only intersect elements in an ordered fashion to avoid double counting
size_t EdgeIterator(const Graph &g) {
  size_t total = 0;

  vector<NodeID> u_and_v;
  u_and_v.reserve(g.num_nodes());

  for(NodeID u : g.vertices()) {
    for(NodeID v : g.out_neigh(u)) {
      if(u < v) {
        auto order = [=](NodeID x) { return x <= max(u, v); };

        vector<NodeID> u_neigh(g.out_neigh(u).begin(), g.out_neigh(u).end());
        auto u_end = remove_if(u_neigh.begin(), u_neigh.end(), order);
        u_neigh.resize(u_end - u_neigh.begin());

        vector<NodeID> v_neigh(g.out_neigh(v).begin(), g.out_neigh(v).end());
        auto v_end = remove_if(v_neigh.begin(), v_neigh.end(), order);
        v_neigh.resize(v_end - v_neigh.begin());

        auto new_end = set_intersection(u_neigh.begin(), u_neigh.end(),
                                        v_neigh.begin(), v_neigh.end(),
                                        u_and_v.begin());

        u_and_v.resize(new_end - u_and_v.begin());
        total += u_and_v.size();
      }
    }
  }

  return total;
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
  BenchmarkKernel(cli, g, EdgeIterator, PrintTriangleStats, TCVerifier);
  return 0;
}
