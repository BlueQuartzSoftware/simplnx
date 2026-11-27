# Connected Component Image Filter

Label the objects in a binary image.

## Group (Subgroup)

NXImageProcessing (Segmentation)

## Description

Labels connected objects in a single-component integer image. Zero is background. Every nonzero voxel is foreground, including negative values. The output is a UInt32 image. Background has label 0, and objects receive consecutive labels starting at 1 in the order they are first encountered.

**Fully Connected** is off by default. The filter joins voxels that share a face. Turn it on to also join voxels that touch at an edge or a corner.

### Provisional Label Limit Handling

The filter gives each uninterrupted foreground run along X a provisional ID. Several runs can join into a single object, so the number of runs can exceed the final object count.

- **Error** (default) accepts ID 4,294,967,295. If the next run needs a higher ID, execution stops before assigning it.
- **Warn and Continue (Legacy Behavior)** reports one warning and continues. The next ID wraps to 0, the background label, and later IDs repeat earlier ones. A run that receives ID 0 becomes background, and objects that touch it can become background too. A run that receives a repeated ID joins the earlier object with that ID. Downstream counts and statistics can be wrong. Which touching objects are lost can depend on image dimensions and available memory. Use this mode only to reproduce legacy results.

## Algorithm

The filter scans along X first, then Y and Z. It joins neighboring runs according to the selected connectivity and assigns each object its final label. When an image does not fit in memory, the filter can read and write disk-backed data in blocks and reread the input. Working storage grows with the number of runs, so an allocation can fail before the label limit is reached.

% Auto generated parameter table will be inserted here

## Example Pipelines

- `05_AM_XCT_Porosity_Segmentation.d3dpipeline` labels pores.
- `07_AM_Melt-Pool_and_Track_Inspection.d3dpipeline` labels the melt pool and spatter regions.
- `08_Powder_Particle_Watershed_Segmentation.d3dpipeline` labels watershed markers.
- `10_Binary_Mask_Repair_and_Skeletonization.d3dpipeline` labels the objects of a repaired mask.

## License & Copyright

Please see the description file distributed with this plugin.

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
