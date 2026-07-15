#pragma once
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include "../graph/Graph.h"

using json = nlohmann::json;

class Exporter {
public:
    static void exportNetwork(const Graph& graph, const std::string& filename) {
        std::cout << "Exporting final network to " << filename << "..." << std::endl;
        json output;
        output["nodes"] = json::array();
        output["edges"] = json::array();

        // Dump all nodes and their coordinates
        for (const auto& pair : graph.getNodes()) {
            const Node& n = pair.second;
            output["nodes"].push_back({
                {"id", n.id},
                {"lat", n.lat},
                {"lon", n.lon}
            });
        }

        // Dump all edges, including our new highways and the traffic flow data
        for (const auto& edge : graph.getAllEdges()) {
            output["edges"].push_back({
                {"src", edge.src},
                {"dst", edge.dst},
                {"road_type", edge.road_type},
                {"flow", edge.current_flow},
                {"capacity", edge.capacity}
            });
        }

        std::ofstream file(filename);
        file << output.dump(2);
        file.close();
        std::cout << "[SUCCESS] Map data exported successfully!" << std::endl;
    }
};