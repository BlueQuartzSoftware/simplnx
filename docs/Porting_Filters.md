# Porting Filter Notes #

## Converting Types ##

+ QString => std::string
+ QVector<> => std::vector<>
+ QMap<> => std::map<>
+ QByteArray => std::array<int8> or std::vector<int8>


## Converting `setErrorCondition` ##

```c++
    setErrorCondition(complex::StlConstants::k_ErrorOpeningFile, "Error opening STL file");
```
```c++
    Result<> result =  MakeErrorResult(complex::StlConstants::k_ErrorOpeningFile, "Error opening STL file")
```
then you can optionally return the `result` variable if needed


## QString operations ##

 There are some substitutions for the QString operations. See [https://en.cppreference.com/w/cpp/string/basic_string](https://en.cppreference.com/w/cpp/string/basic_string) for more information about std::string

There is a file `complex/Utilities/StringUtilities.hpp` that has some QString functionality that is needed.

## Getting a Geometry from the DataStructure ##

If you know the path to the Geometry:

```c++
  DataPath triangleGeometryDataPath = pParentDataGroupPath.createChildPath(pGeometryName);
  TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeometryDataPath);
```

## Resizing Attribute Matrix ##

If your codes specifically resize the AttributeMatrix, this is not needed anymore.

## QString Formatting ##

Use the `format` library

```c++
QString msg = QString("Error reading Triangle '%1'. Object Count was %2 and should have been %3").arg(t, objsRead, k_StlElementCount);
```

```c++
std::string msg = fmt::format("Error reading Triangle '{}}'. Object Count was {} and should have been {}", t, objsRead, k_StlElementCount);
```

## Get An Array from the DataStructure ##

Example of getting an array and summing the values using range based for loop.

```c++
 // Let's sum up all the areas.
    Float64Array& faceAreas = dataGraph.getDataRefAs<Float64Array>(triangleAreasDataPath);
    double sumOfAreas = 0.0;
    for(const auto& area : faceAreas)
    {
      sumOfAreas += area;
    }
```

## Chaining Together DataPath + String to form new DataPath ##

```c++
    DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath("Triangle Areas");
```

## Print out the preflight errors during a Unit Test ##

```c++
    auto preflightResult = filter.preflight(dataGraph, args);
    if(preflightResult.outputActions.invalid())
    {
      for(const auto& error : preflightResult.outputActions.errors())
      {
        std::cout << error.code << ": " << error.message << std::endl;
      }
    }
```

## Moving from Pointer based array navigation ##

Previously inside of SIMPL one would have done the following to get the raw pointer
to the data stored in a DataArray:

```c++
float* vertex = triangleGeom->getVertexPointer(0);
```
and then used the `[]` notation to get and set values. With the possibility of out-of-core
being added there is no guarantee that the data would exist at a given pointer offset in memory.
Instead the developer should use:

```c++
AbstractGeometry::SharedVertexList& vertex = *(triangleGeom->getVertices());
```
Note the use of a _Reference Variable_ instead of the pointer. The developer can still use 
code such as `vertex[index]` to get/set a value but the code `vertex = i` to move a pointer
**will not work**.

## Selecting Geometry from a Parameter ##

If you need to have the user select a Geometry then you should use a `DataPathSelectionParameter`.

```c++
  params.insert(std::make_unique<DataPathSelectionParameter>(k_TriangleGeometry_Key, "Triangle Geometry to Sample", "", DataPath{}));
```

## Transferring Data from one Geometry to Another ##

There are several filters (those that create a new geometry from an existing one) where
the user is allowed to "transfer" data from the source geometry onto the newly created
geometry. QuickSurfaceMeshFilter and PointSampleTriangleGeometryFilter both are examples
of how to perform this transfer of data.


## Parallel Algorithms ##

There are several classes that can be used to help the developer write parallel algorithms.

`complex/Utilities/ParallelAlgorithm` and `complex/Utilties/ParallelTaskAlgorithm` are the two main classes depending
on the situation. `AlignSections.cpp` and `CropImageGeoemtry.cpp` both use a task based parallelism. `RotateSampleRefFrameFilter.cpp` shows an example
of using ParallelData3DAlgorithm.


## Constants for Pi and Others ##

```c++
  #include "complex/Common/Numbers.hpp"
``` 
and use it this way:

```c++
  double foo = complex::numbers::k_180OverPi * 232.0;
```

## MessageHandler ##

All filters give you access to the MessageHandler class that sends status, progress, error and warning messages back to the user.

This example uses the `fmt` library to format a message of type `Info` and send it back to the user interface.
```c++
    m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Iteration {} of {}", q, m_InputValues->pIterationSteps));
```
This example shows how to send back progress. The integer argument is a value between 0 and 100 where 0 is just starting and
100 is fully complete.
```c++
    m_MessageHandler(IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt));
```

## Creating Array within an Attribute Matrix ##

If you have a filter that needs to create an array in something like a cell attribute matrix or
a feature attribute matrix then the following filters have examples.

* TriangleNormalFilter
* CalculateFeatureSizesFilter


## Porting SIMPL Filter ##

+ Create Filter class in "PLUGIN_NAME/src/PLUGIN_NAME/Filters/xxxxFilter[.hpp|.cpp]"
+ Update Plugin's top level CMakeLists.txt to include the filter
+ Create Algorithm class in "PLUGIN_NAME/src/PLUGIN_NAME/Filters/Algorithms/xxxxFilter[.hpp|.cpp]"
+ Update Plugin's top level CMakeLists.txt to include the algorithm
+ Ensure the UUID is the proper UUID from the know mappings file.

### Parameters ###

Use proper grouping in the parameters to help the User Interface.

There are potentially 3 sections of parameters:
```
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
```
```
  params.insertSeparator(Parameters::Separator{"Required Input Data Objects"});
```
```
  params.insertSeparator(Parameters::Separator{"Created Output Data Objects"});
```

these should be used as needed by the filter.
