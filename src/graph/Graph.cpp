#include "Graph.h"
#include <stdexcept>

void Graph::addNode(int64_t id, double lat, double lon, double population_density) {
    nodes[id] = Node{id, lat, lon, population_density, -1};
    // Ensure node tracking key exists in adjacency table map
    if (adjacency_list.find(id) == adjacency_list.end()) {
        adjacency_list[id] = std::vector<Edge>();
    }
}

bool Graph::hasNode(int64_t id) const {
    return nodes.find(id) != nodes.end();
}

Node& Graph::getNode(int64_t id) {
    auto it = nodes.find(id);
    if (it == nodes.end()) {
        throw std::runtime_error("Node ID not found in current graph context.");
    }
    return it->second;
}

const std::unordered_map<int64_t, Node>& Graph::getNodes() const {
    return nodes;
}

void Graph::addEdge(int64_t src, int64_t dst, double length_m, const std::string& road_type, double capacity) {
    // Spatial integrity verification
    if (!hasNode(src) || !hasNode(dst)) {
        throw std::runtime_error("Cannot bind edge connections to non-existent nodes.");
    }
    
    Edge edge{src, dst, length_m, road_type, capacity, 0.0};
    adjacency_list[src].push_back(edge);
}

const std::vector<Edge>& Graph::getNeighbors(int64_t node_id) const {
    auto it = adjacency_list.find(node_id);
    if (it == adjacency_list.end()) {
        throw std::runtime_error("Requested node target does not exist in adjacency layout map.");
    }
    return it->second;
}

std::vector<Edge> Graph::getAllEdges() const {
    std::vector<Edge> edges;
    for (const auto& [src_id, edge_list] : adjacency_list) {
        for (const auto& edge : edge_list) {
            edges.push_back(edge);
        }
    }
    return edges;
}

size_t Graph::nodeCount() const {
    return nodes.size();
}

size_t Graph::edgeCount() const {
    size_t count = 0;
    for (const auto& [src_id, edge_list] : adjacency_list) {
        count += edge_list.size();
    }
    return count;
}

void Graph::clear() {
    nodes.clear();
    adjacency_list.clear();
}

void Graph::printSummary() const {
    std::cout << "\n=== Graph Diagnostics Summary ===" << std::endl;
    std::cout << " Total Intersections (Nodes): " << nodeCount() << std::endl;
    std::cout << " Total Road Segments (Edges): " << edgeCount() << std::endl;
    std::cout << "=================================\n" << std::endl;
}