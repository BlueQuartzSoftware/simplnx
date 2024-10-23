# .dream4d File Information


## "/" Root Object

### Required Attributes

|  Name | Type  | Value |
|-------|-------|-------|
| FileVersion | Null Term String | "8.0" |

### HDF5 Groups

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| DataStructure | HDF5 Group | Name will not change |
| Pipeline      | HDF5 Group | Name will not change |

## General Notes on the 'Pipeline' HDF5 Group

This holds a single null term STRING data set that holds the JSON formatted pipeline that was used to create the file. If you are trying to write a compatible .dream3d file this can be omitted.

## General Notes on the 'DataStructure' HDF5 Group

- The name will not change
- Each object within the 'DataStructure' group, including the 'DataStructure' group itself has a unique id associated with it.
    - Name: NextObjectId
    - Type: 64 Bit Unsigned Integer
    - Value: unique value
- Each HDF5 object within the 'DataStructure' HDF5 group represents either a Group, Geometry, AttributeMatrix or DataArray.

### DataObject Attributes

These attributes are on every DataObject:

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| Importable    | Scalar UINT64 | Only important if you are going to be writing a compatible .dream3d file |
| ObjectId      | Scalar UINT64 | Only important if you are going to be writing a compatible .dream3d file |
| ObjectType    | Null Term String | Describes the class of the object |


#### ObjectType Possible Values:

- DynamicListArray
- ScalarData
- BaseGroup +
- AttributeMatrix
- DataGroup
- IDataArray +
- DataArray
- IGeometry +
- IGridGeometry +
- RectGridGeom
- ImageGeom
- INodeGeometry0D +
- VertexGeom
- INodeGeometry1D +
- EdgeGeom
- INodeGeometry2D +
- QuadGeom
- TriangleGeom
- INodeGeometry3D +
- HexahedralGeom
- TetrahedralGeom
- INeighborList +
- NeighborList
- StringArray
- AbstractMontage +
- GridMontage

`+`: These theoretically should **never** show up as they are abstract classes.

### Specific ObjectType Attributes

Based on the ObjectType, there can be additional attriutes that can be read.

#### ImageGeometry

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| \_DIMENSIONS  | 3 x UINT64 | Regular Grid Dimensions as XYZ |
| \_ORIGIN      | 3 x Float32 | Origin as XYZ |
| \_SPACING     | 3 x Float32 | Spacing of each axis as XYZ |
| Cell Data ID  | Scalar UINT64 | Unique ObjectId of the Cell Data AttributeMatrix |

#### Vertex Geometry

#### Edge Geometry

#### Triangle Geometry

#### Quad Geometry

#### Tetrahedral Geometry

#### Hexahedral Geometry

#### AttributeMatrix

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| TupleDims     | N x UINT64 | Dimensions of the contained arrays from **fastest** to **slowest**. Note that for data arrays that are in the "Cell Data" inside an ImageGeometry or RectGrid Geometry, the dimensions will be in reverse order from the `_DIMENSIONS` attribute on the geometry in question. |

#### DataArray<>

The value of the `ObjectType` HDF5 attribute will have the numeric type encoded in the name. For example if the DataArray represents 32 bit floating point values, then the actual string will show up as `DataArray<float32>`. Here are the possibilities:

- DataArray<uint8>
- DataArray<int8>
- DataArray<uint16>
- DataArray<int16>
- DataArray<uint32>
- DataArray<int32>
- DataArray<uint64>
- DataArray<int64>
- DataArray<float32>
- DataArray<float64>

|  Name               |  Type      | Notes                |
|---------------------|------------|----------------------|
| ComponentDimensions | N x UINT64 | Dimensions of the components of the array. |
| TupleDimensions     | N x UINT64 | Dimensions of the contained arrays from **fastest** to **slowest**. Note that for data arrays that are in the "Cell Data" inside an ImageGeometry or RectGrid Geometry, the dimensions will be in reverse order from the `_DIMENSIONS` attribute on the geometry in question. |


