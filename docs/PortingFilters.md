# Porting a Filter

Depending on where the ported filter is coming from determines what you need to
do. The sections are as follows:

- SECTION 1 : Porting From SIMPL to Filters Folder
- SECTION 2 : Porting stubs from existing folder to Filters Folder
- SECTION 3 : Updating the Backwards Compatibility Testing
- SECTION 4 : Developing a Test File
- SECTION 5 : Multithreading
- SECTION 6 : Progress Updating
- SECTION 7 : Utilizing API's to the fullest
- SECTION 8 : Useful Tips and Tricks

## SECTION 1 : Porting From ***SIMPL*** to ***Filters Folder***

This will be the most common type of Filter porting. The steps for this are as
follows:

### Go to FreeNas and pull the custom build of DREAM3D

This custom build has **ALL** ***SIMPL*** plugins compiled so you don't need
to worry about what filters will be available

### Load up ***SIMPL*** DREAM3D and navigate to ***SimplnxFilterGen***

Here you will need to set the command arguments using the following syntax:

```console
-c NameOfFilterToPort -o file/path/to/the/plugin/
```

Some nuances to note for this are as follows:

- The path to the plugin should be .../PluginName/ NOT .../PluginName/src/PluginName/
- The slash at the end of the filepath is necessary to work properly ie [.../PluginName/ NOT .../PluginName]
- The name of the filter should be the SIMPL name not what you want the simplnx name to be

You will need to update the various CMake files inside the target simplnx plugin in order to start compiling the new filter code inside of a simplnx build.

## SECTION 2 : Porting stubs from existing folder to ***Filters Folder***

Some plugins have existing stubs in folders other than the primary ***Filters***
folder.

### Move the Filter and Algorithm files to the active ***Filters Folder***

### Update the Legacy UUID Maps

1. Open the LegacyUUIDMapping header file for this Plugin
2. Find and uncomment the include statement for the filter being moved
3. Find and uncomment the map entry for the filter being moved
  
 When working with the ***LegacyUUIDMapping*** header file in this **Plugin**
 be sure to make sure the commented out tokens are not removed. Their syntax is
 one of the following:

 ```console
 @@HEADER__TOKEN__DO__NOT__DELETE@@
 ```

 or

```console
@@MAP__UPDATE__TOKEN__DO__NOT__DELETE@@
```

### Update the CMakeLists.txt files to reflect the changes

 This includes the ones for the unit tests and the one at the plugin level

## SECTION 3 : Updating the Backwards Compatibility Testing

The backwards compatibility will begin testing immediately as long as the filter is registered in the `PluginNameUUIDLegacyMapping.hpp` file. The only exception is new filters that have no **SIMPL** equivalent.

To run the test in advance select the `conversion_test` ctest target and run it. Most of the errors are from issues in the filter's `FromSIMPLJson()` function. The error messages should guide you towards a solution. The following cases are exceptions that can be hard to diagnose without a priori knowledge.

How To Update Test For Filter Changes/Ports

### Case 1: Add any keys NEW to NX (ie new parameters)

To do this you must update the `k_KeyIgnoreMap`

It has the following structure, be sure to add a comma to the line proceeding

```cpp
// FilterNameHere
std::pair<Uuid, std::vector<std::string>>{Uuid::FromString("nx_filter_uuid_here").value(), std::vector<std::string>{"parameter_key_1", "parameter_key_2"}},
 ```

### Case 2: Porting Filter from SIMPL

Refer to Case 1 for any new parameter key, and ensure the UUIDMapping in the respective plugin is up to date

### Case 3: Porting a New Parameter from SIMPL

Review the `InitializeMap()` function, at the bottom of this method are known parameters that have yet to be ported.

In order to register a new parameter input it with the following structure

```cpp
// NXParameterName <- SIMPLFilterParameterName
CreateMapInput(Uuid::FromString("nx_parameter_uuid_here").value(), parameter_ValueType_object);
```

If you are receiving bad any cast after this a potential fix is explicit casting `parameter_ValueType_object` to ValueType.

If the ValueType for the parameter does not implement a `==` operator, an overload of `CreateMapInput()` must be defined the structure follows:

