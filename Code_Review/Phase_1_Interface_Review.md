# Phase 1: Interface and Abstract Base Class Review

## Overview

This document catalogs every class in the `src/simplnx/` codebase that participates in an interface or abstract base class hierarchy. Classes are categorized as:

- **Pure Interface**: All methods are pure virtual + virtual destructor, no data members, no concrete method bodies
- **Near-Pure Interface**: Almost all methods pure virtual, with only trivial delegation helpers
- **Abstract Base Class**: Mix of pure virtual methods and concrete implementations
- **Concrete Base Class**: No pure virtual methods but designed for inheritance

---

## Summary Statistics

| Category | Count | Classes |
|----------|-------|---------|
| **Pure Interface** | 4 | `IDataFactory`, `IPluginLoader`, `IJsonFilterParser`, `MaskCompare` |
| **Near-Pure Interface** | 2 | `IDataStore`, `IListStore` |
| **Abstract Base Class** | 28 | See below |
| **Concrete Base Class** (misleading "I" prefix) | 1 | `IParallelAlgorithm` |

### Standalone Concrete Classes with No Interface

| Class | Directory | Notes |
|-------|-----------|-------|
| `DataStructure` | DataStructure/ | **Primary Phase 2 target** - no interface at all |
| `DataMap` | DataStructure/ | Container utility |
| `DataPath` | DataStructure/ | Path representation |
| `LinkedPath` | DataStructure/ | Linked path representation |
| `Metadata` | DataStructure/ | Metadata storage |
| `Application` | Core/ | Singleton, no virtual methods |
| `Preferences` | Core/ | No virtual methods |
| `Arguments` | Filter/ | No inheritance |
| `FilterHandle` | Filter/ | No inheritance |
| `FilterList` | Filter/ | No inheritance |
| `Parameters` | Filter/ | No inheritance |
| All Common/ classes | Common/ | Range, Array, BoundingBox, etc. - all standalone |

---

## Pure Interface Classes

### 1. `IDataFactory`
- **File**: `src/simplnx/DataStructure/IO/Generic/IDataFactory.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods**: `getDataType() const = 0`
- **Notes**: Single-method interface. Copy/move deleted.

### 2. `IPluginLoader`
- **File**: `src/simplnx/Plugin/PluginLoader.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods**:
  - `isLoaded() const = 0`
  - `getPlugin() = 0`
  - `getPlugin() const = 0`
- **Concrete subclasses**: `InMemoryPluginLoader`, `PluginLoader`

### 3. `IJsonFilterParser`
- **File**: `src/simplnx/Utilities/Parsing/JSON/IJsonFilterParser.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods**:
  - `fromJson(const std::string&) const = 0`
  - `toJson(AbstractFilter*) const = 0`
- **Concrete subclasses**: `JsonFilterParserV6`, `JsonFilterParserV7`

### 4. `IMaskCompare` (struct)
- **File**: `src/simplnx/Utilities/MaskCompareUtilities.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods**:
  - `bothTrue(usize, usize) const = 0`
  - `bothFalse(usize, usize) const = 0`
  - `isTrue(usize) const = 0`
  - `setValue(usize, bool) = 0`
  - `getNumberOfTuples() const = 0`
  - `getNumberOfComponents() const = 0`
  - `countTrueValues() const = 0`
- **Concrete subclasses**: `BoolMaskCompare`, `UInt8MaskCompare`

---

## Near-Pure Interface Classes

### 5. `IDataStore`
- **File**: `src/simplnx/DataStructure/IDataStore.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (13)**:
  - `getNumberOfTuples()`, `getTupleShape()`, `getNumberOfComponents()`, `getComponentShape()`, `getChunkShape()`, `resizeTuples()`, `getDataType()`, `getStoreType()`, `getTypeSize()`, `deepCopy()`, `createNewInstance()`, `writeBinaryFile()` (x2 overloads)
- **Concrete helpers**: `getSize()`, `size()`, `empty()` (trivial delegations), `getDataFormat()` (virtual with default)
- **Concrete subclass chain**: `AbstractDataStore<T>` -> `DataStore<T>`, `EmptyDataStore<T>`

### 6. `IListStore`
- **File**: `src/simplnx/DataStructure/IListStore.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (9)**:
  - `getNumberOfTuples()`, `getTupleShape()`, `resizeTuples()`, `clearAllLists()`, `getListSize()`, `getNumberOfLists()`, `clear()`, `readHdf5()`, `writeHdf5()`
