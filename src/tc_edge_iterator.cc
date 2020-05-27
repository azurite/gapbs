#include <vector>
#include <algorithm>

#include "benchmark.h"
#include "builder.h"
#include "command_line.h"
#include "graph.h"
#include "tc_verifier.h"

using namespace std;


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

  BenchmarkKernel(cli, g, EdgeIterator, PrintTriangleStats, TCVerifier);
  return 0;
}
