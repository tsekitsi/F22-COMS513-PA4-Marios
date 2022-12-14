/**
 * @author Ashwin K J
 * @file
 */
#include "Diff_Mapping.hpp"
#include "Get_Input.hpp"
#include "Graph.hpp"
#include "Graph_Instruction.hpp"
#include "Graph_Line.hpp"
#include "MVICFG.hpp"
#include "Module.hpp"
#include <chrono>

using namespace hydrogen_framework;

// Added by Marios for Project 4 of COM S 513 (Fall 2022):

class DfsTraverser {
  public:
    /**
     * Constructor.
     */
    DfsTraverser(Graph *g) : traversed(false) {
      graph = g;
    }

    bool getIsTraversed() { return traversed; }
    void setIsTraversed() { traversed = true; }

    /**
     * Performs DFS starting from the virtual entry node for the function "main",
     * and returns the number of maximal acyclic paths added in V2.
     */
    unsigned paths_added() {
      Graph_Instruction *root = graph->findVirtualEntry("main");

      if (!getIsTraversed()) {
        dfs(root);
        setIsTraversed();
      }

      return root->numActivePathsInVersion[2] + root->numRetiredPathsInVersion[2];
    }

    /**
     * Performs DFS starting from the virtual entry node for the function "main",
     * and returns the number of maximal acyclic paths removed in V2.
     */
    unsigned paths_removed() {;
      Graph_Instruction *root = graph->findVirtualEntry("main");
      
      if (!getIsTraversed()) {
        dfs(root);
        setIsTraversed();
      }

      return root->numActivePathsInVersion[1] + root->numRetiredPathsInVersion[1];
    }

    /**
     * Returns 0 if the edge e is in both versions,
     *         1 if the edge e is only in version 1,
     *         2 if the edge e is only in version 2.
    */
    unsigned xorVersion(Graph_Edge *e) {
      auto versions = e->getEdgeVersions();
      if (versions.size() == 1)
        return versions.front();
      else
        return 0;
    }

    /* Iterative implementation: */

    /**
     * Performs DFS on a specific node x of the graph.
     */
    void dfs(Graph_Instruction *x) {
      visited[x->getInstructionID()] = true;

      std::list<Graph_Edge *> edgesOutOfX = x->getInstructionEdges();
      // Remove inwward edges:
      for (auto itr = edgesOutOfX.cbegin(); itr != edgesOutOfX.end(); itr++)
          if (x->getInstructionID() == (*itr)->getEdgeTo()->getInstructionID()) // if x.ID is e.to.ID..
              edgesOutOfX.erase(itr--);

      for (Graph_Edge *xy : edgesOutOfX) {
        Graph_Instruction *y = xy->getEdgeTo();
        
        if (!visited[y->getInstructionID()])
            dfs(y);
      }

      int countr = 0;
      // Processing edge (x, y):
      for (Graph_Edge *xy : edgesOutOfX) {
        Graph_Instruction *y = xy->getEdgeTo();
        unsigned k = xorVersion(xy);

        if (k != 0) { // if (x, y) is only in version 1 or only in version 2..
          unsigned m = (k == 2) ? 1 : 2; // m = the other version.

          x->numActivePathsInVersion[k] += (y->numActivePathsInVersion[k] == 0) ? 1 : y->numActivePathsInVersion[k];

          x->numRetiredPathsInVersion[k] = y->numRetiredPathsInVersion[k];
          x->numRetiredPathsInVersion[m] = y->numRetiredPathsInVersion[m];
        } else { // if (x, y) is in both versions..
          x->numActivePathsInVersion[1] = 0;
          x->numActivePathsInVersion[2] = 0;

          x->numRetiredPathsInVersion[1] = y->numActivePathsInVersion[1] + y->numRetiredPathsInVersion[1];
          x->numRetiredPathsInVersion[2] = y->numActivePathsInVersion[2] + y->numRetiredPathsInVersion[2];
        }
      }

      unsigned temp = x->numActivePathsInVersion[2] + x->numRetiredPathsInVersion[2];
    }
  
  /** Prints misc info. */
  void printMisc() {
    std::cout << "[MARIOS:] The graph has " << visited.size() << " nodes.\n";
    std::cout << "[MARIOS:] The graph has " << graph->getGraphEdges().size() << " edges.\n";
  }

  private:
    Graph *graph;                       /** The graph onto which to perform DFS. */
    std::map<unsigned, bool> visited;   /** Data structure to keep track of visited nodes. */
    bool traversed;                     /** Keep track of whether the graph has been traversed. */
};