- **Concrete helpers**: `size()` (trivial delegation)
- **Concrete subclass chain**: `AbstractListStore<T>` -> `ListStore<T>`

---

## Abstract Base Classes

### DataObject Hierarchy

#### 7. `DataObject`
- **File**: `src/simplnx/DataStructure/DataObject.hpp`
- **Inherits from**: Nothing (root of DataObject hierarchy)
- **Pure virtual methods (3)**: `deepCopy()`, `shallowCopy()`, `getTypeName()`
- **Concrete methods**: `getId()`, `getName()`, `canRename()`, `rename()`, `getDataStructure()`, `getDataPaths()`, many more
- **Notes**: Root of the entire data object hierarchy. Extensive concrete infrastructure.

#### 8. `IArray`
- **File**: `src/simplnx/DataStructure/IArray.hpp`
- **Inherits from**: `DataObject`
- **Own pure virtual methods (12)**: `getArrayType()`, `getSize()`, `size()`, `empty()`, `getTupleShape()`, `getComponentShape()`, `getNumberOfTuples()`, `getNumberOfComponents()`, `swapTuples()`, `resizeTuples()`, `toString()`, `setValueFromString()`

#### 9. `IDataArray`
- **File**: `src/simplnx/DataStructure/IDataArray.hpp`
- **Inherits from**: `IArray`
- **Own pure virtual methods (4)**: `copyTuple()`, `getIDataStore()` (const/non-const), `getDataFormat()`
- **Concrete methods**: Various delegation methods to underlying store
- **Concrete subclass**: `DataArray<T>`

#### 10. `INeighborList`
- **File**: `src/simplnx/DataStructure/INeighborList.hpp`
- **Inherits from**: `IArray`
- **Own pure virtual methods (4)**: `getIListStore()` (const/non-const), `copyTuple()`, `getDataType()`
- **Concrete subclass**: `NeighborList<T>`

#### 11. `BaseGroup`
- **File**: `src/simplnx/DataStructure/BaseGroup.hpp`
- **Inherits from**: `DataObject`
- **Still-abstract**: Inherits `deepCopy()`, `shallowCopy()`, `getTypeName()` as pure virtual
- **Concrete methods**: Container management (`getSize()`, `contains()`, `insert()`, `remove()`, iterators)
- **Concrete subclasses**: `DataGroup`, `AttributeMatrix`

### Geometry Hierarchy

#### 12. `IGeometry`
- **File**: `src/simplnx/DataStructure/Geometry/IGeometry.hpp`
- **Inherits from**: `BaseGroup`
- **Own pure virtual methods (6)**: `getGeomType()`, `getNumberOfCells()`, `findElementSizes()`, `getParametricCenter()`, `getShapeFunctions()`, `validate()`

#### 13. `IGridGeometry`
- **File**: `src/simplnx/DataStructure/Geometry/IGridGeometry.hpp`
- **Inherits from**: `IGeometry`
- **Own pure virtual methods (19)**: `getDimensions()`, `setDimensions()`, `getNumXCells()`, `getNumYCells()`, `getNumZCells()`, coordinate methods (14 overloads), `getIndex()` (x2)
- **Concrete subclasses**: `ImageGeom`, `RectGridGeom`

#### 14. `INodeGeometry0D`
- **File**: `src/simplnx/DataStructure/Geometry/INodeGeometry0D.hpp`
- **Inherits from**: `IGeometry`
- **Concrete methods**: Vertex management, bounding box, coordinate access
- **Concrete subclass**: `VertexGeom`

#### 15. `INodeGeometry1D`
- **File**: `src/simplnx/DataStructure/Geometry/INodeGeometry1D.hpp`
- **Inherits from**: `INodeGeometry0D`
- **Own pure virtual methods (4)**: `getNumberOfVerticesPerEdge()`, `findElementsContainingVert()`, `findElementNeighbors()`, `findElementCentroids()`
- **Concrete subclass**: `EdgeGeom`

