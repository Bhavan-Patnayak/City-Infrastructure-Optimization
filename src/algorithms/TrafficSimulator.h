#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <future>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include "KMeans.h"
#include "../graph/Graph.h"

struct Commuter {
    int64_t start_node;
    int64_t target_hub;
    std::vector<int64_t> planned_route;
};

// Priority Queue state for the A* search
struct AStarState {
    int64_t id;
    double f_score; // g_score (time) + h_score (optimistic time to target)
    
    // We want the priority queue to pop the LOWEST f_score first
    bool operator>(const AStarState& other) const {
        return f_score > other.f_score;
    }
};

class TrafficSimulator {
public:
    // This runs on multiple threads simultaneously.
    // It passes Graph by reference but treats it as Read-Only to prevent race conditions.
    static std::vector<int64_t> runAStar(Graph& graph, int64_t start, int64_t target) {
        if (!graph.hasNode(start) || !graph.hasNode(target)) return {};

        std::priority_queue<AStarState, std::vector<AStarState>, std::greater<AStarState>> open_set;
        std::unordered_map<int64_t, double> g_score;
        std::unordered_map<int64_t, int64_t> came_from;

        open_set.push({start, 0.0});
        g_score[start] = 0.0;

        // Cache the target coordinates so we don't look them up thousands of times inside the loop
        double target_lat = graph.getNode(target).lat;
        double target_lon = graph.getNode(target).lon;

        while (!open_set.empty()) {
            AStarState current = open_set.top();
            open_set.pop();

            if (current.id == target) {
                // We reached the Hub! Reconstruct the exact route taken.
                std::vector<int64_t> path;
                int64_t curr = target;
                while (curr != start) {
                    path.push_back(curr);
                    curr = came_from[curr];
                }
                path.push_back(start);
                std::reverse(path.begin(), path.end());
                return path;
            }

            for (const auto& edge : graph.getNeighbors(current.id)) {
                int64_t neighbor = edge.dst;
                
                // g_score is the REAL travel time taking current traffic congestion into account
                double tentative_g = g_score[current.id] + edge.get_travel_time_seconds();

                if (g_score.find(neighbor) == g_score.end() || tentative_g < g_score[neighbor]) {
                    came_from[neighbor] = current.id;
                    g_score[neighbor] = tentative_g;
                    
                    // h_score is an optimistic guess of remaining time. 
                    // Haversine Distance (meters) / 27.78 m/s (100km/h max speed) = Optimistic Seconds
                    double dist_to_target = KMeans::haversine(
                        graph.getNode(neighbor).lat, graph.getNode(neighbor).lon,
                        target_lat, target_lon
                    );
                    double h_score = dist_to_target / 27.78; 
                    
                    open_set.push({neighbor, tentative_g + h_score});
                }
            }
        }
        return {}; // No path found
    }

    static void simulateTraffic(Graph& city_graph, const std::vector<CityNode>& residential_nodes, const std::vector<Centroid>& hubs, int total_cars = 100000) {
        std::cout << "Initializing Batch-Parallel Traffic Simulation for " << total_cars << " commuters..." << std::endl;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> node_dist(0, residential_nodes.size() - 1);
        std::uniform_int_distribution<> hub_dist(0, hubs.size() - 1);

        int batch_size = 1000;
        int cars_processed = 0;

        while (cars_processed < total_cars) {
            int current_batch = std::min(batch_size, total_cars - cars_processed);
            std::vector<std::future<std::vector<int64_t>>> futures;

            // 1. DISPATCH BATCH (Multi-threaded)
            for (int i = 0; i < current_batch; ++i) {
                int64_t start = residential_nodes[node_dist(gen)].id;
                int64_t target = hubs[hub_dist(gen)].nearest_node_id;
                
                // std::async pulls from the Windows Thread Pool to run A* concurrently
                futures.push_back(std::async(std::launch::async, runAStar, std::ref(city_graph), start, target));
            }

            // 2. COLLECT ROUTES & UPDATE GRAPH (Single-threaded to prevent race conditions)
            for (auto& f : futures) {
                std::vector<int64_t> route = f.get();
                if (route.empty()) continue;

                // Loop through the returned route and inject traffic into the graph
                for (size_t i = 0; i < route.size() - 1; ++i) {
                    int64_t u = route[i];
                    int64_t v = route[i + 1];

                    // Safely modify the current flow on the main thread
                    for (const Edge& edge : city_graph.getNeighbors(u)) {
                        if (edge.dst == v) {
                            const_cast<Edge&>(edge).current_flow += 1.0;
                            break;
                        }
                    }
                    // Do the reverse edge as well, since streets are two-way
                    for (const Edge& edge : city_graph.getNeighbors(v)) {
                        if (edge.dst == u) {
                            const_cast<Edge&>(edge).current_flow += 1.0;
                            break;
                        }
                    }
                }
            }

            cars_processed += current_batch;
            if (cars_processed % 10000 == 0) {
                std::cout << "  [" << cars_processed << "/" << total_cars << "] Commuters routed..." << std::endl;
            }
        }
    }
};