#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/EmptyDataStore.hpp"
#include "complex/DataStructure/IDataStore.hpp"
#include "complex/DataStructure/NeighborList.hpp"
#include "complex/Filter/Output.hpp"
#include "complex/Utilities/TemplateHelpers.hpp"
#include "complex/complex_export.hpp"
#include "complex/DataStructure/AttributeMatrix.hpp"

#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

#define COMPLEX_DEF_STRING_CONVERTOR_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                               \
  CONTAINER_TYPE value;                                                                                                                                                                                \
  try                                                                                                                                                                                                  \
  {                                                                                                                                                                                                    \
    value = FUNCTION(input);                                                                                                                                                                           \
  } catch(const std::invalid_argument& e)                                                                                                                                                              \
  {                                                                                                                                                                                                    \
    return complex::MakeErrorResult<TYPE>(-100, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                                \
  } catch(const std::out_of_range& e)                                                                                                                                                                  \
  {                                                                                                                                                                                                    \
    return complex::MakeErrorResult<TYPE>(-101, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                       \
  }                                                                                                                                                                                                    \
                                                                                                                                                                                                       \
  if(value > std::numeric_limits<TYPE>::max() || value < std::numeric_limits<TYPE>::min())                                                                                                             \
  {                                                                                                                                                                                                    \
    return complex::MakeErrorResult<TYPE>(-101, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                       \
  }                                                                                                                                                                                                    \
                                                                                                                                                                                                       \
  return {static_cast<TYPE>(value)};

#define COMPLEX_DEF_STRING_CONVERTOR_SIGNED_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                        \
  template <>                                                                                                                                                                                          \
  struct ConvertTo<TYPE>                                                                                                                                                                               \
  {                                                                                                                                                                                                    \
    static Result<TYPE> convert(const std::string& input)                                                                                                                                              \
    {                                                                                                                                                                                                  \
      COMPLEX_DEF_STRING_CONVERTOR_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                                 \
    }                                                                                                                                                                                                  \
  };

#define COMPLEX_DEF_STRING_CONVERTOR_UNSIGNED_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                      \
  template <>                                                                                                                                                                                          \
  struct ConvertTo<TYPE>                                                                                                                                                                               \
  {                                                                                                                                                                                                    \
    static Result<TYPE> convert(const std::string& input)                                                                                                                                              \
    {                                                                                                                                                                                                  \
      if(!input.empty() && input.at(0) == '-')                                                                                                                                                         \
      {                                                                                                                                                                                                \
        return complex::MakeErrorResult<TYPE>(-101, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                   \
      }                                                                                                                                                                                                \
                                                                                                                                                                                                       \
      COMPLEX_DEF_STRING_CONVERTOR_INT(CONTAINER_TYPE, TYPE, FUNCTION)                                                                                                                                 \
    }                                                                                                                                                                                                  \
  };

#define COMPLEX_DEF_STRING_CONVERTOR_FLOATING_POINT(TYPE, FUNCTION)                                                                                                                                    \
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
        return complex::MakeErrorResult<TYPE>(-100, fmt::format("Error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                            \
      } catch(const std::out_of_range& e)                                                                                                                                                              \
      {                                                                                                                                                                                                \
        return complex::MakeErrorResult<TYPE>(-101, fmt::format("Overflow error trying to convert '{}' to type '{}' using function '{}'", input, #TYPE, #FUNCTION));                                   \
      }                                                                                                                                                                                                \
      return {value};                                                                                                                                                                                  \
    }                                                                                                                                                                                                  \
  };

namespace complex
{
template <class T>
struct ConvertTo
{
};

/**
 * These macros will create convertor objects that convert from a string to a numeric type
 */

COMPLEX_DEF_STRING_CONVERTOR_UNSIGNED_INT(uint64, uint8, std::stoull)
COMPLEX_DEF_STRING_CONVERTOR_SIGNED_INT(int64, int8, std::stoll)
COMPLEX_DEF_STRING_CONVERTOR_UNSIGNED_INT(uint64, uint16, std::stoull)
COMPLEX_DEF_STRING_CONVERTOR_SIGNED_INT(int64, int16, std::stoll)
COMPLEX_DEF_STRING_CONVERTOR_UNSIGNED_INT(uint64, uint32, std::stoull)
COMPLEX_DEF_STRING_CONVERTOR_SIGNED_INT(int64, int32, std::stoll)
COMPLEX_DEF_STRING_CONVERTOR_UNSIGNED_INT(uint64, uint64, std::stoull)
COMPLEX_DEF_STRING_CONVERTOR_SIGNED_INT(int64, int64, std::stoll)
#ifdef __APPLE__
COMPLEX_DEF_STRING_CONVERTOR_UNSIGNED_INT(usize, usize, std::stoull)
#endif
COMPLEX_DEF_STRING_CONVERTOR_FLOATING_POINT(float32, std::stof)
COMPLEX_DEF_STRING_CONVERTOR_FLOATING_POINT(float64, std::stod)

template <>
struct ConvertTo<bool>
{
  static Result<bool> convert(const std::string& input)
  {
    if(input == "TRUE" || input == "true" || input == "1" || input == "True")
    {
      return {true};
    }
    return {false};
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

// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
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
COMPLEX_EXPORT Result<> CheckValueConvertsToArrayType(const std::string& value, const DataObject& inputDataArray);

/**
 * @brief Replaces every value in an array based on a `mask` array.
 * @tparam T The primitive type used in the data array
 * @param inputArrayPtr InputArray that will have values replaced
 * @param condDataPtr The mask array as a boolean array
 * @param replaceValue The value that will be used for every place the conditional array is TRUE
 */
template <class T, typename ConditionalType>
void ReplaceValue(DataArray<T>& inputArrayPtr, const DataArray<ConditionalType>* condArrayPtr, T replaceValue)
{
  T replaceVal = static_cast<T>(replaceValue);
  usize numTuples = inputArrayPtr.getNumberOfTuples();

  const DataArray<ConditionalType>& conditionalArray = *condArrayPtr;
  for(usize tupleIndex = 0; tupleIndex < numTuples; tupleIndex++)
  {
    if(conditionalArray[tupleIndex])
    {
      inputArrayPtr.initializeTuple(tupleIndex, replaceValue);
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
template <class T>
Result<> ConditionalReplaceValueInArrayFromString(const std::string& valueAsStr, DataObject& inputDataObject, const IDataArray& conditionalDataArray)
{
  using DataArrayType = DataArray<T>;

  auto& inputDataArray = dynamic_cast<DataArrayType&>(inputDataObject);
  Result<T> conversionResult = ConvertTo<T>::convert(valueAsStr);
  if(conversionResult.invalid())
  {
    return MakeErrorResult<>(-4000, "Input String Value could not be converted to the appropriate numeric type.");
  }

  complex::DataType arrayType = conditionalDataArray.getDataType();

  if(complex::DataType::uint8 == arrayType)
  {
    ReplaceValue<T, uint8_t>(inputDataArray, dynamic_cast<const UInt8Array*>(&conditionalDataArray), conversionResult.value());
  }
  else if(complex::DataType::int8 == arrayType)
  {
    ReplaceValue<T, int8_t>(inputDataArray, dynamic_cast<const Int8Array*>(&conditionalDataArray), conversionResult.value());
  }
  else if(complex::DataType::boolean == arrayType)
  {
    ReplaceValue<T, bool>(inputDataArray, dynamic_cast<const BoolArray*>(&conditionalDataArray), conversionResult.value());
  }
  else
  {
    return MakeErrorResult<>(-4001, "Mask array was not of type [BOOL | UINT8 | INT8].");
  }
  return {};
}

/**
 * @brief Replaces a value in an array based on a boolean mask.
 * @param valueAsStr The value that will be used for the replacement
 * @param inputDataObject Input DataArray that will have values replaced (possibly)
 * @param conditionalDataArray The mask array as a boolean array
 * @return
 */
COMPLEX_EXPORT Result<> ConditionalReplaceValueInArray(const std::string& valueAsStr, DataObject& inputDataObject, const IDataArray& conditionalDataArray);

/**
 * @brief Creates a DataStore with the given properties
 * @tparam T Primitive Type (int, float, ...)
 * @param tupleShape The Tuple Dimensions
 * @param componentShape The component dimensions
 * @param mode The mode to assume: PREFLIGHT or EXECUTE. Preflight will NOT allocate any storage. EXECUTE will allocate the memory/storage
 * @return
 */
template <class T>
std::unique_ptr<AbstractDataStore<T>> CreateDataStore(const typename IDataStore::ShapeType& tupleShape, const typename IDataStore::ShapeType& componentShape, IDataAction::Mode mode)
{
  switch(mode)
  {
  case IDataAction::Mode::Preflight: {
    return std::make_unique<EmptyDataStore<T>>(tupleShape, componentShape);
  }
  case IDataAction::Mode::Execute: {
    return std::make_unique<DataStore<T>>(tupleShape, componentShape, static_cast<T>(0));
  }
  default: {
    throw std::runtime_error("Invalid mode");
  }
  }
}

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
Result<> CreateArray(DataStructure& dataStructure, const std::vector<usize>& tupleShape, const std::vector<usize>& compShape, const DataPath& path, IDataAction::Mode mode)
{
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> dataObjectId;

  DataObject* parentObject = nullptr;
  if(parentPath.getLength() != 0)
  {
    parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return MakeErrorResult(-262, fmt::format("Parent object '{}' does not exist", parentPath.toString()));
    }

    dataObjectId = parentObject->getId();
  }

  if(tupleShape.empty())
  {
    return MakeErrorResult(-261, fmt::format("CreateArrayAction: Tuple Shape was empty. Please set the number of tuples."));
  }
  //  size_t numTuples = std::accumulate(tupleShape.cbegin(), tupleShape.cend(), static_cast<size_t>(1), std::multiplies<>());
  //  if(numTuples == 0 && mode == IDataAction::Mode::Execute)
  //  {
  //    return MakeErrorResult(-263, fmt::format("CreateArrayAction: Number of tuples is ZERO. Please set the number of tuples."));
  //  }

  // Validate Number of Components
  if(compShape.empty())
  {
    return MakeErrorResult(-261, fmt::format("CreateArrayAction: Component Shape was empty. Please set the number of components."));
  }
  size_t numComponents = std::accumulate(compShape.cbegin(), compShape.cend(), static_cast<size_t>(1), std::multiplies<>());
  if(numComponents == 0 && mode == IDataAction::Mode::Execute)
  {
    return MakeErrorResult(-263, fmt::format("CreateArrayAction: Number of components is ZERO. Please set the number of components."));
  }

  usize last = path.getLength() - 1;

  std::string name = path[last];

  auto store = CreateDataStore<T>(tupleShape, compShape, mode);
  auto dataArray = DataArray<T>::Create(dataStructure, name, std::move(store), dataObjectId);
  if(dataArray == nullptr)
  {
    if(parentObject->getDataObjectType() == DataObject::Type::AttributeMatrix)
    {
      auto* attrMatrix = dynamic_cast<AttributeMatrix*>(parentObject);
      std::string amShape = fmt::format("Attribute Matrix Tuple Dims: {}", fmt::join(attrMatrix->getShape(), " x "));
      std::string arrayShape = fmt::format("Data Array Tuple Shape: {}", fmt::join(tupleShape, " x "));
      return MakeErrorResult(-264, fmt::format("Unable to create Data Array '{}' inside Attribute matrix '{}'. Mismatch of tuple dimensions. The created Data Array must have the same tuple dimensions or the same total number of tuples.\n{}\n{}", name, dataStructure.getDataPathsForId(parentObject->getId()).front().toString(), amShape, arrayShape));
    }
    else
    {
      return MakeErrorResult(-264, fmt::format("Unable to create DataArray at '{}'", path.toString()));
    }
  }

  return {};
}

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
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> dataObjectId;

  if(parentPath.getLength() != 0)
  {
    auto* parentObject = dataStructure.getData(parentPath);
    if(parentObject == nullptr)
    {
      return MakeErrorResult(-262, fmt::format("Parent object \"{}\" does not exist", parentPath.toString()));
    }

    dataObjectId = parentObject->getId();
  }

  usize last = path.getLength() - 1;

  std::string name = path[last];

  auto neighborList = NeighborList<T>::Create(dataStructure, name, numTuples, dataObjectId);
  if(neighborList == nullptr)
  {
    return MakeErrorResult(-264, fmt::format("Unable to create NeighborList at \"{}\"", path.toString()));
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
 * @brief
 * @tparam T
 * @param filename
 * @param name
 * @param dataGraph
 * @param tupleShape
 * @param componentShape
 * @param parentId
 * @return
 */
template <typename T>
DataArray<T>* ImportFromBinaryFile(const std::string& filename, const std::string& name, DataStructure& dataGraph, const std::vector<size_t>& tupleShape, const std::vector<size_t>& componentShape,
                                   DataObject::IdType parentId = {})
{
  // std::cout << "  Reading file " << filename << std::endl;
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;
  constexpr size_t defaultBlocksize = 1048576;

  if(!fs::exists(filename))
  {
    std::cout << "File Does Not Exist:'" << filename << "'" << std::endl;
    return nullptr;
  }

  std::shared_ptr<DataStoreType> dataStore = std::shared_ptr<DataStoreType>(new DataStoreType({tupleShape}, componentShape, static_cast<T>(0)));
  ArrayType* dataArray = ArrayType::Create(dataGraph, name, dataStore, parentId);

  const size_t fileSize = fs::file_size(filename);
  const size_t numBytesToRead = dataArray->getSize() * sizeof(T);
  if(numBytesToRead != fileSize)
  {
    std::cout << "FileSize '" << fileSize << "' and Allocated Size '" << numBytesToRead << "' do not match" << std::endl;
    return nullptr;
  }

  FILE* inputFile = std::fopen(filename.c_str(), "rb");
  if(inputFile == nullptr)
  {
    return nullptr;
  }

  auto* chunkptr = reinterpret_cast<std::byte*>(dataStore->data());

  // Now start reading the data in chunks if needed.
  size_t chunkSize = std::min(numBytesToRead, defaultBlocksize);

  size_t masterCounter = 0;
  while(masterCounter < numBytesToRead)
  {
    size_t bytesRead = std::fread(chunkptr, sizeof(std::byte), chunkSize, inputFile);
    chunkptr += bytesRead;
    masterCounter += bytesRead;

    size_t bytesLeft = numBytesToRead - masterCounter;

    if(bytesLeft < chunkSize)
    {
      chunkSize = bytesLeft;
    }
  }

  fclose(inputFile);

  return dataArray;
}

/**
 * @brief This function will Resize and DataArray and then replace and exisint DataArray in the DataStructure
 * @param dataStructure
 * @param dataPath The path of the target DataArray
 * @param tupleShape The tuple shape of the resized array
 * @param mode The mode: Preflight or Execute
 * @return
 */
COMPLEX_EXPORT Result<> ResizeAndReplaceDataArray(DataStructure& dataStructure, const DataPath& dataPath, std::vector<usize>& tupleShape, IDataAction::Mode mode);

/**
 * @brief This function will ensure that a user entered numeric value can correctly be parsed into the selected NumericType
 *
 * @param value The string value that is to be parsed
 * @param numericType The NumericType to parse the value into.
 * @return
 */
COMPLEX_EXPORT Result<> CheckValueConverts(const std::string& value, NumericType numericType);

/**
 * @brief This function will ensure that a user entered numeric value can correctly be parsed into the selected DataArray
 *
 * @param value The string value that is to be parsed
 * @param inputDataArray The DataArray that the value would be inserted into.
 * @return
 */
COMPLEX_EXPORT Result<> CheckValueConvertsToArrayType(const std::string& value, const DataObject& inputDataArray);

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
  virtual bool bothTrue(size_t indexA, size_t indexB) const = 0;

  /**
   * @brief Both of the values pointed to by the index *must* be `false` or non-zero. If either of the values or
   * *both* of the values are `true`, this will return `false`.
   * @param indexA
   * @param indexB
   * @return
   */
  virtual bool bothFalse(size_t indexA, size_t indexB) const = 0;

  /**
   * @brief Returns `true` or `false` based on the value at the index
   * @param index index to check
   * @return
   */
  virtual bool isTrue(size_t index) const = 0;

  virtual void setValue(size_t index, bool val) = 0;
};

struct BoolMaskCompare : public MaskCompare
{
  BoolMaskCompare(BoolArray& array)
  : m_Array(array)
  {
  }
  ~BoolMaskCompare() noexcept override = default;
  BoolArray& m_Array;
  bool bothTrue(size_t indexA, size_t indexB) const override
  {
    return m_Array.at(indexA) && m_Array.at(indexB);
  }
  bool bothFalse(size_t indexA, size_t indexB) const override
  {
    return !m_Array.at(indexA) && !m_Array.at(indexB);
  }
  bool isTrue(size_t index) const override
  {
    return m_Array.at(index);
  }
  void setValue(size_t index, bool val) override
  {
    m_Array[index] = val;
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
  bool bothTrue(size_t indexA, size_t indexB) const override
  {
    return m_Array.at(indexA) != 0 && m_Array.at(indexB) != 0;
  }
  bool bothFalse(size_t indexA, size_t indexB) const override
  {
    return m_Array.at(indexA) == 0 && m_Array.at(indexB) == 0;
  }
  bool isTrue(size_t index) const override
  {
    return m_Array.at(index) != 0;
  }
  void setValue(size_t index, bool val) override
  {
    m_Array[index] = static_cast<uint8>(val);
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
COMPLEX_EXPORT std::unique_ptr<MaskCompare> InstantiateMaskCompare(DataStructure& dataStructure, const DataPath& maskArrayPath);

/**
 * @brief Convenience method to create an instance of the MaskCompare subclass
 * @param maskArrayPtr A Pointer to the mask array which can be of either `bool` or `uint8` type.
 * @return
 */
COMPLEX_EXPORT std::unique_ptr<MaskCompare> InstantiateMaskCompare(IDataArray& maskArrayPtr);

} // namespace complex