#### 16. `INodeGeometry2D`
- **File**: `src/simplnx/DataStructure/Geometry/INodeGeometry2D.hpp`
- **Inherits from**: `INodeGeometry1D`
- **Own pure virtual methods (3)**: `getNumberOfVerticesPerFace()`, `findEdges()`, `findUnsharedEdges()`
- **Concrete subclasses**: `TriangleGeom`, `QuadGeom`

#### 17. `INodeGeometry3D`
- **File**: `src/simplnx/DataStructure/Geometry/INodeGeometry3D.hpp`
- **Inherits from**: `INodeGeometry2D`
- **Own pure virtual methods (3)**: `findFaces()`, `findUnsharedFaces()`, `getNumberOfVerticesPerCell()`
- **Concrete subclasses**: `TetrahedralGeom`, `HexahedralGeom`

### Data Store Hierarchy

#### 18. `AbstractDataStore<T>` (template)
- **File**: `src/simplnx/DataStructure/AbstractDataStore.hpp`
- **Inherits from**: `IDataStore`
- **Own pure virtual methods (21+)**: Element access, arithmetic ops, chunk ops, HDF5 I/O
- **Concrete methods**: Iterators, `operator[]`, `fill()`, `copy()`, type queries
- **Concrete subclasses**: `DataStore<T>`, `EmptyDataStore<T>`

#### 19. `AbstractListStore<T>` (template)
- **File**: `src/simplnx/DataStructure/AbstractListStore.hpp`
- **Inherits from**: `IListStore`
- **Own pure virtual methods (14)**: List access, modification, data setting
- **Concrete methods**: Iterators
- **Concrete subclass**: `ListStore<T>`

#### 20. `AbstractStringStore`
- **File**: `src/simplnx/DataStructure/AbstractStringStore.hpp`
- **Inherits from**: Nothing (standalone)
- **Pure virtual methods (12)**: `deepCopy()`, `size()`, `empty()`, tuple operations, element access
- **Concrete methods**: Iterators, comparison operators
- **Concrete subclass**: `StringStore`

### Filter/Parameter Hierarchy

#### 21. `IFilter`
- **File**: `src/simplnx/Filter/IFilter.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (9)**: `name()`, `className()`, `uuid()`, `humanName()`, `parameters()`, `parametersVersion()`, `clone()`, `preflightImpl()`, `executeImpl()`
- **Concrete methods**: `preflight()`, `execute()`, `fromJson()`, `toJson()`, `getDefaultArguments()`
- **Notes**: Hundreds of concrete filter subclasses across Plugins/

#### 22. `IParameter`
- **File**: `src/simplnx/Filter/IParameter.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (11)**: `uuid()`, `name()`, `humanName()`, `helpText()`, `defaultValue()`, `type()`, `acceptedTypes()`, `getVersion()`, `clone()`, `toJsonImpl()`, `fromJsonImpl()`
- **Concrete methods**: `toJson()`, `fromJson()`, `construct()`

#### 23. `AbstractParameter`
- **File**: `src/simplnx/Filter/AbstractParameter.hpp`
- **Inherits from**: `IParameter`
- **Implements** (final): `name()`, `humanName()`, `helpText()`
- **Still has 8 unresolved pure virtuals**

#### 24. `ValueParameter`
- **File**: `src/simplnx/Filter/ValueParameter.hpp`
- **Inherits from**: `AbstractParameter`
- **Implements** (final): `type()`
- **Adds pure virtual**: `validate()`
- **18 concrete subclasses** (BoolParameter, StringParameter, NumberParameter, etc.)

#### 25. `DataParameter`
- **File**: `src/simplnx/Filter/DataParameter.hpp`
- **Inherits from**: `AbstractParameter`
- **Implements** (final): `type()`
- **Adds pure virtuals**: `mutability()`, `validate(DataStructure&, ...)`

