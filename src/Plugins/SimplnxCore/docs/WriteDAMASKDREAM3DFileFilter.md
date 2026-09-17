# Write DAMASK DREAM3D File

## Group (Subgroup)

IO (Output)

## Description

This filter writes a minimal `.dream3d` file for the DREAM3D import functions in DAMASK 3.x and 4.x. The file contains one selected **Image Geometry** and only the arrays required for the selected material representation. The filter gives every exported object a canonical name so DAMASK scripts do not depend on the names used in the source pipeline.

The filter does not create DAMASK `.vti` or `material.yaml` files. DAMASK remains responsible for that conversion through `damask.GeomGrid.load_DREAM3D` and `damask.ConfigMaterial.load_DREAM3D`.

### Pointwise Representation

Pointwise output writes these canonical paths:

```text
DataContainer/CellData/EulerAngles
DataContainer/CellData/Phases
```

DAMASK identifies exact orientation-phase combinations and assigns the corresponding material indices. Experimental EBSD data can produce a large material configuration when most cell orientations are unique. Segment the data in DREAM3D-NX before export when grainwise data is preferred.

Convert pointwise data with:

```python
import damask

grid = damask.GeomGrid.load_DREAM3D("input.dream3d")
material = damask.ConfigMaterial.load_DREAM3D("input.dream3d")

grid.save("geometry.vti")
material.save("material.yaml")
```

### Grainwise Representation

Grainwise output writes these canonical paths:

```text
DataContainer/CellData/FeatureIds
DataContainer/CellFeatureData/EulerAngles
DataContainer/CellFeatureData/Phases
```

The filter preserves feature IDs, including feature zero. DAMASK does not assign a special meaning to feature zero beyond its use for unindexed points. Remove or replace zero-valued cells before export when those cells must not participate in the simulation.

Convert grainwise data with:

```python
import damask

grid = damask.GeomGrid.load_DREAM3D(
    "input.dream3d",
    feature_IDs="FeatureIds",
)
material = damask.ConfigMaterial.load_DREAM3D(
    "input.dream3d",
    grain_data="CellFeatureData",
)

grid.save("geometry.vti")
material.save("material.yaml")
```

### Optional Phase Names

When **Write Phase Names** is enabled, the selected StringArray is written to:

```text
DataContainer/CellEnsembleData/PhaseName
```

DAMASK uses numeric phase labels when the phase-name array is omitted.

### Geometry Units

DAMASK interprets spatial values in SI units. The filter converts the selected **Image Geometry** origin and spacing to meters and records `Meter` as the output geometry unit. The source geometry is not modified.

When the source geometry unit is `Unknown` or `Unspecified`, **Scale to Meters** supplies the conversion factor. For example, use *1.0e-6* when each source unit is one micrometer.

### Output Scope

The output file does not contain unrelated geometries, groups, or arrays from the source DataStructure. Numeric output arrays share their source data stores while the file is written, which avoids a second full-size in-memory copy.

The generated DAMASK material configuration contains material assignments and phase placeholders. Users must add the applicable constitutive phase and homogenization definitions before running a simulation.

% Auto generated parameter table will be inserted here

## Example Pipelines

No example pipeline is currently distributed with this filter.

## References

- [DAMASK processing tools](https://damask-multiphysics.org/documentation/reference/processing_tools/pre-processing.html)
- [DAMASK grid-solver file format](https://damask-multiphysics.org/documentation/reference/file_formats/grid_solver.html)
- [Rowenhorst et al., 2015](https://doi.org/10.1088/0965-0393/23/8/083501)

## License & Copyright

Please see the description file distributed with this **Plugin**

## DREAM3D-NX Help

If you need help, need to file a bug report or want to request a new feature, please head over to the [DREAM3DNX-Issues](https://github.com/BlueQuartzSoftware/DREAM3DNX-Issues/discussions) GitHub site where the community of DREAM3D-NX users can help answer your questions.
