# Phase 8: Post-Refactoring Interface Review

## Overview

This document is an updated review of the `src/simplnx/` codebase **after** completion of Phases 2-7 of the COM-style API refactoring. It catalogs every class that participates in an interface or abstract base class hierarchy, reflecting all changes made during the refactoring effort.

Classes are categorized as:

- **Pure Interface**: All methods are pure virtual + virtual destructor, no data members, no concrete method bodies
- **Near-Pure Interface**: Almost all methods pure virtual, with only trivial delegation helpers
- **Abstract Base Class**: Mix of pure virtual methods and concrete implementations
- **Concrete Base Class**: No pure virtual methods but designed for inheritance

---

## Summary Statistics

| Category | Count | Classes |
|----------|-------|---------|
| **Pure Interface** | 21 | `IFilter`, `IPlugin`, `IPipelineNode`, `IDataObject`, `IGeometry`, `IGridGeometry`, `INodeGeometry0D`, `INodeGeometry1D`, `INodeGeometry2D`, `INodeGeometry3D`, `ISegmentFeatures`, `IMaskCompare`, `IJsonFilterParser`, `IPluginLoader`, `IDataStore`, `IListStore`, `IDataFactory`, `IParameter`, `IArrayThreshold`, `IJsonPipelineParser`, `IStringStore` |
| **Pure Interface (large)** | 1 | `IDataStructure` (39+ pure virtual methods) |
| **Near-Pure Interface** | 0 | *(all promoted to Pure Interface in Phases 9-14)* |
| **Abstract Base Class** | 35 | See detailed sections below |
| **Concrete Base Class** | 1 | `ParallelAlgorithm` (protected ctor, no virtuals) |

### Changes Since Phase 1 Review

| What Changed | Details |
|-------------|---------|
| **New pure interfaces extracted** | `IFilter` (Phase 3), `IPipelineNode` (Phase 4), `IPlugin` (Phase 5), `IGeometry` + `IGridGeometry` + `INodeGeometry0D-3D` (Phase 6), `IDataStructure` (Phase 7), `ISegmentFeatures` (Phase 2), `IParameter` promoted to pure (Phase 10), `IArrayThreshold` extracted (Phase 11), `IJsonPipelineParser` extracted (Phase 11), `IDataObject` extracted (Phase 14), `IStringStore` promoted (Phase 15) |
| **Classes renamed** | `IFilter` -> `AbstractFilter`, `SegmentFeatures` -> `AbstractSegmentFeatures`, `IGeometry` -> `AbstractGeometry`, `IGridGeometry` -> `AbstractGridGeometry`, `INodeGeometry0D-3D` -> `AbstractNodeGeometry0D-3D`, `IArray` -> `AbstractArray` (Phase 11), `IDataArray` -> `AbstractDataArray` (Phase 11), `INeighborList` -> `AbstractNeighborList` (Phase 11), `IArrayThreshold` -> `AbstractArrayThreshold` (Phase 11), `IJsonPipelineParser` -> `AbstractJsonPipelineParser` (Phase 11), `IDataIOManager` -> `AbstractDataIOManager` (Phase 12), `DataObject` -> `AbstractDataObject` (Phase 14), `IDataCreationAction` -> `AbstractDataCreationAction` (Phase 15), `IDataIO` -> `AbstractDataIO` + 6 HDF5 IO classes (Phase 15), `AbstractStringStore` -> `IStringStore` (Phase 15) |
| **Dead code removed** | `src/simplnx/Utilities/Parsing/JSON.deprecated/` directory (15 files) removed in Phase 13 — not compiled or referenced anywhere |
| **Misleading "I" prefix fixed** | `IParallelAlgorithm` -> `ParallelAlgorithm` |
| **Total new pure interfaces** | 15 (up from 4 in Phase 1) |

---

## Pure Interface Classes

### 1. `IFilter`
- **File**: `src/simplnx/Filter/IFilter.hpp`
- **Inherits from**: Nothing
- **Created in**: Phase 3
- **Pure virtual methods (13)**: `name()`, `className()`, `uuid()`, `humanName()`, `defaultTags()`, `parameters()`, `parametersVersion()`, `clone()`, `preflight()`, `execute()`, `toJson()`, `fromJson()`, `getDefaultArguments()`
- **Nested types**: `UniquePointer`, `Message`, `ProgressMessage`, `MessageHandler`, `PreflightValue`, `PreflightResult`, `ExecuteResult`
- **Concrete methods**: 1 (`MakePreflightErrorResult` static helper)

### 2. `IPlugin`
- **File**: `src/simplnx/Plugin/IPlugin.hpp`
- **Inherits from**: Nothing
- **Created in**: Phase 5
- **Pure virtual methods (11)**: `getName()`, `getDescription()`, `getId()`, `getVendor()`, `containsFilterId()`, `createFilter()`, `getFilterHandles()`, `getFilterCount()`, `getDataIOManagers()`, `getSimplToSimplnxMap()`, `setOocTempDirectory()`
- **Nested types**: `IdType`, `FilterContainerType`, `IOManagerPointer`, `IOManagersContainerType`, `SIMPLData`, `SIMPLMapType`

### 3. `IPipelineNode`
- **File**: `src/simplnx/Pipeline/IPipelineNode.hpp`
- **Inherits from**: Nothing
- **Created in**: Phase 4
- **Pure virtual methods (24)**: `getType()`, `getName()`, `getParentPipeline()`, `setParentPipeline()`, `hasParentPipeline()`, `preflight()` (x2), `execute()`, `deepCopy()`, `getFaultState()`, `hasErrors()`, `hasWarnings()`, `isDisabled()`, `isEnabled()`, `setDisabled()`, `setEnabled()`, `getDataStructure()`, `getPreflightStructure()`, `clearDataStructure()`, `clearPreflightStructure()`, `isPreflighted()`, `toJson()`, `getPrecedingPipeline()`, `getPipelineExecutionContext()`
- **Nested types**: `NodeType`, `RenamedPath`, `RenamedPaths`

