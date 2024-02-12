#pragma once

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/EmptyDataStore.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/DataStructure/IO/Generic/DataIOCollection.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/MemoryUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/TemplateHelpers.hpp"
#include "simplnx/simplnx_export.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

namespace
{
#if defined(_MSC_VER)
#define FSEEK64 _fseeki64
#else
#define FSEEK64 std::fseek
#endif
} // namespace

#define SIMPLNX_DEF_STRING_CONVERTOR_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                               \
  CONTAINER_TYPE value;                                                                                                                                                                                \
  try                                                                                                                                                                                                  \
  {                                                                                                                                                                                                    \
    value = FUNCTION(input);                                                                                                                                                                           \
  } catch(const std::invalid_argument& e)                                                                                                                                                              \
  {                                                                                                                                                                                                    \
    return nx::core::MakeErrorResult<TYPE>(-10351, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                             \
  } catch(const std::out_of_range& e)                                                                                                                                                                  \
  {                                                                                                                                                                                                    \
    return nx::core::MakeErrorResult<TYPE>(-10353, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                    \
  }                                                                                                                                                                                                    \
                                                                                                                                                                                                       \
  if(value > std::numeric_limits<TYPE>::max() || value < std::numeric_limits<TYPE>::min())                                                                                                             \
  {                                                                                                                                                                                                    \
    return nx::core::MakeErrorResult<TYPE>(-10353, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                    \
  }                                                                                                                                                                                                    \
                                                                                                                                                                                                       \
  return {static_cast<TYPE>(value)};

#define SIMPLNX_DEF_STRING_CONVERTOR_SIGNED_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                        \
  template <>                                                                                                                                                                                          \
  struct ConvertTo<TYPE>                                                                                                                                                                               \
  {                                                                                                                                                                                                    \
    static Result<TYPE> convert(const std::string& input)                                                                                                                                              \
    {                                                                                                                                                                                                  \
      SIMPLNX_DEF_STRING_CONVERTOR_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                                 \
    }                                                                                                                                                                                                  \
  };

#define SIMPLNX_DEF_STRING_CONVERTOR_UNSIGNED_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                      \
  template <>                                                                                                                                                                                          \
  struct ConvertTo<TYPE>                                                                                                                                                                               \
  {                                                                                                                                                                                                    \
    static Result<TYPE> convert(const std::string& input)                                                                                                                                              \
    {                                                                                                                                                                                                  \
      if(!input.empty() && input.at(0) == '-')                                                                                                                                                         \
      {                                                                                                                                                                                                \
        return nx::core::MakeErrorResult<TYPE>(-10353, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                \
      }                                                                                                                                                                                                \
                                                                                                                                                                                                       \
      SIMPLNX_DEF_STRING_CONVERTOR_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                                 \
    }                                                                                                                                                                                                  \
  };

#define SIMPLNX_DEF_STRING_CONVERTOR_FLOATING_POINT(TYPE, FUNCTION)                                                                                                                                    \
  template <>                                                                                                                                                                                          \
  struct ConvertTo<TYPE>                                                                                                                                                                               \
  {                                                                                                                                                                                                    \
    static Result<TYPE> convert(const std::string& input)                                                                                                                                              \
    {                                                                                                                                                                                                  \
      TYPE value;                                                                                                                                                                                      \
      try                                                                                                                                                                                              \
      {                                                                                                                                                                                                \
        value = static_cast<TYPE>(FUNCTION(input));                                                                                                                                                    \
      } catch(const std::invalid_argument& e)                                                                                                                                                          \
      {                                                                                                                                                                                                \
        return nx::core::MakeErrorResult<TYPE>(-10351, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                         \
      } catch(const std::out_of_range& e)                                                                                                                                                              \
      {                                                                                                                                                                                                \
        return nx::core::MakeErrorResult<TYPE>(-10353, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                \
      }                                                                                                                                                                                                \
      return {value};                                                                                                                                                                                  \
    }                                                                                                                                                                                                  \
  };

namespace nx::core
{
template <class T>
struct ConvertTo
{
};

/**
 * These macros will create convertor objects that convert from a string to a numeric type
 */

SIMPLNX_DEF_STRING_CONVERTOR_UNSIGNED_INT(uint64, uint8, std::stoull)
SIMPLNX_DEF_STRING_CONVERTOR_SIGNED_INT(int64, int8, std::stoll)
SIMPLNX_DEF_STRING_CONVERTOR_UNSIGNED_INT(uint64, uint16, std::stoull)
SIMPLNX_DEF_STRING_CONVERTOR_SIGNED_INT(int64, int16, std::stoll)
SIMPLNX_DEF_STRING_CONVERTOR_UNSIGNED_INT(uint64, uint32, std::stoull)
SIMPLNX_DEF_STRING_CONVERTOR_SIGNED_INT(int64, int32, std::stoll)
SIMPLNX_DEF_STRING_CONVERTOR_UNSIGNED_INT(uint64, uint64, std::stoull)
SIMPLNX_DEF_STRING_CONVERTOR_SIGNED_INT(int64, int64, std::stoll)
#ifdef __APPLE__
SIMPLNX_DEF_STRING_CONVERTOR_UNSIGNED_INT(usize, usize, std::stoull)
#endif
SIMPLNX_DEF_STRING_CONVERTOR_FLOATING_POINT(float32, std::stof)
SIMPLNX_DEF_STRING_CONVERTOR_FLOATING_POINT(float64, std::stod)

template <>
struct ConvertTo<bool>
{
  static Result<bool> convert(const std::string& input)
  {
    if(input == "TRUE" || input == "true" || input == "True")
    {
      return {true};
    }

    if(input == "FALSE" || input == "false" || input == "False")
    {
      return {false};
    }

    Result<int64> intResult = ConvertTo<int64>::convert(input);
    if(intResult.valid())
    {
      return {intResult.value() != 0};
    }

    Result<float64> floatResult = ConvertTo<float64>::convert(input);
    if(floatResult.valid())
    {
      return {floatResult.value() != 0.0};
    }

    return {true};
  }
};

/**
 * @brief Checks if the given string can be correctly converted into the given type
 * @tparam T The primitive type to convert the string into
 * @param valueAsStr The value to convert
 * @param strType The primitive type. The valid values can be found in a constants file
 * @return Result<> object that is either valid or has an error message/code
 */
template <class T>
Result<> CheckValuesUnsignedInt(const std::string& valueAsStr, const std::string& strType)
{
  static_assert(std::is_unsigned_v<T>);

  if(valueAsStr[0] == '-')
  {
    return MakeErrorResult(-255, fmt::format("The value '{}' could not be converted to {} due to the value being outside of the range for {} to {}", valueAsStr, strType, std::numeric_limits<T>::min(),
                                             std::numeric_limits<T>::max()));
  }
  Result<uint64> conversionResult = ConvertTo<uint64>::convert(valueAsStr);
  if(conversionResult.valid()) // If the string was converted to a double, then lets check the range is valid
  {
    uint64 replaceValue = conversionResult.value();
    if(!((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max())))
    {
      return MakeErrorResult(-256, fmt::format("The value '{}' could not be converted to {} due to the value being outside of the range for {} to {}", valueAsStr, strType,
                                               std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    }
  }
  return ConvertResult(std::move(conversionResult));
}

/**
 * @brief
 * @tparam T
 * @param valueAsStr
 * @param strType
 * @return
 */
template <class T>
Result<> CheckValuesSignedInt(const std::string& valueAsStr, const std::string& strType)
{
  static_assert(std::is_signed_v<T>);

  Result<int64> conversionResult = ConvertTo<int64>::convert(valueAsStr);
  if(conversionResult.valid()) // If the string was converted to a double, then lets check the range is valid
  {
    int64 replaceValue = conversionResult.value();
    if(!((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max())))
    {
      return MakeErrorResult(-257, fmt::format("The value '{}' could not be converted to {} due to the value being outside of the range for {} to {}", valueAsStr, strType,
                                               std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    }
  }
  return ConvertResult(std::move(conversionResult));
}

/**
 * @brief
 * @tparam T
 * @param valueAsStr
 * @param strType
 * @return
 */
template <class T>
Result<> CheckValuesFloatDouble(const std::string& valueAsStr, const std::string& strType)
{
  static_assert(std::is_floating_point_v<T>);

  Result<float64> conversionResult = ConvertTo<float64>::convert(valueAsStr);
  if(conversionResult.valid()) // If the string was converted to a double, then lets check the range is valid
  {
    float64 replaceValue = conversionResult.value();
    if(!(((replaceValue >= static_cast<T>(-1) * std::numeric_limits<T>::max()) && (replaceValue <= static_cast<T>(-1) * std::numeric_limits<T>::min())) || (replaceValue == 0) ||
         ((replaceValue >= std::numeric_limits<T>::min()) && (replaceValue <= std::numeric_limits<T>::max()))))
    {
      return MakeErrorResult<>(-258, fmt::format("The {} replace value was invalid. The valid ranges are -{} to -{}, 0, %{} to %{}", std::numeric_limits<T>::max(), strType,
                                                 std::numeric_limits<T>::min(), std::numeric_limits<T>::min(), std::numeric_limits<T>::max()));
    }
  }
  return ConvertResult(std::move(conversionResult));
}

/**
 * @brief Validates whether the string can be converted to the primitive type used in the DataObject.
 *
 * The validate will check overflow and underflow and that the string represents some sort of numeric value
 * @param value
 * @param inputDataArray
 * @return
 */
SIMPLNX_EXPORT Result<> CheckValueConvertsToArrayType(const std::string& value, const DataObject& inputDataArray);

/**
 * @brief Replaces every value in an array based on a `mask` array.
 * @tparam T The primitive type used in the data array
 * @param inputArrayPtr InputArray that will have values replaced
 * @param condDataPtr The mask array as a boolean array
 * @param replaceValue The value that will be used for every place the conditional array is TRUE
 */
template <class T, typename ConditionalType>
void ReplaceValue(DataArray<T>& inputArrayPtr, const DataArray<ConditionalType>* condArrayPtr, T replaceValue, bool invertMask = false)
{
  T replaceVal = static_cast<T>(replaceValue);
  usize numTuples = inputArrayPtr.getNumberOfTuples();

  const DataArray<ConditionalType>& conditionalArray = *condArrayPtr;
  if(invertMask)
  {
    for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
    {
      if(!conditionalArray[tupleIndex])
      {
        inputArrayPtr.initializeTuple(tupleIndex, replaceValue);
      }
    }
  }
  else
  {
    for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
    {
      if(conditionalArray[tupleIndex])
      {
        inputArrayPtr.initializeTuple(tupleIndex, replaceValue);
      }
    }
  }
}

/**
 * @brief Replaces a value in an array based on a boolean mask.
 * @tparam T Primitive type used for the DataArray
 * @param valueAsStr The value that will be used for the replacement
 * @param inputDataObject Input DataArray that will have values replaced (possibly)
 * @param conditionalDataArray The mask array as a boolean array
 * @return True or False whether the replacement algorithm was run. This function can
 * return FALSE if the wrong array type is specified as the template parameter
 */
struct ConditionalReplaceValueInArrayFromString
{
  template <class T>
  Result<> operator()(const std::string& valueAsStr, DataObject& inputDataObject, const IDataArray& conditionalDataArray, const bool invertMask = false)
  {
    using DataArrayType = DataArray<T>;

    auto& inputDataArray = dynamic_cast<DataArrayType&>(inputDataObject);
    Result<T> conversionResult = ConvertTo<T>::convert(valueAsStr);
    if(conversionResult.invalid())
    {
      return MakeErrorResult<>(-4000, "Input String Value could not be converted to the appropriate numeric type.");
    }

    const nx::core::DataType arrayType = conditionalDataArray.getDataType();

    if(nx::core::DataType::uint8 == arrayType)
    {
      ReplaceValue<T, uint8_t>(inputDataArray, dynamic_cast<const UInt8Array*>(&conditionalDataArray), conversionResult.value(), invertMask);
    }
    else if(nx::core::DataType::int8 == arrayType)
    {
      ReplaceValue<T, int8_t>(inputDataArray, dynamic_cast<const Int8Array*>(&conditionalDataArray), conversionResult.value(), invertMask);
    }
    else if(nx::core::DataType::boolean == arrayType)
    {
      ReplaceValue<T, bool>(inputDataArray, dynamic_cast<const BoolArray*>(&conditionalDataArray), conversionResult.value(), invertMask);
    }
    else
    {
      return MakeErrorResult<>(-4001, "Mask array was not of type [BOOL | UINT8 | INT8].");
    }
    return {};
  }
};

/**
 * @brief Replaces a value in an array based on a boolean mask.
 * @param valueAsStr The value that will be used for the replacement
 * @param inputDataObject Input DataArray that will have values replaced (possibly)
 * @param conditionalDataArray The mask array as a boolean array
 * @return
 */
SIMPLNX_EXPORT Result<> ConditionalReplaceValueInArray(const std::string& valueAsStr, DataObject& inputDataObject, const IDataArray& conditionalDataArray, bool invertmask = false);

template <class T>
uint64 CalculateDataSize(const IDataStore::ShapeType& tupleShape, const IDataStore::ShapeType& componentShape)
{
  uint64 numValues = std::accumulate(tupleShape.begin(), tupleShape.end(), 1ULL, std::multiplies<>());
  uint64 numComponents = std::accumulate(componentShape.begin(), componentShape.end(), 1ULL, std::multiplies<>());
  return numValues * numComponents * sizeof(T);
}

/**
 * @brief Creates a DataStore with the given properties
 * @tparam T Primitive Type (int, float, ...)
 * @param tupleShape The Tuple Dimensions
 * @param componentShape The component dimensions
 * @param mode The mode to assume: PREFLIGHT or EXECUTE. Preflight will NOT allocate any storage. EXECUTE will allocate the memory/storage
 * @return
 */
template <class T>
std::shared_ptr<AbstractDataStore<T>> CreateDataStore(const typename IDataStore::ShapeType& tupleShape, const typename IDataStore::ShapeType& componentShape, IDataAction::Mode mode,
                                                      std::string dataFormat = "")
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyDataStore<T>>(tupleShape, componentShape, dataFormat);
  }
  case IDataAction::Mode::Execute: {
    uint64 dataSize = CalculateDataSize<T>(tupleShape, componentShape);
    auto* preferences = Application::GetOrCreateInstance()->getPreferences();
    if(preferences->forceOocData())
    {
      dataFormat = preferences->largeDataFormat();
    }
    auto ioCollection = Application::GetOrCreateInstance()->getIOCollection();
    ioCollection->checkStoreDataFormat(dataSize, dataFormat);
    return ioCollection->createDataStoreWithType<T>(dataFormat, tupleShape, componentShape);
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

SIMPLNX_EXPORT bool CheckMemoryRequirement(DataStructure& dataStructure, uint64 requiredMemory, std::string& format);

/**
 * @brief Creates a DataArray with the given properties
 * @tparam T Primitive Type (int, float, ...)
 * @param dataStructure The DataStructure to use
 * @param tupleShape The Tuple Dimensions
 * @param nComp The number of components in the DataArray
 * @param path The DataPath to where the data will be stored.
 * @param mode The mode to assume: PREFLIGHT or EXECUTE. Preflight will NOT allocate any storage. EXECUTE will allocate the memory/storage
 * @return
 */
template <class T>
Result<> CreateArray(DataStructure& dataStructure, const std::vector<usize>& tupleShape, const std::vector<usize>& compShape, const DataPath& path, IDataAction::Mode mode, std::string dataFormat = "")
{
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> dataObjectId;

  DataObject* parentObject = nullptr;
  if(parentPath.getLength() != 0)
  {
    parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return MakeErrorResult(-260, fmt::format("CreateArray: Parent object '{}' does not exist", parentPath.toString()));
    }

    dataObjectId = parentObject->getId();
  }

  if(tupleShape.empty())
  {
    return MakeErrorResult(-261, fmt::format("CreateArray: Tuple Shape was empty. Please set the number of tuples."));
  }

  // Validate Number of Components
  if(compShape.empty())
  {
    return MakeErrorResult(-262, fmt::format("CreateArray: Component Shape was empty. Please set the number of components."));
  }
  const usize numComponents = std::accumulate(compShape.cbegin(), compShape.cend(), static_cast<usize>(1), std::multiplies<>());
  if(numComponents == 0 && mode == IDataAction::Mode::Execute)
  {
    return MakeErrorResult(-263, fmt::format("CreateArray: Number of components is ZERO. Please set the number of components."));
  }

  const usize last = path.getLength() - 1;

  std::string name = path[last];

  const usize numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<usize>(1), std::multiplies<>());
  uint64 requiredMemory = numTuples * numComponents * sizeof(T);
  if(!CheckMemoryRequirement(dataStructure, requiredMemory, dataFormat))
  {
    uint64 totalMemory = requiredMemory + dataStructure.memoryUsage();
    uint64 availableMemory = Memory::GetTotalMemory();
    return MakeErrorResult(
        -267, fmt::format("CreateArray: Cannot create DataArray '{}'.\n\tTotal memory required for DataStructure: '{}' Bytes.\n\tTotal memory: '{}' Bytes", name, totalMemory, availableMemory));
  }

  auto store = CreateDataStore<T>(tupleShape, compShape, mode, dataFormat);
  auto dataArray = DataArray<T>::Create(dataStructure, name, store, dataObjectId);
  if(dataArray == nullptr)
  {
    if(dataStructure.getId(path).has_value())
    {
      return MakeErrorResult(-264, fmt::format("CreateArray: Cannot create Data Array at path '{}' because it already exists. Choose a different name.", path.toString()));
    }

    if(parentObject->getDataObjectType() == DataObject::Type::AttributeMatrix)
    {
      auto* attrMatrix = dynamic_cast<AttributeMatrix*>(parentObject);
      std::string amShape = fmt::format("Attribute Matrix Tuple Dims: {}", fmt::join(attrMatrix->getShape(), " x "));
      std::string arrayShape = fmt::format("Data Array Tuple Shape: {}", fmt::join(store->getTupleShape(), " x "));
      return MakeErrorResult(-265,
                             fmt::format("CreateArray: Unable to create Data Array '{}' inside Attribute matrix '{}'. Mismatch of tuple dimensions. The created Data Array must have the same tuple "
                                         "dimensions or the same total number of tuples.\n{}\n{}",
                                         name, dataStructure.getDataPathsForId(parentObject->getId()).front().toString(), amShape, arrayShape));
    }
    else
    {
      return MakeErrorResult(-266, fmt::format("CreateArray: Unable to create DataArray at '{}'", path.toString()));
    }
  }

  return {};
}

template <typename T>
std::shared_ptr<AbstractDataStore<T>> ConvertDataStore(const AbstractDataStore<T>& dataStore, const std::string& dataFormat)
{
  if(dataStore.getDataFormat() == dataFormat)
  {
    return nullptr;
  }

  auto ioCollection = Application::GetOrCreateInstance()->getIOCollection();
  std::shared_ptr<AbstractDataStore<T>> newStore = ioCollection->createDataStoreWithType<T>(dataFormat, dataStore.getTupleShape(), dataStore.getComponentShape());
  if(newStore == nullptr)
  {
    return nullptr;
  }

  newStore->copy(dataStore);
  return newStore;
}

template <typename T>
bool ConvertDataArray(const std::shared_ptr<DataArray<T>> dataArray, const std::string& dataFormat)
{
  if(dataArray == nullptr)
  {
    return false;
  }
  const AbstractDataStore<T>& dataStore = dataArray->getDataStoreRef();
  auto convertedDataStore = ConvertDataStore<T>(dataStore, dataFormat);
  if(convertedDataStore == nullptr)
  {
    return false;
  }

  dataArray->setDataStore(convertedDataStore);
  return true;
}

bool ConvertIDataArray(const std::shared_ptr<IDataArray>& dataArray, const std::string& dataFormat);

/**
 * @brief Creates a NeighborList array with the given properties
 * @tparam T Primitive Type (int, float, ...)
 * @param dataStructure The DataStructure to use
 * @param tupleShape The Tuple Dimensions
 * @param path The DataPath to where the list  will be stored.
 * @param mode The mode to assume: PREFLIGHT or EXECUTE. Preflight will NOT allocate any storage. EXECUTE will allocate the memory/storage
 * @return
 */
template <class T>
Result<> CreateNeighbors(DataStructure& dataStructure, usize numTuples, const DataPath& path, IDataAction::Mode mode)
{
  static constexpr StringLiteral prefix = "CreateNeighborListAction: ";
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> dataObjectId;

  if(parentPath.getLength() != 0)
  {
    auto* parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return MakeErrorResult(-5801, fmt::format("{}Parent object \"{}\" does not exist", prefix, parentPath.toString()));
    }

    dataObjectId = parentObject->getId();
  }

  const usize last = path.getLength() - 1;

  std::string name = path[last];

  auto neighborList = NeighborList<T>::Create(dataStructure, name, numTuples, dataObjectId);
  if(neighborList == nullptr)
  {
    return MakeErrorResult(-5802, fmt::format("{}Unable to create NeighborList at \"{}\"", prefix, path.toString()));
  }

  return {};
}

/**
 * @brief Attempts to retrieve a DataArray at a given DataPath in the DataStructure. Throws runtime_error on error
 * @tparam T
 * @param data
 * @param path
 * @return
 */
template <class T>
DataArray<T>* ArrayFromPath(DataStructure& dataStructure, const DataPath& path)
{
  using DataArrayType = DataArray<T>;
  DataObject* object = dataStructure.getData(path);
  if(object == nullptr)
  {
    throw std::runtime_error(fmt::format("DataArray does not exist at DataPath: '{}'", path.toString()));
  }
  auto* dataArray = dynamic_cast<DataArrayType*>(object);
  if(dataArray == nullptr)
  {
    throw std::runtime_error(fmt::format("DataPath does not point to a DataArray. DataPath: '{}'", path.toString()));
  }
  return dataArray;
}

/**
 * @brief
 * @tparam T
 * @param data
 * @param path
 * @return
 */
template <class T>
DataArray<T>& ArrayRefFromPath(DataStructure& data, const DataPath& path)
{
  DataObject* object = data.getData(path);
  auto* dataArray = dynamic_cast<DataArray<T>*>(object);
  if(dataArray == nullptr)
  {
    throw std::runtime_error("Can't obtain DataArray");
  }
  return *dataArray;
}

/**
 * @brief Reads a binary file into a pre-allocated DataArray<T> object
 * @tparam T The POD type. Only C++ native types are supported.
 * @param binaryFilePath The path to the input file
 * @param outputDataArray The DataArray<T> to store the data read from the file
 * @param startByte The byte offset into the file to start reading the data.
 * @param defaultBufferSize The buffer size that is used when reading.
 * @return A Result<> type that contains any warnings or errors that occurred.
 */
template <typename T>
Result<> ImportFromBinaryFile(const fs::path& binaryFilePath, DataArray<T>& outputDataArray, usize startByte = 0, usize defaultBufferSize = 1000000)
{
  FILE* inputFile = std::fopen(binaryFilePath.string().c_str(), "rb");
  if(inputFile == nullptr)
  {
    return MakeErrorResult(-1000, fmt::format("Unable to open the specified file. '{}'", binaryFilePath.string()));
  }

  // Skip some bytes if needed
  if(startByte > 0)
  {
    FSEEK64(inputFile, static_cast<int32>(startByte), SEEK_SET);
  }

  const usize numElements = outputDataArray.getSize();
  // Now start reading the data in chunks if needed.
  usize chunkSize = std::min(numElements, defaultBufferSize);
  std::vector<T> buffer(chunkSize);

  usize elementCounter = 0;
  while(elementCounter < numElements)
  {
    usize elements_read = std::fread(buffer.data(), sizeof(T), chunkSize, inputFile);

    for(usize i = 0; i < elements_read; i++)
    {
      outputDataArray[i + elementCounter] = buffer[i];
    }

    elementCounter += elements_read;

    usize elementsLeft = numElements - elementCounter;

    if(elementsLeft < chunkSize)
    {
      chunkSize = elementsLeft;
    }
  }

  std::fclose(inputFile);

  return {};
}

/**
 * @brief
 * @tparam T
 * @param filename
 * @param name
 * @param dataStructure
 * @param tupleShape
 * @param componentShape
 * @param parentId
 * @return
 */
template <typename T>
DataArray<T>* ImportFromBinaryFile(const std::string& filename, const std::string& name, DataStructure& dataStructure, const std::vector<usize>& tupleShape, const std::vector<usize>& componentShape,
                                   DataObject::IdType parentId = {})
{
  // std::cout << "  Reading file " << filename << std::endl;
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  if(!fs::exists(filename))
  {
    std::cout << "File Does Not Exist:'" << filename << "'" << std::endl;
    return nullptr;
  }

  std::shared_ptr<DataStoreType> dataStore = std::shared_ptr<DataStoreType>(new DataStoreType({tupleShape}, componentShape, static_cast<T>(0)));
  ArrayType* dataArray = ArrayType::Create(dataStructure, name, dataStore, parentId);

  const usize fileSize = fs::file_size(filename);
  const usize numBytesToRead = dataArray->getSize() * sizeof(T);
  if(numBytesToRead != fileSize)
  {
    std::cout << "FileSize '" << fileSize << "' and Allocated Size '" << numBytesToRead << "' do not match" << std::endl;
    return nullptr;
  }

  Result<> result = ImportFromBinaryFile(fs::path(filename), *dataArray);
  if(result.invalid())
  {
    return nullptr;
  }

  return dataArray;
}

/**
 * @brief Creates a deep copy of an array into another location in the DataStructure.
 *
 * WARNING: If there is a DataObject already at the destination path then that data object
 * is removed from the DataStructure and replaced with the new copy
 * @tparam ArrayType The Type of DataArray to copy. IDataArray and StringArray are supported
 * @param dataStructure The DataStructure object
 * @param sourceDataPath The source path to copy from.
 * @param destDataPath The destination path to copy into.
 * @return Result<> object.
 */
template <typename ArrayType>
Result<> DeepCopy(DataStructure& dataStructure, const DataPath& sourceDataPath, const DataPath& destDataPath)
{
  ArrayType& iDataArray = dataStructure.getDataRefAs<ArrayType>(sourceDataPath);
  if(dataStructure.removeData(destDataPath))
  {
    iDataArray.deepCopy(destDataPath);
  }
  else
  {
    return MakeErrorResult(-34600, fmt::format("Could not remove data array at path '{}' which would be replaced through a deep copy.", destDataPath.toString()));
  }
  return {};
}

/**
 * @brief This function will Resize a DataArray and then replace an existing DataArray in the DataStructure
 * @param dataStructure
 * @param dataPath The path of the target DataArray
 * @param tupleShape The tuple shape of the resized array
 * @param mode The mode: Preflight or Execute
 * @return
 */
SIMPLNX_EXPORT Result<> ResizeAndReplaceDataArray(DataStructure& dataStructure, const DataPath& dataPath, std::vector<usize>& tupleShape, IDataAction::Mode mode);

/**
 * @brief This function will ensure that a user entered numeric value can correctly be parsed into the selected NumericType
 *
 * @param value The string value that is to be parsed
 * @param numericType The NumericType to parse the value into.
 * @return
 */
SIMPLNX_EXPORT Result<> CheckValueConverts(const std::string& value, NumericType numericType);

/**
 * @brief This method will ensure that all the arrays are of the same type
 * @param dataStructure DataStructure that contains the data arrays
 * @param dataArrayPaths  The Paths to check
 * @return
 */
SIMPLNX_EXPORT bool CheckArraysAreSameType(const DataStructure& dataStructure, const std::vector<DataPath>& dataArrayPaths);

/**
 * @brief This method will ensure that all the arrays have the same tuple count
 * @param dataStructure DataStructure that contains the data arrays
 * @param dataArrayPaths  The Paths to check
 * @return
 */
SIMPLNX_EXPORT bool CheckArraysHaveSameTupleCount(const DataStructure& dataStructure, const std::vector<DataPath>& dataArrayPaths);

/**
 * @brief Validates that the number of features in the array are equivalent
 * @param dataStructure the DataStructure containing the array
 * @param arrayPath the DataPath to the array in the dataStructure
 * @param featureIds the ids for the array
 * @return void
 */
SIMPLNX_EXPORT Result<> ValidateNumFeaturesInArray(const DataStructure& dataStructure, const DataPath& arrayPath, const Int32Array& featureIds);

/**
 * @brief This function will ensure that a DataArray can be safely resized to the user entered numeric value
 *
 * @param dataStructure
 * @param arrayPath The path to the DataArray to be reshaped.
 * @param newShape The new tuple shape to resize the array to
 * @return
 */
template <typename T>
Result<> ResizeDataArray(DataStructure& dataStructure, const DataPath& arrayPath, const std::vector<usize>& newShape)
{
  auto* dataArray = dataStructure.getDataAs<DataArray<T>>(arrayPath);
  if(dataArray == nullptr)
  {
    return MakeErrorResult(-4830, fmt::format("Could not find array path '{}' in the given data structure", arrayPath.toString()));
  }
  if(dataArray->getTupleShape() == newShape) // array does not need to be reshaped
  {
    return {};
  }
  const auto& surfaceFeaturesParent = dataStructure.getDataAs<AttributeMatrix>(arrayPath.getParent());
  if(surfaceFeaturesParent != nullptr) // tuple shape of the parent attribute matrix doesn't match the new tuple shape
  {
    return MakeErrorResult(-4831, fmt::format("Cannot resize array at path '{}' to tuple shape {} because the parent is an Attribute Matrix with a tuple shape of {} which does not match.",
                                              arrayPath.toString(), newShape, surfaceFeaturesParent->getShape()));
  }

  // the array's parent is not in an Attribute Matrix so we can safely reshape to the new tuple shape
  dataArray->template getIDataStoreRefAs<DataStore<T>>().resizeTuples(newShape);
  return {};
}

/**
 * @brief This function resize the outermost vector of the NeighborList's underlying data to the NeighborList's set
 * number of tuples and initializes each item in the vector to a (non null) pointer to an empty vector
 *
 * @param dataStructure
 * @param neighborListPath The path to the NeighborList to be initialized.
 */
SIMPLNX_EXPORT void InitializeNeighborList(DataStructure& dataStructure, const DataPath& neighborListPath);

/**
 * @brief These structs and functions are meant to make using a "mask array" or "Good Voxels Array" easier
 * for the developer. There is virtual function call overhead with using these structs and functions.
 *
 * An example use of these functions would be the following:
 * @code
 *  std::unique_ptr<MaskCompare> maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->goodVoxelsArrayPath);
 *  if(!maskCompare->bothTrue(arrayIndex, anotherArrayIndex))
 *  {
 *    // Do something based on the if statement...
 *  }
 * @endcode
 */
struct MaskCompare
{
  virtual ~MaskCompare() noexcept = default;

  /**
   * @brief Both of the values pointed to by the index *must* be `true` or non-zero. If either of the values or
   * *both* of the values are false, this will return false.
   * @param indexA First index
   * @param indexB Second index
   * @return
   */
  virtual bool bothTrue(usize indexA, usize indexB) const = 0;

  /**
   * @brief Both of the values pointed to by the index *must* be `false` or non-zero. If either of the values or
   * *both* of the values are `true`, this will return `false`.
   * @param indexA
   * @param indexB
   * @return
   */
  virtual bool bothFalse(usize indexA, usize indexB) const = 0;

  /**
   * @brief Returns `true` or `false` based on the value at the index
   * @param index index to check
   * @return
   */
  virtual bool isTrue(usize index) const = 0;

  virtual void setValue(usize index, bool val) = 0;

  virtual usize getNumberOfTuples() const = 0;

  virtual usize getNumberOfComponents() const = 0;

  virtual usize countTrueValues() const = 0;
};

struct BoolMaskCompare : public MaskCompare
{
  BoolMaskCompare(BoolArray& array)
  : m_Array(array)
  {
  }
  ~BoolMaskCompare() noexcept override = default;

  BoolArray& m_Array;
  bool bothTrue(usize indexA, usize indexB) const override
  {
    return m_Array.at(indexA) && m_Array.at(indexB);
  }
  bool bothFalse(usize indexA, usize indexB) const override
  {
    return !m_Array.at(indexA) && !m_Array.at(indexB);
  }
  bool isTrue(usize index) const override
  {
    return m_Array.at(index);
  }
  void setValue(usize index, bool val) override
  {
    m_Array[index] = val;
  }
  usize getNumberOfTuples() const override
  {
    return m_Array.getNumberOfTuples();
  }
  usize getNumberOfComponents() const override
  {
    return m_Array.getNumberOfComponents();
  }

  usize countTrueValues() const override
  {
    return std::count(m_Array.begin(), m_Array.end(), true);
  }
};

struct UInt8MaskCompare : public MaskCompare
{
  UInt8MaskCompare(UInt8Array& array)
  : m_Array(array)
  {
  }
  ~UInt8MaskCompare() noexcept override = default;
  UInt8Array& m_Array;
  bool bothTrue(usize indexA, usize indexB) const override
  {
    return m_Array.at(indexA) != 0 && m_Array.at(indexB) != 0;
  }
  bool bothFalse(usize indexA, usize indexB) const override
  {
    return m_Array.at(indexA) == 0 && m_Array.at(indexB) == 0;
  }
  bool isTrue(usize index) const override
  {
    return m_Array.at(index) != 0;
  }
  void setValue(usize index, bool val) override
  {
    m_Array[index] = static_cast<uint8>(val);
  }
  usize getNumberOfTuples() const override
  {
    return m_Array.getNumberOfTuples();
  }
  usize getNumberOfComponents() const override
  {
    return m_Array.getNumberOfComponents();
  }

  usize countTrueValues() const override
  {
    const usize falseCount = std::count(m_Array.begin(), m_Array.end(), 0);
    return getNumberOfTuples() - falseCount;
  }
};

/**
 * @brief Convenience method to create an instance of the MaskCompare subclass.
 *
 * An example use of these functions would be the following:
 * @code
 *  std::unique_ptr<MaskCompare> maskCompare = InstantiateMaskCompare(m_DataStructure, m_InputValues->goodVoxelsArrayPath);
 *  if(!maskCompare->bothTrue(arrayIndex, anotherArrayIndex))
 *  {
 *    // Do something based on the if statement...
 *  }
 * @endcode
 *
 * @param dataStructure The DataStructure object to pull the DataArray from
 * @param maskArrayPath The DataPath of the mask array.
 * @return
 */
SIMPLNX_EXPORT std::unique_ptr<MaskCompare> InstantiateMaskCompare(DataStructure& dataStructure, const DataPath& maskArrayPath);

/**
 * @brief Convenience method to create an instance of the MaskCompare subclass
 * @param maskArrayPtr A Pointer to the mask array which can be of either `bool` or `uint8` type.
 * @return
 */
SIMPLNX_EXPORT std::unique_ptr<MaskCompare> InstantiateMaskCompare(IDataArray& maskArrayPtr);

template <typename T>
class CopyTupleUsingIndexList
{
public:
  CopyTupleUsingIndexList(const IDataArray& oldCellArray, IDataArray& newCellArray, nonstd::span<const int64> newIndices)
  : m_OldCellArray(oldCellArray)
  , m_NewCellArray(newCellArray)
  , m_NewToOldIndices(newIndices)
  {
  }
  ~CopyTupleUsingIndexList() = default;

  CopyTupleUsingIndexList(const CopyTupleUsingIndexList&) = default;
  CopyTupleUsingIndexList(CopyTupleUsingIndexList&&) noexcept = default;
  CopyTupleUsingIndexList& operator=(const CopyTupleUsingIndexList&) = delete;
  CopyTupleUsingIndexList& operator=(CopyTupleUsingIndexList&&) noexcept = delete;

  void convert(usize start, usize end) const
  {
    const auto& oldDataStore = m_OldCellArray.getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& newDataStore = m_NewCellArray.getIDataStoreRefAs<AbstractDataStore<T>>();

    for(usize i = start; i < end; i++)
    {
      int64 oldIndexI = m_NewToOldIndices[i];
      if(oldIndexI >= 0)
      {
        if(!newDataStore.copyFrom(i, oldDataStore, oldIndexI, 1))
        {
          std::cout << fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", m_OldCellArray.getName(), oldIndexI, i) << std::endl;
          break;
        }
      }
      else
      {
        newDataStore.fillTuple(i, 0);
      }
    }
  }

  void operator()() const
  {
    convert(0, m_NewToOldIndices.size());
  }

private:
  const IDataArray& m_OldCellArray;
  IDataArray& m_NewCellArray;
  nonstd::span<const int64> m_NewToOldIndices;
};

/**
 * @brief The following functions and classes are meant to make copying data from one IArray into another easier for the developer.
 *
 * An example use of these functions would be the following (where newCellData is an AttributeMatrix in dataStructure ):
 *   ParallelTaskAlgorithm taskRunner;
 *   for (const auto& [dataId, dataObject] : *newCellData)
 *   {
 *     auto* inputDataArray = dataStructure.getDataAs<IArray>(inputCellDataPath.createChildPath(name));
 *     auto* destDataArray = dataStructure.getDataAs<IArray>(destCellDataPath.createChildPath(name));
 *     auto* newDataArray = dataStructure.getDataAs<IArray>(newCellDataPath.createChildPath(name));
 *     const IArray::ArrayType arrayType = destDataArray->getArrayType();
 *     CopyFromArray::RunParallel<CopyFromArray::Combine>(arrayType, destDataArray, taskRunner, inputDataArray, newDataArray);
 *   }
 *   taskRunner.wait();
 * @code
 *
 * @endcode
 */
namespace CopyFromArray
{
/**
 * @brief Appends all of the data from the inputArray into the destination array starting at the given offset. This function DOES NOT do any bounds checking!
 */
template <class K>
void AppendData(const K& inputArray, K& destArray, usize offset)
{
  const usize numElements = inputArray.getNumberOfTuples() * inputArray.getNumberOfComponents();
  for(usize i = 0; i < numElements; ++i)
  {
    destArray[offset + i] = inputArray.at(i);
  }
}

/**
 * @brief Copies all of the data from the inputArray into the destination array using the given tuple offsets.
 */
template <class K>
bool CopyData(const K& inputArray, K& destArray, usize destTupleOffset, usize srcTupleOffset, usize totalSrcTuples)
{
  if(destTupleOffset >= destArray.getNumberOfTuples())
  {
    return false;
  }

  const usize sourceNumComponents = inputArray.getNumberOfComponents();
  const usize numComponents = destArray.getNumberOfComponents();

  if(sourceNumComponents != numComponents)
  {
    return false;
  }

  if((totalSrcTuples * sourceNumComponents + destTupleOffset * numComponents) > destArray.getNumberOfTuples() * numComponents)
  {
    return false;
  }

  auto srcBegin = inputArray.begin() + (srcTupleOffset * sourceNumComponents);
  auto srcEnd = srcBegin + (totalSrcTuples * sourceNumComponents);
  auto dstBegin = destArray.begin() + (destTupleOffset * numComponents);
  std::copy(srcBegin, srcEnd, dstBegin);

  return true;
}

/**
 * @brief This class will append all of the data from the input array of any IArray type to the given destination array of the same IArray type starting at the given tupleOffset. This class DOES NOT
 * do any bounds checking and assumes that the destination array has already been properly resized to fit all of the data
 */
template <typename T>
class AppendArray
{
public:
  AppendArray(IArray& destCellArray, const IArray& inputCellArray, usize tupleOffset)
  : m_ArrayType(destCellArray.getArrayType())
  , m_InputCellArray(&inputCellArray)
  , m_DestCellArray(&destCellArray)
  , m_TupleOffset(tupleOffset)
  {
  }

  ~AppendArray() = default;

  AppendArray(const AppendArray&) = default;
  AppendArray(AppendArray&&) noexcept = default;
  AppendArray& operator=(const AppendArray&) = delete;
  AppendArray& operator=(AppendArray&&) noexcept = delete;

  void operator()() const
  {
    const usize offset = m_TupleOffset * m_DestCellArray->getNumberOfComponents();
    if(m_ArrayType == IArray::ArrayType::NeighborListArray)
    {
      using NeighborListT = NeighborList<T>;
      auto* destArray = dynamic_cast<NeighborListT*>(m_DestCellArray);
      // Make sure the destination array is allocated AND each tuple list is initialized so we can use the [] operator to copy over the data
      if(destArray->getValues().empty() || destArray->getList(0) == nullptr)
      {
        destArray->addEntry(destArray->getNumberOfTuples() - 1, 0);
      }
      AppendData<NeighborListT>(*dynamic_cast<const NeighborListT*>(m_InputCellArray), *destArray, offset);
    }
    if(m_ArrayType == IArray::ArrayType::DataArray)
    {
      using DataArrayT = DataArray<T>;
      AppendData<DataArrayT>(*dynamic_cast<const DataArrayT*>(m_InputCellArray), *dynamic_cast<DataArrayT*>(m_DestCellArray), offset);
    }
    if(m_ArrayType == IArray::ArrayType::StringArray)
    {
      AppendData<StringArray>(*dynamic_cast<const StringArray*>(m_InputCellArray), *dynamic_cast<StringArray*>(m_DestCellArray), offset);
    }
  }

private:
  IArray::ArrayType m_ArrayType = IArray::ArrayType::Any;
  const IArray* m_InputCellArray = nullptr;
  IArray* m_DestCellArray = nullptr;
  usize m_TupleOffset;
};

/**
 * @brief This class will copy over all of the data from the first input array of any IArray type, then the second input array of the same IArray type to the given destination array (of the same
 * IArray type). This class DOES NOT do any bounds checking and assumes that the destination array has already been properly sized to fit all of the data.
 */
template <typename T>
class CombineArrays
{
public:
  CombineArrays(const IArray& inputCellArray1, const IArray& inputCellArray2, IArray& destCellArray)
  : m_ArrayType(destCellArray.getArrayType())
  , m_InputCellArray1(&inputCellArray1)
  , m_InputCellArray2(&inputCellArray2)
  , m_DestCellArray(&destCellArray)
  {
  }

  ~CombineArrays() = default;

  CombineArrays(const CombineArrays&) = default;
  CombineArrays(CombineArrays&&) noexcept = default;
  CombineArrays& operator=(const CombineArrays&) = delete;
  CombineArrays& operator=(CombineArrays&&) noexcept = delete;

  void operator()() const
  {
    if(m_ArrayType == IArray::ArrayType::NeighborListArray)
    {
      using NeighborListT = NeighborList<T>;
      auto* destArray = dynamic_cast<NeighborListT*>(m_DestCellArray);
      // Make sure the destination array is allocated AND each tuple list is initialized so we can use the [] operator to copy over the data
      if(destArray->getValues().empty() || destArray->getList(0) == nullptr)
      {
        destArray->addEntry(destArray->getNumberOfTuples() - 1, 0);
      }
      AppendData<NeighborListT>(*dynamic_cast<const NeighborListT*>(m_InputCellArray1), *destArray, 0);
      AppendData<NeighborListT>(*dynamic_cast<const NeighborListT*>(m_InputCellArray2), *destArray, m_InputCellArray1->getNumberOfTuples());
    }
    if(m_ArrayType == IArray::ArrayType::DataArray)
    {
      using DataArrayT = DataArray<T>;
      AppendData<DataArrayT>(*dynamic_cast<const DataArrayT*>(m_InputCellArray1), *dynamic_cast<DataArrayT*>(m_DestCellArray), 0);
      AppendData<DataArrayT>(*dynamic_cast<const DataArrayT*>(m_InputCellArray2), *dynamic_cast<DataArrayT*>(m_DestCellArray), m_InputCellArray1->getSize());
    }
    if(m_ArrayType == IArray::ArrayType::StringArray)
    {
      AppendData<StringArray>(*dynamic_cast<const StringArray*>(m_InputCellArray1), *dynamic_cast<StringArray*>(m_DestCellArray), 0);
      AppendData<StringArray>(*dynamic_cast<const StringArray*>(m_InputCellArray2), *dynamic_cast<StringArray*>(m_DestCellArray), m_InputCellArray1->getSize());
    }
  }

private:
  IArray::ArrayType m_ArrayType = IArray::ArrayType::Any;
  const IArray* m_InputCellArray1 = nullptr;
  const IArray* m_InputCellArray2 = nullptr;
  IArray* m_DestCellArray = nullptr;
};

/**
 * @brief This class will copy all of the data from the input array of any IArray type to the given destination array of the same IArray using the newToOldIndices list. This class DOES NOT
 * do any bounds checking and assumes that the destination array has already been properly resized to fit all of the data
 *
 * WARNING: This method can be very memory intensive for larger geometries. Use this method with caution!
 */
template <typename T>
class CopyUsingIndexList
{
public:
  CopyUsingIndexList(IArray& destCellArray, const IArray& inputCellArray, const nonstd::span<const int64>& newToOldIndices)
  : m_ArrayType(destCellArray.getArrayType())
  , m_InputCellArray(&inputCellArray)
  , m_DestCellArray(&destCellArray)
  , m_NewToOldIndices(newToOldIndices)
  {
  }

  ~CopyUsingIndexList() = default;

  CopyUsingIndexList(const CopyUsingIndexList&) = default;
  CopyUsingIndexList(CopyUsingIndexList&&) noexcept = default;
  CopyUsingIndexList& operator=(const CopyUsingIndexList&) = delete;
  CopyUsingIndexList& operator=(CopyUsingIndexList&&) noexcept = delete;

  void operator()() const
  {
    for(usize i = 0; i < m_NewToOldIndices.size(); i++)
    {
      int64 oldIndexI = m_NewToOldIndices[i];
      bool copySucceeded = true;
      if(m_ArrayType == IArray::ArrayType::NeighborListArray)
      {
        using NeighborListT = NeighborList<T>;
        auto* destArray = dynamic_cast<NeighborListT*>(m_DestCellArray);
        // Make sure the destination array is allocated AND each tuple list is initialized so we can use the [] operator to copy over the data
        destArray->setList(i, typename NeighborListT::SharedVectorType(new typename NeighborListT::VectorType));
        if(oldIndexI >= 0)
        {
          copySucceeded = CopyData<NeighborListT>(*dynamic_cast<const NeighborListT*>(m_InputCellArray), *destArray, i, oldIndexI, 1);
        }
      }
      else if(m_ArrayType == IArray::ArrayType::DataArray)
      {
        using DataArrayT = DataArray<T>;
        auto* destArray = dynamic_cast<DataArrayT*>(m_DestCellArray);
        if(oldIndexI >= 0)
        {
          copySucceeded = CopyData<DataArrayT>(*dynamic_cast<const DataArrayT*>(m_InputCellArray), *destArray, i, oldIndexI, 1);
        }
        else
        {
          destArray->initializeTuple(i, 0);
        }
      }
      else if(m_ArrayType == IArray::ArrayType::StringArray)
      {
        auto destArray = *dynamic_cast<StringArray*>(m_DestCellArray);
        if(oldIndexI >= 0)
        {
          copySucceeded = CopyData<StringArray>(*dynamic_cast<const StringArray*>(m_InputCellArray), destArray, i, oldIndexI, 1);
        }
        else
        {
          destArray[i] = "";
        }
      }

      if(!copySucceeded)
      {
        std::cout << fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", m_InputCellArray->getName(), oldIndexI, i)
                  << std::endl;
        break;
      }
    }
  }

private:
  IArray::ArrayType m_ArrayType = IArray::ArrayType::Any;
  const IArray* m_InputCellArray = nullptr;
  IArray* m_DestCellArray = nullptr;
  nonstd::span<const int64> m_NewToOldIndices;
};

/**
 * @brief This class will copy all of the data from the RectGrid geometry input array of any IArray type to the given Image geometry destination array of the same IArray type by calculating the mapped
 * RectGrid geometry index from the Image geometry dimensions/spacing. This class DOES NOT do any bounds checking and assumes that the destination array has already been properly resized to fit all of
 * the data
 */
template <typename T>
class MapRectGridDataToImageData
{
public:
  MapRectGridDataToImageData(IArray& destCellArray, const IArray& inputCellArray, const FloatVec3& origin, const SizeVec3& imageGeoDims, const std::vector<float32>& imageGeoSpacing,
                             const SizeVec3& rectGridDims, const Float32Array* xGridValues, const Float32Array* yGridValues, const Float32Array* zGridValues)
  : m_ArrayType(destCellArray.getArrayType())
  , m_InputCellArray(&inputCellArray)
  , m_DestCellArray(&destCellArray)
  , m_Origin(origin)
  , m_ImageGeomDims(imageGeoDims)
  , m_ImageGeomSpacing(imageGeoSpacing)
  , m_RectGridDims(rectGridDims)
  , m_XGridValues(xGridValues)
  , m_YGridValues(yGridValues)
  , m_ZGridValues(zGridValues)
  , m_HalfSpacing(FloatVec3{imageGeoSpacing[0] * 0.5f, imageGeoSpacing[1] * 0.5f, imageGeoSpacing[2] * 0.5f})
  {
  }

  ~MapRectGridDataToImageData() = default;

  MapRectGridDataToImageData(const MapRectGridDataToImageData&) = default;
  MapRectGridDataToImageData(MapRectGridDataToImageData&&) noexcept = default;
  MapRectGridDataToImageData& operator=(const MapRectGridDataToImageData&) = delete;
  MapRectGridDataToImageData& operator=(MapRectGridDataToImageData&&) noexcept = delete;

  void operator()() const
  {
    usize imageIndex = 0;
    usize rgZIdxStart = 1;
    for(usize z = 0; z < m_ImageGeomDims[2]; z++)
    {
      float32 zCoord = m_Origin[2] + (z * m_ImageGeomSpacing[2]) + m_HalfSpacing[2];
      usize zIndex = 0;
      for(usize rgZIdx = rgZIdxStart; rgZIdx < m_ZGridValues->size(); rgZIdx++)
      {
        if(zCoord > m_ZGridValues->at(rgZIdx - 1) && zCoord <= m_ZGridValues->at(rgZIdx))
        {
          zIndex = rgZIdx - 1;
          rgZIdxStart = rgZIdx;
          break;
        }
      }

      usize rgYIdxStart = 1;
      for(usize y = 0; y < m_ImageGeomDims[1]; y++)
      {
        float32 yCoord = m_Origin[1] + (y * m_ImageGeomSpacing[1]) + m_HalfSpacing[1];
        usize yIndex = 0;
        for(usize rgYIdx = rgYIdxStart; rgYIdx < m_YGridValues->size(); rgYIdx++)
        {
          if(yCoord > m_YGridValues->at(rgYIdx - 1) && yCoord <= m_YGridValues->at(rgYIdx))
          {
            yIndex = rgYIdx - 1;
            rgYIdxStart = rgYIdx;
            break;
          }
        }

        usize rgXIdxStart = 1;
        for(usize x = 0; x < m_ImageGeomDims[0]; x++)
        {
          float32 xCoord = m_Origin[0] + (x * m_ImageGeomSpacing[0]) + m_HalfSpacing[0];
          usize xIndex = 0;
          for(usize rgXIdx = rgXIdxStart; rgXIdx < m_XGridValues->size(); rgXIdx++)
          {
            if(xCoord > m_XGridValues->at(rgXIdx - 1) && xCoord <= m_XGridValues->at(rgXIdx))
            {
              xIndex = rgXIdx - 1;
              rgXIdxStart = rgXIdx;
              break;
            }
          }

          // Compute the index into the RectGrid Data Array
          const int64 rectGridIndex = (m_RectGridDims[0] * m_RectGridDims[1] * zIndex) + (m_RectGridDims[0] * yIndex) + xIndex;

          // Use the computed index to copy the data from the RectGrid to the Image Geometry
          bool copySucceeded = true;
          if(m_ArrayType == IArray::ArrayType::NeighborListArray)
          {
            using NeighborListT = NeighborList<T>;
            auto* destArray = dynamic_cast<NeighborListT*>(m_DestCellArray);
            // Make sure the destination array is allocated AND each tuple list is initialized so we can use the [] operator to copy over the data
            destArray->setList(imageIndex, typename NeighborListT::SharedVectorType(new typename NeighborListT::VectorType));
            if(rectGridIndex >= 0)
            {
              copySucceeded = CopyData<NeighborListT>(*dynamic_cast<const NeighborListT*>(m_InputCellArray), *destArray, imageIndex, rectGridIndex, 1);
            }
          }
          else if(m_ArrayType == IArray::ArrayType::DataArray)
          {
            using DataArrayT = DataArray<T>;
            auto* destArray = dynamic_cast<DataArrayT*>(m_DestCellArray);
            if(rectGridIndex >= 0)
            {
              copySucceeded = CopyData<DataArrayT>(*dynamic_cast<const DataArrayT*>(m_InputCellArray), *destArray, imageIndex, rectGridIndex, 1);
            }
            else
            {
              destArray->initializeTuple(imageIndex, 0);
            }
          }
          else if(m_ArrayType == IArray::ArrayType::StringArray)
          {
            auto destArray = *dynamic_cast<StringArray*>(m_DestCellArray);
            if(rectGridIndex >= 0)
            {
              copySucceeded = CopyData<StringArray>(*dynamic_cast<const StringArray*>(m_InputCellArray), destArray, imageIndex, rectGridIndex, 1);
            }
            else
            {
              destArray[imageIndex] = "";
            }
          }
          if(!copySucceeded)
          {
            std::cout << fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", m_InputCellArray->getName(), rectGridIndex,
                                     imageIndex)
                      << std::endl;
            break;
          }

          ++imageIndex;
        }
      }
    }
  }

private:
  IArray::ArrayType m_ArrayType = IArray::ArrayType::Any;
  const IArray* m_InputCellArray = nullptr;
  IArray* m_DestCellArray = nullptr;
  const FloatVec3 m_Origin;
  const SizeVec3 m_ImageGeomDims;
  const std::vector<float32> m_ImageGeomSpacing;
  const SizeVec3 m_RectGridDims;
  const Float32Array* m_XGridValues = nullptr;
  const Float32Array* m_YGridValues = nullptr;
  const Float32Array* m_ZGridValues = nullptr;
  const FloatVec3 m_HalfSpacing;
};

/**
 * @brief This function will make use of the AppendData class with the bool data type only to append data from the input IArray to the destination IArray at the given tupleOffset. This function DOES
 * NOT do any bounds checking!
 */
inline void RunAppendBoolAppend(IArray& destCellArray, const IArray& inputCellArray, usize tupleOffset)
{
  using DataArrayT = DataArray<bool>;
  const usize offset = tupleOffset * destCellArray.getNumberOfComponents();
  AppendData<DataArrayT>(*dynamic_cast<const DataArrayT*>(&inputCellArray), *dynamic_cast<DataArrayT*>(&destCellArray), offset);
}

/**
 * @brief This function will make use of the AppendData class with the bool data type only to combine data from the input IArrays to the destination IArray. This function DOES
 * NOT do any bounds checking!
 */
inline void RunCombineBoolAppend(const IArray& inputCellArray1, const IArray& inputCellArray2, IArray& destCellArray)
{
  using DataArrayT = DataArray<bool>;
  AppendData<DataArrayT>(*dynamic_cast<const DataArrayT*>(&inputCellArray1), *dynamic_cast<DataArrayT*>(&destCellArray), 0);
  AppendData<DataArrayT>(*dynamic_cast<const DataArrayT*>(&inputCellArray2), *dynamic_cast<DataArrayT*>(&destCellArray), inputCellArray1.getSize());
}

/**
 * @brief This function will make use of the CopyUsingIndexList class with the bool data type only to copy data from the input IArray to the destination IArray using the given index list. This
 * function DOES NOT do any bounds checking!
 */
inline void RunBoolCopyUsingIndexList(IArray& destCellArray, const IArray& inputCellArray, const nonstd::span<const int64>& newToOldIndices)
{
  using DataArrayT = DataArray<bool>;
  CopyUsingIndexList<DataArrayT>(*dynamic_cast<DataArrayT*>(&destCellArray), *dynamic_cast<const DataArrayT*>(&inputCellArray), newToOldIndices);
}

/**
 * @brief This function will make use of the MapRectGridDataToImageData class with the bool data type only to copy data from the input IArray to the destination IArray using the given index list. This
 * function DOES NOT do any bounds checking!
 */
inline void RunBoolMapRectToImage(IArray& destCellArray, const IArray& inputCellArray, const FloatVec3& origin, const SizeVec3& imageGeoDims, const std::vector<float32>& imageGeoSpacing,
                                  const SizeVec3& rectGridDims, const Float32Array* xGridValues, const Float32Array* yGridValues, const Float32Array* zGridValues)
{
  using DataArrayT = DataArray<bool>;
  MapRectGridDataToImageData<DataArrayT>(*dynamic_cast<DataArrayT*>(&destCellArray), *dynamic_cast<const DataArrayT*>(&inputCellArray), origin, imageGeoDims, imageGeoSpacing, rectGridDims,
                                         xGridValues, yGridValues, zGridValues);
}

template <class ParallelRunnerT, class... ArgsT>
void RunParallelAppend(IArray& destArray, ParallelRunnerT&& runner, ArgsT&&... args)
{
  const IArray::ArrayType arrayType = destArray.getArrayType();
  DataType dataType = DataType::int32;
  if(arrayType == IArray::ArrayType::NeighborListArray)
  {
    dataType = dynamic_cast<INeighborList*>(&destArray)->getDataType();
  }
  if(arrayType == IArray::ArrayType::DataArray)
  {
    dataType = dynamic_cast<IDataArray*>(&destArray)->getDataType();
    if(dataType == DataType::boolean)
    {
      RunAppendBoolAppend(destArray, std::forward<ArgsT>(args)...);
    }
  }

  ExecuteParallelFunction<AppendArray, NoBooleanType>(dataType, std::forward<ParallelRunnerT>(runner), destArray, std::forward<ArgsT>(args)...);
}

template <class ParallelRunnerT, class... ArgsT>
void RunParallelCombine(IArray& destArray, ParallelRunnerT&& runner, ArgsT&&... args)
{
  const IArray::ArrayType arrayType = destArray.getArrayType();
  DataType dataType = DataType::int32;
  if(arrayType == IArray::ArrayType::NeighborListArray)
  {
    dataType = dynamic_cast<INeighborList*>(&destArray)->getDataType();
  }
  if(arrayType == IArray::ArrayType::DataArray)
  {
    dataType = dynamic_cast<IDataArray*>(&destArray)->getDataType();
    if(dataType == DataType::boolean)
    {
      RunCombineBoolAppend(destArray, std::forward<ArgsT>(args)...);
    }
  }

  ExecuteParallelFunction<CombineArrays, NoBooleanType>(dataType, std::forward<ParallelRunnerT>(runner), destArray, std::forward<ArgsT>(args)...);
}

/**
 * WARNING: This method can be very memory intensive for larger geometries. Use this method with caution!
 */
template <class ParallelRunnerT, class... ArgsT>
void RunParallelCopyUsingIndexList(IArray& destArray, ParallelRunnerT&& runner, ArgsT&&... args)
{
  const IArray::ArrayType arrayType = destArray.getArrayType();
  DataType dataType = DataType::int32;
  if(arrayType == IArray::ArrayType::NeighborListArray)
  {
    dataType = dynamic_cast<INeighborList*>(&destArray)->getDataType();
  }
  if(arrayType == IArray::ArrayType::DataArray)
  {
    dataType = dynamic_cast<IDataArray*>(&destArray)->getDataType();
    if(dataType == DataType::boolean)
    {
      RunBoolCopyUsingIndexList(destArray, std::forward<ArgsT>(args)...);
    }
  }

  ExecuteParallelFunction<CopyUsingIndexList, NoBooleanType>(dataType, std::forward<ParallelRunnerT>(runner), destArray, std::forward<ArgsT>(args)...);
}

template <class ParallelRunnerT, class... ArgsT>
void RunParallelMapRectToImage(IArray& destArray, ParallelRunnerT&& runner, ArgsT&&... args)
{
  const IArray::ArrayType arrayType = destArray.getArrayType();
  DataType dataType = DataType::int32;
  if(arrayType == IArray::ArrayType::NeighborListArray)
  {
    dataType = dynamic_cast<INeighborList*>(&destArray)->getDataType();
  }
  if(arrayType == IArray::ArrayType::DataArray)
  {
    dataType = dynamic_cast<IDataArray*>(&destArray)->getDataType();
    if(dataType == DataType::boolean)
    {
      RunBoolMapRectToImage(destArray, std::forward<ArgsT>(args)...);
    }
  }

  ExecuteParallelFunction<MapRectGridDataToImageData, NoBooleanType>(dataType, std::forward<ParallelRunnerT>(runner), destArray, std::forward<ArgsT>(args)...);
}

} // namespace CopyFromArray

} // namespace nx::core
