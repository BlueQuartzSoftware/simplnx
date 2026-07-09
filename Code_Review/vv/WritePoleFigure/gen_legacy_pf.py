#!/usr/bin/env python3
# Regenerated 2026-07-08 for Code_Review/vv/WritePoleFigure. Legacy (SIMPL 6.5.x) side of the A/B.
# Reconstructed from the decompiled gen_legacy_pf.cpython-312.pyc; paths retargeted to this folder
# and parameterized for hex/cubic. Builds a SIMPL pipeline (DataContainerReader + legacy
# WritePoleFigure x2: color + discrete) that the DREAM3D 6.5.171 / 6.5.172 PipelineRunner executes.
import json
import sys

BASE = '/Users/mjackson/Workspace3/simplnx/Code_Review/vv/WritePoleFigure'
DC = 'ImageDataContainer'


def da(name, path, obj_type, comps):
    return {
        'Component Dimensions': [comps],
        'Flag': 2,
        'Name': name,
        'Object Type': obj_type,
        'Path': path,
        'Tuple Dimensions': [1],
        'Version': 2,
    }


def reader(in_file):
    cellpath = f'/DataContainers/{DC}/CellData'
    enspath = f'/DataContainers/{DC}/CellEnsembleData'
    proxy = {
        'Data Containers': [
            {
                'Attribute Matricies': [
                    {
                        'Data Arrays': [
                            da('Eulers', cellpath, 'DataArray<float>', 3),
                            da('Phases', cellpath, 'DataArray<int32_t>', 1),
                            da('Mask', cellpath, 'DataArray<bool>', 1),
                        ],
                        'Flag': 2,
                        'Name': 'CellData',
                        'Type': 3,
                    },
                    {
                        'Data Arrays': [
                            da('CrystalStructures', enspath, 'DataArray<uint32_t>', 1),
                            da('MaterialName', enspath, 'StringDataArray', 1),
                        ],
                        'Flag': 2,
                        'Name': 'CellEnsembleData',
                        'Type': 11,
                    },
                ],
                'Flag': 2,
                'Geometry': {'Geometry_Type': 0, 'Geometry_Type_Name': 'ImageGeometry'},
                'Name': DC,
            }
        ],
        'Version': 6,
    }
    return {
        'FilterVersion': '6.5.171',
        'Filter_Enabled': True,
        'Filter_Human_Label': 'Read DREAM.3D Data File',
        'Filter_Name': 'DataContainerReader',
        'Filter_Uuid': '{043cbde5-3878-5718-958f-ae75714df0df}',
        'InputFile': in_file,
        'OverwriteExistingDataContainers': False,
        'InputFileDataContainerArrayProxy': proxy,
    }


def dap(am, name):
    return {'Data Container Name': DC, 'Attribute Matrix Name': am, 'Data Array Name': name}


def write_pf(out_dir, prefix, gen_algo, use_mask):
    return {
        'FilterVersion': '6.5.171',
        'Filter_Enabled': True,
        'Filter_Human_Label': 'Export Pole Figure Images',
        'Filter_Name': 'WritePoleFigure',
        'Filter_Uuid': '{a10bb78e-fcff-553d-97d6-830a43c85385}',
        'CellEulerAnglesArrayPath': dap('CellData', 'Eulers'),
        'CellPhasesArrayPath': dap('CellData', 'Phases'),
        'CrystalStructuresArrayPath': dap('CellEnsembleData', 'CrystalStructures'),
        'MaterialNameArrayPath': dap('CellEnsembleData', 'MaterialName'),
        'GoodVoxelsArrayPath': dap('CellData', 'Mask'),
        'UseGoodVoxels': 1 if use_mask else 0,
        'GenerationAlgorithm': gen_algo,
        'ImageLayout': 0,
        'ImageSize': 512,
        'LambertSize': 64,
        'NumColors': 32,
        'OutputPath': out_dir,
        'ImagePrefix': prefix,
        'Title': 'WritePoleFigure AB',
    }


if __name__ == '__main__':
    variant = sys.argv[1] if len(sys.argv) > 1 else 'hex'  # 'hex' or 'cubic'
    out_dir = sys.argv[2]
    out_json = sys.argv[3]
    in_file = f'{BASE}/input/pf_input_legacy.dream3d' if variant == 'hex' else f'{BASE}/input/pf_input_cubic_legacy.dream3d'
    pipe = {
        '0': reader(in_file),
        '1': write_pf(out_dir, 'color_', 0, False),
        '2': write_pf(out_dir, 'discrete_', 1, False),
        'PipelineBuilder': {
            'Name': 'WritePoleFigure legacy AB',
            'Number_Filters': 3,
            'Version': 6,
        },
    }
    with open(out_json, 'w') as fh:
        json.dump(pipe, fh, indent=2)
    print('wrote', out_json, '-> output dir', out_dir)