### 4. `IDataStructure`
- **File**: `src/simplnx/DataStructure/IDataStructure.hpp`
- **Inherits from**: Nothing
- **Created in**: Phase 7
- **Pure virtual methods (39+)**: All non-template public methods of `DataStructure` — `getSize()`, `clear()`, `getId()`, `containsData()` (x2), `getData()` (x7 overloads), `getDataRef()` (x4), `getSharedData()` (x4), `removeData()` (x3), `getLinkedPath()`, `makePath()`, `getDataPathsForId()`, `getAllDataPaths()`, `getAllDataObjectIds()`, `getTopLevelData()`, `getDataMap()`, `insert()`, `getNextId()`, `setAdditionalParent()`, `removeParent()`, `begin()` (x2), `end()` (x2), `validateNumberOfTuples()`, `resetIds()`, `exportHierarchyAsGraphViz()`, `exportHierarchyAsText()`, `setNextId()`, `getRootGroup()`, `flush()`, `memoryUsage()`, `transferDataArraysOoc()`, `validateGeometries()`, `validateAttributeMatrices()`
- **Type aliases**: `SignalType`, `Iterator`, `ConstIterator`
- **Note**: Template methods (`getDataAs<T>`, `getDataRefAs<T>`, etc.) remain on `DataStructure` since C++ templates cannot be virtual.

### 5. `IGeometry`
- **File**: `src/simplnx/DataStructure/Geometry/IGeometry.hpp`
- **Inherits from**: Nothing (standalone)
- **Created in**: Phase 6
- **Pure virtual methods (6)**: `getGeomType()`, `getNumberOfCells()`, `findElementSizes()`, `getParametricCenter()`, `getShapeFunctions()`, `validate()`
- **Enums**: `Type` (8 geometry types), `LengthUnit` (27 units)
- **Type aliases (10)**: `StatusCode`, `MeshIndexType`, `MeshIndexArrayType`, `SharedVertexList`, `SharedEdgeList`, `SharedFaceList`, `SharedTriList`, `SharedQuadList`, `SharedTetList`, `SharedHexList`, `ElementDynamicList`
- **Constants**: `k_VoxelSizes`, `k_GeomTypeStrings`

### 6. `IGridGeometry`
- **File**: `src/simplnx/DataStructure/Geometry/IGridGeometry.hpp`
- **Inherits from**: Nothing (standalone)
- **Created in**: Phase 6
- **Pure virtual methods (19)**: `getDimensions()`, `setDimensions()`, `getNumXCells()`, `getNumYCells()`, `getNumZCells()`, `getPlaneCoordsf()` (x3), `getPlaneCoords()` (x3), `getCoordsf()` (x3), `getCoords()` (x3), `getIndex()` (x2)
- **Constants**: `k_CellAttributeMatrixName`

### 7. `INodeGeometry0D`
- **File**: `src/simplnx/DataStructure/Geometry/INodeGeometry0D.hpp`
- **Inherits from**: Nothing (standalone)
- **Created in**: Phase 6
- **Pure virtual methods**: 0 (marker/constants-only interface)
- **Constants**: `k_SharedVertexListName`, `k_VertexAttributeMatrixName`

### 8. `INodeGeometry1D`
- **File**: `src/simplnx/DataStructure/Geometry/INodeGeometry1D.hpp`
- **Inherits from**: Nothing (standalone)
- **Created in**: Phase 6
- **Pure virtual methods (4)**: `getNumberOfVerticesPerEdge()`, `findElementsContainingVert()`, `findElementNeighbors()`, `findElementCentroids()`
- **Constants (6)**: `k_EdgeAttributeMatrixName`, `k_EdgeFeatureAttributeMatrix`, `k_SharedEdgeListName`, `k_UnsharedEdgesListName`, `k_UnsharedFacesListName`, `k_NumEdgeVerts`

### 9. `INodeGeometry2D`
- **File**: `src/simplnx/DataStructure/Geometry/INodeGeometry2D.hpp`
- **Inherits from**: Nothing (standalone)
- **Created in**: Phase 6
- **Pure virtual methods (3)**: `getNumberOfVerticesPerFace()`, `findEdges()`, `findUnsharedEdges()`
- **Constants (3)**: `k_FaceAttributeMatrixName`, `k_FaceFeatureAttributeMatrixName`, `k_SharedFacesListName`

### 10. `INodeGeometry3D`
- **File**: `src/simplnx/DataStructure/Geometry/INodeGeometry3D.hpp`
- **Inherits from**: Nothing (standalone)
- **Created in**: Phase 6
- **Pure virtual methods (3)**: `findFaces()`, `findUnsharedFaces()`, `getNumberOfVerticesPerCell()`
- **Constants (2)**: `k_PolyhedronDataName`, `k_SharedPolyhedronListName`

### 11. `ISegmentFeatures`
- **File**: `src/simplnx/Utilities/ISegmentFeatures.hpp`
- **Inherits from**: Nothing
- **Created in**: Phase 2
- **Pure virtual methods (5)**: `execute()`, `getSeed()`, `determineGrouping()`, `randomizeFeatureIds()`, `initializeStaticVoxelSeedGenerator()`
- **Nested types**: `SeedGenerator`, `NeighborScheme`, `CompareFunctor`

### 12. `IMaskCompare`
- **File**: `src/simplnx/Utilities/MaskCompareUtilities.hpp`
- **Inherits from**: Nothing
- **Pre-existing** (unchanged)
- **Pure virtual methods (7)**: `bothTrue()`, `bothFalse()`, `isTrue()`, `setValue()`, `getNumberOfTuples()`, `getNumberOfComponents()`, `countTrueValues()`
- **Concrete subclasses**: `BoolMaskCompare`, `UInt8MaskCompare`

