import rasterio
import json
import os

# Use the 'r' prefix for a raw string (easiest)
TIFF_PATH = r"C:\Users\Admin\OneDrive\Documents\ind_ppp_2020.tif"

# OR use double backslashes
# TIFF_PATH = "C:\\Users\\Admin\\OneDrive\\Documents\\ind_ppp_2020.tif"
NODES_PATH = "../data/processed/nodes.json"
OUTPUT_PATH = "../data/processed/nodes_with_density.json"

def extract_population():
    print("Opening the 1.8 GB WorldPop dataset (this might take a few seconds)...")
    
    # 1. Open the nodes file
    with open(NODES_PATH, 'r') as f:
        nodes = json.load(f)

    # 2. Open the massive TIFF file
    # rasterio is smart enough NOT to load the whole 1.8GB into memory at once
    with rasterio.open(TIFF_PATH) as dataset:
        print(f"Dataset opened! Resolution: {dataset.width}x{dataset.height}")
        
        # 3. Look up the population for every single intersection
        # We sample the TIFF using the exact Lat/Lon of our nodes
        coords = [(node['lon'], node['lat']) for node in nodes]
        
        print("Extracting population for Kukatpally intersections...")
        
        # dataset.sample() returns a generator of values at those exact GPS coordinates
        for node, val in zip(nodes, dataset.sample(coords)):
            pop_count = val[0]
            
            # If the value is negative (NoData) or missing, set to 0
            if pop_count < 0:
                pop_count = 0.0
                
            node['population_density'] = round(float(pop_count), 3)

    # 4. Save the updated nodes
    with open(OUTPUT_PATH, 'w') as f:
        json.dump(nodes, f, indent=2)

    print(f"Success! Exact population data assigned to {len(nodes)} nodes.")

if __name__ == "__main__":
    if not os.path.exists(TIFF_PATH):
        print(f"ERROR: Could not find the .tif file at {TIFF_PATH}")
    else:
        extract_population()