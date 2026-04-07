# ReadRawBinaryFilter Improvements Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Improve ReadRawBinaryFilter to support multi-dimensional components, AttributeMatrix placement, fix critical binary read bugs, and enable parameter version migration in the framework.

**Architecture:** The filter's parameters are updated to match CreateDataArrayAdvancedFilter's patterns (DynamicTableParameter for component dims, BoolParameter for optional tuple dims). The preflight is reworked to support AM-aware tuple dimension resolution and stronger file-size validation. Three bugs in the shared ImportFromBinaryFile utility are fixed. IFilter::fromJson is made virtual to enable parameter version migration.

**Tech Stack:** C++20, CMake, Catch2, fmt

**Spec:** `docs/superpowers/specs/2026-04-06-read-raw-binary-improvements-design.md`

---

### Task 1: Fix ImportFromBinaryFile bugs in DataArrayUtilities.hpp

These are independent of the filter parameter changes and fix critical shared utility bugs.

**Files:**
- Modify: `src/simplnx/Utilities/DataArrayUtilities.hpp:221-263`

- [ ] **Step 1: Fix infinite loop when fread returns 0**

In `ImportFromBinaryFile`, after the `std::fread` call at line 243, add a check for zero elements read. This prevents an infinite loop when EOF is hit prematurely or a read error occurs.

```cpp
// In ImportFromBinaryFile, replace the while loop body (lines 241-257):
  usize elementCounter = 0;
  while(elementCounter < numElements)
  {
    usize elementsRead = std::fread(buffer.data(), sizeof(T), chunkSize, inputFilePtr);

    if(elementsRead == 0)
    {
      std::fclose(inputFilePtr);
      return MakeErrorResult(-1001, fmt::format("Unexpected end of file or read error after reading {} of {} elements from '{}'", elementCounter, numElements, binaryFilePath.string()));
    }

    for(usize i = 0; i < elementsRead; i++)
    {
      outputDataArray[i + elementCounter] = buffer[i];
    }

    elementCounter += elementsRead;

    usize elementsLeft = numElements - elementCounter;

    if(elementsLeft < chunkSize)
    {
      chunkSize = elementsLeft;
    }
  }
```

- [ ] **Step 2: Fix int32 cast truncation for skip bytes > 2GB and add fseek error checking**

Replace lines 230-233:

```cpp
  // Skip some bytes if needed
  if(startByte > 0)
  {
    int result = FSEEK64(inputFilePtr, static_cast<int64>(startByte), SEEK_SET);
    if(result != 0)
    {
      std::fclose(inputFilePtr);
      return MakeErrorResult(-1002, fmt::format("Failed to seek to byte offset {} in file '{}'", startByte, binaryFilePath.string()));
    }
  }
```

- [ ] **Step 3: Build to verify**

Run:
```bash
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && cmake --build . --target simplnx -- -j $(sysctl -n hw.ncpu)
```
Expected: Clean build, no errors.

- [ ] **Step 4: Run existing ReadRawBinary tests to verify no regressions**

Run:
```bash
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && ctest -R "SimplnxCore::ReadRawBinary" --verbose
```
Expected: All 5 existing test cases pass.

- [ ] **Step 5: Commit**

```bash
git add src/simplnx/Utilities/DataArrayUtilities.hpp
git commit -m "BUG: Fix infinite loop, int32 truncation, and missing fseek error check in ImportFromBinaryFile"
```

---

### Task 2: Make IFilter::fromJson virtual

This is a one-word framework change that enables parameter version migration for all filters.

**Files:**
- Modify: `src/simplnx/Filter/IFilter.hpp:248`

- [ ] **Step 1: Add virtual keyword to fromJson declaration**

At line 248 of `IFilter.hpp`, change:
```cpp
  Result<Arguments> fromJson(const nlohmann::json& json) const;
```
to:
```cpp
  virtual Result<Arguments> fromJson(const nlohmann::json& json) const;
```

- [ ] **Step 2: Build to verify**

Run:
```bash
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && cmake --build . --target simplnx -- -j $(sysctl -n hw.ncpu)
```
Expected: Clean build. No existing code changes behavior since no filter overrides this yet.

- [ ] **Step 3: Commit**

```bash
git add src/simplnx/Filter/IFilter.hpp
git commit -m "ENH: Make IFilter::fromJson virtual to enable parameter version migration"
```