```cpp
void CreateMapInput(Uuid&& uuid, NXParameter::ValueType&& value)
{
  if(k_ParamMap.contains(uuid))
  {
    k_ParamMap[uuid].first.emplace_back(std::make_any<NXParameter::ValueType>(value));
  }
  else
  {
   k_ParamMap.emplace(uuid, std::make_pair(std::vector<std::any>{std::make_any<NXParameter::ValueType>(value)}, [](const std::any& imported, const std::any& exemplar) -> bool {
                        auto importedRef = GetAnyRef<NXParameter::ValueType>(imported);
                        auto exemplarRef = GetAnyRef<NXParameter::ValueType>(exemplar);
                        return importedRef.member_1 == exemplarRef.member_1 && importedRef.member_2 == exemplarRef.member_2;
                      }));
  }
}
```

### Case 4: Debugging Tips

If you want to see the read in value and the expected value at the same time add the following if:

```cpp
if(parameterName == "parameter_key_here")
{
  std::cout << "hit";
}
 ```

directly after the following line `const std::any& importedValue = argumentsResult.value().at(parameterName);`

The `importedValue` is the imported object and `parameterCheck.first` contains the vector of acceptable values

## SECTION 4 : Developing a Test File

Firstly, it is important to ensure that each unit test does not just instantiate filter. Current standards require the following:

- 1 Test Case to instantiate the filter
- At least 1 Test Case to verify valid filter execution
- At least 1 Test Case to verify invalid filter execution [preflight testing]

Test Files should **NOT** output strings to the terminal. Output should be in the form of catch2 errors.

### Adding a new data file to ***DREAM3D Data Repo***

For adding the data file to the DREAM3D repo one should follow the following steps:

#### Step 1: Create the exemplar DREAM3D file in *SIMPL*

Files from current SIMPL DREAM3D should have the prefix "6_6_" which denotes the version of DREAM3D it was produced from. Files named this way are version 7.

#### Step 2: Compress the file to a tar.gz file and compute the sha 512 hash of the file

These will be used to verify changes in the file and look for updates.

#### Step 3: Go to the simplnx GitHub repo and update with the tar.gz file

GitHub Repo : <https://github.com/bluequartzsoftware/simplnx/releases/tag/Data_Archive>
**Be Sure to Save the Release**

#### Step 4: Go to the simplnx CMakeLists.txt file and update sha 512

Located at line 579 in the CMakeLists.text file in the ***simplnx*** repo, one must update the table accordingly.

### Working with filters outside the current plugin

There will be times you may have to call upon filters from another repo. Typically this occurs in ***simplnx_plugins***. In order to do this one must create an application instance which is done so by wrapping it in a struct that gets nested in a shared pointer to make sure it cleans itself up after each test case. Here is the syntax for doing so:

```cpp
std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
app->loadPlugins(unit_test::k_BuildDir.view(), true);
auto* filterList = Application::Instance()->getFilterList();
```

To use make_shared_enabler you must include:

```cpp
#include "simplnx/UnitTest/UnitTestCommon.hpp"
```

The syntax for use of ***filterList*** is as follows:

```cpp
auto filter = filterList->createFilter(k_EnterFilterHandle);
REQUIRE(nullptr != filter);
```

## SECTION 5 : Multithreading

At the current time, the only filters that should be made parallel are those that could be considered "embarrassingly parallel". It is important to remember that the cost of creating a thread is hefty so it should only be done when there is a sizeable amount of work available for each thread. Simplnx has two types: ParallelTaskAlgorithm and ParallelDataAlgorithm. Task Runner is for parsing multiple objects and Data Runner is for parsing a single object.

### Syntax for Simplnx

This is an exemplar use case and doesn't truly encompass all possible use cases for the functions, but instead serves to show how it should be structured in most cases.  

In an anonymous namespace:
```cpp
class FilterNameImpl  
{  
public:  
  FilterNameImpl(DataObject& object, Type argument)  
  : m_Object(object)  
  , m_Argument(argument)  
  {  
  }  
  ~FilterNameImpl() noexcept = default;  

  void convert(size_t start, size_t end) const  
  {  
    for(size_t i = start; i < end; i++)  
    {  
      // Do something  
    }  
  }  

  void operator()(const Range& range) const  
  {  
    convert(range.min(), range.max());
  }  

private:  
  DataObject& m_Object;  
  Type m_Argument;  
};
```