### 13. `IJsonFilterParser`
- **File**: `src/simplnx/Utilities/Parsing/JSON/IJsonFilterParser.hpp`
- **Inherits from**: Nothing
- **Pre-existing** (unchanged)
- **Pure virtual methods (2)**: `fromJson()`, `toJson()`
- **Concrete subclasses**: `JsonFilterParserV6`, `JsonFilterParserV7`

### 14. `IPluginLoader`
- **File**: `src/simplnx/Plugin/PluginLoader.hpp`
- **Inherits from**: Nothing
- **Pre-existing** (unchanged)
- **Pure virtual methods (3)**: `isLoaded()`, `getPlugin()` (const/non-const)
- **Concrete subclasses**: `InMemoryPluginLoader`, `PluginLoader`

---

### 15. `IDataStore` *(promoted to Pure Interface in Phase 9)*
- **File**: `src/simplnx/DataStructure/IDataStore.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (17)**: `getNumberOfTuples()`, `getTupleShape()`, `getNumberOfComponents()`, `getComponentShape()`, `getChunkShape()`, `getSize()`, `size()`, `empty()`, `resizeTuples()`, `getDataType()`, `getStoreType()`, `getDataFormat()`, `getTypeSize()`, `deepCopy()`, `createNewInstance()`, `writeBinaryFile()` (x2)
- **Concrete methods**: 0
- **Concrete subclass chain**: `AbstractDataStore<T>` -> `DataStore<T>`, `EmptyDataStore<T>`

### 16. `IListStore` *(promoted to Pure Interface in Phase 9)*
- **File**: `src/simplnx/DataStructure/IListStore.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (10)**: `getNumberOfTuples()`, `getTupleShape()`, `resizeTuples()`, `clearAllLists()`, `getListSize()`, `getNumberOfLists()`, `size()`, `clear()`, `readHdf5()`, `writeHdf5()`
- **Concrete methods**: 0
- **Concrete subclass chain**: `AbstractListStore<T>` -> `ListStore<T>`

### 17. `IDataFactory` *(confirmed Pure Interface in Phase 9)*
- **File**: `src/simplnx/DataStructure/IO/Generic/IDataFactory.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (1)**: `getDataType()`
- **Concrete methods**: 0
- **Concrete subclass**: `HDF5::IDataIO`

### 18. `IArrayThreshold` *(extracted in Phase 11)*
- **File**: `src/simplnx/Utilities/IArrayThreshold.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (6)**: `isInverted()`, `setInverted()`, `getUnionOperator()`, `setUnionOperator()`, `getRequiredPaths()`, `toJson()`
- **Nested types**: `UnionOperator` enum
- **Notes**: Extracted from old `IArrayThreshold` class (renamed to `AbstractArrayThreshold`).

