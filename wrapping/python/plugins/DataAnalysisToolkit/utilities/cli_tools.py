import numpy as np
from pathlib import Path
from typing import Tuple
from ..common.Result import Result, make_error_result
import copy
import re

def read_poly_line(line: str) -> Tuple[int, int, int, np.ndarray]:
    vals = line.split(",")
    return int(vals[0]), int(vals[1]), int(vals[2]), np.array(list(map(float, vals[3:])))

def read_hatch_line(line: str) -> Tuple[int, int, np.ndarray]:
    vals = line.split(",")
    return int(vals[0]), int(vals[1]), np.array(list(map(float, vals[2:])))

def mask_coordinates(coords: np.ndarray, z_height: float, units: int, bounding_box: list) -> Result[np.array]:
    x_min, x_max, y_min, y_max, z_min, z_max = bounding_box
    
    coords = coords.reshape((coords.size // 4, 4))

    # Extracting x and y coordinates from the numpy array
    x1 = coords[:, 0] * units
    y1 = coords[:, 1] * units
    x2 = coords[:, 2] * units
    y2 = coords[:, 3] * units
    z_height = z_height * units

    # Check for violations of bounding box condition
    x1_inside = (x1 >= x_min) & (x1 <= x_max)
    y1_inside = (y1 >= y_min) & (y1 <= y_max)
    z_inside = (z_height >= z_min) & (z_height <= z_max)
    x2_inside = (x2 >= x_min) & (x2 <= x_max)
    y2_inside = (y2 >= y_min) & (y2 <= y_max)

    # Check for cases where either x1, y1, or z1 is inside the bounding box while the other is outside
    if np.any(x1_inside & y1_inside & z_inside & (~x2_inside | ~y2_inside)):
        idx = np.where(x1_inside & y1_inside & z_inside & (~x2_inside | ~y2_inside))[0][0]
        return make_error_result(code=-1100, message=f"Point ({x1[idx]}, {y1[idx]}, {z_height}) is inside the bounding box while point ({x2[idx]}, {y2[idx]}, {z_height}) is outside.")
    if np.any(x2_inside & y2_inside & z_inside & (~x1_inside | ~y1_inside)):
        idx = np.where(x2_inside & y2_inside & z_inside & (~x1_inside | ~y1_inside))[0][0]
        return make_error_result(code=-1101, message=f"Point ({x2[idx]}, {y2[idx]}, {z_height}) is inside the bounding box while point ({x1[idx]}, {y1[idx]}, {z_height}) is outside.")
        
    # Creating boolean masks for points inside the bounding box
    mask = ((x1_inside & y1_inside & z_inside) & (x2_inside & y2_inside & z_inside))

    # Filtering numpy array based on the mask
    filtered_array = coords[mask]
    filtered_array = filtered_array.reshape((np.prod(filtered_array.shape)))

    return Result(value=filtered_array)

class Polyline(object):
    def __init__(self, layer_id, z_height, data: dict, poly_id, dir, n, xvals, yvals) -> Result:
        self.layer_id = layer_id
        self.z_height = z_height
        self.poly_id = poly_id
        self.dir = dir
        self.n = n
        self.xvals = xvals
        self.yvals = yvals
        self.data = data
        
class Hatches(object):
    def __init__(self, layer_id, z_height, data: dict, hatch_id, n, start_xvals, start_yvals, end_xvals, end_yvals):
        self.layer_id = layer_id
        self.hatch_id = hatch_id
        self.n = n
        self.z_height = z_height
        self.start_xvals = start_xvals
        self.start_yvals = start_yvals
        self.end_xvals   = end_xvals
        self.end_yvals   = end_yvals
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
            line = re.sub(r"//.*?//", "", line).strip()  # Remove comments
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
                

def parse_geometry(file, units, bounding_box: list = None) -> Result:
    layer_counter = -1 #initialize to -1, increment by one when finding the first layer
    layer_heights = []
    layer_features = []
    features = []
    data = {}
    
    #parse file lines
    line: str = file.readline()
    line = re.sub(r"//.*?//", "", line).strip()  # Remove comments
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
            poly_id, dir, n, coords = read_poly_line(val)
            z_height = layer_heights[-1] * units
            if bounding_box is not None:
                coords_result = mask_coordinates(coords, z_height, units, bounding_box)
                if coords_result.invalid():
                    return Result(errors=coords_result.errors)
                coords = coords_result.value
                n = coords.size // 4
            xvals = coords[0::2] * units
            yvals = coords[1::2] * units
            new_poly = Polyline(layer_counter, z_height, copy.copy(data), poly_id, dir, n, xvals, yvals)
            features.append(new_poly)
    
        elif line.startswith("$$HATCHES") or line.startswith("$HATCHES"):
            #Assuming we found the units already!!!
            key, val = line.split("/")
            hatch_id, n, coords = read_hatch_line(val)
            z_height = layer_heights[-1]
            if bounding_box is not None:
                coords_result = mask_coordinates(coords, z_height, units, bounding_box)
                if coords_result.invalid():
                    return Result(errors=coords_result.errors)
                coords: np.ndarray = coords_result.value
                n = coords.size // 4
            start_xvals = coords[0::4] * units
            start_yvals = coords[1::4] * units
            end_xvals   = coords[2::4] * units
            end_yvals   = coords[3::4] * units
            new_hatch = Hatches(layer_counter, z_height * units, copy.copy(data), hatch_id, n, start_xvals, start_yvals, end_xvals, end_yvals)
            features.append(new_hatch)
        else:
            key, val = line.split("/")
            key = key.replace('$', '')
            key = key.capitalize()
            data[key] = val
        line = file.readline()
        line = re.sub(r"//.*?//", "", line).strip()  # Remove comments
    
    #convert heights into an array, and scale by the units value
    layer_heights = units * np.array(layer_heights)

    # Save the last layer
    if features:
        layer_features.append(features)

    return Result(value=(layer_features, layer_heights))

def parse_file(full_path: Path, bounding_box: list = None) -> Result:    
    layer_heights = None
    layer_features = None
    units = None
    
    #parse file lines
    with open(str(full_path), 'r') as file:
        line = file.readline().strip()
        while line:
            if line:
                line = re.sub(r"//.*?//", "", line).strip()  # Remove comments
                if line.startswith("$$HEADERSTART"):
                    units, hatch_labels = parse_header(file)
                if line.startswith("$$GEOMETRYSTART"):
                    if units is None:
                        raise Exception("No $$HEADERSTART tag was found!") 
                    result = parse_geometry(file, units, bounding_box)
                    if result.invalid():
                        return Result(errors=result.errors)
                    layer_features, layer_heights = result.value
            line = file.readline().strip()
    
    return Result(value=(layer_features, layer_heights, hatch_labels))

if __name__ == "__main__":
    full_path = Path("/Users/bluequartz/Downloads/B10.cli")
    
    try:
        result = parse_file(full_path)
        if result.invalid():
            print(f"Error: {result.errors[0].message}")
        layer_features, layer_heights, hatch_labels = result.value
    except Exception as e:
        print(f"An error occurred while parsing the CLI file '{str(full_path)}': {e}")

    print("Total layers found:   "+str(len(layer_heights)))
    print("Layers with features: "+str(len(layer_features)))
