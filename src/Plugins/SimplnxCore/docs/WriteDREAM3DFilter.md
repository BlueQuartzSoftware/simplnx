# Write DREAM3D-NX File

## Description

This **Filter** writes the current `DataStructure` to disk as an HDF5 file with the `.dream3d` extension. The file contains every `DataArray`, geometry, attribute matrix, and group in the pipeline, along with the pipeline itself so the work can be re-run or inspected later.

### HDF5 Compression

DataArray datasets can be written with gzip (deflate) compression, dramatically reducing `.dream3d` file size at a modest CPU cost. gzip was chosen because it is non-proprietary and built into every standard HDF5 install — any tool that reads HDF5 (DREAM3D 6.x, DREAM3D-NX, HDFView, h5py, ParaView, Avizo, MATLAB) reads compressed `.dream3d` files transparently, with no special build or plugin required.

Two parameters control the behavior:

- **Use HDF5 Compression** (default: enabled). Master on/off switch. When disabled, datasets are written contiguous and uncompressed, matching the pre-compression behavior of older DREAM3D-NX builds.
- **Compression Level** (1–9, default: 5). Gzip level where 1 is fastest with the least compression and 9 is slowest with the most. Level 5 is a good balance for typical voxel and feature data; bumping to 9 usually only shrinks files a few more percent at significantly higher CPU cost.

A few details worth knowing:

- **Small arrays are written uncompressed** regardless of this setting. Arrays whose total size is under 16 KiB skip chunking/compression because the HDF5 chunk-index overhead would exceed the savings.
- **Non-DataArray metadata stays contiguous.** Only `DataArray` and `NeighborList` datasets participate in compression. Pipeline JSON, group markers, and attribute-matrix shape attributes remain unchanged — they are tiny and compressing them would slow file opens with no real benefit.
- **Chunk shape** is chosen automatically to target ~1 MiB per chunk along the tuple (outermost) dimension. This shape balances compression ratio against random-access efficiency and is not user-configurable.
- **Round-trip is lossless.** Compressed files read back byte-identical to their uncompressed equivalents.
- **File format is unchanged.** The `.dream3d` group layout and file-version tag are the same whether compression is on or off. Only the on-disk encoding of individual datasets changes, and HDF5 describes that encoding in each dataset's own metadata.

If a pipeline is imported from a legacy SIMPL (.json) file, compression is explicitly disabled so the converted pipeline preserves the exact on-disk encoding of the original SIMPL writer.

### XDMF File

When **Write Xdmf File** is enabled, the filter also produces a sidecar `.xdmf` file next to the `.dream3d` file. XDMF (eXtensible Data Model and Format) is a lightweight XML document that describes the structure of the HDF5 file in terms visualization tools understand: which datasets are vertices, which are cell data, how the mesh connectivity is laid out, and so on.

The `.xdmf` file does not contain a copy of the data — it is a metadata wrapper that points into the `.dream3d` file via HDF5 paths. The practical effect is that external visualization tools can open the `.xdmf` file and load the pipeline's geometries and attributes directly without any custom DREAM3D plugin. The primary consumers are:

- **ParaView** — open the `.xdmf` to visualize image geometries, triangle meshes, vertex clouds, and any DataArrays attached to them.
- **VisIt** — same workflow, same capabilities.

Enabling XDMF costs almost nothing (the file is small and fast to write) and is the recommended setting whenever visualization outside DREAM3D-NX is a possibility. It can safely be left off for intermediate pipeline outputs that will only be re-read by DREAM3D-NX itself.

% Auto generated parameter table will be inserted here

## Example Pipelines

ALL

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