#### 26. `MutableDataParameter` / `ConstDataParameter`
- **Files**: `src/simplnx/Filter/MutableDataParameter.hpp`, `ConstDataParameter.hpp`
- **Inherit from**: `DataParameter`
- **Implement** (final): `mutability()`
- **Add pure virtual**: `resolve()`
- **12 concrete subclasses** of MutableDataParameter (ArraySelectionParameter, GeometrySelectionParameter, etc.)

### Pipeline Hierarchy

#### 27. `AbstractPipelineNode`
- **File**: `src/simplnx/Pipeline/AbstractPipelineNode.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (7)**: `getType()`, `getName()`, `preflight()` (x2), `execute()`, `deepCopy()`, `toJsonImpl()`
- **Concrete methods**: Extensive state management (signals, fault state, DataStructure tracking)
- **Concrete subclass**: `Pipeline`

#### 28. `AbstractPipelineFilter`
- **File**: `src/simplnx/Pipeline/AbstractPipelineFilter.hpp`
- **Inherits from**: `AbstractPipelineNode`
- **Implements**: `getType()`
- **Adds pure virtual**: `getFilterType()`
- **Concrete subclasses**: `PipelineFilter`, `PlaceholderFilter`

### Action Hierarchy

#### 29. `IDataAction`
- **File**: `src/simplnx/Filter/Output.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (2)**: `apply()`, `clone()`
- **Notes**: Very close to a pure interface but uses protected constructor pattern

#### 30. `IDataCreationAction`
- **File**: `src/simplnx/Filter/Output.hpp`
- **Inherits from**: `IDataAction`
- **Adds pure virtual**: `getAllCreatedPaths()`
- **Adds concrete**: `getCreatedPath()` accessor
- **16 concrete subclasses** (CreateArrayAction, CreateImageGeometryAction, etc.)
- **5 concrete subclasses** of IDataAction directly (DeleteDataAction, EmptyAction, etc.)

### Plugin Hierarchy

