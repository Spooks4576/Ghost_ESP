import osmnx as ox
import pandas as pd

def expand_and_fetch_streets(north, south, east, west, expansions=10, scale_factor=0.5, filename="streets_data.csv"):
    ox.config(use_cache=True, log_console=True)
    all_edges_df = pd.DataFrame()

    for i in range(expansions):
        current_north = north + (north - south) * scale_factor * i
        current_south = south - (north - south) * scale_factor * i
        current_east = east + (east - west) * scale_factor * i
        current_west = west - (east - west) * scale_factor * i

        custom_filter = '["highway"~"residential|road|pedestrian|footway|path|cycleway"]'
        street_graph = ox.graph_from_bbox(current_north, current_south, current_east, current_west, network_type='all', custom_filter=custom_filter)

        edge_data = [{
            'u': u,
            'v': v,
            'street_name': data.get('name', 'Unnamed') if isinstance(data.get('name'), str) else 'Complex Name'
        } for u, v, data in street_graph.edges(data=True) if 'name' in data and isinstance(data['name'], str)]

        edges_df = pd.DataFrame(edge_data)
        if not edges_df.empty:
            gdf_nodes = ox.graph_to_gdfs(street_graph, edges=False)
            gdf_nodes.reset_index(inplace=True)
            if 'osmid' not in gdf_nodes.columns:
                gdf_nodes.rename(columns={'index': 'osmid'}, inplace=True)

            nodes_data = gdf_nodes[['osmid', 'y', 'x']].rename(columns={'y': 'latitude', 'x': 'longitude'})
            merged_data = pd.merge(edges_df, nodes_data, how='left', left_on='u', right_on='osmid')
            all_edges_df = pd.concat([all_edges_df, merged_data], ignore_index=True)

    # Remove duplicates, ensure to handle types correctly
    all_edges_df.drop_duplicates(subset=['u', 'v', 'street_name'], inplace=True)
    all_edges_df.to_csv(filename, index=False)
    print(f"Data saved to {filename}")

# Example initial bounding box
north, south, east, west = 53.5483, 53.5426, -113.8975, -113.9105
expand_and_fetch_streets(north, south, east, west)