In the executing function:

```cpp
ParallelDataAlgorithm dataAlg;  
dataAlg.setRange(0ULL, object.getSize());  
dataAlg.execute(::FilterNameImpl(object, argument));
```

## SECTION 6 : Progress Updating

With out of core functionality on the way, it is now a requirement for each and every filter to have progress updates and checks for cancel. This section shows threadsafe progress updating and message structuring.

### Serial Messaging API

Modern API offers a `MessageHelper` wrapper class that offers an object that has assorted helper functions, such as a throttled messenger.

Include it with the following:

```cpp
#include "simplnx/Utilities/MessageHelper.hpp"
```

You create it in the algorithms execution body like this

```cpp
MessageHelper messageHelper(m_MessageHandler);
```

To send a message immediately to the console, use the following syntax:

```cpp
messageHelper.sendMessage("Header Here:");
```

In practice this should be used to print a section header for updates or a one-time message. **Avoid using this as a progress messenger in a loop since it can incur a significant compute-cost penalty in the form of excessive output OR brach check cost**

For progress messaging a throttled messenger class is provided. To initialize it do the following:

```cpp
ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();
```

The throttled messenger will print every 1000 milliseconds (1 second) by default, to change this initialize it as follows:

```cpp
ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger(static_cast<std::chrono::milliseconds>(number_in_milliseconds));
```

Then provide the statement to be printed wrapped in a lambda as follows:

```cpp
throttledMessenger.sendThrottledMessage([&]() { return fmt::format(" - Your Message || {:.2f}% Complete", CalculatePercentComplete(current_position, max_position));
```

Here is a complete MRE:

```cpp
#include "simplnx/Utilities/MessageHelper.hpp"
...
/// -----------------------------------------------------------------------------
Result<> SomeAlgorithmClass::operator()()
{
  // Create wrapper messing class
  MessageHelper messageHelper(m_MessageHandler);

  // Send Header
  messageHelper.sendMessage("Parsing Input Data:");


  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger(static_cast<std::chrono::milliseconds>(2000));
  for(usize i = 0; i < 10000; i++)
  {
    // Do stuff
    throttledMessenger.sendThrottledMessage([&]() { return fmt::format(" - Searching || {:.2f}% Complete", CalculatePercentComplete(i, 1000));
  }
}
```

This would produce something similar to the following output

```console
Parsing Input Data:
 - Searching || 1.2% Complete
 - Searching || 15.7% Complete
 - Searching || 34.1% Complete
```

### ThreadSafe Progress Messaging

!!! THIS SECTION IS OUT OF DATE

This is an example that aims to reduce the number of times a mutex lock is called.

> void updateThreadSafeProgress(size_t counter)  
> {  
> std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);  
>
> m_ProgressCounter += counter;  
>
> auto now = std::chrono::steady_clock::now();  
> if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > 1000) // every second update  
> {  
> size_t progressInt = static_cast<size_t>((static_cast<double>(m_ProgressCounter) / m_TotalElements) * 100.0);  
> std::string progressMessage = "Calculating... ";  
> m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});  
> m_InitialTime = std::chrono::steady_clock::now();  
> }  
> }  

This function should avoid being called too many times in a thread as it would significantly slow it down.

### Message Structuring

For error messaging the following syntax should be used:

```cpp
MakeErrorResult(-65450, fmt::format("{}({}): Function {}: Error. Message. '{}'", "FunctionName", **FILE**, **LINE**, errorVariable));
```

The number at the start is an arbitrary value save for the fact it must be negative.

## SECTION 7 : Utilizing API's to the fullest

This section aims to tackle ***simplnx*** convenience functions from major API's:

### Utilizing the ExecuteDataFunction

These templated varg functions aim to eliminate the need for type switches, this is done using functors. Below is example syntax:

In an Anonymous namespace:

```cpp
struct FilterNameFunctor  
{  
  template \<class T>  
  void operator()(IDataArray& inputDataRef, bool argument)  
  {
    auto& inputDataRef = dynamic_cast<DataArray<T>&>(inputDataPtr);  

    // DO Something  
  }  
};
```

In the executing function:

```cpp
ExecuteDataFunction(FilterNameFunctor{}, selectedArrayRef.getDataType(), selectedArrayRef, argumentBool);
```

