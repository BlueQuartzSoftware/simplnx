import numpy as np
from pathlib import Path
from typing import List
import copy

class Polyline(object):
    def __init__(self, line, layer_id, z_height, data: dict, units=1.0):
        self.layer_id = layer_id
        self.z_height = z_height*units
        vals = line.split(",")
        self.poly_id = int(vals[0])
        self.dir = int(vals[1])
        self.n = int(vals[2])
        coords = np.array(list(map(float, vals[3:])))
        self.xvals = coords[0::2] * units
        self.yvals = coords[1::2] * units
        self.data = data
        
class Hatches(object):
    def __init__(self, line, layer_id, z_height, data: dict, units=1.0):
        self.layer_id = layer_id
        self.z_height = z_height*units
        vals = line.split(",")
        self.hatch_id = int(vals[0])
        self.n = int(vals[1])
        coords = np.array(list(map(float, vals[2:])))
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
        has_labels = False
        for line in file:
            line = line.strip()
            if not line:
                continue

            if not has_labels and line.startswith("$$LABEL"):
                has_labels = True

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

    return array_names, has_labels
                

def parse_geometry(file, units):
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
            new_poly = Polyline(val, layer_counter, layer_heights[-1], copy.copy(data), units)
            features.append(new_poly)
    
        elif line.startswith("$$HATCHES") or line.startswith("$HATCHES"):
            #Assuming we found the units already!!!
            key, val = line.split("/")
            new_hatch = Hatches(val, layer_counter, layer_heights[-1], copy.copy(data), units)
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

def parse_file(full_path: Path):    
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
                layer_features, layer_heights = parse_geometry(file, units)
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
