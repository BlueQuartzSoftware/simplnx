# Verify Triangle Winding

## Group (Subgroup)

Meshing (Cleanup)

## Description

***WARNING***: *If your mesh contains multiple disconnected objects in it, you should run **Label Triangle Geometry** beforehand, and convert the output (region ids) into face labels. Recommended procedure for such is described below in **Multi-Object Meshes**.*

This filter attempts to repair the windings for a mesh. This may not be possible due to the nature of how meshes are stored in the software.

### Key Nuances

While this filter doesn't modify anything more than the *Shared Faces List*, it has wider downstream implications for the mesh so it is recommended that it is executed as close to the start of the pipeline as possible. This ensures you don't have to recalculate extensive amounts of face data. This is especially pertinent in **Multi-Object Meshes**, where there are prerequisites to be satisfied as well.

The meshes treat both sides of each triangle as valid, the upside to this is that the amount of space needed to store meshes is reduced, but it means that giving every feature the proper winding is impossible. The order of execution is based on feature label numbering, so randomizing order may lead to more visual distinction.

This filter requires that vertices are unique, well formed meshes should not have an issue with this. The filter will check this at runtime, returning an error if duplicates are found. If this happens, run *Identify Duplicate Vertices* followed by *Remove flagged Vertices* prior to this.

This filter offers recalculating triangle normals as an option, however it is just a convience option. It simply calls Calculate Triangle Normals at the end.

Both *Surface Nets* and *Quick Mesh* (the only synthetic mesh generating currently) already utilize this algorithm at the end of their execution. Thus this filter will be redundant to run on meshes generated from the software's integrated algorithms.

This filter detemines candidates for winding reversal feature by feature based on the volume of the feature. This means its possible you may input a mesh that contains a feature with negative volume (that is untounched during winding repair) that gets all of it's windings reversed as a result. However, to our knowledge there does not exist a case where the feature would not be "inside-out" prior to execution. Essentially, this filter will addtionally preform winding reversal for you if it contains invalid features.

### Multi-Object Meshes

If your mesh has more than one disconnected object in it, *Label Traingle Geometry* should be run beforehand. This can be confusing because the output of the filter is not a face labels array. To convert the output to face labels, you need to merge this array with another array of zeros (created with *Create Data Array*) using *Combine Attribute Arrays*. The order is not strictly enforced but the internal paradigm is to place the zeros array above the *region ids* array (Option A below). The difference n output is as follows:

**Option A**:

- Zero's array first
- Region Ids array second

```console
{{0,1}, {0,1}, {0,1}, ...}
```

OR

**Option B**:

- Region Ids array first
- Zero's array second

```console
{{1,0}, {1,0}, {1,0}, ...}
```

The output of this combination will become your face labels for the mesh.

The relative pipeline for multiobject meshes should look something like this:

1. Identify Duplicate Vertices (optional)
2. Remove Flagged Vertices (optional)
3. Label CAD Geometry (LabelTriangleGeometry)
4. Create Data Array (fill with 0's)
5. Combine Attribute Arrays (make face labels [Zero Array + Region Ids])
6. Verify Triangle Winding

% Auto generated parameter table will be inserted here

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
