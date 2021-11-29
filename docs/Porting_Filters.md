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

 There are some substitions for the QString operations. See [https://en.cppreference.com/w/cpp/string/basic_string](https://en.cppreference.com/w/cpp/string/basic_string) for more information about std::string

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



