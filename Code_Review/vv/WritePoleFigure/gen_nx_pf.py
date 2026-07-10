#!/usr/bin/env python3
# Regenerated 2026-07-08 for Code_Review/vv/WritePoleFigure. NX side of the WritePoleFigure A/B.
# Reconstructed from the decompiled gen_nx_pf.cpython-312.pyc; paths retargeted to this folder and
# parameterized for the hex and cubic inputs. Builds a DREAM3D-NX pipeline (ReadDREAM3DFilter +
# WritePoleFigure x2: Color + Discrete) and writes it next to this script.
import json
import sys

BASE = '/Users/mjackson/Workspace3/simplnx/Code_Review/vv/WritePoleFigure'


def a(v):
    return {'value': v, 'version': 1}


def wpf(prefix, gen_algo, geom_name, outdir):
    return {
        'args': {
            'title': a('WritePoleFigure AB'),
            'generation_algorithm_index': a(gen_algo),
            'lambert_size': a(64),
            'num_colors': a(32),
            'discrete_marker_radius': a(3),
            'image_layout_index': a(0),
            'image_size': a(512),
            'output_path': a(outdir),
            'image_prefix': a(prefix),
            'use_mask': a(False),
            'cell_euler_angles_array_path': a('Imported Data/Eulers'),
            'cell_phases_array_path': a('Imported Data/Phases'),
            'mask_array_path': a('Imported Data/Mask'),
            'crystal_structures_array_path': a('EnsembleAttributeMatrix/CrystalStructures'),
            'material_name_array_path': a('EnsembleAttributeMatrix/PhaseNames'),
            'save_as_image_geometry': a(True),
            'write_image_to_disk': a(True),
            'output_image_geometry_path': a(geom_name),
            'save_intensity_plots': a(False),
            'normalize_to_mrd': a(True),
            'intensity_geometry_path': a(geom_name + ' Intensity'),
            'intensity_plot_1_name': a('<0001>'),
            'intensity_plot_2_name': a('<11-20>'),
            'intensity_plot_3_name': a('<10-10>'),
            'hex_convention_index': a(0),
        },
        'comments': '',
        'filter': {'name': 'nx::core::WritePoleFigureFilter', 'uuid': '00cbb97e-a5c2-43e6-9a35-17a0f9ce26ed'},
        'isDisabled': False,
    }


def pipeline(in_file, outdir):
    return {
        'name': 'WritePoleFigure NX AB',
        'pipeline': [
            {
                'args': {'import_data_object': a({'file_path': in_file, 'data_paths': None})},
                'comments': '',
                'filter': {'name': 'nx::core::ReadDREAM3DFilter', 'uuid': '0dbd31c7-19e0-4077-83ef-f4a6459a0e2d'},
                'isDisabled': False,
            },
            wpf('nx_color_', 0, 'PoleFigure_Color', outdir),
            wpf('nx_discrete_', 1, 'PoleFigure_Discrete', outdir),
        ],
        'version': 1,
    }


if __name__ == '__main__':
    variant = sys.argv[1] if len(sys.argv) > 1 else 'hex'  # 'hex' or 'cubic'
    in_file = f'{BASE}/input/pf_input_{variant}.dream3d'
    outdir = f'{BASE}/out_nx' if variant == 'hex' else f'{BASE}/out_cubic_nx'
    out_pipe = f'{BASE}/pf_nx_{variant}.d3dpipeline'
    with open(out_pipe, 'w') as fh:
        json.dump(pipeline(in_file, outdir), fh, indent=2)
    print('wrote', out_pipe, '-> output dir', outdir)
