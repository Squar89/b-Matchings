#include "blimit.hpp"
#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>

typedef std::pair <unsigned int, unsigned int> edge_t;//<node_id, weight>
typedef std::vector <edge_t>::iterator edgesVecIt_t;

//return left :>: right
bool Greater(const edge_t left, const edge_t right) {
    return (left.second > right.second) || (left.second == right.second && left.first > right.first);
}

class Node {
private:
    unsigned int id;
    unsigned int b;
    unsigned long long p;
public:
    unsigned int matchedCount;
    unsigned int replacedCount;
    unsigned long long sortedItPosition;
    std::vector <edge_t> nodeEdges;
    std::vector <unsigned int> seenNodes;//TODO wykorzystaj lub usuń
    std::vector <edge_t> matchedEdges;
    std::mutex alterMatched;
    edgesVecIt_t current, sortedEnd, end;

    Node() : id(0), b(0), p(0), matchedCount(0), replacedCount(0), sortedItPosition(0) {}

    Node(Node&& other) noexcept {
        id = other.id;
        b = other.b;
        p = other.p;
        matchedCount = other.matchedCount;
        replacedCount = other.replacedCount;
        sortedItPosition = other.sortedItPosition;
        nodeEdges = std::move(other.nodeEdges);
        matchedEdges = std::move(other.matchedEdges);
        seenNodes = std::move(other.seenNodes);
    }

    explicit Node(unsigned int x) : id(x), b(0), p(0), matchedCount(0), replacedCount(0), sortedItPosition(0) {}

    bool operator==(const Node &other) const {
        return id == other.id;
    }

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
        matchedCount = 0;
        replacedCount = 0;
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

    void SortEdges() {
        if (sortedItPosition + p <= nodeEdges.size()) {
            std::partial_sort(sortedEnd, sortedEnd + p, end, Greater);
            sortedItPosition += p;
        }
        else {
            std::partial_sort(sortedEnd, end, end, Greater);
            sortedItPosition = nodeEdges.size();
        }
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
};

class Graph {
public:
    std::unordered_map <unsigned int, Node> verticesMap;
    std::vector <Node*> que, tempQue;
    std::mutex replace;

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

    unsigned int SuitorAlgorithm() {
        unsigned int result = 0;
        while (!que.empty()) {
            for (auto vertex : que) {
                while (vertex->matchedCount < vertex->GetB() && vertex->current != vertex->end) {
                    if (vertex->current == vertex->sortedEnd) {
                        vertex->SortEdges();
                    }

                    auto &candidate = verticesMap.at(vertex->current->first);//TODO dodaj seenNodes żeby ogarniać multiset?
                    edge_t proposedEdge = std::make_pair(vertex->GetId(), vertex->current->second);

                    if (candidate.GetB() > 0 && (candidate.matchedEdges.size() < candidate.GetB()
                        || Greater(proposedEdge, candidate.matchedEdges.at(candidate.GetB() - 1)))) {
                        std::lock_guard<std::mutex> matchedLock(candidate.alterMatched);

                        if (candidate.matchedEdges.size() < candidate.GetB()) {
                            vertex->matchedCount++;
                            candidate.matchedEdges.emplace_back(vertex->GetId(), vertex->current->second);
                        } else if (Greater(proposedEdge, candidate.matchedEdges.at(candidate.GetB() - 1))) {
                            vertex->matchedCount++;

                            std::lock_guard<std::mutex> replaceLock(replace);
                            auto &replacedNode = verticesMap.at(candidate.matchedEdges[candidate.GetB() - 1].first);
                            if (replacedNode.replacedCount == 0) {
                                tempQue.push_back(&replacedNode);//TODO stwórz oddzielną kolejke dla każdego wątku
                            }
                            replacedNode.replacedCount++;

                            candidate.matchedEdges[candidate.GetB() - 1] = std::make_pair(vertex->GetId(),
                                                                                          vertex->current->second);
                        }
                    }

                    (vertex->current)++;
                }
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
    printf("Done reading input\n");

    for (unsigned int method = 0; method <= limitB; method++) {
        G.SetupAlgorithm(method);

        result = G.SuitorAlgorithm();

        printf("%d\n", result);
        // this is just to show the blimit with which the program is linked
        // std::cerr << "bvalue node 44: " << bvalue(method, 44) << std::endl;
    }
}