### 19. `IJsonPipelineParser` *(refactored to pure interface in Phase 11)*
- **File**: `src/simplnx/Utilities/Parsing/JSON/IJsonPipelineParser.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (2)**: `fromJson()`, `toJson()`
- **Notes**: Data member and concrete method moved to `AbstractJsonPipelineParser`.

### 20. `IDataObject` *(extracted in Phase 14)*
- **File**: `src/simplnx/DataStructure/IDataObject.hpp`
- **Inherits from**: Nothing
- **Created in**: Phase 14
- **Pure virtual methods (21)**: `getDataObjectType()`, `isGroup()`, `getTypeName()`, `getId()`, `getDataStructure()` (x2), `getDataStructureRef()` (x2), `getName()`, `canRename()`, `rename()`, `getParentIds()`, `clearParents()`, `getDataPaths()`, `getMetadata()` (x2), `hasParent()`, `flush()`, `memoryUsage()`
- **Nested types**: `Type` enum (27 values), `EnumType`, `IdType`, `OptionalId`, `ParentCollectionType`
- **Notes**: Root pure interface for the entire data object hierarchy. `deepCopy()`/`shallowCopy()` remain on `AbstractDataObject` as they return concrete types.

---

## Abstract Base Classes

### DataObject Hierarchy

#### 18. `AbstractDataObject` *(renamed from DataObject in Phase 14)*
- **File**: `src/simplnx/DataStructure/AbstractDataObject.hpp`
- **Inherits from**: `IDataObject` (Phase 14)
- **Pure virtual methods (3)**: `deepCopy()`, `shallowCopy()`, `getTypeName()`
- **Concrete overrides (18)**: All other IDataObject methods
- **Data members**: 5
- **Notes**: Root of the entire data object hierarchy. Renamed from `DataObject` in Phase 14.

#### 19. `BaseGroup`
- **File**: `src/simplnx/DataStructure/BaseGroup.hpp`
- **Inherits from**: `DataObject`
- **Own pure virtual methods**: 0 (3 inherited from DataObject remain unimplemented)
- **Concrete methods**: ~25 (container management, iterators)
- **Data members**: 1 (`m_DataMap`)

#### 20. `AbstractArray` *(renamed from IArray in Phase 11)*
- **File**: `src/simplnx/DataStructure/AbstractArray.hpp`
- **Inherits from**: `DataObject`
- **Own pure virtual methods (12)**: `getArrayType()`, `getSize()`, `size()`, `empty()`, `getTupleShape()`, `getComponentShape()`, `getNumberOfTuples()`, `getNumberOfComponents()`, `swapTuples()`, `resizeTuples()`, `toString()`, `setValueFromString()`
- **Notes**: Near-pure — all own methods are pure virtual, no own data members. `k_TypeName = "IArray"` preserved for serialization.

#### 21. `AbstractDataArray` *(renamed from IDataArray in Phase 11)*
- **File**: `src/simplnx/DataStructure/AbstractDataArray.hpp`
- **Inherits from**: `AbstractArray`
- **Own pure virtual methods (4)**: `copyTuple()`, `getIDataStore()` (const/non-const), `getDataFormat()`
- **Concrete methods**: ~10 (delegation to IDataStore)
- **Notes**: `k_TypeName = "IDataArray"` preserved for serialization. `DataObject::Type::AbstractDataArray` (numeric value 6 unchanged).

#### 22. `AbstractNeighborList` *(renamed from INeighborList in Phase 11)*
- **File**: `src/simplnx/DataStructure/AbstractNeighborList.hpp`
- **Inherits from**: `AbstractArray`
- **Own pure virtual methods (4)**: `getIListStore()` (const/non-const), `copyTuple()`, `getDataType()`
- **Concrete methods**: ~12
- **Data members**: 1
- **Notes**: `k_TypeName = "INeighborList"` preserved for serialization. `DataObject::Type::AbstractNeighborList` (numeric value 22 unchanged).

### Geometry Hierarchy (Refactored in Phase 6)

#### 23. `AbstractGeometry`
- **File**: `src/simplnx/DataStructure/Geometry/AbstractGeometry.hpp`
- **Inherits from**: `BaseGroup`, `IGeometry` (multiple inheritance)
- **Re-declares IGeometry pure virtuals (6)**: All `= 0` (deferred to concrete subclasses)
- **Concrete methods**: ~15 (element sizes, units, dimensionality)
- **Data members**: 4
- **Key**: `k_TypeName = "IGeometry"` (serialization compatibility), `using IGeometry::Type;` (ambiguity resolution)

#### 24. `AbstractGridGeometry`
- **File**: `src/simplnx/DataStructure/Geometry/AbstractGridGeometry.hpp`
- **Inherits from**: `AbstractGeometry`, `IGridGeometry` (multiple inheritance)
- **Re-declares IGridGeometry pure virtuals (19)**: All `= 0`
- **Concrete methods**: ~11 (cell data management, `validate()`)
- **Data members**: 1
- **Key**: `k_TypeName = "IGridGeometry"`
- **Concrete subclasses**: `ImageGeom`, `RectGridGeom`

#### 25. `AbstractNodeGeometry0D`
- **File**: `src/simplnx/DataStructure/Geometry/AbstractNodeGeometry0D.hpp`
- **Inherits from**: `AbstractGeometry`, `INodeGeometry0D` (multiple inheritance)
- **Concrete methods**: ~26 (vertex management, bounding box, coordinate access)
- **Data members**: 2
- **Key**: `k_TypeName = "INodeGeometry0D"`
- **Concrete subclass**: `VertexGeom`

#### 26. `AbstractNodeGeometry1D`
- **File**: `src/simplnx/DataStructure/Geometry/AbstractNodeGeometry1D.hpp`
- **Inherits from**: `AbstractNodeGeometry0D`, `INodeGeometry1D` (multiple inheritance)
- **Re-declares 3 INodeGeometry1D pure virtuals as `= 0`**: `findElementsContainingVert()`, `findElementNeighbors()`, `findElementCentroids()`
- **Concrete methods**: ~37 (edge management)
- **Data members**: 5
- **Key**: `k_TypeName = "INodeGeometry1D"`
- **Concrete subclass**: `EdgeGeom`

#### 27. `AbstractNodeGeometry2D`
- **File**: `src/simplnx/DataStructure/Geometry/AbstractNodeGeometry2D.hpp`
- **Inherits from**: `AbstractNodeGeometry1D`, `INodeGeometry2D` (multiple inheritance)
- **Re-declares 3 INodeGeometry2D pure virtuals as `= 0`**: `getNumberOfVerticesPerFace()`, `findEdges()`, `findUnsharedEdges()`
- **Concrete methods**: ~27 (face management)
- **Data members**: 3
- **Key**: `k_TypeName = "INodeGeometry2D"`
- **Concrete subclasses**: `TriangleGeom`, `QuadGeom`

#### 28. `AbstractNodeGeometry3D`
- **File**: `src/simplnx/DataStructure/Geometry/AbstractNodeGeometry3D.hpp`
- **Inherits from**: `AbstractNodeGeometry2D`, `INodeGeometry3D` (multiple inheritance)
- **Re-declares 3 INodeGeometry3D pure virtuals as `= 0`**: `findFaces()`, `findUnsharedFaces()`, `getNumberOfVerticesPerCell()`
- **Concrete methods**: ~27 (polyhedra management)
- **Data members**: 3
- **Key**: `k_TypeName = "INodeGeometry3D"`
- **Concrete subclasses**: `TetrahedralGeom`, `HexahedralGeom`

### Filter/Parameter Hierarchy (Refactored in Phase 3)

#### 29. `AbstractFilter`
- **File**: `src/simplnx/Filter/AbstractFilter.hpp`
- **Inherits from**: `IFilter`
- **Own pure virtual methods (2)**: `preflightImpl()`, `executeImpl()` (both protected)
- **Concrete overrides (6)**: `defaultTags()`, `preflight()`, `execute()`, `toJson()`, `fromJson()`, `getDefaultArguments()`
- **Still defers (7)**: `name()`, `className()`, `uuid()`, `humanName()`, `parameters()`, `parametersVersion()`, `clone()`
- **Notes**: Template Method pattern — `preflight()` delegates to `preflightImpl()`, `execute()` to `executeImpl()`

#### 30. `IParameter` *(promoted to Pure Interface in Phase 10)*
- **File**: `src/simplnx/Filter/IParameter.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (14)**: `uuid()`, `name()`, `humanName()`, `helpText()`, `defaultValue()`, `type()`, `acceptedTypes()`, `getVersion()`, `clone()`, `toJson()`, `fromJson()`, `construct()`
- **Concrete methods**: 0
- **Notes**: Promoted to pure interface in Phase 10. `toJson()`, `fromJson()`, `construct()` moved to `AbstractParameter`. Template-method hooks `toJsonImpl()`/`fromJsonImpl()` moved to `AbstractParameter` as protected pure virtuals.

