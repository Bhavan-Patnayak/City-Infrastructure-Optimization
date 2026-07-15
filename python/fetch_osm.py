import osmnx as ox
import json
import os

# Coordinates for Kukatpally (approximate bounding box)
# North, South, East, West
NORTH, SOUTH, EAST, WEST = 17.495, 17.450, 78.415, 78.375
OUTPUT_DIR = "../data/processed"

def fetch_data():
    print(f"Downloading road network using bounding box...")
    
    # Use bounding box instead of place name
    # Use the bounding box as a tuple (north, south, east, west)
    # Pass the bounding box as (West, South, East, North)
    G = ox.graph_from_bbox(bbox=(WEST, SOUTH, EAST, NORTH), network_type='drive')
    
    nodes_list = []
    edges_list = []

    # Process nodes
    for node_id, data in G.nodes(data=True):
        nodes_list.append({
            "id": node_id,
            "lat": data['y'],
            "lon": data['x']
        })

    # Process edges
    for u, v, data in G.edges(data=True):
        edges_list.append({
            "src": u,
            "dst": v,
            "length_m": float(data.get('length', 0)),
            "road_type": data.get('highway', 'residential'),
            "capacity": 500.0
        })

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    
    with open(os.path.join(OUTPUT_DIR, "nodes.json"), "w") as f:
        json.dump(nodes_list, f, indent=2)
        
    with open(os.path.join(OUTPUT_DIR, "edges.json"), "w") as f:
        json.dump(edges_list, f, indent=2)

    print(f"Success! {len(nodes_list)} nodes and {len(edges_list)} edges saved.")

if __name__ == "__main__":
    fetch_data()