---

### Task 3: Update ReadRawBinaryFilter parameters and header

Update the parameter keys, add new parameters, and declare the fromJson override.

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.hpp`
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.cpp` (parameters() and parametersVersion() only)

- [ ] **Step 1: Update parameter keys in ReadRawBinaryFilter.hpp**

Replace the Parameter Keys section (lines 32-38) with:
```cpp
  // Parameter Keys
  static constexpr StringLiteral k_InputFile_Key = "input_file";
  static constexpr StringLiteral k_ScalarType_Key = "scalar_type_index";
  static constexpr StringLiteral k_AdvancedOptions_Key = "set_tuple_dimensions";
  static constexpr StringLiteral k_TupleDims_Key = "tuple_dimensions";
  static constexpr StringLiteral k_CompDims_Key = "component_dimensions";
  static constexpr StringLiteral k_Endian_Key = "endian_index";
  static constexpr StringLiteral k_SkipHeaderBytes_Key = "skip_header_bytes";
  static constexpr StringLiteral k_CreatedAttributeArrayPath_Key = "created_attribute_array_path";
```

Note: `k_NumberOfComponents_Key` is removed and replaced with `k_CompDims_Key`. `k_AdvancedOptions_Key` is new.

- [ ] **Step 2: Add fromJson override declaration**

After the `FromSIMPLJson` declaration, add:
```cpp
  /**
   * @brief Converts JSON to arguments, handling parameter version migration.
   * @param json
   * @return Result<Arguments>
   */
  Result<Arguments> fromJson(const nlohmann::json& json) const override;
```

- [ ] **Step 3: Update parameters() in ReadRawBinaryFilter.cpp**

Replace the `parameters()` method body (lines 62-83) with:
```cpp
Parameters ReadRawBinaryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFile_Key, "Input File", "The input binary file path", fs::path(), FileSystemPathParameter::ExtensionsType{},
                                                          FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<NumericTypeParameter>(k_ScalarType_Key, "Input Numeric Type", "Data type of the binary data", NumericType::int8));
  params.insert(std::make_unique<ChoicesParameter>(k_Endian_Key, "Endian", "The endianness of the data", 0, ChoicesParameter::Choices{"Little", "Big"}));
  params.insert(std::make_unique<UInt64Parameter>(k_SkipHeaderBytes_Key, "Skip Header Bytes", "Number of bytes to skip before reading data", 0));

  params.insertSeparator(Parameters::Separator{"Tuple Dimensions"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(
      k_AdvancedOptions_Key, "Set Tuple Dimensions [not required if creating inside an existing Attribute Matrix]",
      "This allows the user to set the tuple dimensions directly rather than just inheriting them. This option is NOT required if you are creating the Data Array in an Attribute Matrix", true));

  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "TUPLE DIM {}"));
    const DynamicTableInfo::TableDataType defaultTable{{1.0F}};
    params.insert(std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "Data Array Tuple Dimensions (Slowest to Fastest Dimensions)",
                                                          "Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.", defaultTable, tableInfo));
  }

  params.insertSeparator(Parameters::Separator{"Component Dimensions"});
  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "COMP DIM {}"));
    const DynamicTableInfo::TableDataType defaultTable{{1.0F}};
    params.insert(std::make_unique<DynamicTableParameter>(k_CompDims_Key, "Data Array Component Dimensions (Slowest to Fastest Dimensions)", "Slowest to Fastest Component Dimensions.", defaultTable,
                                                          tableInfo));
  }

  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_CreatedAttributeArrayPath_Key, "Output Attribute Array", "The complete path to the created Attribute Array",
                                                         DataPath(std::vector<std::string>{"Imported Array"})));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_AdvancedOptions_Key, k_TupleDims_Key, true);

  return params;
}
```

- [ ] **Step 4: Update parametersVersion()**

Replace the `parametersVersion()` method:
```cpp
IFilter::VersionType ReadRawBinaryFilter::parametersVersion() const
{
  return 2;

  // Version 1 -> 2
  // Change 1: k_NumberOfComponents_Key ("number_of_components") UInt64Parameter
  //            replaced with k_CompDims_Key ("component_dimensions") DynamicTableParameter
  // Change 2: Added k_AdvancedOptions_Key ("set_tuple_dimensions") BoolParameter
}
```

