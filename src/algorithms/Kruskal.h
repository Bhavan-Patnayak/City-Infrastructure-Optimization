#pragma once
#include <vector>
#include <algorithm>
#include "KMeans.h" 

struct HighwayEdge {
    int src_idx;
    int dst_idx;
    double cost_distance;
    int64_t src_node_id;
    int64_t dst_node_id;

    bool operator<(const HighwayEdge& other) const {
        return cost_distance < other.cost_distance;
    }
};

class DisjointSet {
    std::vector<int> parent;
    std::vector<int> rank;

public:
    DisjointSet(int n) {
        parent.resize(n);
        rank.resize(n, 0);
        for (int i = 0; i < n; ++i) {
            parent[i] = i;
        }
    }

    int find(int i) {
        if (parent[i] == i)
            return i;
        return parent[i] = find(parent[i]);
    }

    void unite(int i, int j) {
        int root_i = find(i);
        int root_j = find(j);

        if (root_i != root_j) {
            if (rank[root_i] < rank[root_j]) {
                parent[root_i] = root_j;
            } else if (rank[root_i] > rank[root_j]) {
                parent[root_j] = root_i;
            } else {
                parent[root_j] = root_i;
                rank[root_i]++;
            }
        }
    }
};

class Kruskal {
public:
    static std::vector<HighwayEdge> buildMST(const std::vector<Centroid>& hubs) {
        std::vector<HighwayEdge> all_possible_highways;
        int n = hubs.size();

        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                double dist = KMeans::haversine(hubs[i].lat, hubs[i].lon, hubs[j].lat, hubs[j].lon);
                all_possible_highways.push_back({
                    i, j, dist, hubs[i].nearest_node_id, hubs[j].nearest_node_id
                });
            }
        }

        std::sort(all_possible_highways.begin(), all_possible_highways.end());

        std::vector<HighwayEdge> mst;
        DisjointSet ds(n);

        for (const auto& edge : all_possible_highways) {
            if (ds.find(edge.src_idx) != ds.find(edge.dst_idx)) {
                mst.push_back(edge);
                ds.unite(edge.src_idx, edge.dst_idx);
                if (mst.size() == n - 1) break;
            }
        }

        return mst;
    }

    static std::vector<HighwayEdge> buildRingRoad(const std::vector<Centroid>& hubs, const std::vector<HighwayEdge>& mst) {
        std::vector<HighwayEdge> all_possible_highways;
        int n = hubs.size();

        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                double dist = KMeans::haversine(hubs[i].lat, hubs[i].lon, hubs[j].lat, hubs[j].lon);
                all_possible_highways.push_back({
                    i, j, dist, hubs[i].nearest_node_id, hubs[j].nearest_node_id
                });
            }
        }

        std::vector<HighwayEdge> rejected_edges;
        for (const auto& edge : all_possible_highways) {
            bool in_mst = false;
            for (const auto& mst_edge : mst) {
                if ((edge.src_idx == mst_edge.src_idx && edge.dst_idx == mst_edge.dst_idx) ||
                    (edge.src_idx == mst_edge.dst_idx && edge.dst_idx == mst_edge.src_idx)) {
                    in_mst = true;
                    break;
                }
            }
            if (!in_mst) {
                rejected_edges.push_back(edge);
            }
        }

        std::sort(rejected_edges.begin(), rejected_edges.end(), [](const HighwayEdge& a, const HighwayEdge& b) {
            return a.cost_distance > b.cost_distance; 
        });

        std::vector<HighwayEdge> ring_roads;
        int edges_to_add = std::min(2, (int)rejected_edges.size());
        for (int i = 0; i < edges_to_add; ++i) {
            ring_roads.push_back(rejected_edges[i]);
        }

        return ring_roads;
    }
};