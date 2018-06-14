/* Utility/path-enumeration.hpp
 *
 * Author:  Jordan J. Henkel
 * Created: 9.22.2017
 * Description:
 *  - Utility to perform CFG unrolling through the use
 *    of weakly topological ordering (and hierarchical ordering)
 *    Includes ball-larus on the uCFG
 *    Terms: Bourdoncle Components, Weak Topological Ordering (WTO),
 *           Hierarchical Ordering, Ball-Larus Path Profiling
 */

#pragma once

namespace c2ocaml {
namespace frontend {

// These will come in handy
typedef std::pair<std::deque<uint16_t>, uint32_t> UVert;
typedef std::pair<UVert, UVert> UEdge;
typedef std::vector<UEdge> UGraph;

// These are for when we introduce path numbering
typedef std::pair<mpz_class, mpz_class> PathRange;
typedef std::pair<PathRange, UVert> UPEdge;
typedef std::map<UVert, std::vector<UPEdge>> UPAdjacency;

// These are handy for debugging the below code
inline std::ostream &
operator<<(std::ostream &out,
           const std::pair<std::deque<uint16_t>, uint32_t> &v) {
  if (v.first.size() == 0) {
    return out << "\"[" << v.second << "]\"";
  }

  out << "\"[";
  for (auto &i : v.first) {
    out << (uint32_t)i << " ";
  }
  return out << "| " << v.second << "]\"";
}

inline std::ostream &operator<<(std::ostream &out, const UPAdjacency adjF) {
  for (auto &p : adjF) {
    for (auto &e : p.second) {
      std::cout << p.first << " -> " << e.second << std::endl;
    }
  }

  return out;
}

class PathEnumerator {
  static inline std::vector<std::deque<uint16_t>>
  GenerateVertices(uint32_t i, uint32_t K,
                   const std::vector<uint16_t> &depthMask,
                   const std::vector<bool> &loopHeads, uint16_t D = 0) {

    // This is our base case, so we return a singleton set with
    // just the empty list in it
    if (D == depthMask[i]) {
      return std::vector<std::deque<uint16_t>>{std::deque<uint16_t>()};
    }

    // This will hold the (recursively computed) results
    auto res = std::vector<std::deque<uint16_t>>();

    // This gives us an idea of whether or not we need to duplicate the
    // node just one extra time (loop heads require this)
    auto isHead =
        (loopHeads[i] && D == static_cast<uint16_t>(depthMask[i] - 1));

    // Iterate k times and increment the counter
    for (uint16_t k = 0; k < (isHead ? K + 1 : K); ++k) {
      for (auto &prefix : GenerateVertices(i, K, depthMask, loopHeads, D + 1)) {
        // Starts with k
        auto newQ = std::deque<uint16_t>{k};
        // Then the rest of the values
        for (auto &val : prefix) {
          newQ.push_back(val);
        }
        // Finally, add it to our set
        res.push_back(newQ);
      }
    }

    // All possible post-fixes
    return res;
  }

public:
  static inline std::string Enumerate(types::gcc_func input, uint16_t K = 1) {
    // Some constants
    const uint8_t LPL_NORMAL_EDGE = 0;
    const uint8_t LPL_EXIT_EDGE = 1;
    const uint8_t LPL_BACK_EDGE = 2;
    const uint8_t LPL_ENTRY_EDGE = 3;

    // There are N vertices in the CFG
    auto N = n_basic_blocks_for_fn(input);

    // This will hold a mask over the possible edges in this
    // CFG. A zero is a normal edge; a one is a loop exit edge;
    // a two is a backedge; a three is a loop entry edge
    // (each treated differently in our product construction)
    std::map<std::pair<uint32_t, uint32_t>, uint8_t> edgeMask;

    // Add all of the edges from the CFG to the map
    util::for_each_bb(input, [&](types::gcc_bb bb, int32_t index) {
      UNUSED(index);
      util::for_each_bb_succ(bb, [&](types::gcc_edge edge) {
        edgeMask[std::pair<uint32_t, uint32_t>(
            static_cast<uint32_t>(edge->src->index),
            static_cast<uint32_t>(edge->dest->index))] = LPL_NORMAL_EDGE;
        return constants::GCC_EXECUTE_SUCCESS;
      });
      return constants::GCC_EXECUTE_SUCCESS;
    });

    // This is a mask over the vertices that tells us whether
    // a given vertex is a loop head (needs special treatment)
    std::vector<bool> loopHeads(N, false);

    // This is a mask over the vertices that we will use to
    // construct our product graph
    std::vector<uint16_t> depthMask(N, 0);

    // These represent exit edges from our loops. These need
    // to be duplicated in a special way
    std::vector<std::pair<uint32_t, uint32_t>> loopExits;

    // Use GCC's nice loop info to compute these masks and lists
    util::for_each_loop(input, [&](types::gcc_loop loop) {
      // Don't know what to do without this!
      assert(loop->header);

      // Set it in our bitmask
      loopHeads[loop->header->index] = true;

      // Get the blocks in BFS order
      auto bfsBlocks = get_loop_body_in_bfs_order(loop);

      // Update each block in our depth mask
      // (Which means add a new index to the mask-list)
      for (size_t i = 0; i < loop->num_nodes; i++) {
        depthMask[bfsBlocks[i]->index] += 1;

        // Check for back-edge from this to header
        auto pairing = std::pair<uint32_t, uint32_t>(
            static_cast<uint32_t>(bfsBlocks[i]->index),
            static_cast<uint32_t>(loop->header->index));

        // If the edge exists then it is a back edge
        if (edgeMask.find(pairing) != edgeMask.end()) {
          edgeMask[pairing] = LPL_BACK_EDGE;
        }
      }

      // Clean this up
      free(bfsBlocks);

      // Grab all of the exit edges
      util::for_each_loop_exit(loop, [&](types::gcc_edge exit) {
        // And push them into our list
        edgeMask[std::pair<uint32_t, uint32_t>(
            static_cast<uint32_t>(exit->src->index),
            static_cast<uint32_t>(exit->dest->index))] = LPL_EXIT_EDGE;
      });
    });

    // Now we find edges incoming to loop heads
    util::for_each_bb(input, [&](types::gcc_bb bb, int32_t index) {
      UNUSED(index);
      util::for_each_bb_succ(bb, [&](types::gcc_edge edge) {
        // Skip if not a loop head
        if (!loopHeads[edge->dest->index]) {
          return constants::GCC_EXECUTE_SUCCESS;
        }

        // Need to be going into a loop (down a level)
        if (depthMask[edge->src->index] >= depthMask[edge->dest->index]) {
          return constants::GCC_EXECUTE_SUCCESS;
        }

        // Else set the mask
        edgeMask[std::pair<uint32_t, uint32_t>(
            static_cast<uint32_t>(edge->src->index),
            static_cast<uint32_t>(edge->dest->index))] = LPL_ENTRY_EDGE;
        return constants::GCC_EXECUTE_SUCCESS;
      });
      return constants::GCC_EXECUTE_SUCCESS;
    });

    // Now, we can start to duplicate nodes
    std::vector<UVert> unrolledCFG;

    // Perform the product over all N vertices with the depth mask
    // information we've collected (and loopHeads info too)
    for (auto i = 0; i < N; ++i) {
      // Just a normal vertex (not part of a loop or self-loop)
      if (depthMask[i] == 0) {
        unrolledCFG.push_back(UVert(std::deque<uint16_t>{}, i));
        continue;
      }

      // Generate a bunch of vertices
      for (auto &prefix : GenerateVertices(i, K, depthMask, loopHeads)) {
        // and add them to our new product graph
        unrolledCFG.push_back(UVert(prefix, i));
      }
    }

    // Matching helper
    auto match = [](std::deque<uint16_t> a, std::deque<uint16_t> b,
                    uint32_t offset = 0) {
      // Enforce |a| <= |b|
      assert(a.size() <= b.size());

      auto ait = a.begin();
      auto bit = b.begin();

      while (ait != (a.end() - offset)) {
        // Prefixes disagree at i!
        if (*ait != *bit) {
          return false;
        }
        ++ait;
        ++bit;
      }

      // Prefixes match!
      return true;
    };

    // The final graph
    UGraph uCFG;

    // Go through each pair of new vertices
    for (auto &v1 : unrolledCFG) {
      for (auto &v2 : unrolledCFG) {
        // Find what 'kind' of edge we have
        auto edge =
            edgeMask.find(std::pair<uint32_t, uint32_t>(v1.second, v2.second));

        // If the edge doesn't exist projecte onto the original graph
        // then it wont exist in the product
        if (edge == edgeMask.end()) {
          continue;
        }

        // Check it against our three types
        if (edge->second == LPL_NORMAL_EDGE) {
          // In this case they much match at their own level
          if (match(v1.first, v2.first)) {
            uCFG.push_back(UEdge(v1, v2));
          }
        } else if (edge->second == LPL_BACK_EDGE) {
          // In this case the destination needs to be one level
          // ahead of the source
          if (match(v1.first, v2.first, 1) &&
              v1.first.back() + 1 == v2.first.back()) {
            uCFG.push_back(UEdge(v1, v2));
          }
        } else if (edge->second == LPL_EXIT_EDGE) {
          // We always take the edge in the prefixes match
          if (match(v2.first, v1.first)) {
            uCFG.push_back(UEdge(v1, v2));
          }
        } else if (edge->second == LPL_ENTRY_EDGE) {
          // Prefixes must match and second is zero-th copy
          if (match(v1.first, v2.first) && v2.first.back() == 0) {
            uCFG.push_back(UEdge(v1, v2));
          }
        }
      }
    }

#ifdef LPL_DEBUG_GRAPH
    std::cout << "digraph Ucfg {" << std::endl;
    for (auto &edge : uCFG) {
      std::cout << "  " << edge.first << " -> " << edge.second << std::endl;
    }
    std::cout << "}" << std::endl;
#endif

    // Return ball larus on the unrolled cfg
    return BallLarus(N, uCFG);
  }