- [ ] **Step 5: Add required includes**

Add to the includes in `ReadRawBinaryFilter.cpp`:
```cpp
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
```

- [ ] **Step 6: Build to verify compilation**

Run:
```bash
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && cmake --build . --target SimplnxCore -- -j $(sysctl -n hw.ncpu)
```
Expected: Build will have errors because `preflightImpl` and `executeImpl` still reference the old `k_NumberOfComponents_Key`. That is expected — those are updated in the next tasks.

- [ ] **Step 7: Commit (even with build errors in preflight/execute — those are next)**

```bash
git add src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.hpp src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.cpp
git commit -m "ENH: Update ReadRawBinaryFilter parameters for v2 (component dims table, AM-aware tuple dims)"
```

---

### Task 4: Rewrite preflightImpl

Implement the new preflight validation flow with AM-aware tuple dimension resolution and stronger file-size validation.

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.cpp` (preflightImpl only)

- [ ] **Step 1: Replace preflightImpl**

Replace the entire `preflightImpl` method (lines 98-174) with:

```cpp
IFilter::PreflightResult ReadRawBinaryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                            const ExecutionContext& executionContext) const
{
  auto pInputFileValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFile_Key);
  auto pScalarTypeValue = filterArgs.value<NumericType>(k_ScalarType_Key);
  auto pSkipHeaderBytesValue = filterArgs.value<uint64>(k_SkipHeaderBytes_Key);
  auto pCreatedAttributeArrayPathValue = filterArgs.value<DataPath>(k_CreatedAttributeArrayPath_Key);
  auto useDims = filterArgs.value<bool>(k_AdvancedOptions_Key);
  auto pCompDimsData = filterArgs.value<DynamicTableParameter::ValueType>(k_CompDims_Key);
  auto pTupleDimsData = filterArgs.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);

  // Step 2: Validate component dimensions
  const std::vector<double>& compDimsRow = pCompDimsData.at(0);
  ShapeType compDims;
  compDims.reserve(compDimsRow.size());
  for(size_t idx = 0; idx < compDimsRow.size(); idx++)
  {
    if(compDimsRow[idx] == 0)
    {
      return {MakeErrorResult<OutputActions>(-78701, fmt::format("Component dimension at index {} cannot be 0", idx))};
    }
    compDims.push_back(static_cast<usize>(compDimsRow[idx]));
  }
  usize numComponents = std::accumulate(compDims.begin(), compDims.end(), static_cast<usize>(1), std::multiplies<>());

  Result<OutputActions> resultOutputActions;

  // Step 3: Resolve tuple dimensions (AttributeMatrix-aware)
  ShapeType tupleDims;
  auto* parentAM = dataStructure.getDataAs<AttributeMatrix>(pCreatedAttributeArrayPathValue.getParent());

  if(parentAM != nullptr)
  {
    // Parent is an AttributeMatrix — inherit its shape
    tupleDims = parentAM->getShape();
    if(useDims)
    {
      resultOutputActions.warnings().push_back(
          {-78702, "You checked Set Tuple Dimensions, but selected a DataPath that has an Attribute Matrix as the parent. "
                   "The Attribute Matrix tuples will override your custom dimensions. It is recommended to uncheck Set Tuple Dimensions for the sake of clarity."});
    }
  }
  else
  {
    if(!useDims)
    {
      return {MakeErrorResult<OutputActions>(-78703, fmt::format("The DataArray to be created '{}' is not within an AttributeMatrix, so the dimensions cannot be determined implicitly. "
                                                                 "Check Set Tuple Dimensions to set the dimensions.",
                                                                 pCreatedAttributeArrayPathValue.toString()))};
    }
    const std::vector<double>& tupleDimsRow = pTupleDimsData.at(0);
    tupleDims.reserve(tupleDimsRow.size());
    for(size_t idx = 0; idx < tupleDimsRow.size(); idx++)
    {
      if(tupleDimsRow[idx] == 0)
      {
        return {MakeErrorResult<OutputActions>(-78704, fmt::format("Tuple dimension at index {} cannot be 0", idx))};
      }
      tupleDims.push_back(static_cast<usize>(tupleDimsRow[idx]));
    }
  }

  usize numTuples = std::accumulate(tupleDims.begin(), tupleDims.end(), static_cast<usize>(1), std::multiplies<>());

  // Step 4: Validate file size

  usize inputFileSize = fs::file_size(pInputFileValue);
  if(inputFileSize == 0)
  {
    return {MakeErrorResult<OutputActions>(-78705, fmt::format("File '{}' is empty.", pInputFileValue.string()))};
  }

  if(pSkipHeaderBytesValue >= inputFileSize)
  {
    return {MakeErrorResult<OutputActions>(-78706, fmt::format("Skip Header Bytes ({}) is greater than or equal to the file size ({}) for file '{}'.", pSkipHeaderBytesValue, inputFileSize,
                                                               pInputFileValue.string()))};
  }

  usize totalBytesToRead = inputFileSize - pSkipHeaderBytesValue;
  usize typeSize = GetNumericTypeSize(pScalarTypeValue);

  if(totalBytesToRead % typeSize != 0)
  {
    return {MakeErrorResult<OutputActions>(
        -78707, fmt::format("After skipping {} bytes, the data in file '{}' does not convert into an exact number of elements using the chosen scalar type '{}'. "
                            "Are you sure this is the correct scalar type?",
                            pSkipHeaderBytesValue, pInputFileValue.string(), DataTypeToString(ConvertNumericTypeToDataType(pScalarTypeValue))))};
  }

  // Step 5: Validate data fits
  usize totalElementsInFile = totalBytesToRead / typeSize;
  usize requiredElements = numTuples * numComponents;

  if(requiredElements > totalElementsInFile)
  {
    return {MakeErrorResult<OutputActions>(
        -78708, fmt::format("The file does not contain enough data for the requested array dimensions. "
                            "Required elements: {} (tuples: {} x components: {}). Available elements in file: {}.",
                            requiredElements, numTuples, numComponents, totalElementsInFile))};
  }

  if(requiredElements < totalElementsInFile)
  {
    resultOutputActions.warnings().push_back(
        {-78709, fmt::format("Only a subset of the file data will be read. Required elements: {} (tuples: {} x components: {}). Available elements in file: {}.", requiredElements, numTuples,
                             numComponents, totalElementsInFile)});
  }

  // Step 6: Create output array action
  {
    auto action = std::make_unique<CreateArrayAction>(ConvertNumericTypeToDataType(pScalarTypeValue), tupleDims, compDims, pCreatedAttributeArrayPathValue);
    resultOutputActions.value().appendAction(std::move(action));
  }

  return {std::move(resultOutputActions)};
}
```

- [ ] **Step 2: Build to verify**

Run:
```bash
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && cmake --build . --target SimplnxCore -- -j $(sysctl -n hw.ncpu)
```
Expected: Build may still fail in `executeImpl` due to old `k_NumberOfComponents_Key` reference. That's fixed in the next task.

- [ ] **Step 3: Commit**

```bash
git add src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.cpp
git commit -m "ENH: Rewrite ReadRawBinaryFilter preflight with AM support and stronger file validation"
```

---

### Task 5: Update execute path and ReadRawBinaryInputValues

Update the algorithm struct and execute logic to use the new component dimensions.

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReadRawBinary.hpp`
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReadRawBinary.cpp`
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.cpp` (executeImpl only)

