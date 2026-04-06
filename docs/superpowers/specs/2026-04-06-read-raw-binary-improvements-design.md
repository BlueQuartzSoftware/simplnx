# ReadRawBinaryFilter Improvements Design

## Overview

Improve the ReadRawBinaryFilter to match CreateDataArrayAdvancedFilter's parameter patterns, support AttributeMatrix placement, fix critical bugs in the binary read path, and enable parameter version migration in the framework.

## 1. Parameter Changes

### Current Parameters (v1)

| Key | Type | Description |
|-----|------|-------------|
| `input_file` | `FileSystemPathParameter` | Input binary file |
| `scalar_type_index` | `NumericTypeParameter` | Data type (int8, uint8, ..., float64) |
| `tuple_dimensions` | `DynamicTableParameter` | Tuple dims (always visible) |
| `number_of_components` | `UInt64Parameter` | Single scalar component count |
| `endian_index` | `ChoicesParameter` | Little or Big endian |
| `skip_header_bytes` | `UInt64Parameter` | Bytes to skip before data |
| `created_attribute_array_path` | `ArrayCreationParameter` | Output array path |

### New Parameters (v2)

Changes from v1:

- **Add** `set_tuple_dimensions` (`BoolParameter`, default `true`). Description: "Set Tuple Dimensions [not required if creating inside an existing Attribute Matrix]". Links to show/hide `tuple_dimensions`.
- **Replace** `number_of_components` (`UInt64Parameter`) with `component_dimensions` (`DynamicTableParameter`). 1 static row, dynamic columns labeled "COMP DIM {}". Default `{{1.0}}`. Matches CreateDataArrayAdvancedFilter's `k_CompDims_Key` pattern.
- All other parameters unchanged.

### Parameter Version: 1 -> 2

Documented in `parametersVersion()` comments:
```
// Version 1 -> 2
// Change 1: k_NumberOfComponents_Key ("number_of_components") UInt64Parameter
//            replaced with k_CompDims_Key ("component_dimensions") DynamicTableParameter
// Change 2: Added k_AdvancedOptions_Key ("set_tuple_dimensions") BoolParameter
```

## 2. Preflight Validation Flow

The preflight follows this sequence:

### Step 1: Extract parameters

### Step 2: Validate component dimensions
- Extract component dims from `DynamicTableParameter` first row, convert each value to `usize`.
- Error if any component dimension is 0.
- Calculate `numComponents = product of all component dims`.

### Step 3: Resolve tuple dimensions (AttributeMatrix-aware)

Check if `outputArrayPath.getParent()` is an `AttributeMatrix`:

**If parent IS an AttributeMatrix:**
- `tupleDims = parentAM->getShape()` (inherit shape).
- If user also checked "Set Tuple Dimensions", emit warning that AM shape overrides custom dims.

**If parent is NOT an AttributeMatrix:**
- If "Set Tuple Dimensions" is unchecked: error "not within an AttributeMatrix, so dimensions cannot be determined implicitly. Check Set Tuple Dimensions."
- If checked: extract from table, error if any dimension is 0.

Calculate `numTuples = product of all tuple dims`.

### Step 4: Validate file size
- `fileSize = fs::file_size(inputFile)`. Error if empty.
- Error if `skipHeaderBytes >= fileSize`.
- `totalBytesToRead = fileSize - skipHeaderBytes`.
- `typeSize = GetNumericTypeSize(scalarType)`.
- Error if `totalBytesToRead % typeSize != 0` (wrong scalar type).

### Step 5: Validate data fits
- `totalElementsInFile = totalBytesToRead / typeSize`.
- `requiredElements = numTuples * numComponents`.
- Error if `requiredElements > totalElementsInFile`: "The file does not contain enough data for the requested array dimensions."
- Warning if `requiredElements < totalElementsInFile`: "Only a subset of the file data will be read."

### Step 6: Create output array action
- `CreateArrayAction(dataType, tupleDims, compDims, outputPath)`.

## 3. Bug Fixes

### Bug 1: Infinite loop in `ImportFromBinaryFile` (DataArrayUtilities.hpp)

When `fread` returns 0 (premature EOF, read error), `elementCounter` never advances and the `while(elementCounter < numElements)` loop spins forever.

