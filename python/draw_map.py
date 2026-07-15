import json
import folium
from folium.plugins import HeatMap
import os
import math

# Helper to calculate real-world distances between coordinates
def haversine(lat1, lon1, lat2, lon2):
    R = 6371.0 # Earth radius in km
    dLat = math.radians(lat2 - lat1)
    dLon = math.radians(lon2 - lon1)
    a = math.sin(dLat / 2)**2 + math.cos(math.radians(lat1)) * math.cos(math.radians(lat2)) * math.sin(dLon / 2)**2
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1 - a))
    return R * c

def draw_city():
    print("Loading AI-generated city layout...")
    file_path = '../data/processed/final_network.json'
    
    
    if not os.path.exists(file_path):
        print(f"Error: Could not find {file_path}. Did you run the C++ engine?")
        return
        
    with open(file_path, 'r') as f:
        data = json.load(f)
        
    nodes = {n['id']: (n['lat'], n['lon']) for n in data['nodes']}
    
    # --- METRICS TRACKING ---
    local_flow_km = 0
    highway_flow_km = 0
    mst_km = 0
    orr_km = 0
    local_bottlenecks = 0
    total_local_roads = 0
    
    heat_data = [] 

    # Analyze every road segment
    for edge in data['edges']:
        if edge['src'] not in nodes or edge['dst'] not in nodes:
            continue
            
        p1 = nodes[edge['src']]
        p2 = nodes[edge['dst']]
        
        flow = edge['flow']
        cap = edge['capacity']
        rtype = edge['road_type']
        
        # Calculate distance in km
        dist = haversine(p1[0], p1[1], p2[0], p2[1])
        vehicle_km = flow * dist
        
        if rtype == "highway_mst":
            mst_km += dist
            highway_flow_km += vehicle_km
        elif rtype == "highway_ring":
            orr_km += dist
            highway_flow_km += vehicle_km
        else:
            total_local_roads += 1
            local_flow_km += vehicle_km
            
            # Bottleneck = 50% over capacity
            if cap > 0 and (flow / cap) > 1.5:
                local_bottlenecks += 1
                
        # Add to Heatmap Data if there is traffic
        if flow > 0:
            congestion_ratio = flow / cap if cap > 0 else 0
            mid_lat = (p1[0] + p2[0]) / 2.0
            mid_lon = (p1[1] + p2[1]) / 2.0
            if congestion_ratio > 0.5: 
                 heat_data.append([mid_lat, mid_lon, congestion_ratio])

    # --- GENERATE THE SCORECARD ---
    total_flow = local_flow_km + highway_flow_km
    highway_pct = (highway_flow_km / total_flow) * 100 if total_flow > 0 else 0
    
    # Estimate original bottlenecks (if the AI highways didn't exist to absorb the flow)
    # If X% of traffic is on highways, local roads would have X% more catastrophic failures
    baseline_bottlenecks = int(local_bottlenecks + (total_local_roads * (highway_pct / 100.0)))
    reduction = ((baseline_bottlenecks - local_bottlenecks) / baseline_bottlenecks) * 100 if baseline_bottlenecks > 0 else 0
    
    grade = "A+" if reduction > 80 else "A-" if reduction > 60 else "B" if reduction > 40 else "C"

    print("\n==================================================")
    print(" 🏙️  KUKATPALLY INFRASTRUCTURE SCORECARD")
    print("==================================================")
    print("📊 TRAFFIC DISTRIBUTION:")
    print(f"   - Traffic Forced on Local Roads: {100 - highway_pct:.1f}% (Baseline: 100%)")
    print(f"   - Traffic Diverted to AI Highways: {highway_pct:.1f}%")
    print("\n🚦 CONGESTION METRICS (Flow > 150% Capacity):")
    print(f"   - Original Kukatpally (Est. Bottlenecks): ~{baseline_bottlenecks}")
    print(f"   - Remodeled Kukatpally (Actual Bottlenecks): {local_bottlenecks}")
    print(f"   - Gridlock Reduction: {reduction:.1f}% 📉")
    print("\n🛣️ NEW INFRASTRUCTURE BUILT:")
    print(f"   - MST Backbone: {mst_km/2:.1f} km") # Divide by 2 because edges are bidirectional
    print(f"   - Outer Ring Road: {orr_km/2:.1f} km")
    print(f"\n⭐ OVERALL NETWORK GRADE: {grade}")
    print("==================================================\n")

    # --- DRAW THE MAP ---
    m = folium.Map(location=[17.47, 78.39], zoom_start=14, tiles='CartoDB dark_matter')
    
    # Add HeatMap
    if heat_data:
        HeatMap(
            heat_data,
            radius=15, blur=20, min_opacity=0.3, 
            gradient={0.4: 'yellow', 0.7: 'orange', 1.0: 'red'}
        ).add_to(m)
        
    output_file = "kukatpally_heatmap.html"
    m.save(output_file)
    print(f"[SUCCESS] Heatmap generated! Open 'python/{output_file}' in your web browser.")

if __name__ == "__main__":
    draw_city()