- [ ] **Step 1: Update ReadRawBinaryInputValues struct in ReadRawBinary.hpp**

Replace `uint64 numberOfComponentsValue;` (line 16) with:
```cpp
  ShapeType componentDimsValue;
```

Add include at top of file:
```cpp
#include "simplnx/Common/Types.hpp"
```

- [ ] **Step 2: Update ReadRawBinary::execute() in ReadRawBinary.cpp**

Replace the component count validation (lines 125-129) with:
```cpp
  usize numComponents = std::accumulate(m_InputValues.componentDimsValue.begin(), m_InputValues.componentDimsValue.end(), static_cast<usize>(1), std::multiplies<>());
  if(binaryIDataArray->getNumberOfComponents() != numComponents)
  {
    return MakeErrorResult(-1071, fmt::format("DataArray at path '{}' has {} components but expected {}.", m_InputValues.createdAttributeArrayPathValue.toString(),
                                              binaryIDataArray->getNumberOfComponents(), numComponents));
  }
```

Add include at top of `ReadRawBinary.cpp`:
```cpp
#include <numeric>
```

- [ ] **Step 3: Update executeImpl in ReadRawBinaryFilter.cpp**

Replace `inputValues.numberOfComponentsValue` assignment (line 184) with:
```cpp
  auto compDimsData = filterArgs.value<DynamicTableParameter::ValueType>(k_CompDims_Key);
  const std::vector<double>& compDimsRow = compDimsData.at(0);
  ShapeType compDims;
  compDims.reserve(compDimsRow.size());
  for(size_t idx = 0; idx < compDimsRow.size(); idx++)
  {
    compDims.push_back(static_cast<usize>(compDimsRow[idx]));
  }
  inputValues.componentDimsValue = compDims;
```

