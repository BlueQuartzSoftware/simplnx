# Porting a Filter

Depending on where the ported filter is coming from determines what you need to
do. The sections are as follows:
<ul>
    <li>SECTION 1 : Porting From SIMPL to Filters Folder</li>
    <li>SECTION 2 : Porting stubs from existing folder to Filters Folder</li>
    <li>SECTION 3 : Developing a Test File</li>
    <li>SECTION 4 : Multithreading</li>
    <li>SECTION 5 : Progress Updating</li>
    <li>SECTION 6 : Utilizing API's to the fullest</li>
    <li>SECTION 7 : Useful Tips and Tricks</li>
</ul>

## SECTION 1 : Porting From ***SIMPL*** to ***Filters Folder***

This will be the most common type of Filter porting. The steps for this are as
follows:

### Go to FreeNas and pull the custom build of DREAM3D

This custom build has **ALL** ***SIMPL*** plugins compiled so you don't need
to worry about what filters will be available

### Load up ***SIMPL*** DREAM3D and navigate to ***SimplnxFilterGen***

Here you will need to set the command arguments using the following syntax:

> -c NameOfFilterToPort -o file/path/to/the/plugin/

Some nuances to note for this are as follows:

<ul>
   <li>The path to the plugin should be .../PluginName/ NOT .../PluginName/src/PluginName/</li>
   <li>The slash at the end of the filepath is necessary to work properly ie [.../PluginName/ NOT .../PluginName]</li>
   <li>The name of the filter should be the SIMPL name not what you want the simplnx name to be</li>
</ul>

You will need to update the various CMake files inside the target simplnx plugin in order to start compiling the new filter code inside of a simplnx build.


## SECTION 2 : Porting stubs from existing folder to ***Filters Folder***

Some plugins have existing stubs in folders other than the primary ***Filters***
folder.

### Move the Filter and Algorithm files to the active ***Filters Folder***

### Update the Legacy UUID Maps
<ol>
  <li> Open the LegacyUUIDMapping header file for this Plugin </li>
  <li> Find and uncomment the include statement for the filter being moved </li>
  <li> Find and uncomment the map entry for the filter being moved </li>
</ol>
  
 When working with the ***LegacyUUIDMapping*** header file in this **Plugin** 
 be sure to make sure the commented out tokens are not removed. Their syntax is 
 one of the following:
 > ***@@__HEADER__TOKEN__DO__NOT__DELETE__@@***
 
 or
 
 > ***@@__MAP__UPDATE__TOKEN__DO__NOT__DELETE__@@***
 
 ### Update the CMakeLists.txt files to reflect the changes
 
 This includes the ones for the unit tests and the one at the plugin level


## SECTION 3 : Developing a Test File

Firstly, it is important to ensure that each unit test does not just instantiate filter. Current standards require the following:
<ul>
    <li> 1 Test Case to instantiate the filter </li>
    <li> At least 1 Test Case to verify valid filter execution </li>
    <li> At least 1 Test Case to verify invalid filter execution [preflight testing] </li>
</ul>

Test Files should **NOT** output strings to the terminal. Output should be in the form of catch2 errors.

### Adding a new data file to ***DREAM3D Data Repo***

For adding the data file to the DREAM3D repo one should follow the following steps:

> Create the exemplar DREAM3D file in ***SIMPL***

Files from current SIMPL DREAM3D should have the prefix "6_6_" which denotes the version of DREAM3D it was produced from. Files named this way are version 7.

> Compress the file to a tar.gz file and compute the sha 512 hash of the file

These will be used to verify changes in the file and look for updates.

> Go to the simplnx GitHub repo and update with the tar.gz file

GitHub Repo : <https://github.com/bluequartzsoftware/simplnx/releases/tag/Data_Archive>   
**Be Sure to Save the Release**

> Go to the simplnx CMakeLists.txt file and update sha 512

Located at line 579 in the CMakeLists.text file in the ***simplnx*** repo, one must update the table accordingly.

### Working with filters outside the current plugin 

There will be times you may have to call upon filters from another repo. Typically this occurs in ***simplnx_plugins***. In order to do this one must create an application instance which is done so by wrapping it in a struct that gets nested in a shared pointer to make sure it cleans itself up after each test case. Here is the syntax for doing so:

> std::shared_ptr<UnitTest::make_shared_enabler> app = std::make_shared<UnitTest::make_shared_enabler>();
> app->loadPlugins(unit_test::k_BuildDir.view(), true);
> auto* filterList = Application::Instance()->getFilterList();

