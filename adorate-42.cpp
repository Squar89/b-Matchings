#include "blimit.hpp"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <set>
#include <mutex>
#include <algorithm>
#include <limits>
#include <thread>

typedef std::pair <unsigned int, unsigned int> edge_t;//<node_id, weight>
typedef std::vector <edge_t>::iterator edgesVecIt_t;

//return left :>: right
bool Greater(const edge_t left, const edge_t right) {
    return (left.second > right.second) || (left.second == right.second && left.first > right.first);
}

struct EdgesComparator {
    bool operator()(const edge_t left, const edge_t right) {
        return Greater(left, right);
    }
};

class Node {
private:
    unsigned int id;
    unsigned int b;
    unsigned int p;
    bool sorted;
public:
    unsigned int matchedCount;
    unsigned int replacedCount;
    std::vector <edge_t> nodeEdges;
    std::set <edge_t, EdgesComparator> matchedEdges;
    std::mutex alterMatched;
    edgesVecIt_t current, sortedEnd;

    Node() : id(0), b(0), p(0), sorted(false), matchedCount(0), replacedCount(0) {}

    Node(Node&& other) noexcept {
        id = other.id;
        b = other.b;
        p = other.p;
        sorted = other.sorted;
        matchedCount = other.matchedCount;
        replacedCount = other.replacedCount;
        nodeEdges = std::move(other.nodeEdges);
        matchedEdges = std::move(other.matchedEdges);
    }

    explicit Node(unsigned int x) : id(x), b(0), p(0), sorted(false), matchedCount(0), replacedCount(0) {}

    bool operator==(const Node &other) const {
        return id == other.id;
    }

    void AddEdgeN(unsigned int neighbourId, unsigned int weight) {
        nodeEdges.emplace_back(neighbourId, weight);
    }

    void UpdateBValue(unsigned int method) {
        b = bvalue(method, id);
        p = 7 * b;
    }

    void ClearStructures() {
        matchedCount = 0;
        replacedCount = 0;
        matchedEdges.clear();
    }

    void SetIterators() {
        current = nodeEdges.begin();
        if (!sorted) {
            sortedEnd = nodeEdges.begin();
        }
    }

    void SortEdges() {
        if (nodeEdges.empty()) {
            return;
        }

        if (p < std::distance(sortedEnd, nodeEdges.end())) {
            auto temp = sortedEnd;
            std::advance(sortedEnd, p);
            std::partial_sort(temp, sortedEnd, nodeEdges.end(), Greater);
        }
        else {
            std::sort(sortedEnd, nodeEdges.end(), Greater);
            sortedEnd = nodeEdges.end();
        }

        sorted = true;
    }

    unsigned int GetSum() {
        unsigned int result = 0;

        for (auto edge : matchedEdges) {
            result += edge.second;
        }

        return result;
    }

    unsigned int GetId() {
        return id;
    }

    unsigned int GetB() {
        return b;
    }

    /*
    void printNode() {
        std::cout << "Node id: " << id << std::endl;
        for (auto &edge : nodeEdges) {
            std::cout << "    -> " << edge.first << " weight: " << edge.second << std::endl;
        }
    }
    */
};

class Graph {
public:
    std::unordered_map <unsigned int, Node> verticesMap;
    std::vector <Node*> que, tempQue;
    std::mutex replace, queMutex;

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
        que.clear();
        tempQue.clear();

        for (auto& vertex : verticesMap) {
            vertex.second.ClearStructures();
            vertex.second.UpdateBValue(method);
            vertex.second.SetIterators();
            que.push_back(&vertex.second);
        }
    }

    void ProcessQueue(unsigned int begin, unsigned int end) {
        std::vector <Node*> threadQue;

        for (int i = begin; i < end; i++) {
            auto vertex = que[i];

            while (vertex->matchedCount < vertex->GetB() && vertex->current != vertex->nodeEdges.end()) {
                if (vertex->current == vertex->sortedEnd) {
                    vertex->SortEdges();
                }

                auto &candidate = verticesMap.at(vertex->current->first);
                edge_t proposedEdge = std::make_pair(vertex->GetId(), vertex->current->second);

                {
                    std::lock_guard<std::mutex> matchedLock(candidate.alterMatched);
                    if (candidate.GetB() > 0 && (candidate.matchedEdges.size() < candidate.GetB()
                                                 || Greater(proposedEdge, *(candidate.matchedEdges.rbegin())))) {
                        if (candidate.matchedEdges.size() < candidate.GetB()) {
                            vertex->matchedCount++;
                            candidate.matchedEdges.emplace(vertex->GetId(), vertex->current->second);
                        }
                        else if (Greater(proposedEdge, *(candidate.matchedEdges.rbegin()))) {
                            vertex->matchedCount++;

                            std::lock_guard<std::mutex> replaceLock(replace);
                            auto &replacedNode = verticesMap.at((candidate.matchedEdges.rbegin())->first);
                            if (replacedNode.replacedCount == 0) {
                                threadQue.push_back(&replacedNode);
                            }
                            replacedNode.replacedCount++;

                            candidate.matchedEdges.erase(--candidate.matchedEdges.end());
                            candidate.matchedEdges.emplace(vertex->GetId(), vertex->current->second);
                        }
                    }
                }
                (vertex->current)++;
            }
        }

        if (!threadQue.empty()) {
            std::lock_guard<std::mutex> queueLock(queMutex);
            for (auto i : threadQue) {
                tempQue.push_back(i);
            }
        }
    }

    unsigned int SuitorAlgorithm(int numberOfThreads) {
        unsigned int result = 0;
        std::vector <std::thread> threads;

        while (!que.empty()) {
            unsigned int startPosition = 0;
            unsigned int portionSize = que.size()/numberOfThreads;

            if (portionSize == 0) {
                portionSize = 1;
            }

            for (unsigned int i = 0; i < numberOfThreads - 1; i++) {
                threads.push_back(std::thread{[this, startPosition, portionSize]
                                              {ProcessQueue(startPosition, startPosition + portionSize);}});
                startPosition += portionSize;
                if (startPosition == que.size() - 1) {
                    break;
                }
            }
            ProcessQueue(startPosition, que.size());

            while (!threads.empty()) {
                threads[threads.size() - 1].join();
                threads.pop_back();
            }

            que.clear();
            for (auto vertex : tempQue) {
                vertex->matchedCount -= vertex->replacedCount;
                vertex->replacedCount = 0;
                que.push_back(vertex);
            }
            tempQue.clear();
        }

        for (auto &vertex : verticesMap) {
            result += vertex.second.GetSum();
        }

        return result / 2;
    }

    /*
    void PrintGraph() {
        std::cout << "Printing graph of " << verticesMap.size() << " nodes:\n";
        for (auto &vertex : verticesMap) {
            vertex.second.printNode();
        }
    }
    */
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

        result = G.SuitorAlgorithm(numThreads);

        printf("%d\n", result);
        // this is just to show the blimit with which the program is linked
        // std::cerr << "bvalue node 44: " << bvalue(method, 44) << std::endl;
    }
}