# 🏙️ Kukatpally Urban Network Redesign

### AI-Driven Infrastructure Optimization & Micro-Simulation Engine

A high-performance C++ and Python pipeline that mathematically remodels the urban infrastructure of Kukatpally, Hyderabad. By combining real-world OpenStreetMap (OSM) data with satellite demographics, this project uses graph theory algorithms (K-Means++, Kruskal's MST) to design optimal highway backbones and stress-tests them with a multithreaded A* traffic simulator.

---

## 🚀 Key Features & Algorithms

*   **Geospatial Data Pipeline:** Maps a 1.8 GB WorldPop satellite raster dataset onto 4,200+ real-world intersections using Python, Rasterio, and Bilinear Interpolation to establish dynamic population density.
*   **Population-Weighted K-Means++:** A custom implementation of K-Means clustering (using the Haversine metric) to mathematically isolate 5 primary economic gravity centers/hotspots.
*   **Budget-Conscious Backbone (Kruskal’s MST):** Uses a Minimum Spanning Tree with Disjoint Set Union (DSU) and path compression to calculate the absolutely cheapest way to connect all major hubs.
*   **Outer Ring Road (ORR) Bypass Heuristic:** A custom cycle-insertion algorithm that adds high-speed bypass networks around the MST to prevent central gridlock.
*   **High-Throughput Parallel Simulation:** A C++ batch-parallel microscopic traffic simulator routing 50,000+ commuters concurrently using `std::async` and **A* Search**.
*   **BPR Congestion Modeling:** Dynamically calculates traffic jams and travel delays using the Bureau of Public Roads (BPR) link performance function.
*   **Geospatial Visualization:** Generates interactive dark-mode maps and heatmaps of traffic flow bottlenecks using Python Folium.

---

## 🛠️ Tech Stack

*   **Core Engine:** C++ (Standard Library, Multithreading, `std::async`)
*   **Data Pipeline:** Python, Rasterio, GDAL, NumPy
*   **Visualization:** Folium, Leaflet.js
*   **Data Formatting:** JSON (`nlohmann/json`)
*   **Version Control:** Git & GitHub

---

## 📊 The Results (Traffic Scorecard)

By simulating 50,000 commuters through the remodeled network, the AI achieved the following results compared to the original, unmodified OpenStreetMap layout of Kukatpally:

*   **Gridlock Reduction:** Significantly reduced the number of local roads operating over 150% capacity.
*   **Traffic Diversion:** Successfully diverted the majority of long-distance commuter traffic off local residential streets and onto the newly generated high-speed MST and ORR bypasses.
*   **Total Infrastructure Added:** ~16 km of optimal, high-capacity highway.

*(Run the Python visualizer to see the dynamic scorecard in your terminal!)*

---

## ⚙️ How to Run the Project

### 1. Prerequisites
*   A C++ Compiler (GCC, Clang, or MSVC)
*   CMake (v3.10+)
*   Python 3.8+

### 2. Build the C++ Engine
```bash
mkdir build
cd build
cmake ..
cmake --build .
