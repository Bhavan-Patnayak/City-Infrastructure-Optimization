#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>

struct Node {
    int64_t id;
    double lat;
    double lon;
    double population_density = 0.0;
    int cluster_id = -1; // -1 means unassigned initially
};

struct Edge {
    int64_t src;
    int64_t dst;
    double length_m;
    std::string road_type; // "local", "highway", "orr"
    double capacity;       // vehicles / hour
    double current_flow = 0.0;

    // Time-based weight calculation for routing calculations
    double get_travel_time_seconds() const {
        double speed_ms = 8.33; // Default local road: 30 km/h (~8.33 m/s)
        if (road_type == "highway") speed_ms = 22.22; // 80 km/h
        else if (road_type == "orr") speed_ms = 27.78; // 100 km/h

        double free_flow_time = length_m / speed_ms;

        // Apply congestion penalty if flow exists
        if (current_flow > 0.0 && capacity > 0.0) {
            double utilization = current_flow / capacity;
            return free_flow_time * (1.0 + 0.15 * std::pow(utilization, 4));
        }
        return free_flow_time;
    }
};

class Graph {
private:
    std::unordered_map<int64_t, Node> nodes;
    std::unordered_map<int64_t, std::vector<Edge>> adjacency_list;

public:
    Graph() = default;

    // Node operations
    void addNode(int64_t id, double lat, double lon, double population_density = 0.0);
    bool hasNode(int64_t id) const;
    Node& getNode(int64_t id);
    const std::unordered_map<int64_t, Node>& getNodes() const;

    // Edge operations
    void addEdge(int64_t src, int64_t dst, double length_m, const std::string& road_type, double capacity);
    const std::vector<Edge>& getNeighbors(int64_t node_id) const;
    std::vector<Edge> getAllEdges() const;

    // Utility
    size_t nodeCount() const;
    size_t edgeCount() const;
    void clear();
    void printSummary() const;
};

#endif // GRAPH_H