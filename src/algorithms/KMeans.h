#pragma once
#include <vector>
#include <cmath>
#include <limits>
#include <iostream>
#include <random>
#include <algorithm>

struct CityNode {
    int64_t id;
    double lat;
    double lon;
    double population;
    int cluster = -1;
};

struct Centroid {
    double lat;
    double lon;
    int64_t nearest_node_id; 
};

class KMeans {
public:
    static double haversine(double lat1, double lon1, double lat2, double lon2) {
        constexpr double R = 6371000.0; 
        constexpr double TO_RAD = 3.14159265358979323846 / 180.0;
        double dLat = (lat2 - lat1) * TO_RAD;
        double dLon = (lon2 - lon1) * TO_RAD;
        double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(lat1 * TO_RAD) * std::cos(lat2 * TO_RAD) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return R * c;
    }

    static std::vector<Centroid> findHotspots(std::vector<CityNode>& nodes, int k, int max_iterations = 100) {
        if (nodes.empty() || k <= 0) return {};
        if (k > nodes.size()) k = nodes.size();

        std::vector<Centroid> centroids(k);
        std::random_device rd;
        std::mt19937 gen(rd());
        
        std::uniform_int_distribution<> dis(0, nodes.size() - 1);
        int first_idx = dis(gen);
        centroids[0].lat = nodes[first_idx].lat;
        centroids[0].lon = nodes[first_idx].lon;
        centroids[0].nearest_node_id = nodes[first_idx].id;

        std::vector<double> min_dist_sq(nodes.size(), std::numeric_limits<double>::max());

        for (int c = 1; c < k; ++c) {
            double total_weight = 0.0;
            for (size_t i = 0; i < nodes.size(); ++i) {
                double dist = haversine(nodes[i].lat, nodes[i].lon, centroids[c-1].lat, centroids[c-1].lon);
                double dist_sq = dist * dist;
                if (dist_sq < min_dist_sq[i]) min_dist_sq[i] = dist_sq;
                double weight = min_dist_sq[i] * std::max(0.1, nodes[i].population);
                total_weight += weight;
            }

            std::uniform_real_distribution<double> dist_random(0.0, total_weight);
            double target = dist_random(gen);
            double cumulative = 0.0;
            int next_idx = nodes.size() - 1; 

            for (size_t i = 0; i < nodes.size(); ++i) {
                double weight = min_dist_sq[i] * std::max(0.1, nodes[i].population);
                cumulative += weight;
                if (cumulative >= target) {
                    next_idx = i;
                    break;
                }
            }
            centroids[c].lat = nodes[next_idx].lat;
            centroids[c].lon = nodes[next_idx].lon;
            centroids[c].nearest_node_id = nodes[next_idx].id;
        }

        bool changed = true;
        int iterations = 0;

        while (changed && iterations < max_iterations) {
            changed = false;
            iterations++;

            for (auto& node : nodes) {
                double min_dist = std::numeric_limits<double>::max();
                int best_cluster = 0;
                for (int i = 0; i < k; ++i) {
                    double dist = haversine(node.lat, node.lon, centroids[i].lat, centroids[i].lon);
                    if (dist < min_dist) {
                        min_dist = dist;
                        best_cluster = i;
                    }
                }
                if (node.cluster != best_cluster) {
                    node.cluster = best_cluster;
                    changed = true;
                }
            }

            std::vector<double> sum_lat(k, 0.0);
            std::vector<double> sum_lon(k, 0.0);
            std::vector<double> sum_pop(k, 0.0);

            for (const auto& node : nodes) {
                int c = node.cluster;
                double weight = std::max(0.1, node.population); 
                sum_lat[c] += node.lat * weight;
                sum_lon[c] += node.lon * weight;
                sum_pop[c] += weight;
            }

            for (int i = 0; i < k; ++i) {
                if (sum_pop[i] > 0) {
                    centroids[i].lat = sum_lat[i] / sum_pop[i];
                    centroids[i].lon = sum_lon[i] / sum_pop[i];
                }
            }
        }

        for (int i = 0; i < k; ++i) {
            double min_dist = std::numeric_limits<double>::max();
            for (const auto& node : nodes) {
                double dist = haversine(node.lat, node.lon, centroids[i].lat, centroids[i].lon);
                if (dist < min_dist) {
                    min_dist = dist;
                    centroids[i].nearest_node_id = node.id;
                }
            }
        }

        return centroids;
    }
};