  static inline std::string BallLarus(uint32_t N, UGraph &uCFG) {
    // STEP ONE: build an adjacency list representation with
    // extra data-fields for the path profiling information
    UPAdjacency asAdjF;
    UPAdjacency asAdjB;

    // NOTE: we are reversing edges here since both our
    // algs are going to operatie in reverse order
    for (auto &edge : uCFG) {
      // Our new edge which, as of now, has paths 0 -- 0 and
      // goes to a new node with zero paths to exit
      auto asUPEF = UPEdge(PathRange(0, 0), edge.second);
      auto asUPEB = UPEdge(PathRange(0, 0), edge.first);

      // Check to see if it exists, if not we create the vector
      if (asAdjF.find(edge.first) == asAdjF.end()) {
        asAdjF[edge.first] = std::vector<UPEdge>{asUPEF};
      } else {
        // Just push
        asAdjF[edge.first].push_back(asUPEF);
      }

      // Same here for backwards
      if (asAdjB.find(edge.second) == asAdjB.end()) {
        asAdjB[edge.second] = std::vector<UPEdge>{asUPEB};
      } else {
        asAdjB[edge.second].push_back(asUPEB);
      }
    }

    // Exit node, we'll use this a bit
    auto ex = UVert(std::deque<uint16_t>(), 1);

    // NOTE: we need to patch up any node that has absolutely no
    // successors (and is NOT the exit). These are likely early
    // terminations such as abort or assert(false) or exit()...
    for (auto &edge : uCFG) {
      if (asAdjF.find(edge.second) != asAdjF.end()) {
        // Has successors, good!
        continue;
      }

      if (edge.second.second == 1) {
        // Is the real exit, good!
        continue;
      }

      // Uh oh, patch this up (in both directions)
      asAdjF[edge.second] = std::vector<UPEdge>{UPEdge(PathRange(0, 0), ex)};
      asAdjB[ex].push_back(UPEdge(PathRange(0, 0), edge.second));
    }

    // Now we are going to walk back up from exit and number paths
    std::stack<UVert> stack;
    std::map<UVert, mpz_class> numPaths;
    std::map<UVert, bool> visited;

    // Add the exit vert
    stack.push(ex);
    visited[ex] = true;
    numPaths[ex] = 1;

    while (!stack.empty()) {
      // Grab the next vert
      auto cur = stack.top();
      stack.pop();

      // Generate forward sum
      for (auto &edge : asAdjF[cur]) {
        numPaths[cur] += numPaths[edge.second];
      }

      // Get all predecessors
      for (auto &edge : asAdjB[cur]) {
        // Make sure we have our predecessors
        // numbered first
        bool good = true;
        for (auto &e2 : asAdjF[edge.second]) {
          if (numPaths[e2.second] == 0) {
            good = false;
            break;
          }
        }

        // If we dont revisit this later (or skip if visited)
        if (!good || visited[edge.second]) {
          continue;
        }

        // If we do, carry on
        stack.push(edge.second);
        visited[edge.second] = true;
      }
    }

    // Add the entry vertex
    auto en = UVert(std::deque<uint16_t>(), 0);
    visited = std::map<UVert, bool>();
    stack.push(en);
    visited[en] = true;

    while (!stack.empty()) {
      // Grab the next vert
      auto cur = stack.top();
      stack.pop();

      // Generate forward sum
      mpz_class sum = 0;
      for (auto &edge : asAdjF[cur]) {
        // Compute a path range based on the sums
        edge.first = PathRange(sum, sum + numPaths[edge.second] - 1);
        // Accumulate paths count
        sum += numPaths[edge.second];

        if (!visited[edge.second]) {
          stack.push(edge.second);
          visited[edge.second] = true;
        }
      }
    }

#ifdef LPL_DEBUG_BL
    std::cout << "digraph {" << std::endl;
    std::cout << asAdjF << std::endl << "}" << std::endl;
#endif

    // Start dumping the code
    std::stringstream out;
    std::map<std::string, int> arraypos;

    // Output the cfg size and number of paths from entry to exit
    out << "  in let cfg = Cfg.cfg (" << std::endl;
    out << "    " << N << "," << std::endl;
    out << "    Z.of_string \"" << numPaths[en] << "\"," << std::endl;
    out << "    [|" << std::endl;

    int idx = 0;
    for (auto &v : asAdjF) {
      // Track it's array index
      std::stringstream tmp;
      tmp << v.first; 
      arraypos[tmp.str()] = idx; 
      idx += 1;
    }

    // Now go through the graph
    for (auto &v : asAdjF) {
      mpz_class last = 0; 
      out << "      Cfg.vert (" << v.first << ", block_" << v.first.second << ", [|" << std::endl;
      // Dump all of the edges and path ranges
      for (auto &e : v.second) {
        std::stringstream tmp2;
        tmp2 << e.second;
        out << "          Cfg.edge (" << arraypos[tmp2.str()] << ", " << tmp2.str() << ", block_" << e.second.second << ", Z.of_string \"" << e.first.first
            << "\", Z.of_string \"" << e.first.second << "\");" << std::endl;
        last = e.first.first;
      }
      out << "        |]" << std::endl;
      out << "      );" << std::endl;
    }
    out << "    |]" << std::endl;
    out << "  )";

    // Return our generated code
    return out.str();
  }
};
}
} // c2ocaml::frontend