#### 31. `AbstractParameter`
- **File**: `src/simplnx/Filter/AbstractParameter.hpp`
- **Inherits from**: `IParameter`
- **Implements (final) (3)**: `name()`, `humanName()`, `helpText()`
- **Implements (override) (3)**: `toJson()`, `fromJson()`, `construct()`
- **Own protected pure virtuals (2)**: `toJsonImpl()`, `fromJsonImpl()` (template-method hooks, moved from IParameter in Phase 10)
- **Data members**: 3

#### 32. `ValueParameter`
- **File**: `src/simplnx/Filter/ValueParameter.hpp`
- **Inherits from**: `AbstractParameter`
- **Implements (final) (1)**: `type()`
- **Adds pure virtual (1)**: `validate()`

#### 33. `DataParameter`
- **File**: `src/simplnx/Filter/DataParameter.hpp`
- **Inherits from**: `AbstractParameter`
- **Implements (final) (1)**: `type()`
- **Adds pure virtuals (2)**: `mutability()`, `validate(DataStructure&, ...)`

#### 34. `MutableDataParameter`
- **File**: `src/simplnx/Filter/MutableDataParameter.hpp`
- **Inherits from**: `DataParameter`
- **Implements (final) (1)**: `mutability()`
- **Adds pure virtual (1)**: `resolve(DataStructure&, ...)`

#### 35. `ConstDataParameter`
- **File**: `src/simplnx/Filter/ConstDataParameter.hpp`
- **Inherits from**: `DataParameter`
- **Implements (final) (1)**: `mutability()`
- **Adds pure virtual (1)**: `resolve(const DataStructure&, ...)`

### Data Action Hierarchy

#### 36. `IDataAction`
- **File**: `src/simplnx/Filter/Output.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (2)**: `apply()`, `clone()`
- **Notes**: Pure interface with protected constructor pattern.

#### 37. `AbstractDataCreationAction` *(renamed from IDataCreationAction in Phase 15)*
- **File**: `src/simplnx/Filter/Output.hpp`
- **Inherits from**: `IDataAction`
- **Adds pure virtual (1)**: `getAllCreatedPaths()`
- **Concrete (1)**: `getCreatedPath()`
- **Data members**: 1

### Plugin Hierarchy (Refactored in Phase 5)

#### 38. `AbstractPlugin`
- **File**: `src/simplnx/Plugin/AbstractPlugin.hpp`
- **Inherits from**: `IPlugin`
- **Still pure virtual (1)**: `getSimplToSimplnxMap()` (override = 0)
- **Concrete overrides (10)**: All other IPlugin methods
- **Data members**: 7

### Pipeline Hierarchy (Refactored in Phase 4)

#### 39. `AbstractPipelineNode`
- **File**: `src/simplnx/Pipeline/AbstractPipelineNode.hpp`
- **Inherits from**: `IPipelineNode`
- **Re-declares as pure (6)**: `getType()`, `getName()`, `preflight()` (x2), `execute()`, `deepCopy()`
- **Own pure virtual (1)**: `toJsonImpl()` (protected)
- **Concrete overrides (18)**: All other IPipelineNode methods
- **Data members**: 11+

#### 40. `AbstractPipelineFilter`
- **File**: `src/simplnx/Pipeline/AbstractPipelineFilter.hpp`
- **Inherits from**: `AbstractPipelineNode`
- **Implements (1)**: `getType()` -> returns `NodeType::Filter`
- **Adds pure virtual (1)**: `getFilterType()`

### Data Store Hierarchy

#### 41. `AbstractDataStore<T>` (template)
- **File**: `src/simplnx/DataStructure/AbstractDataStore.hpp`
- **Inherits from**: `IDataStore`
- **Own pure virtual methods (20)**: Element access, arithmetic ops, chunk ops, HDF5 I/O
- **Concrete methods**: ~18 (iterators, `operator[]`, `fill()`, `copy()`, type queries)
- **Concrete subclasses**: `DataStore<T>`, `EmptyDataStore<T>`

#### 42. `AbstractListStore<T>` (template)
- **File**: `src/simplnx/DataStructure/AbstractListStore.hpp`
- **Inherits from**: `IListStore`
- **Own pure virtual methods (14)**: List access, modification, data setting
- **Concrete methods**: 6 (iterators)
- **Concrete subclass**: `ListStore<T>`

#### 43. `IStringStore` *(renamed from AbstractStringStore in Phase 15)*
- **File**: `src/simplnx/DataStructure/IStringStore.hpp`
- **Inherits from**: Nothing (standalone)
- **Pure virtual methods (12)**: `deepCopy()`, `size()`, `empty()`, tuple operations, element access
- **Concrete methods**: 8 (iterators, comparison operators — trivial delegation to pure virtual `operator[]` and `size()`)
- **Concrete subclass**: `StringStore`
- **Notes**: Renamed to `IStringStore` in Phase 15 — has no data members and all concrete methods are trivial delegations, qualifying it as a pure interface by project convention.

### Montage Hierarchy

#### 44. `AbstractMontage`
- **File**: `src/simplnx/DataStructure/Montage/AbstractMontage.hpp`
- **Inherits from**: `BaseGroup`
- **Own pure virtual methods (5)**: `getTooltipGenerator()`, `getGeometry()` (x2), `getTileIndex()`, `setGeometry()`
- **Concrete subclass**: `GridMontage`

#### 45. `AbstractTileIndex`
- **File**: `src/simplnx/DataStructure/Montage/AbstractTileIndex.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (3)**: `getGeometry()`, `isValid()`, `getToolTipGenerator()`
- **Concrete subclass**: `GridTileIndex`