/**
 * Main function
 */
int main(int argc, char *argv[]) {
  /* Getting the input */
  if (argc < 2) {
    std::cerr << "Insufficient arguments\n"
              << "The correct format is as follows:\n"
              << "<Path-to-Module1> <Path-to-Module2> .. <Path-to-ModuleN> :: "
              << "<Path-to-file1-for-Module1> .. <Path-to-fileN-for-Module1> :: "
              << "<Path-to-file2-for-Module2> .. <Path-to-fileN-for-Module2> ..\n"
              << "Note that '::' is the demarcation\n";
    return 1;
  } // End check for min argument
  Hydrogen framework;
  if (!framework.validateInputs(argc, argv)) {
    return 2;
  } // End check for valid Input
  if (!framework.processInputs(argc, argv)) {
    return 3;
  } // End check for processing Inputs
  std::list<Module *> mod = framework.getModules();
  /* Create ICFG */
  unsigned graphVersion = 1;
  Module *firstMod = mod.front();
  Graph *MVICFG = buildICFG(firstMod, graphVersion);
  /* Start timer */
  auto mvicfgStart = std::chrono::high_resolution_clock::now();
  /* Create MVICFG */
  for (auto iterModule = mod.begin(), iterModuleEnd = mod.end(); iterModule != iterModuleEnd; ++iterModule) {
    auto iterModuleNext = std::next(iterModule);
    /* Proceed as long as there is a next module */
    if (iterModuleNext != iterModuleEnd) {
      /* Container for added and deleted MVICFG lines */
      std::list<Graph_Line *> addedLines;
      std::list<Graph_Line *> deletedLines;
      std::map<Graph_Line *, Graph_Line *> matchedLines; /**<Map From ICFG Graph_Line to MVICFG Graph_Line */
      std::list<Diff_Mapping> diffMap = generateLineMapping(*iterModule, *iterModuleNext);
      Graph *ICFG = buildICFG(*iterModuleNext, ++graphVersion);
      for (auto iter : diffMap) {
        /* iter.printFileInfo(); */
        std::list<Graph_Line *> iterAdd = addToMVICFG(MVICFG, ICFG, iter, graphVersion);
        std::list<Graph_Line *> iterDel = deleteFromMVICFG(MVICFG, ICFG, iter, graphVersion);
        std::map<Graph_Line *, Graph_Line *> iterMatch = matchedInMVICFG(MVICFG, ICFG, iter, graphVersion);
        addedLines.insert(addedLines.end(), iterAdd.begin(), iterAdd.end());
        deletedLines.insert(deletedLines.end(), iterDel.begin(), iterDel.end());
        matchedLines.insert(iterMatch.begin(), iterMatch.end());
      } // End loop for diffMap
      /* Update Map Edges */
      getEdgesForAddedLines(MVICFG, ICFG, addedLines, diffMap, graphVersion);
      /* Update the matched lines to get new temporary variable mapping for old lines */
      updateMVICFGVersion(MVICFG, addedLines, deletedLines, diffMap, graphVersion);
      /* Update Map Version */
      MVICFG->setGraphVersion(graphVersion);
    } // End check for iterModuleEnd
  } // End loop for Module
  /* Stop timer */
  auto mvicfgStop = std::chrono::high_resolution_clock::now();
  auto mvicfgBuildTime = std::chrono::duration_cast<std::chrono::milliseconds>(mvicfgStop - mvicfgStart);
  MVICFG->printGraph("MVICFG");
  std::cout << "Finished Building MVICFG in " << mvicfgBuildTime.count() << "ms\n";
  
  DfsTraverser dt(MVICFG);
  
  std::cout << "[MARIOS:] There are " << dt.paths_added() << " paths added.\n";
  std::cout << "[MARIOS:] There are " << dt.paths_removed() << " paths removed.\n";
  
  dt.printMisc();
  
  /* Write output to file */
  std::ofstream rFile("Result.txt", std::ios::trunc);
  if (!rFile.is_open()) {
    std::cerr << "Unable to open file for printing the output\n";
    return 5;
  } // End check for Result file
  rFile << "Input Args:\n";
  for (auto i = 0; i < argc; ++ i) {
    rFile << argv[i] << "  ";
  } // End loop for writing arguments
  rFile << "\n";
  rFile << "Finished Building MVICFG in " << mvicfgBuildTime.count() << "ms\n";
  rFile.close();
  return 0;
} // End main