## SECTION 8 : Useful Tips and Tricks

### Porting Checklist

- [ ] Parameters should be generally broken down into "Input Parameters", "Required Data Objects", "Created Data Objects". There can be exceptions to this.
- [ ] ChoicesParameter selections should be an enumeration defined in the filer header
- [ ] Documentation copied from SIMPL Repo and updated (if necessary)
- [ ] Parameter argument variables are `k_CamelCase_Key`
- [ ] Parameter argument strings are `lower_snake_case`

```cpp
static inline constexpr StringLiteral k_AlignmentType_Key = "alignment_type";
```

### Misc. Code Style requirements

- [ ] Filters should have both the Filter class and Algorithm class for anything beyond trivial needs

### Converting Types

- `QString => std::string`
- `QVector<> => std::vector<>`
- `QMap<> => std::map<>`
- `QByteArray => std::array<int8> or std::vector<int8>`

### Converting `setErrorCondition` from SIMPL to SIMPLNX

SIMPL

```cpp
setErrorCondition(nx::core::StlConstants::k_ErrorOpeningFile, "Error opening STL file");
```

SIMPLNX

```cpp
Result<> result =  MakeErrorResult(nx::core::StlConstants::k_ErrorOpeningFile, "Error opening STL file")
```

then you can optionally return the `result` variable if needed

### QString operations