#### 31. `AbstractPlugin`
- **File**: `src/simplnx/Plugin/AbstractPlugin.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (1)**: `getSimplToSimplnxMap()`
- **Concrete methods**: Nearly everything else (`getName()`, `createFilter()`, `getFilterHandles()`, etc.)
- **Notes**: Barely abstract - only 1 pure virtual. Prime candidate for extracting an `IPlugin` interface.

### Montage Hierarchy

#### 32. `AbstractMontage`
- **File**: `src/simplnx/DataStructure/Montage/AbstractMontage.hpp`
- **Inherits from**: `BaseGroup`
- **Own pure virtual methods (5)**: `getTooltipGenerator()`, `getGeometry()` (x2), `getTileIndex()`, `setGeometry()`
- **Concrete subclass**: `GridMontage`

#### 33. `AbstractTileIndex`
- **File**: `src/simplnx/DataStructure/Montage/AbstractTileIndex.hpp`
- **Inherits from**: Nothing
- **Pure virtual methods (3)**: `getGeometry()`, `isValid()`, `getToolTipGenerator()`
- **Concrete subclass**: `GridTileIndex`

### Messaging Hierarchy

#### 34. `AbstractDataStructureMessage`
- **File**: `src/simplnx/DataStructure/Messaging/AbstractDataStructureMessage.hpp`
- **Pure virtual methods (1)**: `getMsgType()`
- **Concrete subclasses**: `DataAddedMessage`, `DataRemovedMessage`, `DataRenamedMessage`, `DataReparentedMessage`

#### 35. `AbstractPipelineMessage`
- **File**: `src/simplnx/Pipeline/Messaging/AbstractPipelineMessage.hpp`
- **Pure virtual methods (1)**: `toString()`
- **9 concrete subclasses** (PipelineNodeMessage, NodeAddedMessage, etc.)

#### 36. `PipelineNodeObserver`
- **File**: `src/simplnx/Pipeline/Messaging/PipelineNodeObserver.hpp`
- **Pure virtual methods (1, protected)**: `onNotify()`
- **Concrete subclass**: `Pipeline` (protected inheritance)

### IO Hierarchy

#### 37. `IDataIOManager`
- **File**: `src/simplnx/DataStructure/IO/Generic/IDataIOManager.hpp`
- **Pure virtual methods (1)**: `formatName()`
- **Concrete methods**: Factory management, data store creation functions

#### 38. `HDF5::IDataIO`
- **File**: `src/simplnx/DataStructure/IO/HDF5/IDataIO.hpp`
- **Inherits from**: `IDataFactory`
- **Own pure virtual methods (3)**: `readData()`, `writeDataObject()`, `getTypeName()`
- **All concrete HDF5 IO classes** inherit from this

### Utilities Hierarchy

#### 39. `IArrayThreshold`
- **File**: `src/simplnx/Utilities/ArrayThreshold.hpp`
- **Pure virtual methods (1)**: `getRequiredPaths()`
- **Concrete subclasses**: `ArrayThreshold`, `ArrayThresholdSet`

#### 40. `AlignSections`
- **File**: `src/simplnx/Utilities/AlignSections.hpp`
- **Pure virtual methods (1, protected)**: `findShifts()`
- **Notes**: Template Method pattern. Subclassed in Plugins.

#### 41. `SampleSurfaceMesh`
- **File**: `src/simplnx/Utilities/SampleSurfaceMesh.hpp`
- **Pure virtual methods (1, protected)**: `generatePoints()`
- **Notes**: Template Method pattern. Subclassed in Plugins.

#### 42. `SegmentFeatures`
- **File**: `src/simplnx/Utilities/SegmentFeatures.hpp`
- **Pure virtual methods**: None (virtual methods have default implementations)
- **Notes**: Abstract by convention, not by language enforcement. Subclassed in Plugins.

#### 43. `HDF5::ObjectIO`
- **File**: `src/simplnx/Utilities/Parsing/HDF5/IO/ObjectIO.hpp`
- **Pure virtual methods (2, protected)**: `open()`, `close()`
- **Concrete subclasses**: `GroupIO` -> `FileIO`, `DatasetIO`

#### 44. `CSV::AbstractDataParser`
- **File**: `src/simplnx/Utilities/FileUtilities.hpp`
- **Pure virtual methods (1)**: `parse()`
- **Concrete subclass**: `CSVDataParser<ArrayType, T>`

#### 45. `IJsonPipelineParser`
- **File**: `src/simplnx/Utilities/Parsing/JSON/IJsonPipelineParser.hpp`
- **Pure virtual methods (2)**: `fromJson()`, `toJson()`
- **Notes**: **BUG** - destructor is NOT virtual despite being used polymorphically
- **Concrete subclasses**: `JsonPipelineParserV6`, `JsonPipelineParserV7`

### Misleading "I" Prefix

#### `IParallelAlgorithm`
- **File**: `src/simplnx/Utilities/IParallelAlgorithm.hpp`
- **Pure virtual methods**: **None**
- **Virtual methods**: **None**
- **Notes**: Despite the "I" prefix, this is a concrete base class with no polymorphism. Protected constructor/destructor prevents direct instantiation. Should be renamed to `ParallelAlgorithmBase` or converted to a true interface.
- **Concrete subclasses**: `ParallelDataAlgorithm`, `ParallelData2DAlgorithm`, `ParallelData3DAlgorithm`, `ParallelTaskAlgorithm`

---

## Full Inheritance Hierarchy Tree

```
[PURE INTERFACES]

IDataFactory                              [Pure Interface]
  +-- HDF5::IDataIO                       [Abstract Base]
       +-- All HDF5 IO classes            [Concrete]

IPluginLoader                             [Pure Interface]
  +-- InMemoryPluginLoader                [Concrete]
  +-- PluginLoader                        [Concrete]

IJsonFilterParser                         [Pure Interface]
  +-- JsonFilterParserV6                  [Concrete]
  +-- JsonFilterParserV7                  [Concrete]

MaskCompare                               [Pure Interface]
  +-- BoolMaskCompare                     [Concrete]
  +-- UInt8MaskCompare                    [Concrete]


[NEAR-PURE INTERFACES -> ABSTRACT BASES -> CONCRETE]