**Fix:** Check if `elementsRead == 0` after `fread` and return an error:
```cpp
if(elementsRead == 0)
{
  std::fclose(inputFilePtr);
  return MakeErrorResult(-1001, "Unexpected end of file or read error.");
}
```

### Bug 2: `int32` cast truncates skip bytes > 2GB (DataArrayUtilities.hpp)

`FSEEK64(inputFilePtr, static_cast<int32>(startByte), SEEK_SET)` truncates values > 2GB.

**Fix:** Cast to `int64` instead of `int32`.

### Bug 3: No fseek error checking (DataArrayUtilities.hpp)

Seek failure is silently ignored, causing reads from the wrong position.

**Fix:** Check `FSEEK64` return value; return error if non-zero.

## 4. Execute Path Changes

- Update `ReadRawBinaryInputValues` struct: replace `uint64 numberOfComponentsValue` with `ShapeType componentDimsValue`.
- Update the validation check in `ReadRawBinary::execute()` to compare `getNumberOfComponents()` against the product of the new component dims.
- No other execute changes. The array is already correctly sized by `CreateArrayAction` in preflight. The `ImportFromBinaryFile` bug fixes are the only changes to the read path.

## 5. Framework Change: Virtual `fromJson`

Make `IFilter::fromJson()` virtual so filters can override it to migrate old parameter formats.

**Change to `IFilter.hpp`:**
```cpp
virtual Result<Arguments> fromJson(const nlohmann::json& json) const;
```

One-word change. All existing filters inherit the base implementation unchanged. The `parameters_version` value already stored in pipeline JSON becomes load-bearing.

**ReadRawBinaryFilter override:**
```cpp
Result<Arguments> ReadRawBinaryFilter::fromJson(const nlohmann::json& json) const override
{
  auto version = json.value("parameters_version", 1);
  if(version < 2)
  {
    nlohmann::json migrated = json;
    if(migrated.contains("number_of_components"))
    {
      uint64 numComp = migrated["number_of_components"].get<uint64>();
      migrated["component_dimensions"] = {{static_cast<double>(numComp)}};
      migrated.erase("number_of_components");
    }
    if(!migrated.contains("set_tuple_dimensions"))
    {
      migrated["set_tuple_dimensions"] = true;
    }
    return IFilter::fromJson(migrated);
  }
  return IFilter::fromJson(json);
}
```

**`FromSIMPLJson`** also updated to convert the legacy DREAM3D 6.x `NumberOfComponents` integer to the new table format.

## 6. Testing

Unit test updates for ReadRawBinaryFilter:

1. **Existing tests** -- Update to use new parameter keys. Verify existing behavior preserved.
2. **AttributeMatrix placement** -- Create ImageGeom with cell data AM, set output path inside AM, verify tuple dims inherited from AM shape.
3. **AM with "Set Tuple Dimensions" checked** -- Verify warning generated when user provides custom dims but parent is an AM.
4. **Not in AM with "Set Tuple Dimensions" unchecked** -- Verify error returned.
5. **Multi-dimensional components** -- Set component dims to `{{3.0, 3.0}}` (3x3 tensor), verify correct component shape and data read.
6. **Skip header bytes exceeds file size** -- Verify preflight error (no infinite loop).
7. **File too small for requested dimensions** -- Verify preflight error.
8. **Subset read** -- Set tuple dims smaller than file, verify warning and correct data.

The `ImportFromBinaryFile` bug fixes are exercised indirectly through the skip-header-bytes and file-size tests.

## Files Modified

| File | Change |
|------|--------|
| `src/simplnx/Filter/IFilter.hpp` | Make `fromJson` virtual |
| `src/simplnx/Utilities/DataArrayUtilities.hpp` | Fix infinite loop, int32 cast, fseek error check |
| `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.hpp` | New parameter keys, `fromJson` override declaration |
| `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/ReadRawBinaryFilter.cpp` | New parameters, preflight rework, `fromJson` override, version bump |
| `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReadRawBinary.hpp` | Update `ReadRawBinaryInputValues` struct |
| `src/Plugins/SimplnxCore/src/SimplnxCore/Filters/Algorithms/ReadRawBinary.cpp` | Update component validation in execute |
| `src/Plugins/SimplnxCore/test/ReadRawBinaryTest.cpp` | New and updated test cases |
