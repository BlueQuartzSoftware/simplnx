import numpy as np
from pathlib import Path
from typing import List
import copy

def mask_coordinates(coords: np.ndarray, z_height: float, units: int, bounding_box: list) -> np.array:
    x_min, x_max, y_min, y_max, z_min, z_max = bounding_box
    
    coords = coords.reshape((coords.size // 4, 4))

    # Extracting x and y coordinates from the numpy array
    x1 = coords[:, 0] * units
    y1 = coords[:, 1] * units
    x2 = coords[:, 2] * units
    y2 = coords[:, 3] * units
    z_height = z_height * units

    # Creating boolean masks for points inside the bounding box
    mask = ((x1 >= x_min) & (x1 <= x_max) & (y1 >= y_min) & (y1 <= y_max) &
            (x2 >= x_min) & (x2 <= x_max) & (y2 >= y_min) & (y2 <= y_max) &
            (z_height >= z_min) & (z_height <= z_max))

    # Filtering numpy array based on the mask
    filtered_array = coords[mask]
    filtered_array = filtered_array.reshape((np.prod(filtered_array.shape)))

    return filtered_array

class Polyline(object):
    def __init__(self, line, layer_id, z_height, data: dict, units=1.0, bounding_box: list = None):
        self.layer_id = layer_id
        self.z_height = z_height * units
        vals = line.split(",")
        self.poly_id = int(vals[0])
        self.dir = int(vals[1])
        self.n = int(vals[2])
        coords = np.array(list(map(float, vals[3:])))
        if bounding_box is not None:
            coords = mask_coordinates(coords, z_height, units, bounding_box)
            self.n = coords.size // 4
        self.xvals = coords[0::2] * units
        self.yvals = coords[1::2] * units
        self.data = data
        
class Hatches(object):
    def __init__(self, line, layer_id, z_height, data: dict, units=1.0, bounding_box: list = None):
        self.layer_id = layer_id
        self.z_height = z_height * units
        vals = line.split(",")
        self.hatch_id = int(vals[0])
        self.n = int(vals[1])
        coords = np.array(list(map(float, vals[2:])))
        if bounding_box is not None:
            coords: np.ndarray = mask_coordinates(coords, z_height, units, bounding_box)
            self.n = coords.size // 4
        self.start_xvals = coords[0::4] * units
        self.start_yvals = coords[1::4] * units
        self.end_xvals   = coords[2::4] * units
        self.end_yvals   = coords[3::4] * units
        self.data = data

def parse_header(file):
    units = 1.  #default to 1, will be overwritten by anything read from the fileheader.
    hatch_labels = {}
    line = file.readline().strip()
    while not line.startswith("$$HEADEREND"):
        if line and line.startswith("$$UNITS"):
            key, val = line.split("/")
            units = float(val)
        elif line and line.startswith("$$LABEL"):
            key, val = line.split("/")
            hatch_id, hatch_desc = val.split(",")
            hatch_labels[int(hatch_id)] = hatch_desc.strip()
        line = file.readline()
        
    return units, hatch_labels

def parse_geometry_array_names(full_path: Path):
    array_names = []

    with open(str(full_path), 'r') as file:
        record = False
        num_of_labels = 0
        for line in file:
            line = line.strip()
            if not line:
                continue

            if line.startswith("$$LABEL"):
                num_of_labels = num_of_labels + 1

            if '$$GEOMETRYSTART' in line:
                record = True
                continue

            if line.startswith("$$HATCHES") or line.startswith("$HATCHES") or line.startswith("$$POLYLINE") or line.startswith("$POLYLINE"):
                break

            if '/' in line and record:
                tag, value_str = line.split("/", 1)
                tag = tag.replace('$', '')
                tag = tag.capitalize()
                if tag and tag not in array_names:
                    array_names.append(tag)

    return array_names, num_of_labels
                

def parse_geometry(file, units, bounding_box: list = None):
    layer_counter = -1 #initialize to -1, increment by one when finding the first layer
    layer_heights = []
    layer_features = []
    features = []
    data = {}
    
    #parse file lines
    line: str = file.readline().strip()
    while not line.startswith("$$GEOMETRYEND"):
        if not line:
            # Do nothing, read the next line
            pass
        elif line.startswith("$$LAYER"):
            if layer_counter>=0:
                layer_features.append(features) #save the old stuff before starting the new stuff
            layer_counter += 1
            key, val = line.split("/")
            layer_heights.append(float(val))
            key = key.replace('$', '')
            key = key.capitalize()
            data[key] = val
            features = []  #reinitiate
        
        #sometimes these don't start with double dollar signs.
        elif line.startswith("$$POLYLINE") or line.startswith("$POLYLINE"):
            #Assuming we found the units already!!!
            key, val = line.split("/")
            new_poly = Polyline(val, layer_counter, layer_heights[-1], copy.copy(data), units, bounding_box)
            features.append(new_poly)
    
        elif line.startswith("$$HATCHES") or line.startswith("$HATCHES"):
            #Assuming we found the units already!!!
            key, val = line.split("/")
            new_hatch = Hatches(val, layer_counter, layer_heights[-1], copy.copy(data), units, bounding_box)
            features.append(new_hatch)
        else:
            key, val = line.split("/")
            key = key.replace('$', '')
            key = key.capitalize()
            data[key] = val
        line = file.readline().strip()
    
    #convert heights into an array, and scale by the units value
    layer_heights = units * np.array(layer_heights)

    # Save the last layer
    if features:
        layer_features.append(features)

    return layer_features, layer_heights

def parse_file(full_path: Path, bounding_box: list = None):    
    layer_heights = None
    layer_features = None
    units = None
    
    #parse file lines
    with open(str(full_path), 'r') as file:
        line = file.readline().strip()
        while line:
            if not line:
                pass
            elif line.startswith("$$HEADERSTART"):
                units, hatch_labels = parse_header(file)
            elif line.startswith("$$GEOMETRYSTART"):
                if units is None:
                    raise Exception("No $$HEADERSTART tag was found!") 
                layer_features, layer_heights = parse_geometry(file, units, bounding_box)
            line = file.readline().strip()
    
    return layer_features, layer_heights, hatch_labels

if __name__ == "__main__":
    full_path = Path("/Users/bluequartz/Downloads/B10.cli")
    
    try:
        layer_features, layer_heights = parse_file(full_path)
    except Exception as e:
        print(f"An error occurred while parsing the CLI file '{str(full_path)}': {e}")

    print("Total layers found:   "+str(len(layer_heights)))
    print("Layers with features: "+str(len(layer_features)))
