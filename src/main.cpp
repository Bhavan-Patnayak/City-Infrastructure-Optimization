#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include "graph/Graph.h"
#include "algorithms/KMeans.h"
#include "algorithms/Kruskal.h"
#include "algorithms/TrafficSimulator.h"
#include "utils/Exporter.h"

using json = nlohmann::json;

// Loads nodes and edges from the processed OSM dataset and inserts them into our graph structure
std::vector<CityNode> loadNetworkData(Graph& city_graph, const std::string& nodes_path, const std::string& edges_path) {
    std::cout << "Loading real-world mapping data..." << std::endl;
    std::vector<CityNode> raw_nodes;

    // --- 1. Load Nodes ---
    std::ifstream nodes_file(nodes_path);
    if (!nodes_file.is_open()) throw std::runtime_error("Could not open nodes file: " + nodes_path);
    
    json nodes_json = json::parse(nodes_file);
    for (const auto& item : nodes_json) {
        // Fall back to 0.0 if the node lacks a population density property
        double pop = item.contains("population_density") ? item["population_density"].get<double>() : 0.0;
        int64_t id = item["id"].get<int64_t>();
        double lat = item["lat"].get<double>();
        double lon = item["lon"].get<double>();

        city_graph.addNode(id, lat, lon, pop);
        raw_nodes.push_back({id, lat, lon, pop, -1});
    }
    std::cout << "Successfully loaded " << raw_nodes.size() << " intersections." << std::endl;

    // --- 2. Load Edges ---
    std::ifstream edges_file(edges_path);
    if (!edges_file.is_open()) throw std::runtime_error("Could not open edges file: " + edges_path);
    
    json edges_json = json::parse(edges_file);
    int edge_count = 0;
    for (const auto& item : edges_json) {
        if (city_graph.hasNode(item["src"].get<int64_t>()) && city_graph.hasNode(item["dst"].get<int64_t>())) {
            
            // Clean up road type attributes returned by OSM (which are sometimes parsed as JSON arrays)
            std::string parsed_road_type = "residential";
            if (item["road_type"].is_array() && !item["road_type"].empty()) {
                parsed_road_type = item["road_type"][0].get<std::string>();
            } else if (item["road_type"].is_string()) {
                parsed_road_type = item["road_type"].get<std::string>();
            }

            city_graph.addEdge(
                item["src"].get<int64_t>(),
                item["dst"].get<int64_t>(),
                item["length_m"].get<double>(),
                parsed_road_type,
                item["capacity"].get<double>()
            );
            edge_count++;
        }
    }
    std::cout << "Successfully loaded " << edge_count << " drivable road segments." << std::endl;
    return raw_nodes;
}

int main() {
    std::cout << "\n=== City Infrastructure Network Design System ===" << std::endl;
    std::cout << "Target Area: Kukatpally, Hyderabad" << std::endl;
    
    Graph city_graph;

    try {
        // Use coordinates enriched with the extracted WorldPop density map
        std::string nodes_file = "../data/processed/nodes_with_density.json";
        std::string edges_file = "../data/processed/edges.json";
        
        std::vector<CityNode> nodes_list = loadNetworkData(city_graph, nodes_file, edges_file);
        
        city_graph.printSummary();

        // ---------------------------------------------------------
        // PHASE 1: DEMAND CLUSTERING (K-Means++)
        // ---------------------------------------------------------
        std::cout << "\n--- Phase 1: Identifying Gravity Centers ---" << std::endl;
        std::cout << "Running Population-Weighted K-Means++ Clustering..." << std::endl;
        
        int k_hubs = 5; 
        std::vector<Centroid> hotspots = KMeans::findHotspots(nodes_list, k_hubs);
        
        std::cout << "\n[SUCCESS] Identified " << k_hubs << " major population hotspots:\n";
        for (int i = 0; i < hotspots.size(); i++) {
            std::cout << "  Hub " << i+1 << " -> Snap to Intersection ID: " << hotspots[i].nearest_node_id 
                      << " (Lat: " << hotspots[i].lat << ", Lon: " << hotspots[i].lon << ")\n";
        }
        
        // ---------------------------------------------------------
        // PHASE 2: BUDGET-CONSCIOUS BACKBONE (MST)
        // ---------------------------------------------------------
        std::cout << "\n--- Phase 2: Building the Highway Backbone ---" << std::endl;
        std::cout << "Running Kruskal's Algorithm (Union-Find)..." << std::endl;
        
        std::vector<HighwayEdge> mst_highways = Kruskal::buildMST(hotspots);
        
        double total_mst_cost = 0;
        for (const auto& hw : mst_highways) {
            std::cout << "  [BUILD] Highway from Hub " << hw.src_idx + 1 
                      << " to Hub " << hw.dst_idx + 1 
                      << " | Length: " << hw.cost_distance << " meters\n";
            total_mst_cost += hw.cost_distance;
            
            // Inject new double-lane highways directly into the city network graph
            city_graph.addEdge(hw.src_node_id, hw.dst_node_id, hw.cost_distance, "highway_mst", 5000.0);
            city_graph.addEdge(hw.dst_node_id, hw.src_node_id, hw.cost_distance, "highway_mst", 5000.0);
        }
        
        std::cout << "\n[SUCCESS] MST Complete. Total New Highway Infrastructure: " 
                  << (total_mst_cost / 1000.0) << " km." << std::endl;

        // ---------------------------------------------------------
        // PHASE 3: THE OUTER RING ROAD (ORR)
        // ---------------------------------------------------------
        std::cout << "\n--- Phase 3: Designing the Outer Ring Road ---" << std::endl;
        std::cout << "Calculating optimal bypass corridors to prevent MST gridlock..." << std::endl;
        
        std::vector<HighwayEdge> ring_roads = Kruskal::buildRingRoad(hotspots, mst_highways);
        
        double total_orr_cost = 0;
        for (const auto& hw : ring_roads) {
            std::cout << "  [BUILD] Ring Road from Hub " << hw.src_idx + 1 
                      << " to Hub " << hw.dst_idx + 1 
                      << " | Length: " << hw.cost_distance << " meters\n";
            total_orr_cost += hw.cost_distance;
            
            // Add bypass Ring Roads to handle heavy regional vehicle flow
            city_graph.addEdge(hw.src_node_id, hw.dst_node_id, hw.cost_distance, "highway_ring", 8000.0);
            city_graph.addEdge(hw.dst_node_id, hw.src_node_id, hw.cost_distance, "highway_ring", 8000.0);
        }

        std::cout << "\n[SUCCESS] Ring Road Complete. Added " 
                  << (total_orr_cost / 1000.0) << " km of bypass infrastructure." << std::endl;

        // ---------------------------------------------------------
        // PHASE 4: TRAFFIC FLOW SIMULATION (A*)
        // ---------------------------------------------------------
        std::cout << "\n--- Phase 4: Stress-Testing the Network ---" << std::endl;
        // Simulating 50,000 commuters traversing the network toward gravity centers
        TrafficSimulator::simulateTraffic(city_graph, nodes_list, hotspots, 50000);
        
        std::cout << "\n[SUCCESS] Traffic Simulation Complete! Graph flows updated." << std::endl;

        // ---------------------------------------------------------
        // PHASE 6: GEOSPATIAL EXPORT
        // ---------------------------------------------------------
        std::cout << "\n--- Phase 6: Exporting for Visualization ---" << std::endl;
        // Leverages the exporter tool created in the Canvas document
        Exporter::exportNetwork(city_graph, "../data/processed/final_network.json");

    } catch (const std::exception& e) {
        std::cerr << "CRITICAL ERROR: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}