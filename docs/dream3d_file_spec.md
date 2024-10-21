# DREAM3D File Specification

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

Based on the ObjectType, there can be additional attriutes that can be read based on the ObjectType value

#### AttributeMatrix

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| TupleDims     | N x UINT64 | Dimensions of the contained arrays from **fastest** to **slowest**. Note that for data arrays that are in the "Cell Data" inside an ImageGeometry or RectGrid Geometry, the dimensions will be in reverse order from the `_DIMENSIONS` attribute on the geometry in question. |

#### DataArray\<XXXXX\>

The value of the `ObjectType` HDF5 attribute will have the numeric type encoded in the name. For example if the DataArray represents 32 bit floating point values, then the actual string will show up as `DataArray<float32>`. Here are the possibilities:

- DataArray\<uint8\>
- DataArray\<int8\>
- DataArray\<uint16\>
- DataArray\<int16\>
- DataArray\<uint32\>
- DataArray\<int32\>
- DataArray\<uint64\>
- DataArray\<int64\>
- DataArray\<float32\>
- DataArray\<float64\>

|  Name               |  Type      | Notes                |
|---------------------|------------|----------------------|
| ComponentDimensions | N x UINT64 | Dimensions of the components of the array. |
| TupleDimensions     | N x UINT64 | Dimensions of the contained arrays from **fastest** to **slowest**. Note that for data arrays that are in the "Cell Data" inside an ImageGeometry or RectGrid Geometry, the dimensions will be in reverse order from the `_DIMENSIONS` attribute on the geometry in question. |

#### ImageGeometry

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| \_DIMENSIONS  | 3 x UINT64 | Regular Grid Dimensions as XYZ |
| \_ORIGIN      | 3 x Float32 | Origin as XYZ |
| \_SPACING     | 3 x Float32 | Spacing of each axis as XYZ |
| Cell Data ID  | Scalar UINT64 | Unique ObjectId of the Cell Data AttributeMatrix |

#### Vertex Geometry

The HDF5 Group contains the following required attributes:

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| ObjectType     | STRING    | VertexGeom           |
| Vertex List ID | UINT64    | ObjectId of the SharedVertexList data array. |
| Vertex Data ID | UINT64    | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the vertex |

The Vertex Geometry HDF5 Group should contain at the minimum 2 Data Arrays:

|  Name             |  Type       | Notes                |
|-------------------|-------------|----------------------|
| SharedVertexList  | 3 x Float32 | Each set represents the XYZ coordinate of a vertex |

#### Edge Geometry

The HDF5 Group contains the following required attributes:

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| ObjectType     | STRING    | EdgeGeom |
| Vertex List ID | UINT64    | ObjectId of the SharedVertexList data array. |
| Vertex Data ID | UINT64    | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the vertex |
| Edge List ID  | UINT64     | ObjectId of the SharedEdgeList data array |
| Edge Data ID  | UINT64     | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the edges |


The Edge Geometry HDF5 Group should contain at the minimum 2 Data Arrays that represent the
vertices (Nodes) and faces (Elements) of the Edge Geometry. Shown below are the default
names that are used. The arrays can be named differently by the user.

|  Name             |  Type       | Notes                |
|-------------------|-------------|----------------------|
| SharedVertexList  | 3 x Float32 | Each set represents the XYZ coordinate of a vertex |
| SharedEdgeList    | 2 x UINT64  | Each set represents the 2 vertices of the Edge where each value is an index into the SharedVertexList |

#### Triangle Geometry

The HDF5 Group contains the following required attributes:

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| ObjectType     | STRING    | TriangleGeom |
| Vertex List ID | UINT64    | ObjectId of the SharedVertexList data array. |
| Vertex Data ID | UINT64    | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the vertex |
| Face List ID  | UINT64     | ObjectId of the SharedTriangleList data array |
| Face Data ID  | UINT64     | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the triangle faces  |

The Triangle Geometry HDF5 Group should contain at the minimum 2 Data Arrays that represent the
vertices (Nodes) and faces (Elements) of the Triangle Geometry. Shown below are the default
names that are used. The arrays can be named differently by the user.

|  Name             |  Type       | Notes                |
|-------------------|-------------|----------------------|
| SharedVertexList  | 3 x Float32 | Each set represents the XYZ coordinate of a vertex |
| SharedFaceList     | 3 x UINT64  | Each set represents the 3 vertices of the triangle where each value is an index into the SharedVertexList |

#### Quad Geometry

The HDF5 Group contains the following required attributes:

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| ObjectType     | STRING    | QuadGeom |
| Vertex List ID | UINT64    | ObjectId of the SharedVertexList data array. |
| Vertex Data ID | UINT64    | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the vertex |
| Face List ID  | UINT64     | ObjectId of the SharedFaces data array |
| Face Data ID  | UINT64     | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the Quad faces  |

The Quad Geometry HDF5 Group should contain at the minimum 2 Data Arrays that represent the
vertices (Nodes) and faces (Elements) of the Quad Geometry. Shown below are the default
names that are used. The arrays can be named differently by the user.

|  Name             |  Type       | Notes                |
|-------------------|-------------|----------------------|
| SharedVertexList  | 3 x Float32 | Each set represents the XYZ coordinate of a vertex |
| SharedFaceList    | 4 x UINT64  | Each set represents the 4 vertices of the triangle where each value is an index into the SharedVertexList |

#### Tetrahedral Geometry

The HDF5 Group contains the following required attributes:

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| ObjectType     | STRING    | QuadGeom |
| Vertex List ID | UINT64    | ObjectId of the SharedVertexList data array. |
| Vertex Data ID | UINT64    | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the vertex |
| Cell List ID  | UINT64     | ObjectId of the SharedCell data array |
| Cell Data ID  | UINT64     | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the Tetrahedral cells  |

The Tetrahedral Geometry HDF5 Group should contain at the minimum 2 Data Arrays that represent the
vertices (Nodes) and cells (Elements) of the Tetrahedral Geometry. Shown below are the default
names that are used. The arrays can be named differently by the user.

|  Name             |  Type       | Notes                |
|-------------------|-------------|----------------------|
| SharedVertexList  | 3 x Float32 | Each set represents the XYZ coordinate of a vertex |
| SharedCellList    | 4 x UINT64  | Each set represents the 4 vertices of the triangle where each value is an index into the SharedVertexList |

#### Hexahedral Geometry

The HDF5 Group contains the following required attributes:

|  Name         |  Type      | Notes                |
|---------------|------------|----------------------|
| ObjectType     | STRING    | QuadGeom |
| Vertex List ID | UINT64    | ObjectId of the SharedVertexList data array. |
| Vertex Data ID | UINT64    | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the vertex |
| Cell List ID  | UINT64     | ObjectId of the SharedCell data array |
| Cell Data ID  | UINT64     | ObjectId of the Attribute Matrix that holds data arrays geometrically located at the Hexahedral cells  |

The Hexahedral Geometry HDF5 Group should contain at the minimum 2 Data Arrays that represent the
vertices (Nodes) and cells (Elements) of the Hexahedral Geometry. Shown below are the default
names that are used. The arrays can be named differently by the user.

|  Name             |  Type       | Notes                |
|-------------------|-------------|----------------------|
| SharedVertexList  | 3 x Float32 | Each set represents the XYZ coordinate of a vertex |
| SharedCellList    | 8 x UINT64  | Each set represents the 8 vertices of the triangle where each value is an index into the SharedVertexList |
