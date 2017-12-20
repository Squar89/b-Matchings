#include "blimit.hpp"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>

typedef std::pair <unsigned int, unsigned int> edge_t;//<node_id, weight>
typedef std::vector <edge_t>::iterator edgesVecIt_t;

//return left :>: right
bool Greater(edge_t left, edge_t right) {
    return (left.second > right.second) || (left.second == right.second && left.first > right.first);
}

class Node {
private:
    unsigned int id;
    unsigned int b;
    unsigned long long p;
    unsigned long long sortedItPosition;
    std::vector <edge_t> nodeEdges;
    std::vector <unsigned int> seenNodes;
    std::vector <edge_t> matchedEdges;
public:
    std::mutex alterMatched;
    edgesVecIt_t current, sortedEnd, end;

    Node() : id(0), b(0), p(0), sortedItPosition(0) {}

    Node(Node&& other) noexcept {
        id = other.id;
        b = other.b;
        p = other.p;
        sortedItPosition = other.sortedItPosition;
        nodeEdges = std::move(other.nodeEdges);
        matchedEdges = std::move(other.matchedEdges);
        seenNodes = std::move(other.seenNodes);
    }

    explicit Node(unsigned int x) : id(x), b(0), p(0), sortedItPosition(0) {}

    void AddEdgeN(unsigned int neighbourId, unsigned int weight) {
        nodeEdges.emplace_back(neighbourId, weight);
    }

    void printNode() {
        std::cout << "Node id: " << id << std::endl;
        for (auto &edge : nodeEdges) {
            std::cout << "    -> " << edge.first << " weight: " << edge.second << std::endl;
        }
    }

    void UpdateBValue(unsigned int method) {
        b = bvalue(method, id);
        p = 7 * b;
    }

    void ClearStructures() {
        seenNodes.clear();
        matchedEdges.clear();
    }

    void SetIterators() {
        current = nodeEdges.begin();
        end = nodeEdges.end();
        if (sortedItPosition == 0) {
            sortedEnd = nodeEdges.begin();
        }
    }

    void SortEdges() {//TODO wywołuj tą funkcję w algorytmie, nie w przygotowaniach do niego
        if (sortedItPosition + p <= nodeEdges.size()) {
            std::partial_sort(sortedEnd, sortedEnd + p, end, Greater);
            sortedItPosition += p;
        }
        else {
            std::partial_sort(sortedEnd, end, end, Greater);
            sortedItPosition = nodeEdges.size();
        }
    }
};

class Graph {
public:
    std::unordered_map <unsigned int, Node> verticesMap;
    std::vector <Node*> que;

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

    void SetupAlgorithm(unsigned int method) {
        for (auto &vertex : verticesMap) {
            vertex.second.ClearStructures();
            vertex.second.UpdateBValue(method);
            vertex.second.SetIterators();
            que.push_back(&vertex.second);
        }
    }

    unsigned int SuitorAlgorithm() {//TODO
        return 0;
    }

    void PrintGraph() {
        std::cout << "Printing graph of " << verticesMap.size() << " nodes:\n";
        for (auto &vertex : verticesMap) {
            vertex.second.printNode();
        }
    }
};

void ReadInput(std::string &inputPath, Graph &G) {
    unsigned int a, b, c;
    std::ifstream inputFile;
    inputFile.open(inputPath);

    while (!inputFile.eof()) {
        if (inputFile.peek() == '#') {
            inputFile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
        else {
            inputFile >> a >> b >> c;
            if (!inputFile.fail()) {
                G.AddEdgeG(a, b, c);
            }
        }
    }
    inputFile.close();

}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "usage: "<< argv[0] <<" thread-count inputfile b-limit"<< std::endl;
        return 1;
    }

    int numThreads, limitB;
    unsigned int result;
    std::string inputPath;
    Graph G;

    numThreads = std::stoi(argv[1]);
    inputPath = argv[2];
    limitB = std::stoi(argv[3]);

    ReadInput(inputPath, G);

    for (unsigned int method = 0; method <= limitB; method++) {
        G.SetupAlgorithm(method);

        result = G.SuitorAlgorithm();

        printf("%d\n", result);
        // this is just to show the blimit with which the program is linked
        // std::cerr << "bvalue node 44: " << bvalue(method, 44) << std::endl;
    }
}