To use make_shared_enabler you must include:

> #include "simplnx/UnitTest/UnitTestCommon.hpp"

The syntax for use of ***filterList*** is as follows:

> auto filter = filterList->createFilter(k_EnterFilterHandle);
> REQUIRE(nullptr != filter);


## SECTION 4 : Multithreading

At the current time, the only filters that should be made parallel are those that could be considered "embarrassingly parallel". It is important to remember that the cost of creating a thread is hefty so it should only be done when there is a sizeable amount of work available for each thread. Simplnx has two types: ParallelTaskAlgorithm and ParallelDataAlgorithm. Task Runner is for parsing multiple objects and Data Runner is for parsing a single object. 

### Syntax for Simplnx

This is an examplar use case and doesn't truly encompass all possible use cases for the functions, but instead serves to show how it should be structured in most cases.  

In an anonymous namespace:

> class FilterNameImpl  
> {  
> public:  
>   FilterNameImpl(DataObject& object, Type argument)  
>   : m_Object(object)  
>   , m_Argument(argument)  
>   {  
>   }  
>   ~FilterNameImpl() noexcept = default;  
>
>   void convert(size_t start, size_t end) const  
>   {  
>     for(size_t i = start; i < end; i++)  
>     {  
>       // Do something  
>     }  
>   }  
>
>   void operator()(const Range& range) const  
>   {  
>     convert(range.min(), range.max());   
>   }  
>
> private:  
> DataObject& m_Object;  
> Type m_Argument;  
> };  

In the exectuting function:

> ParallelDataAlgorithm dataAlg;  
> dataAlg.setRange(0ULL, object.getSize());  
> dataAlg.execute(::FilterNameImpl(object, argument));  


## SECTION 5 : Progress Updating

With out of core functionality on the way, it is now a requirement for each and every filter to have progress updates and checks for cancel. This section shows threadsafe progress updating and message structuring.

### ThreadSafe Progress Messaging

This is an example that aims to reduce the number of times a mutex lock is called. 

> void updateThreadSafeProgress(size_t counter)  
> {  
>   std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);  
>
>   m_ProgressCounter += counter;  
>
>   auto now = std::chrono::steady_clock::now();  
>   if(std::chrono::duration_cast<std::chrono::milliseconds>(now - m_InitialTime).count() > 1000) // every second update  
>   {  
>     size_t progressInt = static_cast<size_t>((static_cast<double>(m_ProgressCounter) / m_TotalElements) * 100.0);  
>     std::string progressMessage = "Calculating... ";  
>     m_MessageHandler(IFilter::ProgressMessage{IFilter::Message::Type::Progress, progressMessage, static_cast<int32_t>(progressInt)});  
>     m_InitialTime = std::chrono::steady_clock::now();  
>   }  
> }  

This function should avoid being called too many times in a thread as it would significantly slow it down.

### Message Structuring

For error messaging the following syntax should be used:

> MakeErrorResult(-65450, fmt::format("{}({}): Function {}: Error. Message. '{}'", "FunctionName", __FILE__, __LINE__, errorVariable));

The number at the start is an arbitrary value save for the fact it must be negative.


## SECTION 6 : Utilizing API's to the fullest

This section aims to tackle ***simplnx*** convenience functions from major API's:

### Utilizing the ExecuteDataFunction

These templated varg functions aim to eliminate the need for type switches, this is done using functors. Below is example syntax:

In an Anonymous namespace:

> struct FilterNameFunctor  
> {  
>   template \<class T>  
>   void operator()(IDataArray& inputDataRef, bool argument)  
>   {
>     auto& inputDataRef = dynamic_cast<DataArray<T>&>(inputDataPtr);  
>
>     // DO Something  
>   }  
> };  

In the executing function:

> ExecuteDataFunction(FilterNameFunctor{}, selectedArrayRef.getDataType(), selectedArrayRef, argumentBool);  


## SECTION 7 : Useful Tips and Tricks

This section is just for basic genralized tips to help make our code better:
<ul>
    <li>When you just need a std::string consider using std::string_view for better preformance and easy substring handling</li>
    <li>If you need to use a string of some sort repeatedly it should be stored in anonymous namespace as a constant at the top of the file</li>
    <li>Be sure to seperate input parameters according to input, required objects, and created objects</li>
    <li>Ensure naming scheme matches the rules laid out in the pull request template</li>
    <li>Do **NOT** use C-Style arrays, use std::array<T,N> instead. </li>
</ul>
