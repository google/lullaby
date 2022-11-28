/*
Copyright 2017-2022 Google Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS-IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef REDUX_MODULES_BASE_DEPENDENCY_GRAPH_H_
#define REDUX_MODULES_BASE_DEPENDENCY_GRAPH_H_

#include <cstddef>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "redux/modules/base/logging.h"

namespace redux {

// A directed acyclic graph of nodes of type T that is useful for resolving
// dependencies between nodes.
//
// Once nodes and dependencies are added to the graph, it can be traversed such
// that dependent nodes will be visited before their dependencies.
template <typename T>
class DependencyGraph {
 public:
  // Adds a node to the graph. A dependency to/from this node can later be added
  // via AddDependency.
  void AddNode(T node) { TryAddNode(node); }

  // Adds a dependency between the given nodes, registering the nodes as needed.
  void AddDependency(T node, T dependency) {
    const size_t node_index = TryAddNode(node);
    const size_t dependency_index = TryAddNode(dependency);
    incoming_edges_[node_index].push_back(dependency_index);
    sorted_indices_.clear();
  }

  // Traverses the nodes in the graph such that dependent nodes will be visited
  // before the nodes that depend on them.
  template <typename Fn>
  void Traverse(const Fn& fn) const {
    SortGraphTopologically();
    CHECK_EQ(nodes_.size(), sorted_indices_.size());
    for (const size_t index : sorted_indices_) {
      fn(nodes_[index]);
    }
  }

  // Visits all the edges in the graph. Useful for visualizing the graph itself.
  //
  // For example, to create a GraphViz diagram, one could do something like:
  //   cout << "digraph D {" << endl;
  //   graph.ForAllEdges([](auto src, auto dst) {
  //     cout << src << " -> " << dst << endl;
  //   });
  //   cout << "}" << endl;
  template <typename Fn>
  void ForAllEdges(const Fn& fn) const {
    for (size_t i = 0; i < incoming_edges_.size(); ++i) {
      const size_t dst_index = i;
      for (const size_t src_index : incoming_edges_[i]) {
        fn(nodes_[src_index], nodes_[dst_index]);
      }
    }
  }

 private:
  struct NodeWithIncomingEdges {
    explicit NodeWithIncomingEdges(T node) : node(std::move(node)) {}

    T node;
    std::vector<size_t> incoming_edges;
  };

  size_t TryAddNode(T node) {
    auto iter = node_index_.find(node);
    if (iter != node_index_.end()) {
      // Node already added.
      return iter->second;
    }

    sorted_indices_.clear();

    const size_t index = nodes_.size();
    node_index_[node] = index;
    nodes_.emplace_back(node);
    incoming_edges_.emplace_back();
    return index;
  }

  void SortGraphTopologically() const {
    if (!sorted_indices_.empty()) {
      // The graph is already sorted.
      return;
    }

    using EdgeSet = absl::flat_hash_set<size_t>;
    using NodeWithIncomingEdges = std::pair<size_t, EdgeSet>;

    std::vector<NodeWithIncomingEdges> remaining_edges_by_node;
    for (size_t i = 0; i < nodes_.size(); ++i) {
      NodeWithIncomingEdges& n = remaining_edges_by_node.emplace_back();

      n.first = i;
      for (const size_t edge : incoming_edges_[i]) {
        n.second.emplace(edge);
      }
    }

    std::vector<size_t> roots;
    for (const auto& iter : remaining_edges_by_node) {
      if (iter.second.empty()) {
        roots.push_back(iter.first);
      }
    }

    while (!roots.empty()) {
      const size_t root = roots.back();
      roots.pop_back();
      sorted_indices_.push_back(root);

      for (NodeWithIncomingEdges& iter : remaining_edges_by_node) {
        EdgeSet& incoming_edges = iter.second;
        if (incoming_edges.erase(root)) {
          if (incoming_edges.empty()) {
            // If the node has no other incoming arcs then keep track of it as a
            // root.
            roots.push_back(iter.first);
          }
        }
      }
    }

    for (const auto& iter : remaining_edges_by_node) {
      CHECK(iter.second.empty());
    }
  }

  std::vector<T> nodes_;
  absl::flat_hash_map<T, size_t> node_index_;
  std::vector<std::vector<size_t>> incoming_edges_;
  mutable std::vector<size_t> sorted_indices_;
};

}  // namespace redux

#endif  // REDUX_MODULES_BASE_DEPENDENCY_GRAPH_H_
