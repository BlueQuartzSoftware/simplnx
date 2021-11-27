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