IDataStore                                [Near-Pure Interface]
  +-- AbstractDataStore<T>                [Abstract Base, template]
       +-- DataStore<T>                   [Concrete]
       +-- EmptyDataStore<T>              [Concrete]

IListStore                                [Near-Pure Interface]
  +-- AbstractListStore<T>                [Abstract Base, template]
       +-- ListStore<T>                   [Concrete]

AbstractStringStore                       [Abstract Base]
  +-- StringStore                         [Concrete]


[DATA OBJECT HIERARCHY]

DataObject                                [Abstract Base - root]
  |
  +-- BaseGroup                           [Abstract Base]
  |    +-- DataGroup                      [Concrete]
  |    +-- AttributeMatrix                [Concrete]
  |    +-- IGeometry                      [Abstract Base]
  |    |    +-- IGridGeometry             [Abstract Base]
  |    |    |    +-- ImageGeom            [Concrete]
  |    |    |    +-- RectGridGeom         [Concrete]
  |    |    +-- INodeGeometry0D           [Abstract Base]
  |    |         +-- VertexGeom           [Concrete]
  |    |         +-- INodeGeometry1D      [Abstract Base]
  |    |              +-- EdgeGeom        [Concrete]
  |    |              +-- INodeGeometry2D [Abstract Base]
  |    |                   +-- TriangleGeom    [Concrete]
  |    |                   +-- QuadGeom        [Concrete]
  |    |                   +-- INodeGeometry3D [Abstract Base]
  |    |                        +-- TetrahedralGeom [Concrete]
  |    |                        +-- HexahedralGeom  [Concrete]
  |    +-- AbstractMontage                [Abstract Base]
  |         +-- GridMontage               [Concrete]
  |
  +-- IArray                              [Abstract Base]
  |    +-- IDataArray                     [Abstract Base]
  |    |    +-- DataArray<T>              [Concrete]
  |    +-- INeighborList                  [Abstract Base]
  |    |    +-- NeighborList<T>           [Concrete]
  |    +-- StringArray                    [Concrete]
  |
  +-- ScalarData<T>                       [Concrete]
  +-- DynamicListArray<T,K>               [Concrete]


[FILTER/PARAMETER HIERARCHY]

IFilter                                   [Abstract Base]
  +-- Hundreds of filter classes          [Concrete, in Plugins/]

IParameter                                [Abstract Base]
  +-- AbstractParameter                   [Abstract Base]
       +-- ValueParameter                 [Abstract Base]
       |    +-- 17 concrete parameters    [Concrete]
       |    +-- VectorParameterBase       [Abstract Base]
       |         +-- VectorParameter<T>   [Concrete]
       +-- DataParameter                  [Abstract Base]
            +-- MutableDataParameter      [Abstract Base]
            |    +-- 12 concrete params   [Concrete]
            +-- ConstDataParameter        [Abstract Base]

IDataAction                               [Abstract Base / Near-Interface]
  +-- IDataCreationAction                 [Abstract Base]
  |    +-- 16 concrete actions            [Concrete]
  +-- 5 concrete actions                  [Concrete]


[PIPELINE HIERARCHY]

AbstractPipelineNode                      [Abstract Base]
  +-- Pipeline                            [Concrete] (also inherits PipelineNodeObserver)
  +-- AbstractPipelineFilter              [Abstract Base]
       +-- PipelineFilter                 [Concrete]
       +-- PlaceholderFilter              [Concrete]

PipelineNodeObserver                      [Abstract Base]
  +-- Pipeline                            [Concrete, protected inheritance]

AbstractPipelineMessage                   [Abstract Base]
  +-- 9 concrete message classes          [Concrete]


[PLUGIN HIERARCHY]

AbstractPlugin                            [Abstract Base]
  +-- Concrete plugin classes             [Concrete, in Plugins/]


[IO HIERARCHY]

IDataIOManager                            [Abstract Base]
  +-- CoreDataIOManager, etc.             [Concrete]

HDF5::ObjectIO                            [Abstract Base]
  +-- GroupIO                             [Concrete]
  |    +-- FileIO                         [Concrete]
  +-- DatasetIO                           [Concrete]