Also add includes for `DynamicTableParameter`:
```cpp
#include "simplnx/Parameters/DynamicTableParameter.hpp"
```

- [ ] **Step 4: Build to verify**

Run:
```bash
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && cmake --build . --target SimplnxCore -- -j $(sysctl -n hw.ncpu)
```
Expected: Clean build, no errors.

- [ ] **Step 5: Commit**

```bash
git add src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReadRawBinary.hpp src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReadRawBinary.cpp src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.cpp
git commit -m "ENH: Update ReadRawBinary execute path for multi-dimensional component dims"
```

---

### Task 6: Implement fromJson version migration and update FromSIMPLJson

**Files:**
- Modify: `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.cpp`

- [ ] **Step 1: Implement fromJson override**

Add the following method to `ReadRawBinaryFilter.cpp`, before the `FromSIMPLJson` method:

```cpp
Result<Arguments> ReadRawBinaryFilter::fromJson(const nlohmann::json& json) const
{
  auto version = json.value("parameters_version", 1);
  if(version < 2)
  {
    nlohmann::json migrated = json;
    // Convert old UInt64 "number_of_components" to new DynamicTable "component_dimensions"
    if(migrated.contains("number_of_components"))
    {
      uint64 numComp = migrated["number_of_components"].get<uint64>();
      migrated["component_dimensions"] = DynamicTableParameter::ValueType{{static_cast<double>(numComp)}};
      migrated.erase("number_of_components");
    }
    // Add default for new parameter
    if(!migrated.contains("set_tuple_dimensions"))
    {
      migrated["set_tuple_dimensions"] = true;
    }
    migrated["parameters_version"] = 2;
    return IFilter::fromJson(migrated);
  }
  return IFilter::fromJson(json);
}
```

- [ ] **Step 2: Update FromSIMPLJson**

In the `FromSIMPLJson` method, replace the line that converts `NumberOfComponents`:
```cpp
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_NumberOfComponentsKey, k_NumberOfComponents_Key));
```
with:
```cpp
  // Convert old integer NumberOfComponents to DynamicTable component_dimensions
  if(json.contains(SIMPL::k_NumberOfComponentsKey))
  {
    auto compResult = SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_NumberOfComponentsKey, "number_of_components_temp");
    if(compResult.valid())
    {
      uint64 numComp = args.value<uint64>("number_of_components_temp");
      args.insertOrAssign(k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<double>(numComp)}}));
      args.erase("number_of_components_temp");
    }
    else
    {
      results.push_back(std::move(compResult));
    }
  }
```

- [ ] **Step 3: Build to verify**

Run:
```bash
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && cmake --build . --target SimplnxCore -- -j $(sysctl -n hw.ncpu)
```
Expected: Clean build.

- [ ] **Step 4: Commit**

```bash
git add src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.cpp
git commit -m "ENH: Implement fromJson version migration and update FromSIMPLJson for ReadRawBinaryFilter"
```

---

### Task 7: Update existing tests for new parameter format

Update the test helper function and existing test cases to use the new parameter keys.

**Files:**
- Modify: `src/Plugins/SimplnxCore/test/ReadRawBinaryTest.cpp`

- [ ] **Step 1: Update CreateFilterArguments helper**

Replace the `CreateFilterArguments` function (lines 49-64) with:
```cpp
Arguments CreateFilterArguments(NumericType scalarType, usize N, usize file_size, usize skipBytes)
{
  Arguments args;

  args.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_TestOutput));
  args.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, std::make_any<NumericType>(scalarType));
  args.insertOrAssign(ReadRawBinaryFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<float64>(file_size)}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<float64>(N)}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(endian::little)));
  args.insertOrAssign(ReadRawBinaryFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(skipBytes));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

  return args;
}
```

