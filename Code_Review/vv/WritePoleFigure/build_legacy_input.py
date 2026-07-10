#!/usr/bin/env python3
# Reconstructed 2026-07-08. The original build_legacy_input.py (which minted the legacy-format
# pf_input_legacy.dream3d for the WritePoleFigure A/B) was lost with no .pyc, so it is rebuilt here
# from (a) the array contract in gen_legacy_pf.py's reader() and (b) the exact legacy (SIMPL v7.0)
# HDF5 layout cloned from a known-good legacy file (6_6_stats_test_v2.dream3d).
#
# Extracts the 502 orientations from the NX Pole_Figure_Exemplars_v6 input and writes a
# DREAM3D 6.5.x-format .dream3d (ImageDataContainer, 502x1x1 ImageGeometry) that the 6.5.171 /
# 6.5.172 DataContainerReader can read.
#
# Usage: build_legacy_input.py <hex|cubic> <out.dream3d>
import sys
import h5py
import numpy as np

BASE = '/Users/mjackson/Workspace3/simplnx/Code_Review/vv/WritePoleFigure'
DC = 'ImageDataContainer'
ASCII = h5py.string_dtype(encoding='ascii')


def legacy_arr(group, name, data, object_type, comp, ntuples):
    # Legacy cell/ensemble array: dataset stored ZYX-major as (z, y, x, comp) flattened here to
    # (ntuples, comp); attrs carry x-major TupleDimensions + the SIMPL metadata.
    ds = group.create_dataset(name, data=data)
    ds.attrs.create('ComponentDimensions', np.array([comp], dtype=np.uint64))
    ds.attrs.create('DataArrayVersion', np.array([2], dtype=np.int32))
    ds.attrs['ObjectType'] = np.bytes_(object_type)
    ds.attrs['Tuple Axis Dimensions'] = np.bytes_(f'x={ntuples}')
    ds.attrs.create('TupleDimensions', np.array([ntuples], dtype=np.uint64))


def main():
    variant = sys.argv[1]
    out = sys.argv[2]
    src = f'{BASE}/input/pf_input_{variant}.dream3d'

    with h5py.File(src, 'r') as s:
        eulers = s['DataStructure/Imported Data/Eulers'][:].reshape(-1, 3).astype(np.float32)
        phases = s['DataStructure/Imported Data/Phases'][:].reshape(-1, 1).astype(np.int32)
        mask = s['DataStructure/Imported Data/Mask'][:].reshape(-1, 1).astype(np.uint8)
        xtal = s['DataStructure/EnsembleAttributeMatrix/CrystalStructures'][:].reshape(-1, 1).astype(np.uint32)
    n = eulers.shape[0]
    nens = xtal.shape[0]

    with h5py.File(out, 'w') as f:
        f.attrs['FileVersion'] = np.bytes_('7.0')
        f.attrs['DREAM3D Version'] = np.bytes_('6.5.171')
        for empty in ('DataContainerBundles', 'Montages', 'Pipeline'):
            f.create_group(empty)
        dcs = f.create_group('DataContainers')
        dc = dcs.create_group(DC)

        geom = dc.create_group('_SIMPL_GEOMETRY')
        geom.attrs['GeometryName'] = np.bytes_('ImageGeometry')
        geom.attrs.create('GeometryType', np.array([0], dtype=np.uint32))
        geom.attrs['GeometryTypeName'] = np.bytes_('ImageGeometry')
        geom.attrs.create('SpatialDimensionality', np.array([3], dtype=np.uint32))
        geom.attrs.create('UnitDimensionality', np.array([3], dtype=np.uint32))
        geom.create_dataset('DIMENSIONS', data=np.array([n, 1, 1], dtype=np.int64))
        geom.create_dataset('ORIGIN', data=np.array([0, 0, 0], dtype=np.float32))
        geom.create_dataset('SPACING', data=np.array([1, 1, 1], dtype=np.float32))

        cell = dc.create_group('CellData')
        cell.attrs.create('AttributeMatrixType', np.array([3], dtype=np.uint32))
        cell.attrs.create('TupleDimensions', np.array([n, 1, 1], dtype=np.uint64))
        legacy_arr(cell, 'Eulers', eulers, 'DataArray<float>', 3, n)
        legacy_arr(cell, 'Phases', phases, 'DataArray<int32_t>', 1, n)
        legacy_arr(cell, 'Mask', mask, 'DataArray<bool>', 1, n)

        ens = dc.create_group('CellEnsembleData')
        ens.attrs.create('AttributeMatrixType', np.array([11], dtype=np.uint32))
        ens.attrs.create('TupleDimensions', np.array([nens], dtype=np.uint64))
        legacy_arr(ens, 'CrystalStructures', xtal, 'DataArray<uint32_t>', 1, nens)
        names = np.array(['Invalid Phase'] + ['Primary'] * (nens - 1), dtype=object)
        mn = ens.create_dataset('MaterialName', shape=(nens,), dtype=ASCII, data=names)
        mn.attrs.create('ComponentDimensions', np.array([1], dtype=np.uint64))
        mn.attrs.create('DataArrayVersion', np.array([2], dtype=np.int32))
        mn.attrs['ObjectType'] = np.bytes_('StringDataArray')
        mn.attrs['Tuple Axis Dimensions'] = np.bytes_(f'x={nens}')
        mn.attrs.create('TupleDimensions', np.array([nens], dtype=np.uint64))

    print(f'wrote {out}  ({n} orientations, {nens} ensembles, variant={variant})')


if __name__ == '__main__':
    main()