[UTILITIES HIERARCHY]

IArrayThreshold                           [Abstract Base]
  +-- ArrayThreshold                      [Concrete]
  +-- ArrayThresholdSet                   [Concrete]

AlignSections                             [Abstract Base, Template Method]
  +-- Subclasses in Plugins/              [Concrete]

SampleSurfaceMesh                         [Abstract Base, Template Method]
  +-- Subclasses in Plugins/              [Concrete]

SegmentFeatures                           [Abstract Base by convention]
  +-- Subclasses in Plugins/              [Concrete]

CSV::AbstractDataParser                   [Abstract Base]
  +-- CSVDataParser<T>                    [Concrete]

IJsonPipelineParser                       [Abstract Base]
  +-- JsonPipelineParserV6                [Concrete]
  +-- JsonPipelineParserV7                [Concrete]

IParallelAlgorithm                        [Concrete Base - misleading name]
  +-- ParallelDataAlgorithm               [Concrete]
  +-- ParallelData2DAlgorithm             [Concrete]
  +-- ParallelData3DAlgorithm             [Concrete]
  +-- ParallelTaskAlgorithm               [Concrete]


[STANDALONE - NO INTERFACE]

DataStructure                             ** Phase 2 target **
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

## Key Findings for COM-Style Refactoring

### 1. Very Few True Pure Interfaces Exist
Only 4 classes qualify as true pure interfaces: `IDataFactory`, `IPluginLoader`, `IJsonFilterParser`, and `IMaskCompare`. The vast majority of "I"-prefixed classes are actually abstract base classes with significant concrete implementations.

### 2. Naming Inconsistency
The "I" prefix is used inconsistently:
- **True interfaces**: `IDataFactory`, `IPluginLoader`, `IJsonFilterParser`
- **Abstract base classes using "I" prefix**: `IFilter`, `IParameter`, `IDataAction`, `IDataArray`, `INeighborList`, `IArray`, `IGeometry`, `IGridGeometry`, `INodeGeometry0D/1D/2D/3D`, `IDataStore`, `IListStore`, `IDataIOManager`, `IArrayThreshold`, `IParallelAlgorithm`
- **Abstract base classes using "Abstract" prefix**: `AbstractParameter`, `AbstractDataStore`, `AbstractListStore`, `AbstractStringStore`, `AbstractPlugin`, `AbstractPipelineNode`, `AbstractPipelineFilter`, `AbstractMontage`, `AbstractTileIndex`, `AbstractPipelineMessage`, `AbstractDataStructureMessage`, `AbstractDataParser`

For a COM-style refactoring, the "I" prefix should be reserved exclusively for pure interfaces.

### 3. `DataStructure` Has No Interface
The `DataStructure` class is a standalone concrete class with no interface or abstract base class. This is the primary target for Phase 2 (creating `IDataStructure`).

### 4. Classes Most in Need of Interface Extraction

| Class | Why | Priority |
|-------|-----|----------|
| `DataStructure` | No interface at all, core class | **High** |
| `AbstractPlugin` | Only 1 pure virtual, mostly concrete. Extract `IPlugin` | **High** |
| `IFilter` | Has concrete methods mixed in. Extract pure `IFilter` interface | **Medium** |
| `AbstractPipelineNode` | Heavy concrete base. Extract `IPipelineNode` | **Medium** |
| `IGeometry` and subclasses | "I" prefix but not pure interfaces | **Low** (complex hierarchy) |

### 5. Design Issues Found

| Issue | Location | Description |
|-------|----------|-------------|
| Abstract by convention only | `SegmentFeatures` | Virtual methods have defaults instead of being pure virtual |

### 6. Well-Designed Patterns Already in Use
- **Template Method Pattern**: `AlignSections`, `SampleSurfaceMesh` - public `execute()` calls protected pure virtual hooks
- **Layered Abstract Hierarchy**: `IParameter` -> `AbstractParameter` -> `ValueParameter`/`DataParameter` -> concrete
- **Store Abstraction**: `IDataStore` -> `AbstractDataStore<T>` -> `DataStore<T>` - clean separation of interface, shared implementation, and concrete storage