There are some substitutions for the QString operations.
See [https://en.cppreference.com/w/cpp/string/basic_string](https://en.cppreference.com/w/cpp/string/basic_string) for
more information about std::string

There is a file `simplnx/Utilities/StringUtilities.hpp` that has some QString functionality that is needed.

### Getting a Geometry from the DataStructure

If you know the path to the Geometry:

```cpp
DataPath triangleGeometryDataPath = pParentDataGroupPath.createChildPath(pGeometryName);
TriangleGeom& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(triangleGeometryDataPath);
```

### Resizing Attribute Matrix

If your codes specifically resize the AttributeMatrix, this is not needed anymore.

### QString Formatting

Use the `format` library

```cpp
QString msg = QString("Error reading Triangle '%1'. Object Count was %2 and should have been %3").arg(t, objsRead, k_StlElementCount);
```

```cpp
std::string msg = fmt::format("Error reading Triangle '{}}'. Object Count was {} and should have been {}", t, objsRead, k_StlElementCount);
```

### Get An Array from the DataStructure

Example of getting an array and summing the values using range based for loop.

```cpp
// Let's sum up all the areas.
Float64Array& faceAreasArray = dataGraph.getDataRefAs<Float64Array>(triangleAreasDataPath);
AbstractFloat64DataStore& faceAreas = faceAreasArray.getDataStoreRef();
double sumOfAreas = 0.0;
for(const auto& area : faceAreas)
{
  sumOfAreas += area;
}
```

### DataArray Performance

- When iterating over values, either to read or write, use the reference returned by `DataArray<T>::getDataStoreRef()`.
- When writing values in a multi-threaded function, use the getValue and setValue methods in AbstractDataStore to ensure that values being both read and written at the same time. The [] operators are not capable of protecting against data corruption.
  - In situation where values are only being read from the array, the [] operators are both safe and faster to use.

#### Chaining Together DataPath + String to form new DataPath

```cpp
DataPath triangleAreasDataPath = geometryPath.createChildPath(triangleFaceDataGroupName).createChildPath("Triangle Areas");
```

#### Print out the preflight errors during a Unit Test

```cpp
auto preflightResult = filter.preflight(dataGraph, args);
if(preflightResult.outputActions.invalid())
{
  for(const auto& error : preflightResult.outputActions.errors())
  {
    std::cout << error.code << ": " << error.message << std::endl;
  }
}
```

### Moving from Pointer based array navigation

Previously inside of SIMPL one would have done the following to get the raw pointer
to the data stored in a DataArray:

```cpp
float* vertex = triangleGeom->getVertexPointer(0);
```

and then used the `[]` notation to get and set values. With the possibility of out-of-core
being added there is no guarantee that the data would exist at a given pointer offset in memory.
Instead the developer should use:

```cpp
AbstractGeometry::SharedVertexList& vertex = *(triangleGeom->getVertices());
```

Note the use of a *Reference Variable* instead of the pointer. The developer can still use
code such as `vertex[index]` to get/set a value but the code `vertex = i` to move a pointer
**will not work**.

### Selecting Geometry from a Parameter

If you need to have the user select a Geometry then you should use a `GeometrySelectionParameter`.

```cpp
params.insert(std::make_unique<GeometrySelectionParameter>(k_GridGeomPath_Key, "Input Image Geometry", "DataPath to input Image Geometry", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
```

### Transferring Data from one Geometry to Another

There are several filters (those that create a new geometry from an existing one) where
the user is allowed to "transfer" data from the source geometry onto the newly created
geometry. QuickSurfaceMeshFilter and PointSampleTriangleGeometryFilter both are examples
of how to perform this transfer of data.

### Parallel Algorithms

There are several classes that can be used to help the developer write parallel algorithms.

`simplnx/Utilities/ParallelAlgorithm` and `simplnx/Utilities/ParallelTaskAlgorithm` are the two main classes depending
on the situation. `AlignSections.cpp` and `CropImageGeometryFilter.cpp` both use a task based
parallelism. `RotateSampleRefFrameFilter.cpp` shows an example
of using ParallelData3DAlgorithm.

### Constants for Pi and Others

```cpp
#include "simplnx/Common/Numbers.hpp"
```

and use it this way:

```cpp
double foo = nx::core::numbers::k_180OverPi * 232.0;
```

### MessageHandler

All filters give you access to the MessageHandler class that sends status, progress, error and warning messages back to
the user.

This example uses the `fmt` library to format a message of type `Info` and send it back to the user interface.

```cpp
m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Iteration {} of {}", q, m_InputValues->pIterationSteps));
```

This example shows how to send back progress. The integer argument is a value between 0 and 100 where 0 is just starting
and
100 is fully complete.

```cpp
m_MessageHandler(IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt));
```

### Creating Array within an Attribute Matrix

If you have a filter that needs to create an array in something like a cell attribute matrix or
a feature attribute matrix then the following filters have examples.

- TriangleNormalFilter
- ComputeFeatureSizesFilter

### Replace EXECUTE_FUNCTION_TEMPLATE

You have code that does this:

```cpp
EXECUTE_FUNCTION_TEMPLATE(this, Detail::ExecuteTemplate, m_InArrayPtr.lock(), this, m_InArrayPtr.lock());
```

and now you are porting that to `simplnx`. The old `Detail::ExecuteTemplate` needs to be converted into a "struct" based
functor like the following:

```cpp
struct ExecuteTemplate
{
  template <typename T>
  void operator()([ARGUMENTS GO HERE], const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& messageHandler)
  {
  .... your code goes here
  }
};
```

then you replace the macro with the following template function:

```cpp
ExecuteDataFunction(ExecuteTemplate{}, srcIDataArray.getDataType(), [ARGUMENTS GO HERE], m_ShouldCancel, m_MessageHandler);
```

The first 2 arguments to the above function are used by the function, any additional arguments are passed directly to
your functor implementation.

### Porting SIMPL Filter

- Create Filter class in "PLUGIN_NAME/src/PLUGIN_NAME/Filters/xxxxFilter[.hpp|.cpp]"
- Update Plugin's top level CMakeLists.txt to include the filter
- Create Algorithm class in "PLUGIN_NAME/src/PLUGIN_NAME/Filters/Algorithms/xxxxFilter[.hpp|.cpp]"
- Update Plugin's top level CMakeLists.txt to include the algorithm
- Ensure the UUID is the proper UUID from the know mappings file.

#### Parameters

Use proper grouping in the parameters to help the User Interface.

There are potentially 3 sections of parameters:

```cpp
params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
```

```cpp
params.insertSeparator(Parameters::Separator{"Input Data Objects"});
```

```cpp
params.insertSeparator(Parameters::Separator{"Output Output Data Objects"});
```

these should be used as needed by the filter.

### Processing a Geometry In Place

Sometimes a filter needs allow the user to process it's geometry "in place" in order to ease the number of filters that are needed to remove temporary DataObjects. If your filter needs this kind of capability, then take a look at the "CropImageGeometryFilter" or "RotateSampleRefFrame" filters.
