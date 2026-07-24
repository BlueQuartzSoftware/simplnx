# Create Attribute Matrix

## Group (Subgroup)

Core (Generation)

## Description

This **Filter** creates a new empty **Attribute Matrix** at a user-specified location in the **Data Structure**. The matrix is created with a user-specified set of *tuple dimensions* but contains no arrays initially; subsequent filters can add arrays to it.

### What is an Attribute Matrix?

An **Attribute Matrix** is a container for **Data Arrays** that share the same *number of tuples* and the same *tuple shape*. All arrays inside one Attribute Matrix represent values "at the same set of points" -- for example, all per-cell arrays of an Image Geometry live in a single Cell Attribute Matrix, all per-feature arrays for a segmented dataset live in a single Feature Attribute Matrix, and all per-ensemble (phase) arrays live in an Ensemble Attribute Matrix.

This shared-tuple-shape contract is enforced: any array added to an existing Attribute Matrix must match the matrix's tuple dimensions. Use an Attribute Matrix when the values you are grouping are all parallel arrays over the same domain.

### When to Use This Filter

- Pre-building the destination for a future filter that creates new feature-level or ensemble-level arrays.
- Creating a custom Attribute Matrix for organizational purposes (e.g., grouping all "computed" arrays separately from the "raw" arrays in the same Cell-level domain).
- Restructuring imported data so that arrays sharing a common dimension end up in the same matrix.

For containers that hold heterogeneous data (arrays of different sizes, geometries, sub-groups), use [Create Data Group](CreateDataGroupFilter.md) instead.

### Tuple Dimensions

For a 3D volume with axes X=3, Y=4, Z=5, the *Tuple Dimensions* are (3, 4, 5). For a 1D feature-list with 100 features, the tuple dimensions are (100). The product of all dimensions equals the total number of tuples that any array added to this matrix must have.

### Required Input Sources

- **Parent Data Object Path** -- an existing **DataGroup** or **Geometry** under which the new Attribute Matrix will be created. Typically the output of [Create Image Geometry](CreateImageGeometryFilter.md), [Create Geometry](CreateGeometryFilter.md), or [Create Data Group](CreateDataGroupFilter.md).

% Auto generated parameter table will be inserted here

## Example Pipelines

## License & Copyright

Please see the description file distributed with this **Plugin**