- [ ] **Step 2: Update error code constants**

Replace the error code constants (lines 44-46) with:
```cpp
constexpr int32 k_RbrWrongType = -78707;
constexpr int32 k_RbrSkippedTooMuch = -78706;
```

Remove `k_RbrNumComponentsError` — the old component error code is no longer used (replaced by the file-data-fits check). Update `TestCase3_Execute` to check for the new error code `-78708` instead.

- [ ] **Step 3: Update TestCase3_Execute for new error code**

In `TestCase3_Execute`, change the error code check:
```cpp
  REQUIRE(errors[0].code == -78708);
```

- [ ] **Step 4: Remove unused NumberParameter include**

Remove:
```cpp
#include "simplnx/Parameters/NumberParameter.hpp"
```

Add:
```cpp
#include "simplnx/Parameters/BoolParameter.hpp"
```

- [ ] **Step 5: Build and run tests**

Run:
```bash
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && cmake --build . --target SimplnxCore -- -j $(sysctl -n hw.ncpu)
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && ctest -R "SimplnxCore::ReadRawBinary" --verbose
```
Expected: All 5 existing test cases pass with new parameter format.

- [ ] **Step 6: Commit**

```bash
git add src/Plugins/SimplnxCore/test/ReadRawBinaryTest.cpp
git commit -m "TEST: Update ReadRawBinaryFilter tests for v2 parameters"
```

---

### Task 8: Add new test cases

Add tests for the new functionality: AM placement, multi-dimensional components, skip-header edge cases, and subset reads.

**Files:**
- Modify: `src/Plugins/SimplnxCore/test/ReadRawBinaryTest.cpp`

- [ ] **Step 1: Add test for AttributeMatrix placement**