### Messaging Hierarchy

#### 46. `AbstractDataStructureMessage`
- **File**: `src/simplnx/DataStructure/Messaging/AbstractDataStructureMessage.hpp`
- **Pure virtual methods (1)**: `getMsgType()`
- **Concrete subclasses**: `DataAddedMessage`, `DataRemovedMessage`, `DataRenamedMessage`, `DataReparentedMessage`

#### 47. `AbstractPipelineMessage`
- **File**: `src/simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp`
- **Pure virtual methods (1)**: `toString()`
- **9 concrete subclasses**

#### 48. `PipelineNodeObserver`
- **File**: `src/simplnx/Pipeline/Messaging/PipelineNodeObserver.hpp`
- **Pure virtual methods (1, protected)**: `onNotify()`
- **Concrete subclass**: `Pipeline` (protected inheritance)

### IO Hierarchy

#### 49. `AbstractDataIOManager` *(renamed from IDataIOManager in Phase 12)*
- **File**: `src/simplnx/DataStructure/IO/Generic/AbstractDataIOManager.hpp`
- **Pure virtual methods (1)**: `formatName()`
- **Concrete methods**: ~9 (factory management)
- **Data members**: 3
- **Notes**: Renamed from `IDataIOManager` in Phase 12. Not a pure interface — has data members and concrete methods.

#### 50. `HDF5::AbstractDataIO` *(renamed from HDF5::IDataIO in Phase 15)*
- **File**: `src/simplnx/DataStructure/IO/HDF5/AbstractDataIO.hpp`
- **Inherits from**: `IDataFactory`
- **Own pure virtual methods (3)**: `readData()`, `writeDataObject()`, `getTypeName()`
- **Notes**: Renamed along with 6 other HDF5 IO base classes (`IGeometryIO` → `AbstractGeometryIO`, `IGridGeometryIO` → `AbstractGridGeometryIO`, `INodeGeom0dIO` → `AbstractNodeGeom0dIO`, `INodeGeom1dIO` → `AbstractNodeGeom1dIO`, `INodeGeom2dIO` → `AbstractNodeGeom2dIO`, `INodeGeom3dIO` → `AbstractNodeGeom3dIO`)

#### 51. `HDF5::ObjectIO`
- **File**: `src/simplnx/Utilities/Parsing/HDF5/IO/ObjectIO.hpp`
- **Pure virtual methods (2, protected)**: `open()`, `close()`
- **Concrete methods**: ~23
- **Concrete subclasses**: `GroupIO` -> `FileIO`, `DatasetIO`

### Utilities Hierarchy

#### 52. `AbstractSegmentFeatures`
- **File**: `src/simplnx/Utilities/AbstractSegmentFeatures.hpp`
- **Inherits from**: `ISegmentFeatures`
- **Renamed from**: `SegmentFeatures` (Phase 2)
- **Overrides all 5 ISegmentFeatures methods** with default implementations
- **Data members**: 6

#### 53. `AlignSections`
- **File**: `src/simplnx/Utilities/AlignSections.hpp`
- **Pure virtual methods (1, protected)**: `findShifts()`
- **Notes**: Template Method pattern. No corresponding `IAlignSections` interface.

#### 54. `SampleSurfaceMesh`
- **File**: `src/simplnx/Utilities/SampleSurfaceMesh.hpp`
- **Pure virtual methods (1, protected)**: `generatePoints()`
- **Notes**: Template Method pattern. No corresponding `ISampleSurfaceMesh` interface.

#### 55. `AbstractArrayThreshold` *(renamed from IArrayThreshold in Phase 11)*
- **File**: `src/simplnx/Utilities/ArrayThreshold.hpp`
- **Inherits from**: `IArrayThreshold` (new pure interface)
- **Concrete overrides (5)**: `isInverted()`, `setInverted()`, `getUnionOperator()`, `setUnionOperator()`, `toJson()`
- **Data members**: 2
- **Notes**: Renamed from `IArrayThreshold` in Phase 11. Pure interface `IArrayThreshold` extracted to `src/simplnx/Utilities/IArrayThreshold.hpp`.

#### 56. `AbstractJsonPipelineParser` *(renamed from IJsonPipelineParser in Phase 11)*
- **File**: `src/simplnx/Utilities/Parsing/JSON/AbstractJsonPipelineParser.hpp`
- **Inherits from**: `IJsonPipelineParser` (now pure interface)
- **Concrete methods**: 1 (`getFilterList()`)
- **Data members**: 1 (`m_FilterList`)
- **Notes**: Renamed from `IJsonPipelineParser` in Phase 11. `IJsonPipelineParser` made pure interface.

#### 57. `CSV::AbstractDataParser`
- **File**: `src/simplnx/Utilities/FileUtilities.hpp`
- **Pure virtual methods (1)**: `parse()`
- **Concrete methods**: 3
- **Concrete subclass**: `CSVDataParser<ArrayType, T>`

---

## Concrete Base Class

#### 58. `ParallelAlgorithm`
- **File**: `src/simplnx/Utilities/ParallelAlgorithm.hpp`
- **Renamed from**: `IParallelAlgorithm` (Phase 2)
- **Pure virtual methods**: None
- **Notes**: Protected constructor prevents direct instantiation. No virtual methods at all.
- **Concrete subclasses**: `ParallelDataAlgorithm`, `ParallelData2DAlgorithm`, `ParallelData3DAlgorithm`, `ParallelTaskAlgorithm`

---

## Full Inheritance Hierarchy Tree

