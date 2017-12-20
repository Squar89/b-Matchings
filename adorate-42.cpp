#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include "blimit.hpp"

typedef std::pair <unsigned int, unsigned int> edge;//<node_id, weight>
typedef std::vector <edge>::iterator edgesVecIt;

class Node {
private:
    unsigned int id;
public:
    unsigned int b;
    std::vector <edge> nodeEdges;
    std::vector <edge> matchedEdges;
    std::vector <unsigned int> seenNodes;
    std::mutex alterMatched;
    edgesVecIt current, sortedEnd, end;

    Node() : id(0), b(0) {}

    Node(Node&& other) noexcept {
        id = other.id;
        b = other.b;
        nodeEdges = std::move(other.nodeEdges);
        matchedEdges = std::move(other.matchedEdges);
        seenNodes = std::move(other.seenNodes);
    }

    explicit Node(unsigned int x) : id(x), b(0) {}

    void AddEdgeN(unsigned int neighbourId, unsigned int weight) {
        nodeEdges.emplace_back(neighbourId, weight);
    }
};

class Graph {
public:
    std::unordered_map <unsigned int, Node> verticesMap;

    Graph() = default;

    void AddEdgeG(unsigned int id1, unsigned int id2, unsigned int weight) {
        if (verticesMap.find(id1) == verticesMap.end()) {
            verticesMap.emplace(id1, Node(id1));
        }
        if (verticesMap.find(id2) == verticesMap.end()) {
            verticesMap.emplace(id2, Node(id2));
        }

        verticesMap[id1].AddEdgeN(id2, weight);
        verticesMap[id2].AddEdgeN(id1, weight);
    }

    void PrintGraph() {

    }
};

void ReadInput(std::string &inputPath, Graph &G) {
    unsigned int a, b, c;
    std::ifstream inputFile;
    inputFile.open(inputPath);

    while (!inputFile.eof() && !inputFile.fail()) {
        if (inputFile.peek() != '#') {
            inputFile >> a >> b >> c;
            G.AddEdgeG(a, b, c);
        }
    }
    inputFile.close();

}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "usage: "<<argv[0]<<" thread-count inputfile b-limit"<< std::endl;
        return 1;
    }

    int numThreads, limitB;
    std::string inputPath;
    Graph G;

    numThreads = std::stoi(argv[1]);
    inputPath = argv[2];
    limitB = std::stoi(argv[3]);

    ReadInput(inputPath, G);

    for (unsigned int method = 0; method <= limitB; method++) {

    }
}