Add after the existing test cases:
```cpp
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case6_AMPlacement)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  constexpr usize xDim = 10;
  constexpr usize yDim = 20;
  constexpr usize zDim = 5;
  constexpr usize tupleCount = xDim * yDim * zDim;
  constexpr usize numComp = 1;
  constexpr usize dataArraySize = tupleCount * numComp;

  std::vector<int32> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int32>(0));

  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  REQUIRE(CreateTestDataFile<int32>(exemplaryData));

  // Create a DataStructure with an ImageGeom and cell data AttributeMatrix
  DataStructure dataStructure;
  ImageGeom* imageGeom = ImageGeom::Create(dataStructure, "ImageGeom");
  imageGeom->setDimensions({xDim, yDim, zDim});
  AttributeMatrix* cellAM = AttributeMatrix::Create(dataStructure, "CellData", {zDim, yDim, xDim}, imageGeom->getId());
  imageGeom->setCellData(*cellAM);

  DataPath outputPath = DataPath({"ImageGeom", "CellData", "BinaryData"});

  Arguments args;
  args.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_TestOutput));
  args.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insertOrAssign(ReadRawBinaryFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.0}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<float64>(numComp)}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(endian::little)));
  args.insertOrAssign(ReadRawBinaryFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, outputPath);

  ReadRawBinaryFilter filter;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<int32>>(outputPath));
  const auto& createdData = dataStructure.getDataRefAs<DataArray<int32>>(outputPath);
  REQUIRE(createdData.getNumberOfTuples() == tupleCount);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

- [ ] **Step 2: Add test for multi-dimensional components**

```cpp
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case7_MultiCompDims)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  // 3x3 tensor = 9 components per tuple, 100 tuples
  constexpr usize tupleCount = 100;
  constexpr usize numComp = 9; // 3x3
  constexpr usize dataArraySize = tupleCount * numComp;

  std::vector<float32> exemplaryData(dataArraySize);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<float32>(0));

  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  REQUIRE(CreateTestDataFile<float32>(exemplaryData));

  ReadRawBinaryFilter filter;
  Arguments args;
  args.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_TestOutput));
  args.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::float32));
  args.insertOrAssign(ReadRawBinaryFilter::k_AdvancedOptions_Key, std::make_any<bool>(true));
  args.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{static_cast<float64>(tupleCount)}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{3.0, 3.0}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(endian::little)));
  args.insertOrAssign(ReadRawBinaryFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(preflightResult.outputActions);

  auto executeResult = filter.execute(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_VALID(executeResult.result);

  REQUIRE_NOTHROW(dataStructure.getDataRefAs<DataArray<float32>>(k_CreatedArrayPath));
  const auto& createdData = dataStructure.getDataRefAs<DataArray<float32>>(k_CreatedArrayPath);
  REQUIRE(createdData.getNumberOfTuples() == tupleCount);
  REQUIRE(createdData.getNumberOfComponents() == numComp);

  const auto& store = createdData.getDataStoreRef();
  bool isSame = true;
  for(usize i = 0; i < dataArraySize; ++i)
  {
    if(store[i] != exemplaryData[i])
    {
      isSame = false;
      break;
    }
  }
  REQUIRE(isSame);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

- [ ] **Step 3: Add test for file too small for requested dimensions**

```cpp
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case8_FileTooSmall)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  // Write 100 int32 values but request 200 tuples
  constexpr usize actualTuples = 100;
  constexpr usize requestedTuples = 200;

  std::vector<int32> exemplaryData(actualTuples);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int32>(0));

  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  REQUIRE(CreateTestDataFile<int32>(exemplaryData));

  ReadRawBinaryFilter filter;
  Arguments args = CreateFilterArguments(NumericType::int32, 1, requestedTuples, 0);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == -78708);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

- [ ] **Step 4: Add test for not-in-AM with Set Tuple Dimensions unchecked**

```cpp
TEST_CASE("SimplnxCore::ReadRawBinaryFilter(Case9_NoAMNoTupleDims)", "[SimplnxCore][ReadRawBinaryFilter]")
{
  UnitTest::LoadPlugins();
  fs::create_directories(k_TestOutput.parent_path());

  std::vector<int32> exemplaryData(100);
  std::iota(exemplaryData.begin(), exemplaryData.end(), static_cast<int32>(0));

  auto fileGuard = MakeScopeGuard([]() noexcept { fs::remove(k_TestOutput); });
  REQUIRE(CreateTestDataFile<int32>(exemplaryData));

  ReadRawBinaryFilter filter;
  Arguments args;
  args.insertOrAssign(ReadRawBinaryFilter::k_InputFile_Key, std::make_any<FileSystemPathParameter::ValueType>(k_TestOutput));
  args.insertOrAssign(ReadRawBinaryFilter::k_ScalarType_Key, std::make_any<NumericType>(NumericType::int32));
  args.insertOrAssign(ReadRawBinaryFilter::k_AdvancedOptions_Key, std::make_any<bool>(false));
  args.insertOrAssign(ReadRawBinaryFilter::k_TupleDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.0}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_CompDims_Key, std::make_any<DynamicTableParameter::ValueType>(DynamicTableParameter::ValueType{{1.0}}));
  args.insertOrAssign(ReadRawBinaryFilter::k_Endian_Key, std::make_any<ChoicesParameter::ValueType>(static_cast<uint64>(endian::little)));
  args.insertOrAssign(ReadRawBinaryFilter::k_SkipHeaderBytes_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ReadRawBinaryFilter::k_CreatedAttributeArrayPath_Key, k_CreatedArrayPath);

  DataStructure dataStructure;
  auto preflightResult = filter.preflight(dataStructure, args);
  SIMPLNX_RESULT_REQUIRE_INVALID(preflightResult.outputActions);

  const std::vector<Error>& errors = preflightResult.outputActions.errors();
  REQUIRE(errors.size() == 1);
  REQUIRE(errors[0].code == -78703);

  UnitTest::CheckArraysInheritTupleDims(dataStructure);
}
```

- [ ] **Step 5: Add required includes for new tests**

Add at the top of the test file:
```cpp
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
```

- [ ] **Step 6: Build and run all tests**

Run:
```bash
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && cmake --build . --target SimplnxCore -- -j $(sysctl -n hw.ncpu)
cd /Users/mjackson/Workspace7/DREAM3D-Build/NX-Com-Qt69-Vtk95-Rel && ctest -R "SimplnxCore::ReadRawBinary" --verbose
```
Expected: All 9 test cases pass (5 original + 4 new).

- [ ] **Step 7: Commit**

```bash
git add src/Plugins/SimplnxCore/test/ReadRawBinaryTest.cpp
git commit -m "TEST: Add tests for AM placement, multi-dim components, file-too-small, and no-AM-no-dims"
```