```
[PURE INTERFACES - Extracted During Phases 2-7]

IFilter                                   [Pure Interface, Phase 3]
  +-- AbstractFilter                      [Abstract Base]
       +-- Hundreds of filter classes     [Concrete, in Plugins/]

IPlugin                                   [Pure Interface, Phase 5]
  +-- AbstractPlugin                      [Abstract Base]
       +-- Concrete plugin classes        [Concrete, in Plugins/]

IPipelineNode                             [Pure Interface, Phase 4]
  +-- AbstractPipelineNode                [Abstract Base]
       +-- Pipeline                       [Concrete]
       +-- AbstractPipelineFilter         [Abstract Base]
            +-- PipelineFilter            [Concrete]
            +-- PlaceholderFilter          [Concrete]

IDataStructure                            [Pure Interface, Phase 7]
  +-- DataStructure                       [Concrete]

ISegmentFeatures                          [Pure Interface, Phase 2]
  +-- AbstractSegmentFeatures             [Abstract Base]
       +-- Subclasses in Plugins/         [Concrete]


[PURE INTERFACES - Pre-existing]

IDataFactory                              [Pure Interface]
  +-- HDF5::AbstractDataIO                [Abstract Base, renamed Phase 15]
       +-- All HDF5 IO classes            [Concrete]

IPluginLoader                             [Pure Interface]
  +-- InMemoryPluginLoader                [Concrete]
  +-- PluginLoader                        [Concrete]

IJsonFilterParser                         [Pure Interface]
  +-- JsonFilterParserV6                  [Concrete]
  +-- JsonFilterParserV7                  [Concrete]

IMaskCompare                              [Pure Interface]
  +-- BoolMaskCompare                     [Concrete]
  +-- UInt8MaskCompare                    [Concrete]


[GEOMETRY HIERARCHY - Dual Inheritance Pattern (Phase 6)]

IGeometry (pure interface)                IGridGeometry (pure interface)
    |                                          |
    v                                          v
AbstractGeometry (ABC)  ----+---- AbstractGridGeometry (ABC)
    |                                          |
    |                                    [ImageGeom, RectGridGeom]
    |
    +-- INodeGeometry0D (pure interface)
    |        |
    |        v
    +-- AbstractNodeGeometry0D (ABC)
             |
             +-- INodeGeometry1D (pure interface)
             |        |
             |        v
             +-- AbstractNodeGeometry1D (ABC)
                      |
                      +-- INodeGeometry2D (pure interface)
                      |        |
                      |        v
                      +-- AbstractNodeGeometry2D (ABC)
                               |
                               +-- INodeGeometry3D (pure interface)
                               |        |
                               |        v
                               +-- AbstractNodeGeometry3D (ABC)
                                        |
                                  [TetrahedralGeom, HexahedralGeom]


[PURE INTERFACES -> ABSTRACT BASES -> CONCRETE (Phase 9)]

IDataStore                                [Pure Interface (Phase 9)]
  +-- AbstractDataStore<T>                [Abstract Base, template]
       +-- DataStore<T>                   [Concrete]
       +-- EmptyDataStore<T>              [Concrete]

IListStore                                [Pure Interface (Phase 9)]
  +-- AbstractListStore<T>                [Abstract Base, template]
       +-- ListStore<T>                   [Concrete]

IStringStore                              [Pure Interface, promoted Phase 15]
  +-- StringStore                         [Concrete]


[DATA OBJECT HIERARCHY]

IDataObject                               [Pure Interface, Phase 14]
  +-- AbstractDataObject                  [Abstract Base, renamed from DataObject Phase 14]
  |
  +-- BaseGroup                           [Abstract Base]
  |    +-- DataGroup                      [Concrete]
  |    +-- AttributeMatrix                [Concrete]
  |    +-- AbstractGeometry               [Abstract Base] (+ IGeometry)
  |    |    (see Geometry Hierarchy above)
  |    +-- AbstractMontage                [Abstract Base]
  |         +-- GridMontage               [Concrete]
  |
  +-- AbstractArray                       [Abstract Base, renamed from IArray Phase 11]
  |    +-- AbstractDataArray              [Abstract Base, renamed from IDataArray Phase 11]
  |    |    +-- DataArray<T>              [Concrete]
  |    +-- AbstractNeighborList           [Abstract Base, renamed from INeighborList Phase 11]
  |    |    +-- NeighborList<T>           [Concrete]
  |    +-- StringArray                    [Concrete]
  |
  +-- ScalarData<T>                       [Concrete]
  +-- DynamicListArray<T,K>               [Concrete]


[PARAMETER HIERARCHY]

IParameter                                [Pure Interface, Phase 10]
  +-- AbstractParameter                   [Abstract Base]
       +-- ValueParameter                 [Abstract Base]
       |    +-- 17 concrete parameters    [Concrete]
       |    +-- VectorParameterBase       [Abstract Base]
       |         +-- VectorParameter<T>   [Concrete]
       +-- DataParameter                  [Abstract Base]
            +-- MutableDataParameter      [Abstract Base]
            |    +-- 12 concrete params   [Concrete]
            +-- ConstDataParameter        [Abstract Base]


[DATA ACTION HIERARCHY]

IDataAction                               [Pure Interface]
  +-- AbstractDataCreationAction          [Abstract Base, renamed Phase 15]
  |    +-- 16 concrete actions            [Concrete]
  +-- 5 concrete actions                  [Concrete]


[PIPELINE MESSAGING]

AbstractPipelineMessage                   [Abstract Base]
  +-- 9 concrete message classes          [Concrete]

PipelineNodeObserver                      [Abstract Base]
  +-- Pipeline                            [Concrete, protected inheritance]

AbstractDataStructureMessage              [Abstract Base]
  +-- 4 concrete message classes          [Concrete]


[IO HIERARCHY]

AbstractDataIOManager                     [Abstract Base, renamed from IDataIOManager Phase 12]
  +-- CoreDataIOManager, etc.             [Concrete]

HDF5::ObjectIO                            [Abstract Base]
  +-- GroupIO                             [Concrete]
  |    +-- FileIO                         [Concrete]
  +-- DatasetIO                           [Concrete]


[UTILITIES]

AlignSections                             [Abstract Base, Template Method]
  +-- Subclasses in Plugins/              [Concrete]

SampleSurfaceMesh                         [Abstract Base, Template Method]
  +-- Subclasses in Plugins/              [Concrete]

IArrayThreshold                           [Pure Interface, Phase 11]
  +-- AbstractArrayThreshold              [Abstract Base]
       +-- ArrayThreshold                 [Concrete]
       +-- ArrayThresholdSet              [Concrete]

CSV::AbstractDataParser                   [Abstract Base]
  +-- CSVDataParser<T>                    [Concrete]

IJsonPipelineParser                       [Pure Interface, Phase 11]
  +-- AbstractJsonPipelineParser          [Abstract Base]
       +-- JsonPipelineParserV6           [Concrete]
       +-- JsonPipelineParserV7           [Concrete]

AbstractTileIndex                         [Abstract Base]
  +-- GridTileIndex                       [Concrete]

ParallelAlgorithm                         [Concrete Base - protected ctor]
  +-- ParallelDataAlgorithm               [Concrete]
  +-- ParallelData2DAlgorithm             [Concrete]
  +-- ParallelData3DAlgorithm             [Concrete]
  +-- ParallelTaskAlgorithm               [Concrete]


[STANDALONE - NO INTERFACE]

DataMap
DataPath
LinkedPath
Metadata
Application
Preferences
Arguments
FilterHandle
FilterList
Parameters (class)
All Common/ classes
```

---

## Key Findings

### 1. Dramatic Increase in Pure Interfaces

The refactoring increased the number of true pure interfaces from **4 to 14**:

| Phase | Interfaces Extracted |
|-------|---------------------|
| Pre-existing | `IDataFactory`, `IPluginLoader`, `IJsonFilterParser`, `IMaskCompare` |
| Phase 2 | `ISegmentFeatures` |
| Phase 3 | `IFilter` |
| Phase 4 | `IPipelineNode` |
| Phase 5 | `IPlugin` |
| Phase 6 | `IGeometry`, `IGridGeometry`, `INodeGeometry0D`, `INodeGeometry1D`, `INodeGeometry2D`, `INodeGeometry3D` |
| Phase 7 | `IDataStructure` |

### 2. Naming Convention Now Fully Consistent

The "I" prefix is now **correctly** used for pure interfaces throughout the codebase. All former misnomers have been fixed in Phases 11-15:

| Old Name | New Name | Change |
|----------|----------|--------|
| `IArrayThreshold` (ABC) | `AbstractArrayThreshold` + `IArrayThreshold` (pure interface) | Extracted pure interface, renamed ABC |
| `IJsonPipelineParser` (ABC) | `AbstractJsonPipelineParser` + `IJsonPipelineParser` (pure interface) | Extracted pure interface, renamed ABC |
| `IArray` | `AbstractArray` | Renamed (no interface extraction — tightly coupled to DataObject hierarchy) |
| `IDataArray` | `AbstractDataArray` | Renamed (no interface extraction) |
| `INeighborList` | `AbstractNeighborList` | Renamed (no interface extraction) |
| `IDataCreationAction` (ABC) | `AbstractDataCreationAction` | Renamed — has data member and concrete method (Phase 15) |
| `IDataIO` + 6 HDF5 IO classes | `AbstractDataIO` etc. | Renamed — all have data members/concrete methods (Phase 15) |
| `AbstractStringStore` | `IStringStore` | Promoted — no data members, all concrete methods are trivial delegations (Phase 15) |

### 3. Design Patterns Successfully Applied

| Pattern | Where Applied |
|---------|---------------|
| **COM-style Interface/Implementation separation** | All major hierarchies now have pure `I`-prefixed interfaces at the root |
| **Template Method** | `AbstractFilter` (`preflight`/`execute` -> `preflightImpl`/`executeImpl`), `AlignSections`, `SampleSurfaceMesh` |
| **Multiple Inheritance (Interface + ABC)** | Geometry hierarchy — each Abstract class inherits from both its parent Abstract and a standalone I-interface |
| **Protected constructor pattern** | All interfaces use protected defaulted constructors to prevent standalone instantiation |
| **`final` keyword** | `AbstractParameter` hierarchy uses `final` to lock down method implementations |

### 4. Serialization Compatibility Maintained

All renamed geometry classes preserve their original `k_TypeName` string values (e.g., `AbstractGeometry::k_TypeName = "IGeometry"`) to maintain backward compatibility with existing `.dream3d` file serialization.

### 5. Remaining Classes Without Extracted Interfaces

The following abstract base classes do **not** yet have corresponding pure interfaces extracted. These could be candidates for future refactoring phases if further COM-style consistency is desired:

| Class | Current Pure Virtuals | Complexity |
|-------|----------------------|------------|
| `AlignSections` | 1 | Low |
| `SampleSurfaceMesh` | 1 | Low |
| `AbstractDataStructureMessage` | 1 | Low |
| `AbstractPipelineMessage` | 1 | Low |
| `PipelineNodeObserver` | 1 | Low |
| `HDF5::ObjectIO` | 2 | Medium |
| `AbstractDataIOManager` | 1 | Low |
| `AbstractTileIndex` | 3 | Low |
| `AbstractMontage` | 5 | Medium |

### 6. Quality of Extracted Interfaces

All newly extracted interfaces follow consistent design principles:
- Virtual destructor defaulted
- No data members
- Protected default/copy/move constructors (allows derived class copy)
- All public methods pure virtual
- Standalone (no inheritance between interfaces — avoids diamond inheritance)
- Constants and type aliases placed on interfaces where they belong semantically
