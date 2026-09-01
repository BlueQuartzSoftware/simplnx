#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Bit.hpp"
#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/AbstractListStore.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"
#include "simplnx/Utilities/ParallelAlgorithmUtilities.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#if defined(_MSC_VER)
/**
 * @def FSEEK64
 * @brief Selects the 64-bit file-seek function.
 */
#define FSEEK64 _fseeki64
#else
/**
 * @def FSEEK64
 * @brief Selects the 64-bit file-seek function.
 */
#define FSEEK64 std::fseek
#endif

/**
 * @namespace nx::core
 * @brief Contains simplnx core types and functions.
 */
namespace nx::core
{
/**
 * @brief Replaces tuples selected by a mask.
 * @tparam T Input array value type.
 * @tparam ConditionalType Mask array value type.
 * @param inputArrayPtr Array to modify.
 * @param condArrayPtr Mask array.
 * @param replaceValue Value stored in selected tuples.
 * @param invertMask True to select false mask values.
 * @pre condArrayPtr is not null and has one tuple for each input tuple.
 */
template <class T, typename ConditionalType>
void ReplaceValue(DataArray<T>& inputArrayPtr, const DataArray<ConditionalType>* condArrayPtr, T replaceValue, bool invertMask = false)
{
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
 * @struct ConditionalReplaceValueInArrayFromString
 * @brief Replaces selected array values from parsed text.
 */
struct ConditionalReplaceValueInArrayFromString
{
  /**
   * @brief Parses and replaces selected array values.
   * @tparam T Input array value type.
   * @param valueAsStr Text that supplies the replacement value.
   * @param inputDataObject Array to modify.
   * @param conditionalDataArray Mask array.
   * @param invertMask True to select false mask values.
   * @return Error if parsing or mask type validation fails.
   * @throws std::bad_cast If inputDataObject is not DataArray<T>.
   */
  template <class T>
  Result<> operator()(const std::string& valueAsStr, DataObject& inputDataObject, const IDataArray& conditionalDataArray, const bool invertMask = false)
  {
    using DataArrayType = DataArray<T>;

    auto& inputDataArray = dynamic_cast<DataArrayType&>(inputDataObject);
    Result<T> conversionResult = StringInterpretationUtilities::Convert<T>(valueAsStr);
    if(conversionResult.invalid())
    {
      return MakeErrorResult<>(-4000, "Input String Value could not be converted to the appropriate numeric type.");
    }

    const nx::core::DataType arrayType = conditionalDataArray.getDataType();

    if(nx::core::DataType::uint8 == arrayType)
    {
      ReplaceValue<T, uint8>(inputDataArray, dynamic_cast<const UInt8Array*>(&conditionalDataArray), conversionResult.value(), invertMask);
    }
    else if(nx::core::DataType::int8 == arrayType)
    {
      ReplaceValue<T, int8>(inputDataArray, dynamic_cast<const Int8Array*>(&conditionalDataArray), conversionResult.value(), invertMask);
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
 * @brief Replaces selected array values from parsed text.
 * @param valueAsStr Text that supplies the replacement value.
 * @param inputDataObject Array to modify.
 * @param conditionalDataArray Mask array.
 * @param invertmask True to select false mask values.
 * @return Error if parsing or mask type validation fails.
 * @throws std::bad_cast If inputDataObject is not IDataArray.
 */
SIMPLNX_EXPORT Result<> ConditionalReplaceValueInArray(const std::string& valueAsStr, DataObject& inputDataObject, const IDataArray& conditionalDataArray, bool invertmask = false);

/**
 * @brief Converts an array to a selected storage format.
 * @tparam T Array value type.
 * @param dataArray Array whose store is replaced.
 * @param dataFormat Target storage format.
 * @return True if conversion succeeds.
 */
template <typename T>
bool ConvertDataArrayDataStore(const std::shared_ptr<DataArray<T>> dataArray, const std::string& dataFormat)
{
  if(dataArray == nullptr)
  {
    return false;
  }
  const AbstractDataStore<T>& dataStore = dataArray->getDataStoreRef();
  auto convertedDataStore = DataStoreUtilities::ConvertDataStore<T>(dataStore, dataFormat);
  if(convertedDataStore == nullptr)
  {
    return false;
  }

  dataArray->setDataStore(convertedDataStore);
  return true;
}

/**
 * @brief Converts an untyped array to a selected storage format.
 * @param dataArray Array whose store is replaced.
 * @param dataFormat Target storage format.
 * @return True if conversion succeeds.
 * @pre dataArray is not null.
 */
bool ConvertIDataArray(const std::shared_ptr<IDataArray>& dataArray, const std::string& dataFormat);

/**
 * @brief Creates a NeighborList with resolver-selected storage.
 * @tparam T Neighbor-list value type.
 * @param dataStructure Data structure that owns the list.
 * @param tupleShape Tuple dimensions.
 * @param path Destination list path.
 * @param mode Preflight or execute mode.
 * @param dataFormat Explicit storage format, or empty for resolver selection.
 * @return Error if parent lookup or list creation fails.
 * @pre path has at least one segment.
 *
 * The data structure owns the created NeighborList.
 */
template <class T>
Result<> CreateNeighbors(DataStructure& dataStructure, const ShapeType& tupleShape, const DataPath& path, IDataAction::Mode mode, const std::string& dataFormat = "")
{
  static constexpr StringLiteral prefix = "CreateNeighborListAction: ";
  auto parentPath = path.getParent();

  std::optional<DataObject::IdType> dataObjectId;

  if(parentPath.getLength() != 0)
  {
    auto* parentObjectPtr = dataStructure.getData(parentPath);
    if(parentObjectPtr == nullptr)
    {
      return MakeErrorResult(-5801, fmt::format("{}Parent object \"{}\" does not exist", prefix, parentPath.toString()));
    }

    dataObjectId = parentObjectPtr->getId();
  }

  const usize last = path.getLength() - 1;

  std::string name = path[last];
  // The resolver selects the backing storage.
  auto listStore = DataStoreUtilities::CreateListStore<T>(dataStructure, path, tupleShape, mode, dataFormat);
  NeighborList<T>* neighborList = NeighborList<T>::Create(dataStructure, name, listStore, dataObjectId);

  if(neighborList == nullptr)
  {
    return MakeErrorResult(-5802, fmt::format("{}Unable to create NeighborList at \"{}\"", prefix, path.toString()));
  }

  return {};
}

/**
 * @brief Returns a typed array at a data path.
 * @tparam T Array value type.
 * @param dataStructure Data structure to search.
 * @param path Array path.
 * @return Reference to a DataArray<T> owned by dataStructure.
 * @throws std::runtime_error If path does not identify DataArray<T>.
 */
template <class T>
DataArray<T>& ArrayRefFromPath(DataStructure& dataStructure, const DataPath& path)
{
  DataObject* objectPtr = dataStructure.getData(path);
  auto* dataArrayPtr = dynamic_cast<DataArray<T>*>(objectPtr);
  if(dataArrayPtr == nullptr)
  {
    throw std::runtime_error("Can't obtain DataArray");
  }
  return *dataArrayPtr;
}

/**
 * @brief Reads a binary file into a preallocated array in bounded pages.
 *
 * Each page is optionally byte-swapped locally and committed with
 * copyFromBuffer(), so disk-backed destinations do not incur one write per value.
 * @tparam T Binary element type.
 * @param binaryFilePath Input file path.
 * @param outputDataArray Destination array.
 * @param startByte Byte offset in the input file.
 * @param defaultBufferSize Maximum elements in one page.
 * @param swapEndian True to byte-swap each page before storage.
 * @return Error if opening, seeking, reading, or storage fails.
 */
template <typename T>
Result<> ImportFromBinaryFile(const std::filesystem::path& binaryFilePath, DataArray<T>& outputDataArray, usize startByte = 0, usize defaultBufferSize = 1000000, bool swapEndian = false)
{
  FILE* inputFilePtr = std::fopen(binaryFilePath.string().c_str(), "rb");
  if(inputFilePtr == nullptr)
  {
    return MakeErrorResult(-1000, fmt::format("Unable to open the specified file. '{}'", binaryFilePath.string()));
  }

  if(startByte > 0)
  {
    int result = FSEEK64(inputFilePtr, static_cast<int64>(startByte), SEEK_SET);
    if(result != 0)
    {
      std::fclose(inputFilePtr);
      return MakeErrorResult(-1002, fmt::format("Failed to seek to byte offset {} in file '{}'", startByte, binaryFilePath.string()));
    }
  }

  const usize numElements = outputDataArray.getSize();
  if(numElements == 0)
  {
    std::fclose(inputFilePtr);
    return {};
  }
  if(defaultBufferSize == 0)
  {
    std::fclose(inputFilePtr);
    return MakeErrorResult(-1002, "The binary import buffer size must be greater than zero.");
  }
  usize chunkSize = std::min(numElements, defaultBufferSize);
  auto buffer = std::make_unique<T[]>(chunkSize);

  usize elementCounter = 0;
  while(elementCounter < numElements)
  {
    usize elementsRead = std::fread(buffer.get(), sizeof(T), chunkSize, inputFilePtr);

    if(elementsRead == 0)
    {
      std::fclose(inputFilePtr);
      return MakeErrorResult(-1001, fmt::format("Unexpected end of file or read error after reading {} of {} elements from '{}'", elementCounter, numElements, binaryFilePath.string()));
    }

    if(swapEndian)
    {
      std::transform(buffer.get(), buffer.get() + elementsRead, buffer.get(), [](T value) { return nx::core::byteswap(value); });
    }

    Result<> copyResult = outputDataArray.getDataStoreRef().copyFromBuffer(elementCounter, nonstd::span<const T>(buffer.get(), elementsRead));
    if(copyResult.invalid())
    {
      std::fclose(inputFilePtr);
      return copyResult;
    }

    elementCounter += elementsRead;

    usize elementsLeft = numElements - elementCounter;

    if(elementsLeft < chunkSize)
    {
      chunkSize = elementsLeft;
    }
  }

  std::fclose(inputFilePtr);

  return {};
}

/**
 * @brief Imports a binary file into an in-memory array.
 * @tparam T Binary element type.
 * @param filename Input file path.
 * @param name Array name.
 * @param dataStructure Data structure that owns the array.
 * @param tupleShape Tuple dimensions.
 * @param componentShape Component dimensions.
 * @param parentId Parent object identifier.
 * @return Array owned by dataStructure, or nullptr if validation or import fails.
 * @throws std::filesystem::filesystem_error If file status or size cannot be read.
 */
template <typename T>
DataArray<T>* ImportFromBinaryFile(const std::string& filename, const std::string& name, DataStructure& dataStructure, const ShapeType& tupleShape, const ShapeType& componentShape,
                                   DataObject::IdType parentId = {})
{
  // std::cout << "  Reading file " << filename << std::endl;
  using DataStoreType = DataStore<T>;
  using ArrayType = DataArray<T>;

  if(!std::filesystem::exists(filename))
  {
    std::cout << "File Does Not Exist:'" << filename << "'\n";
    return nullptr;
  }

  std::shared_ptr<DataStoreType> dataStore = std::shared_ptr<DataStoreType>(new DataStoreType({tupleShape}, componentShape, static_cast<T>(0)));
  ArrayType* dataArrayPtr = ArrayType::Create(dataStructure, name, dataStore, parentId);

  const usize fileSize = std::filesystem::file_size(filename);
  const usize numBytesToRead = dataArrayPtr->getSize() * sizeof(T);
  if(numBytesToRead != fileSize)
  {
    std::cout << "FileSize '" << fileSize << "' and Allocated Size '" << numBytesToRead << "' do not match\n";
    return nullptr;
  }

  Result<> result = ImportFromBinaryFile(std::filesystem::path(filename), *dataArrayPtr);
  if(result.invalid())
  {
    return nullptr;
  }

  return dataArrayPtr;
}

/**
 * @brief Replaces a destination object with an array deep copy.
 * @tparam ArrayType Supported array type.
 * @param dataStructure Data structure that owns both paths.
 * @param sourceDataPath Source array path.
 * @param destDataPath Destination array path.
 * @return Error if destination removal fails.
 * @throws std::out_of_range If sourceDataPath does not exist.
 * @throws std::bad_cast If sourceDataPath does not identify ArrayType.
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
 * @brief Resizes and replaces an array.
 * @param dataStructure Data structure that owns the array.
 * @param dataPath Target array path.
 * @param tupleShape New tuple dimensions.
 * @param mode Preflight or execute mode.
 * @return Error if replacement fails.
 *
 * The function removes the original before it creates the replacement. A
 * creation failure leaves dataPath absent.
 */
SIMPLNX_EXPORT Result<> ResizeAndReplaceDataArray(DataStructure& dataStructure, const DataPath& dataPath, ShapeType& tupleShape, IDataAction::Mode mode);

/**
 * @brief Checks whether arrays have the same data type.
 * @param dataStructure Data structure that owns the arrays.
 * @param dataArrayPaths Array paths to compare.
 * @return True when all arrays have the same type.
 * @pre dataArrayPaths identify IDataArray objects.
 */
SIMPLNX_EXPORT bool CheckArraysAreSameType(const DataStructure& dataStructure, const std::vector<DataPath>& dataArrayPaths);

/**
 * @brief Checks whether arrays have the same tuple count.
 * @param dataStructure Data structure that owns the arrays.
 * @param dataArrayPaths Array paths to compare.
 * @return True when all arrays have the same tuple count.
 * @pre dataArrayPaths identify IArray objects.
 */
SIMPLNX_EXPORT bool CheckArraysHaveSameTupleCount(const DataStructure& dataStructure, const std::vector<DataPath>& dataArrayPaths);

/**
 * @brief Validates feature identifiers against a source array.
 * @param dataStructure Data structure that owns the source.
 * @param sourceDataPath AttributeMatrix or array indexed by featureIds.
 * @param featureIds Feature identifier array.
 * @param ignoreNegativeValues True to ignore negative identifiers.
 * @param messageHandler Receives validation progress.
 * @return Error for negative, out-of-range, or unreadable identifiers.
 */
SIMPLNX_EXPORT Result<> ValidateFeatureIdsToFeatureAttributeMatrixIndexing(const DataStructure& dataStructure, const DataPath& sourceDataPath, const Int32Array& featureIds, bool ignoreNegativeValues,
                                                                           const IFilter::MessageHandler& messageHandler);
/**
 * @brief Validates feature identifiers with cancellation.
 * @param dataStructure Data structure that owns the source.
 * @param sourceDataPath AttributeMatrix or array indexed by featureIds.
 * @param featureIds Feature identifier array.
 * @param ignoreNegativeValues True to ignore negative identifiers.
 * @param messageHandler Receives validation progress.
 * @param shouldCancel Optional cancellation flag.
 * @return Error for invalid identifiers or source reads.
 *
 * The routine checks cancellation between bounded batches and does not modify output.
 * Cancellation returns a valid result without completing validation.
 */
SIMPLNX_EXPORT Result<> ValidateFeatureIdsToFeatureAttributeMatrixIndexing(const DataStructure& dataStructure, const DataPath& sourceDataPath, const Int32Array& featureIds, bool ignoreNegativeValues,
                                                                           const IFilter::MessageHandler& messageHandler, const std::atomic_bool* shouldCancel);

/**
 * @brief Ensures NeighborList storage reaches its declared tuple count.
 * @param dataStructure Data structure that owns the list.
 * @param neighborListPath NeighborList path.
 * @pre neighborListPath identifies a nonempty INeighborList object.
 *
 * The function sets the final tuple to an empty list. NeighborList::setList()
 * grows missing preceding tuple slots through the list-store resize operation.
 */
SIMPLNX_EXPORT void InitializeNeighborList(DataStructure& dataStructure, const DataPath& neighborListPath);

/**
 * @class CopyTupleUsingIndexList
 * @brief Copies or initializes tuples from a destination-to-source map.
 * @tparam T Array value type.
 */
template <typename T>
class CopyTupleUsingIndexList
{
public:
  /**
   * @brief Creates a tuple remapper.
   * @param oldCellArray Source array.
   * @param newCellArray Destination array.
   * @param newIndices Destination-to-source tuple map.
   * @pre Referenced arrays and newIndices storage outlive this remapper.
   */
  CopyTupleUsingIndexList(const IDataArray& oldCellArray, IDataArray& newCellArray, nonstd::span<const int64> newIndices)
  : m_OldCellArray(oldCellArray)
  , m_NewCellArray(newCellArray)
  , m_NewToOldIndices(newIndices)
  {
  }
  /**
   * @brief Destroys the tuple remapper.
   */
  ~CopyTupleUsingIndexList() = default;

  /**
   * @brief Copies the tuple remapper.
   * @param other Tuple remapper to copy.
   */
  CopyTupleUsingIndexList(const CopyTupleUsingIndexList& other) = default;

  /**
   * @brief Moves the tuple remapper.
   * @param other Tuple remapper to move.
   */
  CopyTupleUsingIndexList(CopyTupleUsingIndexList&& other) noexcept = default;
  CopyTupleUsingIndexList& operator=(const CopyTupleUsingIndexList&) = delete;
  CopyTupleUsingIndexList& operator=(CopyTupleUsingIndexList&&) noexcept = delete;

  /**
   * @brief Copies a range of mapped destination tuples.
   * @param start First destination tuple index.
   * @param end One past the last destination tuple index.
   */
  void convert(usize start, usize end) const
  {
    const auto& oldDataStore = m_OldCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>();
    auto& newDataStore = m_NewCellArray.template getIDataStoreRefAs<AbstractDataStore<T>>();

    for(usize i = start; i < end; i++)
    {
      int64 oldIndexI = m_NewToOldIndices[i];
      if(oldIndexI >= 0)
      {
        if(newDataStore.copyFrom(i, oldDataStore, oldIndexI, 1).invalid())
        {
          std::cout << fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", m_OldCellArray.getName(), oldIndexI,
                                   m_NewCellArray.getName(), i)
                    << std::endl;
          break;
        }
      }
      else
      {
        newDataStore.fillTuple(i, 0);
      }
    }
  }

  /**
   * @brief Copies every mapped destination tuple.
   */
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
 * @namespace Indexing
 * @brief Contains multidimensional index utilities.
 */
namespace Indexing
{
/**
 * @brief Flattens a position with the rightmost dimension fastest.
 * @param position Multidimensional position.
 * @param shape Dimensions of the indexed array.
 * @return Flat array index.
 * @throws std::runtime_error If position and shape dimensions differ.
 */
inline usize Flatten(const std::vector<usize>& position, const std::vector<usize>& shape)
{
  using index_type = usize;
  const usize dimensions = position.size();

  if(shape.size() != dimensions)
  {
    throw std::runtime_error("Could not flatten position due to mismatched dimensions");
  }

  index_type index = 0;
  index_type mult = 1;
  const bool usingColumnMajor = true;
  for(index_type i = 0; i < dimensions; i++)
  {
    const index_type offset = (usingColumnMajor) ? dimensions - i - 1 : i;
    index += position[offset] * mult;
    mult *= shape[offset];
  }

  return index;
}

/**
 * @brief Finds a position from a flat index with the rightmost dimension fastest.
 * @param index Flat array index.
 * @param shape Dimensions of the indexed array.
 * @return Multidimensional position.
 */
inline std::vector<uint64> FindPosition(uint64 index, const std::vector<uint64>& shape)
{
  using index_type = uint64;
  using shape_type = std::vector<index_type>;

  const bool usingColumnMajor = true;
  const usize dimensions = shape.size();
  shape_type position(dimensions);
  for(index_type i = 0; i < dimensions; i++)
  {
    const index_type offset = (usingColumnMajor) ? dimensions - i - 1 : i;
    position[offset] = index % shape[offset];
    index /= shape[offset];
  }
  return position;
}

/**
 * @brief Advances a multidimensional index.
 * @param idx Index to advance.
 * @param extent Exclusive extent of each dimension.
 */
inline void IncrementLikeOdometer(std::vector<usize>& idx, const std::vector<usize>& extent)
{
  // Carry proceeds from the fastest dimension to the slowest dimension.
  for(usize d = idx.size(); d > 0;)
  {
    --d;
    ++idx[d];

    if(idx[d] < extent[d])
    {
      break;
    }
    else
    {
      idx[d] = 0;
    }
  }
}
} // namespace Indexing

/**
 * @brief Creates an initialized array that matches another array type.
 * @param destDataStructure Data structure that owns the new array.
 * @param array Source array that supplies type and component shape.
 * @param newArrayName New array name.
 * @param tupleShape New tuple dimensions.
 * @param defaultValue Text that supplies the initial value.
 * @param parentId Optional parent object identifier.
 * @return Array owned by destDataStructure, or an error if conversion or creation fails.
 * @pre array is not null.
 */
SIMPLNX_EXPORT Result<IArray*> CreateDefaultValueArrayFromArray(DataStructure& destDataStructure, IArray* array, const std::string& newArrayName, const ShapeType& tupleShape,
                                                                const std::string& defaultValue, const std::optional<DataObject::IdType> parentId = {});

/**
 * @brief Computes minimum and maximum values for each component.
 * @tparam T Array value type.
 * @param dataArray Array to inspect.
 * @return Minimum and maximum values for each component, or {{0, 0}} when dataArray is null.
 */
template <typename T>
std::vector<std::array<T, 2>> GetComponentMinMax(std::shared_ptr<DataArray<T>> dataArray)
{
  if(dataArray == nullptr)
  {
    return {{0, 0}};
  }
  const usize numTuples = dataArray->getNumberOfTuples();
  const usize numComps = dataArray->getNumberOfComponents();
  std::vector<std::array<T, 2>> componentRanges(numComps, std::array<T, 2>{std::numeric_limits<T>::max(), std::numeric_limits<T>::lowest()});
  for(int i = 0; i < numTuples; ++i)
  {
    for(int j = 0; j < numComps; ++j)
    {
      const auto value = dataArray->getValue(i * numComps + j);
      if(value > componentRanges[j][1])
      {
        componentRanges[j][1] = value;
      }
      if(value < componentRanges[j][0])
      {
        componentRanges[j][0] = value;
      }
    }
  }
  return componentRanges;
}

/**
 * @namespace CopyFromArray
 * @brief Contains storage-aware array copy and append utilities.
 */
namespace CopyFromArray
{
/**
 * @brief Appends array values at a flat destination offset.
 * @tparam K Array type.
 * @param inputArray Source array.
 * @param destArray Destination array.
 * @param offset First destination value offset.
 *
 * Numeric DataArrays use bounded bulk pages. Variable-length arrays retain
 * element-wise access because a fixed page cannot represent their tuples.
 */
template <class K>
void AppendData(const K& inputArray, K& destArray, usize offset)
{
  const usize numElements = inputArray.getNumberOfTuples() * inputArray.getNumberOfComponents();
  if constexpr(requires { inputArray.getDataStoreRef(); })
  {
    using ValueType = typename std::remove_reference_t<decltype(inputArray.getDataStoreRef())>::value_type;
    constexpr usize k_ChunkSize = 65536;
    // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized I/O buffer; std::array cannot represent this extent.
    auto buffer = std::make_unique<ValueType[]>(std::min(numElements, k_ChunkSize));
    const auto& srcStore = inputArray.getDataStoreRef();
    auto& dstStore = destArray.getDataStoreRef();
    for(usize i = 0; i < numElements; i += k_ChunkSize)
    {
      usize count = std::min(k_ChunkSize, numElements - i);
      srcStore.copyIntoBuffer(i, nonstd::span<ValueType>(buffer.get(), count));
      dstStore.copyFromBuffer(offset + i, nonstd::span<const ValueType>(buffer.get(), count));
    }
  }
  else
  {
    for(usize i = 0; i < numElements; ++i)
    {
      destArray.setValue(offset + i, inputArray.at(i));
    }
  }
}

/**
 * @brief Copies a tuple range between arrays with the same component count.
 * @tparam K Array type.
 * @param inputArray Source array.
 * @param destArray Destination array.
 * @param destTupleOffset First destination tuple.
 * @param srcTupleOffset First source tuple.
 * @param totalSrcTuples Tuple count to copy.
 * @return Error if the destination range or component count is not valid.
 * @pre The source tuple range is valid.
 *
 * Out-of-core arrays use a fixed 65,536-element buffer. Resident arrays keep
 * the single std::copy fast path.
 */
template <class K>
Result<> CopyData(const K& inputArray, K& destArray, usize destTupleOffset, usize srcTupleOffset, usize totalSrcTuples)
{
  if(destTupleOffset >= destArray.getNumberOfTuples())
  {
    return MakeErrorResult(
        -2032, fmt::format("The destination tuple offset ({}) is not smaller than the total number of tuples in the destination array ({})", destTupleOffset, destArray.getNumberOfTuples()));
  }

  const usize sourceNumComponents = inputArray.getNumberOfComponents();
  const usize numComponents = destArray.getNumberOfComponents();

  if(sourceNumComponents != numComponents)
  {
    return MakeErrorResult(-2033,
                           fmt::format("The number of components of the input array ({}) does not match the number of components of the destination array ({})", sourceNumComponents, numComponents));
  }

  auto elementsToCopy = totalSrcTuples * sourceNumComponents + destTupleOffset * numComponents;
  auto availableElements = destArray.getNumberOfTuples() * numComponents;
  if(elementsToCopy > availableElements)
  {
    return MakeErrorResult(-2034, fmt::format("The total number of elements to copy ({}) is larger than the total available elements ({}).", elementsToCopy, availableElements));
  }

  const usize numElements = totalSrcTuples * sourceNumComponents;
  if constexpr(requires { inputArray.getDataStoreRef(); })
  {
    const auto& srcStore = inputArray.getDataStoreRef();
    auto& dstStore = destArray.getDataStoreRef();
    if(srcStore.getStoreType() == IDataStore::StoreType::OutOfCore || dstStore.getStoreType() == IDataStore::StoreType::OutOfCore)
    {
      using ValueType = typename std::remove_reference_t<decltype(srcStore)>::value_type;
      constexpr usize k_ChunkSize = 65536;
      // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized I/O buffer; std::array cannot represent this extent.
      auto buffer = std::make_unique<ValueType[]>(std::min(numElements, k_ChunkSize));
      usize srcStart = srcTupleOffset * sourceNumComponents;
      usize dstStart = destTupleOffset * numComponents;
      for(usize offset = 0; offset < numElements; offset += k_ChunkSize)
      {
        usize count = std::min(k_ChunkSize, numElements - offset);
        srcStore.copyIntoBuffer(srcStart + offset, nonstd::span<ValueType>(buffer.get(), count));
        dstStore.copyFromBuffer(dstStart + offset, nonstd::span<const ValueType>(buffer.get(), count));
      }
    }
    else
    {
      auto srcBegin = inputArray.begin() + (srcTupleOffset * sourceNumComponents);
      auto srcEnd = srcBegin + numElements;
      auto dstBegin = destArray.begin() + (destTupleOffset * numComponents);
      std::copy(srcBegin, srcEnd, dstBegin);
    }
  }
  else
  {
    auto srcBegin = inputArray.begin() + (srcTupleOffset * sourceNumComponents);
    auto srcEnd = srcBegin + numElements;
    auto dstBegin = destArray.begin() + (destTupleOffset * numComponents);
    std::copy(srcBegin, srcEnd, dstBegin);
  }

  return {};
}

/**
 * @brief Copies a tuple range from vector storage into an array.
 * @tparam T Source value type.
 * @tparam K Destination array type.
 * @param src Source values.
 * @param dst Destination array.
 * @param dstTupleOffset First destination tuple.
 * @param srcTupleOffset First source tuple.
 * @param totalSrcTuples Tuple count to copy.
 * @param srcNumComponents Source component count.
 * @return Error if the destination range or component count is not valid.
 * @pre The source tuple range is valid.
 */
template <class T, class K>
Result<> CopyData(const std::vector<T>& src, K& dst, usize dstTupleOffset, usize srcTupleOffset, usize totalSrcTuples, usize srcNumComponents)
{
  static_assert(std::is_same_v<T, typename K::value_type>, "Element type mismatch between std::vector<T> and DataArray<value_type>");

  const usize dstNumComponents = dst.getNumberOfComponents();
  if(srcNumComponents != dstNumComponents)
  {
    return MakeErrorResult(-2033, fmt::format("Component mismatch: source vector comps ({}) vs dest array comps ({})", srcNumComponents, dstNumComponents));
  }

  if(srcTupleOffset * srcNumComponents >= src.size())
  {
    return MakeErrorResult(-2032, fmt::format("Source tuple offset ({}) is not smaller than total source tuples ({})", srcTupleOffset, src.size() / srcNumComponents));
  }

  if(dstTupleOffset >= dst.getNumberOfTuples())
  {
    return MakeErrorResult(-2032, fmt::format("Destination tuple offset ({}) is not smaller than destination tuples ({})", dstTupleOffset, dst.getNumberOfTuples()));
  }

  const usize dstAvailableElems = dst.getNumberOfTuples() * dstNumComponents;
  const usize elementsToCopy = totalSrcTuples * dstNumComponents + dstTupleOffset * dstNumComponents;
  if(elementsToCopy > dstAvailableElems)
  {
    return MakeErrorResult(-2034, fmt::format("The total number of elements to copy ({}) is larger than the total available elements ({}).", elementsToCopy, dstAvailableElems));
  }

  const usize numElements = totalSrcTuples * srcNumComponents;
  usize srcStart = srcTupleOffset * srcNumComponents;
  usize dstStart = dstTupleOffset * dstNumComponents;
  if constexpr(requires { dst.getDataStoreRef(); })
  {
    dst.getDataStoreRef().copyFromBuffer(dstStart, nonstd::span<const T>(src.data() + srcStart, numElements));
  }
  else if constexpr(requires { dst.copyFromBuffer(dstStart, nonstd::span<const T>(src.data(), 1)); })
  {
    dst.copyFromBuffer(dstStart, nonstd::span<const T>(src.data() + srcStart, numElements));
  }
  else
  {
    auto srcBegin = src.begin() + srcStart;
    auto srcEnd = srcBegin + numElements;
    auto dstBegin = dst.begin() + dstStart;
    std::copy(srcBegin, srcEnd, dstBegin);
  }

  return {};
}

/**
 * @brief Copies a row-major multidimensional tuple block.
 * @tparam K Array type.
 * @param inputArray Source array.
 * @param destArray Destination array.
 * @param srcStart Source start tuple in each dimension.
 * @param dstStart Destination start tuple in each dimension.
 * @param extent Tuple count in each dimension.
 * @return Error if ranks, ranges, or component counts are not valid.
 * @pre destArray has the same tuple rank as inputArray.
 *
 * The layout is row-major. The last index is fastest.
 */
template <class K>
Result<> CopyDataND(const K& inputArray, K& destArray, const std::vector<usize>& srcStart, const std::vector<usize>& dstStart, const std::vector<usize>& extent)
{
  const auto& inputShape = inputArray.getTupleShape();
  const auto& destShape = destArray.getTupleShape();
  const usize rank = inputShape.size();

  if(rank == 0)
  {
    return MakeErrorResult(-2030, "CopyDataND: Input array has no tuple dimensions (rank 0); unable to perform an N‑D copy.");
  }
  if(srcStart.size() != rank)
  {
    return MakeErrorResult(-2031, fmt::format("CopyDataND: srcStart length ({}) does not match input array rank ({}); provide one start index per dimension.", srcStart.size(), rank));
  }
  if(dstStart.size() != rank)
  {
    return MakeErrorResult(-2032, fmt::format("CopyDataND: dstStart length ({}) does not match input array rank ({}); provide one destination start index per dimension.", dstStart.size(), rank));
  }
  if(extent.size() != rank)
  {
    return MakeErrorResult(-2033, fmt::format("CopyDataND: extent length ({}) does not match input array rank ({}); provide one extent (tuple count) per dimension.", extent.size(), rank));
  }

  for(usize d = 0; d < rank; ++d)
  {
    if(srcStart[d] + extent[d] > inputShape[d])
    {
      return MakeErrorResult(-2034, fmt::format("CopyDataND: Source block exceeds bounds in dimension {} (srcStart={} + extent={} > srcSize={}).", d, srcStart[d], extent[d], inputShape[d]));
    }
    if(dstStart[d] + extent[d] > destArray.getTupleShape()[d])
    {
      return MakeErrorResult(
          -2035, fmt::format("CopyDataND: Destination block exceeds bounds in dimension {} (dstStart={} + extent={} > dstSize={}).", d, dstStart[d], extent[d], destArray.getTupleShape()[d]));
    }
  }

  const usize comps = inputArray.getNumberOfComponents();
  if(comps != destArray.getNumberOfComponents())
  {
    return MakeErrorResult(-2036, fmt::format("CopyDataND: Component count mismatch between source ({}) and destination ({}); both arrays must have identical component counts.", comps,
                                              destArray.getNumberOfComponents()));
  }

  std::vector<usize> currentIdx(rank, 0);
  const usize tuplesToCopy = std::accumulate(extent.begin(), extent.end(), usize{1}, std::multiplies<>());

  usize dstOffset = 0;
  for(usize n = 0; n < tuplesToCopy; ++n)
  {
    const usize srcLinearIdx = Indexing::Flatten(currentIdx, inputShape) + Indexing::Flatten(srcStart, inputShape);
    const usize dstLinearIdx = Indexing::Flatten(dstStart, destShape) + dstOffset;

    if constexpr(std::is_same_v<K, StringArray>)
    {
      destArray[dstLinearIdx] = inputArray[srcLinearIdx];
    }
    else if constexpr(std::is_base_of_v<INeighborList, K>)
    {
      destArray.setList(static_cast<int32>(dstLinearIdx), inputArray.getList(static_cast<int32>(srcLinearIdx)));
    }
    else
    {
      auto copyResult = CopyData(inputArray, destArray, dstLinearIdx, srcLinearIdx, 1);
      if(copyResult.invalid())
      {
        return copyResult;
      }
    }

    Indexing::IncrementLikeOdometer(currentIdx, extent);

    dstOffset++;
  }

  return {};
}

/**
 * @enum Direction
 * @brief Identifies an array append axis.
 */
enum class Direction
{
  X, ///< Selects the X axis.
  Y, ///< Selects the Y axis.
  Z  ///< Selects the Z axis.
};

/**
 * @brief Shifts data to make room along the X axis.
 * @tparam K Array type.
 * @param dataArray Array to shift.
 * @param originalDestDims Existing destination dimensions.
 * @param newDestDims Expanded destination dimensions.
 * @return Error from the tuple copies.
 *
 * The caller validates dimensions and capacity.
 */
template <class K>
Result<> ShiftDataX(K& dataArray, const std::vector<usize>& originalDestDims, const std::vector<usize>& newDestDims)
{
  auto shiftZDim = static_cast<int64>(newDestDims[0]);
  auto shiftYDim = static_cast<int64>(newDestDims[1]);
  auto shiftDestXDim = newDestDims[2];
  auto shiftSrcXDim = originalDestDims[2];

  for(int64 z = shiftZDim - 1; z >= 0; --z)
  {
    for(int64 y = shiftYDim - 1; y >= 0; --y)
    {
      usize srcOffset = (z * shiftYDim * shiftSrcXDim) + (y * shiftSrcXDim);
      usize destOffset = ((z * shiftYDim * shiftDestXDim) + (y * shiftDestXDim));
      if(srcOffset == destOffset)
      {
        continue;
      }

      auto result = CopyData(dataArray, dataArray, destOffset, srcOffset, shiftSrcXDim);
      if(result.invalid())
      {
        return result;
      }
    }
  }

  return {};
}

/**
 * @brief Shifts data to make room along the Y axis.
 * @tparam K Array type.
 * @param dataArray Array to shift.
 * @param originalDestDims Existing destination dimensions.
 * @param newDestDims Expanded destination dimensions.
 * @return Error from the tuple copies.
 *
 * The caller validates dimensions and capacity.
 */
template <class K>
Result<> ShiftDataY(K& dataArray, const std::vector<usize>& originalDestDims, const std::vector<usize>& newDestDims)
{
  auto shiftZDim = static_cast<int64>(newDestDims[0]);
  auto shiftDestYDim = newDestDims[1];
  auto shiftSrcYDim = static_cast<int64>(originalDestDims[1]);
  auto shiftXDim = newDestDims[2];

  for(int64 z = shiftZDim - 1; z >= 0; --z)
  {
    for(int64 y = shiftSrcYDim - 1; y >= 0; --y)
    {
      usize srcOffset = (z * shiftSrcYDim * shiftXDim) + (y * shiftXDim);
      usize destOffset = ((z * shiftDestYDim * shiftXDim) + (y * shiftXDim));
      if(srcOffset == destOffset)
      {
        continue;
      }

      auto result = CopyData(dataArray, dataArray, destOffset, srcOffset, shiftXDim);
      if(result.invalid())
      {
        return result;
      }
    }
  }

  return {};
}

/**
 * @brief Appends arrays along the X axis.
 * @tparam K Array type.
 * @param inputArrays Source arrays.
 * @param inputTupleShapes Source tuple dimensions.
 * @param destArray Destination array.
 * @param newDestDims Destination dimensions.
 * @param offset First X offset.
 * @param mirror True to mirror rows after append.
 * @return Error from tuple copies.
 *
 * The caller validates dimensions and capacity.
 */
template <class K>
Result<> AppendDataX(const std::vector<const K*>& inputArrays, const std::vector<std::vector<usize>>& inputTupleShapes, K& destArray, const std::vector<usize>& newDestDims, usize offset,
                     bool mirror = false)
{
  auto appendZDim = static_cast<int64>(newDestDims[0]);
  auto appendYDim = static_cast<int64>(newDestDims[1]);
  auto appendDestXDim = newDestDims[2];

  for(int z = 0; z < appendZDim; ++z)
  {
    for(int y = 0; y < appendYDim; ++y)
    {
      usize xOffset = offset;
      for(usize i = 0; i < inputArrays.size(); ++i)
      {
        const K& inputArray = *inputArrays[i];
        auto appendSrcXDim = inputTupleShapes[i][2];
        usize srcOffset = (z * appendYDim * appendSrcXDim) + (y * appendSrcXDim);
        usize destOffset = ((z * appendYDim * appendDestXDim) + (y * appendDestXDim) + xOffset);
        auto result = CopyData(inputArray, destArray, destOffset, srcOffset, appendSrcXDim);
        if(result.invalid())
        {
          return result;
        }
        xOffset += inputTupleShapes[i][2];
      }

      if(mirror)
      {
        auto numComps = destArray.getNumberOfComponents();
        if constexpr(requires { destArray.getDataStoreRef(); })
        {
          if(destArray.getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore)
          {
            // The OOC path reverses one scanline in memory to avoid per-tuple storage I/O.
            using ValueType = typename std::remove_reference_t<decltype(destArray.getDataStoreRef())>::value_type;
            usize scanlineElements = appendDestXDim * numComps;
            // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized scanline; std::array cannot represent this extent.
            auto scanline = std::make_unique<ValueType[]>(scanlineElements);
            auto& store = destArray.getDataStoreRef();
            usize rowStart = ((z * appendYDim * appendDestXDim) + (y * appendDestXDim)) * numComps;
            store.copyIntoBuffer(rowStart, nonstd::span<ValueType>(scanline.get(), scanlineElements));
            // Reverse complete tuples so component values remain together.
            for(usize x = 0; x < appendDestXDim / 2; ++x)
            {
              usize mirrorX = appendDestXDim - 1 - x;
              for(usize c = 0; c < numComps; ++c)
              {
                std::swap(scanline[x * numComps + c], scanline[mirrorX * numComps + c]);
              }
            }
            store.copyFromBuffer(rowStart, nonstd::span<const ValueType>(scanline.get(), scanlineElements));
          }
          else
          {
            for(usize x = 0; x < appendDestXDim / 2; ++x)
            {
              usize tupleIdx = (z * appendYDim * appendDestXDim) + (y * appendDestXDim) + x;
              usize endTupleIdx = tupleIdx + 1;
              usize mirrorTupleIdx = (z * appendYDim * appendDestXDim) + (y * appendDestXDim) + (appendDestXDim - 1 - x);
              std::swap_ranges(destArray.begin() + (tupleIdx * numComps), destArray.begin() + (endTupleIdx * numComps), destArray.begin() + (mirrorTupleIdx * numComps));
            }
          }
        }
        else
        {
          for(usize x = 0; x < appendDestXDim / 2; ++x)
          {
            usize tupleIdx = (z * appendYDim * appendDestXDim) + (y * appendDestXDim) + x;
            usize endTupleIdx = tupleIdx + 1;
            usize mirrorTupleIdx = (z * appendYDim * appendDestXDim) + (y * appendDestXDim) + (appendDestXDim - 1 - x);
            std::swap_ranges(destArray.begin() + (tupleIdx * numComps), destArray.begin() + (endTupleIdx * numComps), destArray.begin() + (mirrorTupleIdx * numComps));
          }
        }
      }
    }
  }

  return {};
}

/**
 * @brief Appends arrays along the Y axis.
 * @tparam K Array type.
 * @param inputArrays Source arrays.
 * @param inputTupleShapes Source tuple dimensions.
 * @param destArray Destination array.
 * @param newDestDims Destination dimensions.
 * @param offset First Y offset.
 * @param mirror True to mirror rows after append.
 * @return Error from tuple copies.
 *
 * The caller validates dimensions and capacity.
 */
template <class K>
Result<> AppendDataY(const std::vector<const K*>& inputArrays, const std::vector<std::vector<usize>>& inputTupleShapes, K& destArray, const std::vector<usize>& newDestDims, usize offset,
                     bool mirror = false)
{
  auto appendZDim = static_cast<int64>(newDestDims[0]);
  auto appendDestYDim = newDestDims[1];
  auto appendXDim = static_cast<int64>(newDestDims[2]);

  usize yOffset = offset;
  for(usize i = 0; i < inputArrays.size(); ++i)
  {
    auto appendSrcYDim = inputTupleShapes[i][1];
    for(int z = 0; z < appendZDim; ++z)
    {
      for(int y = 0; y < appendSrcYDim; ++y)
      {
        const K* inputArray = inputArrays[i];
        usize srcOffset = ((z * appendSrcYDim * appendXDim) + (y * appendXDim));
        usize destOffset = ((z * appendDestYDim * appendXDim) + ((y + yOffset) * appendXDim));
        auto result = CopyData(*inputArray, destArray, destOffset, srcOffset, appendXDim);
        if(result.invalid())
        {
          return result;
        }
      }
    }
    yOffset += inputTupleShapes[i][1];
  }

  if(mirror)
  {
    auto numComps = destArray.getNumberOfComponents();
    if constexpr(requires { destArray.getDataStoreRef(); })
    {
      if(destArray.getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore)
      {
        // The OOC path exchanges row pairs in bulk to avoid per-tuple storage I/O.
        using ValueType = typename std::remove_reference_t<decltype(destArray.getDataStoreRef())>::value_type;
        usize rowElements = static_cast<usize>(appendXDim) * numComps;
        // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized row; std::array cannot represent this extent.
        auto rowA = std::make_unique<ValueType[]>(rowElements);
        // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized row; std::array cannot represent this extent.
        auto rowB = std::make_unique<ValueType[]>(rowElements);
        auto& store = destArray.getDataStoreRef();
        for(int64 z = 0; z < appendZDim; ++z)
        {
          for(usize y = 0; y < appendDestYDim / 2; ++y)
          {
            usize mirrorY = appendDestYDim - 1 - y;
            usize offsetA = (static_cast<usize>(z) * appendDestYDim * static_cast<usize>(appendXDim) + y * static_cast<usize>(appendXDim)) * numComps;
            usize offsetB = (static_cast<usize>(z) * appendDestYDim * static_cast<usize>(appendXDim) + mirrorY * static_cast<usize>(appendXDim)) * numComps;
            store.copyIntoBuffer(offsetA, nonstd::span<ValueType>(rowA.get(), rowElements));
            store.copyIntoBuffer(offsetB, nonstd::span<ValueType>(rowB.get(), rowElements));
            store.copyFromBuffer(offsetA, nonstd::span<const ValueType>(rowB.get(), rowElements));
            store.copyFromBuffer(offsetB, nonstd::span<const ValueType>(rowA.get(), rowElements));
          }
        }
      }
      else
      {
        for(int64 z = 0; z < appendZDim; ++z)
        {
          for(usize y = 0; y < appendDestYDim / 2; ++y)
          {
            usize mirrorY = appendDestYDim - 1 - y;
            usize rowIdx = static_cast<usize>(z) * appendDestYDim * static_cast<usize>(appendXDim) + y * static_cast<usize>(appendXDim);
            usize mirrorRowIdx = static_cast<usize>(z) * appendDestYDim * static_cast<usize>(appendXDim) + mirrorY * static_cast<usize>(appendXDim);
            usize rowElements = static_cast<usize>(appendXDim) * numComps;
            std::swap_ranges(destArray.begin() + (rowIdx * numComps), destArray.begin() + (rowIdx * numComps + rowElements), destArray.begin() + (mirrorRowIdx * numComps));
          }
        }
      }
    }
    else
    {
      for(int64 z = 0; z < appendZDim; ++z)
      {
        for(usize y = 0; y < appendDestYDim / 2; ++y)
        {
          usize mirrorY = appendDestYDim - 1 - y;
          usize rowIdx = static_cast<usize>(z) * appendDestYDim * static_cast<usize>(appendXDim) + y * static_cast<usize>(appendXDim);
          usize mirrorRowIdx = static_cast<usize>(z) * appendDestYDim * static_cast<usize>(appendXDim) + mirrorY * static_cast<usize>(appendXDim);
          usize rowElements = static_cast<usize>(appendXDim) * numComps;
          std::swap_ranges(destArray.begin() + (rowIdx * numComps), destArray.begin() + (rowIdx * numComps + rowElements), destArray.begin() + (mirrorRowIdx * numComps));
        }
      }
    }
  }

  return {};
}

/**
 * @brief Appends arrays along the Z axis.
 * @tparam K Array type.
 * @param inputArrays Source arrays.
 * @param inputTupleShapes Source tuple dimensions.
 * @param destArray Destination array.
 * @param newDestDims Destination dimensions.
 * @param offset First Z offset.
 * @param mirror True to mirror slices after append.
 * @return Error from tuple copies.
 *
 * The caller validates dimensions and capacity.
 */
template <class K>
Result<> AppendDataZ(const std::vector<const K*>& inputArrays, const std::vector<std::vector<usize>>& inputTupleShapes, K& destArray, const std::vector<usize>& newDestDims, usize offset,
                     bool mirror = false)
{
  usize destOffset = offset;
  for(usize i = 0; i < inputArrays.size(); ++i)
  {
    const K* inputArray = inputArrays[i];
    auto totalInputTuples = std::accumulate(inputTupleShapes[i].begin(), inputTupleShapes[i].end(), static_cast<usize>(1), std::multiplies<>());
    auto result = CopyData(*inputArray, destArray, destOffset, 0, totalInputTuples);
    if(result.invalid())
    {
      return result;
    }
    destOffset += totalInputTuples;
  }

  if(mirror)
  {
    auto appendDestZDim = newDestDims[0];
    auto sliceTupleCount = newDestDims[1] * newDestDims[2];
    auto numComps = destArray.getNumberOfComponents();
    if constexpr(requires { destArray.getDataStoreRef(); })
    {
      if(destArray.getDataStoreRef().getStoreType() == IDataStore::StoreType::OutOfCore)
      {
        // The OOC path exchanges whole slices to avoid per-tuple storage I/O.
        using ValueType = typename std::remove_reference_t<decltype(destArray.getDataStoreRef())>::value_type;
        usize sliceElements = sliceTupleCount * numComps;
        // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized slice; std::array cannot represent this extent.
        auto bufA = std::make_unique<ValueType[]>(sliceElements);
        // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized slice; std::array cannot represent this extent.
        auto bufB = std::make_unique<ValueType[]>(sliceElements);
        auto& store = destArray.getDataStoreRef();
        for(usize i = 0; i < appendDestZDim / 2; ++i)
        {
          usize offsetA = i * sliceElements;
          usize offsetB = (appendDestZDim - 1 - i) * sliceElements;
          store.copyIntoBuffer(offsetA, nonstd::span<ValueType>(bufA.get(), sliceElements));
          store.copyIntoBuffer(offsetB, nonstd::span<ValueType>(bufB.get(), sliceElements));
          store.copyFromBuffer(offsetA, nonstd::span<const ValueType>(bufB.get(), sliceElements));
          store.copyFromBuffer(offsetB, nonstd::span<const ValueType>(bufA.get(), sliceElements));
        }
      }
      else
      {
        for(int i = 0; i < appendDestZDim / 2; ++i)
        {
          usize tupleIdx = i * sliceTupleCount;
          usize endTupleIdx = tupleIdx + sliceTupleCount;
          usize mirrorTupleIdx = (appendDestZDim - 1 - i) * sliceTupleCount;
          std::swap_ranges(destArray.begin() + (tupleIdx * numComps), destArray.begin() + (endTupleIdx * numComps), destArray.begin() + (mirrorTupleIdx * numComps));
        }
      }
    }
    else
    {
      for(int i = 0; i < appendDestZDim / 2; ++i)
      {
        usize tupleIdx = i * sliceTupleCount;
        usize endTupleIdx = tupleIdx + sliceTupleCount;
        usize mirrorTupleIdx = (appendDestZDim - 1 - i) * sliceTupleCount;
        std::swap_ranges(destArray.begin() + (tupleIdx * numComps), destArray.begin() + (endTupleIdx * numComps), destArray.begin() + (mirrorTupleIdx * numComps));
      }
    }
  }

  return {};
}

/**
 * @brief Shifts and appends arrays along the X axis.
 * @tparam K Array type.
 * @param inputArrays Source arrays.
 * @param inputTupleShapes Source tuple dimensions.
 * @param destArray Destination array.
 * @param originalDestDims Existing dimensions.
 * @param newDestDims Expanded dimensions.
 * @param mirror True to mirror rows after append.
 * @return Error from shifting or appending.
 */
template <class K>
Result<> ShiftAndAppendDataX(const std::vector<const K*>& inputArrays, const std::vector<std::vector<usize>>& inputTupleShapes, K& destArray, const std::vector<usize>& originalDestDims,
                             const std::vector<usize>& newDestDims, bool mirror = false)
{
  auto result = ShiftDataX(destArray, originalDestDims, newDestDims);
  if(result.invalid())
  {
    return result;
  }

  return AppendDataX(inputArrays, inputTupleShapes, destArray, newDestDims, originalDestDims[2], mirror);
}

/**
 * @brief Shifts and appends arrays along the Y axis.
 * @tparam K Array type.
 * @param inputArrays Source arrays.
 * @param inputTupleShapes Source tuple dimensions.
 * @param destArray Destination array.
 * @param originalDestDims Existing dimensions.
 * @param newDestDims Expanded dimensions.
 * @param mirror True to mirror rows after append.
 * @return Error from shifting or appending.
 */
template <class K>
Result<> ShiftAndAppendDataY(const std::vector<const K*>& inputArrays, const std::vector<std::vector<usize>>& inputTupleShapes, K& destArray, const std::vector<usize>& originalDestDims,
                             const std::vector<usize>& newDestDims, bool mirror = false)
{
  auto result = ShiftDataY(destArray, originalDestDims, newDestDims);
  if(result.invalid())
  {
    return result;
  }

  return AppendDataY(inputArrays, inputTupleShapes, destArray, newDestDims, originalDestDims[1], mirror);
}

/**
 * @brief Appends arrays along a selected axis.
 * @tparam K Array type.
 * @param inputArrays Source arrays.
 * @param inputTupleShapes Source tuple dimensions.
 * @param destArray Destination array.
 * @param originalDestDims Existing dimensions.
 * @param newDestDims Expanded dimensions.
 * @param direction Append axis.
 * @param mirror True to mirror after append.
 * @return Error from shifting or appending.
 */
template <class K>
Result<> AppendData(const std::vector<const K*>& inputArrays, const std::vector<std::vector<usize>>& inputTupleShapes, K& destArray, const std::vector<usize>& originalDestDims,
                    const std::vector<usize>& newDestDims, Direction direction = Direction::Z, bool mirror = false)
{
  switch(direction)
  {
  case Direction::X: {
    return ShiftAndAppendDataX(inputArrays, inputTupleShapes, destArray, originalDestDims, newDestDims, mirror);
  }
  case Direction::Y: {
    return ShiftAndAppendDataY(inputArrays, inputTupleShapes, destArray, originalDestDims, newDestDims, mirror);
  }
  default: {
    auto totalTuples = std::accumulate(originalDestDims.begin(), originalDestDims.end(), static_cast<usize>(1), std::multiplies<>());
    return AppendDataZ(inputArrays, inputTupleShapes, destArray, newDestDims, totalTuples, mirror);
  }
  }
}

/**
 * @brief Combines arrays along a selected axis.
 * @tparam K Array type.
 * @param inputArrays Source arrays.
 * @param inputTupleShapes Source tuple dimensions.
 * @param destArray Destination array.
 * @param newDestDims Destination dimensions.
 * @param direction Combine axis.
 * @param mirror True to mirror after combine.
 * @return Error from appending.
 */
template <class K>
Result<> CombineData(const std::vector<const K*>& inputArrays, const std::vector<std::vector<usize>>& inputTupleShapes, K& destArray, const std::vector<usize>& newDestDims,
                     Direction direction = Direction::Z, bool mirror = false)
{
  switch(direction)
  {
  case Direction::X: {
    return AppendDataX(inputArrays, inputTupleShapes, destArray, newDestDims, 0, mirror);
  }
  case Direction::Y: {
    return AppendDataY(inputArrays, inputTupleShapes, destArray, newDestDims, 0, mirror);
  }
  default: {
    return AppendDataZ(inputArrays, inputTupleShapes, destArray, newDestDims, 0, mirror);
  }
  }
}

/**
 * @class AppendArray
 * @brief Dispatches append operations for a selected value type.
 * @tparam T Array value type.
 */
template <typename T>
class AppendArray
{
public:
  /**
   * @brief Creates an append dispatcher.
   * @param destCellArray Destination array.
   * @param inputCellArrays Source arrays.
   * @param inputTupleShapes Source tuple dimensions.
   * @param originalDestDims Existing destination dimensions.
   * @param newDestDims Expanded destination dimensions.
   * @param direction Append axis.
   * @param mirror True to mirror after append.
   * @pre Source and destination arrays outlive this dispatcher.
   */
  AppendArray(IArray& destCellArray, const std::vector<const IArray*>& inputCellArrays, const std::vector<std::vector<usize>>& inputTupleShapes, const std::vector<usize>& originalDestDims,
              const std::vector<usize>& newDestDims, Direction direction = Direction::Z, bool mirror = false)
  : m_ArrayType(destCellArray.getArrayType())
  , m_InputCellArrays(inputCellArrays)
  , m_InputTupleShapes(inputTupleShapes)
  , m_DestCellArray(&destCellArray)
  , m_OriginalDestDims(originalDestDims)
  , m_NewDestDims(newDestDims)
  , m_Direction(direction)
  , m_Mirror(mirror)
  {
  }

  /**
   * @brief Destroys the append dispatcher.
   */
  ~AppendArray() = default;

  /**
   * @brief Copies the append dispatcher.
   * @param other Append dispatcher to copy.
   */
  AppendArray(const AppendArray& other) = default;

  /**
   * @brief Moves the append dispatcher.
   * @param other Append dispatcher to move.
   */
  AppendArray(AppendArray&& other) noexcept = default;
  AppendArray& operator=(const AppendArray&) = delete;
  AppendArray& operator=(AppendArray&&) noexcept = delete;

  /**
   * @brief Appends the selected arrays.
   */
  void operator()() const
  {
    if(m_ArrayType == IArray::ArrayType::NeighborListArray)
    {
      using NeighborListType = NeighborList<T>;
      auto* destArrayPtr = dynamic_cast<NeighborListType*>(m_DestCellArray);
      // NeighborList copies need an initialized destination list.
      if(destArrayPtr->getNumberOfLists() == 0 || destArrayPtr->getList(0).size() == 0)
      {
        destArrayPtr->addEntry(destArrayPtr->getNumberOfTuples() - 1, 0);
      }

      std::vector<const NeighborListType*> castedArrays;
      castedArrays.reserve(m_InputCellArrays.size());
      std::transform(m_InputCellArrays.begin(), m_InputCellArrays.end(), std::back_inserter(castedArrays),
                     [](const IArray* elem) -> const NeighborListType* { return dynamic_cast<const NeighborListType*>(elem); });

      AppendData<NeighborListType>(castedArrays, m_InputTupleShapes, *destArrayPtr, m_OriginalDestDims, m_NewDestDims, m_Direction, m_Mirror);
    }
    if(m_ArrayType == IArray::ArrayType::DataArray)
    {
      using DataArrayType = DataArray<T>;
      std::vector<const DataArrayType*> castedArrays;
      castedArrays.reserve(m_InputCellArrays.size());
      std::transform(m_InputCellArrays.begin(), m_InputCellArrays.end(), std::back_inserter(castedArrays),
                     [](const IArray* elem) -> const DataArrayType* { return dynamic_cast<const DataArrayType*>(elem); });
      AppendData<DataArrayType>(castedArrays, m_InputTupleShapes, *dynamic_cast<DataArrayType*>(m_DestCellArray), m_OriginalDestDims, m_NewDestDims, m_Direction, m_Mirror);
    }
    if(m_ArrayType == IArray::ArrayType::StringArray)
    {
      std::vector<const StringArray*> castedArrays;
      castedArrays.reserve(m_InputCellArrays.size());
      std::transform(m_InputCellArrays.begin(), m_InputCellArrays.end(), std::back_inserter(castedArrays),
                     [](const IArray* elem) -> const StringArray* { return dynamic_cast<const StringArray*>(elem); });
      AppendData<StringArray>(castedArrays, m_InputTupleShapes, *dynamic_cast<StringArray*>(m_DestCellArray), m_OriginalDestDims, m_NewDestDims, m_Direction, m_Mirror);
    }
  }

private:
  IArray::ArrayType m_ArrayType = IArray::ArrayType::Any;
  std::vector<const IArray*> m_InputCellArrays;
  std::vector<std::vector<usize>> m_InputTupleShapes;
  IArray* m_DestCellArray = nullptr;
  std::vector<usize> m_OriginalDestDims;
  std::vector<usize> m_NewDestDims;
  Direction m_Direction = Direction::Z;
  bool m_Mirror = false;
};

/**
 * @class CombineArrays
 * @brief Dispatches combine operations for a selected value type.
 * @tparam T Array value type.
 */
template <typename T>
class CombineArrays
{
public:
  /**
   * @brief Creates a combine dispatcher.
   * @param destCellArray Destination array.
   * @param inputCellArrays Source arrays.
   * @param inputTupleShapes Source tuple dimensions.
   * @param newDestDims Destination dimensions.
   * @param direction Combine axis.
   * @param mirror True to mirror after combine.
   * @pre Source and destination arrays outlive this dispatcher.
   */
  CombineArrays(IArray& destCellArray, const std::vector<const IArray*>& inputCellArrays, const std::vector<std::vector<usize>>& inputTupleShapes, const std::vector<usize>& newDestDims,
                Direction direction = Direction::Z, bool mirror = false)
  : m_ArrayType(destCellArray.getArrayType())
  , m_InputCellArrays(inputCellArrays)
  , m_InputTupleShapes(inputTupleShapes)
  , m_NewDestDims(newDestDims)
  , m_DestCellArray(&destCellArray)
  , m_Direction(direction)
  , m_Mirror(mirror)
  {
  }

  /**
   * @brief Destroys the combine dispatcher.
   */
  ~CombineArrays() = default;

  /**
   * @brief Copies the combine dispatcher.
   * @param other Combine dispatcher to copy.
   */
  CombineArrays(const CombineArrays& other) = default;

  /**
   * @brief Moves the combine dispatcher.
   * @param other Combine dispatcher to move.
   */
  CombineArrays(CombineArrays&& other) noexcept = default;
  CombineArrays& operator=(const CombineArrays&) = delete;
  CombineArrays& operator=(CombineArrays&&) noexcept = delete;

  /**
   * @brief Combines the selected arrays.
   */
  void operator()() const
  {
    if(m_ArrayType == IArray::ArrayType::NeighborListArray)
    {
      using NeighborListT = NeighborList<T>;
      auto* destArray = dynamic_cast<NeighborListT*>(m_DestCellArray);
      // NeighborList copies need an initialized destination list.
      if(destArray->getVectors().empty() || destArray->getList(0).empty())
      {
        destArray->addEntry(destArray->getNumberOfTuples() - 1, 0);
      }
      std::vector<const NeighborListT*> castedArrays;
      castedArrays.reserve(m_InputCellArrays.size());
      std::transform(m_InputCellArrays.begin(), m_InputCellArrays.end(), std::back_inserter(castedArrays),
                     [](const IArray* elem) -> const NeighborListT* { return dynamic_cast<const NeighborListT*>(elem); });
      CombineData<NeighborListT>(castedArrays, m_InputTupleShapes, *destArray, m_NewDestDims, m_Direction, m_Mirror);
    }
    if(m_ArrayType == IArray::ArrayType::DataArray)
    {
      using DataArrayType = DataArray<T>;
      std::vector<const DataArrayType*> castedArrays;
      castedArrays.reserve(m_InputCellArrays.size());
      std::transform(m_InputCellArrays.begin(), m_InputCellArrays.end(), std::back_inserter(castedArrays),
                     [](const IArray* elem) -> const DataArrayType* { return dynamic_cast<const DataArrayType*>(elem); });
      CombineData<DataArrayType>(castedArrays, m_InputTupleShapes, *dynamic_cast<DataArrayType*>(m_DestCellArray), m_NewDestDims, m_Direction, m_Mirror);
    }
    if(m_ArrayType == IArray::ArrayType::StringArray)
    {
      std::vector<const StringArray*> castedArrays;
      castedArrays.reserve(m_InputCellArrays.size());
      std::transform(m_InputCellArrays.begin(), m_InputCellArrays.end(), std::back_inserter(castedArrays),
                     [](const IArray* elem) -> const StringArray* { return dynamic_cast<const StringArray*>(elem); });
      CombineData<StringArray>(castedArrays, m_InputTupleShapes, *dynamic_cast<StringArray*>(m_DestCellArray), m_NewDestDims, m_Direction, m_Mirror);
    }
  }

private:
  IArray::ArrayType m_ArrayType = IArray::ArrayType::Any;
  std::vector<const IArray*> m_InputCellArrays;
  std::vector<std::vector<usize>> m_InputTupleShapes;
  std::vector<usize> m_NewDestDims;
  IArray* m_DestCellArray = nullptr;
  Direction m_Direction = Direction::Z;
  bool m_Mirror = false;
};

/**
 * @class CopyUsingIndexList
 * @brief Copies arrays through a destination-to-source index map.
 * @tparam T Array value type.
 *
 * Large index maps require memory proportional to destination tuple count.
 */
template <typename T>
class CopyUsingIndexList
{
public:
  /**
   * @brief Creates an index-map copy dispatcher.
   * @param destCellArray Destination array.
   * @param inputCellArray Source array.
   * @param newToOldIndices Destination-to-source index map.
   * @pre Source and destination arrays and newToOldIndices storage outlive this dispatcher.
   */
  CopyUsingIndexList(IArray& destCellArray, const IArray& inputCellArray, const nonstd::span<const int64>& newToOldIndices)
  : m_ArrayType(destCellArray.getArrayType())
  , m_InputCellArray(&inputCellArray)
  , m_DestCellArray(&destCellArray)
  , m_NewToOldIndices(newToOldIndices)
  {
  }

  /**
   * @brief Destroys the index-map copy dispatcher.
   */
  ~CopyUsingIndexList() = default;

  /**
   * @brief Copies the index-map copy dispatcher.
   * @param other Index-map copy dispatcher to copy.
   */
  CopyUsingIndexList(const CopyUsingIndexList& other) = default;

  /**
   * @brief Moves the index-map copy dispatcher.
   * @param other Index-map copy dispatcher to move.
   */
  CopyUsingIndexList(CopyUsingIndexList&& other) noexcept = default;
  CopyUsingIndexList& operator=(const CopyUsingIndexList&) = delete;
  CopyUsingIndexList& operator=(CopyUsingIndexList&&) noexcept = delete;

  /**
   * @brief Copies mapped values and initializes invalid mappings.
   */
  void operator()() const
  {
    for(usize i = 0; i < m_NewToOldIndices.size(); i++)
    {
      int64 oldIndexI = m_NewToOldIndices[i];
      Result<> copySucceeded;
      if(m_ArrayType == IArray::ArrayType::NeighborListArray)
      {
        using NeighborListT = NeighborList<T>;
        auto* destArray = dynamic_cast<NeighborListT*>(m_DestCellArray);
        // The destination list must be initialized before the tuple copy.
        destArray->setList(i, typename NeighborListT::SharedVectorType(new typename NeighborListT::VectorType));
        if(oldIndexI >= 0)
        {
          copySucceeded = CopyData<NeighborListT>(*dynamic_cast<const NeighborListT*>(m_InputCellArray), *destArray, i, oldIndexI, 1);
        }
      }
      else if(m_ArrayType == IArray::ArrayType::DataArray)
      {
        using DataArrayType = DataArray<T>;
        auto* destArray = dynamic_cast<DataArrayType*>(m_DestCellArray);
        if(oldIndexI >= 0)
        {
          copySucceeded = CopyData<DataArrayType>(*dynamic_cast<const DataArrayType*>(m_InputCellArray), *destArray, i, oldIndexI, 1);
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

      if(copySucceeded.invalid())
      {
        std::cout << fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", m_InputCellArray->getName(), oldIndexI,
                                 m_DestCellArray->getName(), i)
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

namespace
{
/**
 * @brief Maps ImageGeom coordinates to RectGrid bins on one axis.
 * @param dimSize Destination coordinate count.
 * @param originComp ImageGeom origin on this axis.
 * @param spacingComp ImageGeom spacing on this axis.
 * @param halfSpacingComp Half spacing for voxel-center sampling.
 * @param gridValues Monotonic RectGrid bounds.
 * @return One RectGrid bin index for each destination coordinate.
 *
 * Axis-aligned geometry lets the utility precompute one mapping per axis position.
 */
std::vector<usize> ComputeAxisBinIndices(usize dimSize, float32 originComp, float32 spacingComp, float32 halfSpacingComp, const Float32Array& gridValues)
{
  std::vector<usize> binIndices(dimSize, 0);
  const usize gridValueCount = gridValues.size();
  usize gridIdxStart = 1;
  for(usize i = 0; i < dimSize; i++)
  {
    const float32 coord = originComp + (static_cast<float32>(i) * spacingComp) + halfSpacingComp;
    for(usize gridIdx = gridIdxStart; gridIdx < gridValueCount; gridIdx++)
    {
      if(coord > gridValues.at(gridIdx - 1) && coord <= gridValues.at(gridIdx))
      {
        binIndices[i] = gridIdx - 1;
        gridIdxStart = gridIdx;
        break;
      }
    }
  }
  return binIndices;
}
} // namespace

/**
 * @class MapRectGridDataToImageData
 * @brief Maps RectGrid array values to ImageGeom cells.
 * @tparam T Array value type.
 */
template <typename T>
class MapRectGridDataToImageData
{
public:
  /**
   * @brief Creates a RectGrid-to-ImageGeom mapper.
   * @param destCellArray Destination array.
   * @param inputCellArray Source array.
   * @param origin ImageGeom origin.
   * @param imageGeoDims ImageGeom dimensions.
   * @param imageGeoSpacing ImageGeom spacing.
   * @param rectGridDims RectGrid dimensions.
   * @param xGridValues RectGrid X bounds.
   * @param yGridValues RectGrid Y bounds.
   * @param zGridValues RectGrid Z bounds.
   * @pre Source and destination arrays and grid-value arrays outlive this mapper.
   * @pre xGridValues, yGridValues, and zGridValues are not null.
   */
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

  /**
   * @brief Destroys the RectGrid-to-ImageGeom mapper.
   */
  ~MapRectGridDataToImageData() = default;

  /**
   * @brief Copies the RectGrid-to-ImageGeom mapper.
   * @param other RectGrid-to-ImageGeom mapper to copy.
   */
  MapRectGridDataToImageData(const MapRectGridDataToImageData& other) = default;

  /**
   * @brief Moves the RectGrid-to-ImageGeom mapper.
   * @param other RectGrid-to-ImageGeom mapper to move.
   */
  MapRectGridDataToImageData(MapRectGridDataToImageData&& other) noexcept = default;
  MapRectGridDataToImageData& operator=(const MapRectGridDataToImageData&) = delete;
  MapRectGridDataToImageData& operator=(MapRectGridDataToImageData&&) noexcept = delete;

  /**
   * @brief Maps and copies every destination cell.
   *
   * DataArrays use bulk buffers. Variable-length arrays use tuple copies.
   */
  void operator()() const
  {
    // Axis maps are reused across every row and slice.
    const std::vector<usize> zIndices = ComputeAxisBinIndices(m_ImageGeomDims[2], m_Origin[2], m_ImageGeomSpacing[2], m_HalfSpacing[2], *m_ZGridValues);
    const std::vector<usize> yIndices = ComputeAxisBinIndices(m_ImageGeomDims[1], m_Origin[1], m_ImageGeomSpacing[1], m_HalfSpacing[1], *m_YGridValues);
    const std::vector<usize> xIndices = ComputeAxisBinIndices(m_ImageGeomDims[0], m_Origin[0], m_ImageGeomSpacing[0], m_HalfSpacing[0], *m_XGridValues);

    if(m_ArrayType == IArray::ArrayType::DataArray)
    {
      mapDataArray(zIndices, yIndices, xIndices);
    }
    else
    {
      mapVariableLengthArray(zIndices, yIndices, xIndices);
    }
  }

private:
  /**
   * @brief Maps numeric arrays with bounded row buffers.
   * @param zIndices Source Z index for each destination Z index.
   * @param yIndices Source Y index for each destination Y index.
   * @param xIndices Source X index for each destination X index.
   *
   * One source row can serve repeated destination rows. Row buffers avoid
   * per-voxel cache access and remain bounded by axis dimensions.
   */
  void mapDataArray(const std::vector<usize>& zIndices, const std::vector<usize>& yIndices, const std::vector<usize>& xIndices) const
  {
    auto* destArray = dynamic_cast<DataArray<T>*>(m_DestCellArray);
    const auto* srcArray = dynamic_cast<const DataArray<T>*>(m_InputCellArray);
    auto& destStore = destArray->getDataStoreRef();
    const auto& srcStore = srcArray->getDataStoreRef();

    const usize numComponents = destArray->getNumberOfComponents();
    const usize destRowLength = m_ImageGeomDims[0] * numComponents;
    const usize srcRowLength = m_RectGridDims[0] * numComponents;

    // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized row; std::array cannot represent this extent.
    auto destRowBuffer = std::make_unique<T[]>(destRowLength);
    // NOLINTNEXTLINE(modernize-avoid-c-arrays) -- Runtime-sized row; std::array cannot represent this extent.
    auto srcRowBuffer = std::make_unique<T[]>(srcRowLength);

    bool haveCachedSrcRow = false;
    usize cachedYIndex = 0;
    usize cachedZIndex = 0;

    for(usize z = 0; z < m_ImageGeomDims[2]; z++)
    {
      const usize zIndex = zIndices[z];
      for(usize y = 0; y < m_ImageGeomDims[1]; y++)
      {
        const usize yIndex = yIndices[y];

        // Reuse a source row when consecutive destination rows map to it.
        if(!haveCachedSrcRow || yIndex != cachedYIndex || zIndex != cachedZIndex)
        {
          const usize srcRowStart = ((m_RectGridDims[0] * m_RectGridDims[1] * zIndex) + (m_RectGridDims[0] * yIndex)) * numComponents;
          srcStore.copyIntoBuffer(srcRowStart, nonstd::span<T>(srcRowBuffer.get(), srcRowLength));
          cachedYIndex = yIndex;
          cachedZIndex = zIndex;
          haveCachedSrcRow = true;
        }

        for(usize x = 0; x < m_ImageGeomDims[0]; x++)
        {
          const usize xIndex = xIndices[x];
          const int64 rectGridIndex = static_cast<int64>((m_RectGridDims[0] * m_RectGridDims[1] * zIndex) + (m_RectGridDims[0] * yIndex) + xIndex);
          T* destTuple = destRowBuffer.get() + (x * numComponents);
          if(rectGridIndex >= 0)
          {
            const T* srcTuple = srcRowBuffer.get() + (xIndex * numComponents);
            std::copy_n(srcTuple, numComponents, destTuple);
          }
          else
          {
            std::fill_n(destTuple, numComponents, static_cast<T>(0));
          }
        }

        const usize destRowStart = ((z * m_ImageGeomDims[1] * m_ImageGeomDims[0]) + (y * m_ImageGeomDims[0])) * numComponents;
        destStore.copyFromBuffer(destRowStart, nonstd::span<const T>(destRowBuffer.get(), destRowLength));
      }
    }
  }

  /**
   * @brief Maps variable-length arrays with tuple copies.
   * @param zIndices Source Z index for each destination Z index.
   * @param yIndices Source Y index for each destination Y index.
   * @param xIndices Source X index for each destination X index.
   *
   * Variable-length tuples cannot use fixed row buffers. Axis maps still avoid
   * repeated geometry searches.
   */
  void mapVariableLengthArray(const std::vector<usize>& zIndices, const std::vector<usize>& yIndices, const std::vector<usize>& xIndices) const
  {
    usize imageIndex = 0;
    for(usize z = 0; z < m_ImageGeomDims[2]; z++)
    {
      const usize zIndex = zIndices[z];
      for(usize y = 0; y < m_ImageGeomDims[1]; y++)
      {
        const usize yIndex = yIndices[y];
        for(usize x = 0; x < m_ImageGeomDims[0]; x++)
        {
          const usize xIndex = xIndices[x];

          const int64 rectGridIndex = static_cast<int64>((m_RectGridDims[0] * m_RectGridDims[1] * zIndex) + (m_RectGridDims[0] * yIndex) + xIndex);

          Result<> copySucceeded;
          if(m_ArrayType == IArray::ArrayType::NeighborListArray)
          {
            using NeighborListT = NeighborList<T>;
            auto* destArrayPtr = dynamic_cast<NeighborListT*>(m_DestCellArray);
            // NeighborList tuple copies require an initialized destination list.
            destArrayPtr->setList(imageIndex, typename NeighborListT::SharedVectorType(new typename NeighborListT::VectorType));
            if(rectGridIndex >= 0)
            {
              copySucceeded = CopyData<NeighborListT>(*dynamic_cast<const NeighborListT*>(m_InputCellArray), *destArrayPtr, imageIndex, rectGridIndex, 1);
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
          if(copySucceeded.invalid())
          {
            std::cout << fmt::format("Array copy failed: Source Array Name: {} Source Tuple Index: {}\nDest Array Name: {}  Dest. Tuple Index {}\n", m_InputCellArray->getName(), rectGridIndex,
                                     m_DestCellArray->getName(), imageIndex)
                      << std::endl;
            break;
          }

          ++imageIndex;
        }
      }
    }
  }

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
 * @brief Appends bool arrays through the bool-specific path.
 * @param destCellArray Destination bool array.
 * @param inputCellArrays Source arrays.
 * @param inputTupleShapes Source tuple dimensions.
 * @param originalDestDims Existing destination dimensions.
 * @param newDestDims Expanded destination dimensions.
 * @param direction Append axis.
 * @param mirror True to mirror after append.
 */
inline void RunAppendBoolAppend(IArray& destCellArray, const std::vector<const IArray*>& inputCellArrays, const std::vector<std::vector<usize>>& inputTupleShapes,
                                const std::vector<usize>& originalDestDims, const std::vector<usize>& newDestDims, Direction direction = Direction::Z, bool mirror = false)
{
  using DataArrayType = DataArray<bool>;
  std::vector<const DataArrayType*> castedArrays;
  castedArrays.reserve(inputTupleShapes.size());
  std::transform(inputCellArrays.cbegin(), inputCellArrays.cend(), std::back_inserter(castedArrays),
                 [](const IArray* elem) -> const DataArrayType* { return dynamic_cast<const DataArrayType*>(elem); });
  AppendData<DataArrayType>(castedArrays, inputTupleShapes, *dynamic_cast<DataArrayType*>(&destCellArray), originalDestDims, newDestDims, direction, mirror);
}

/**
 * @brief Combines bool arrays through the bool-specific path.
 * @param destCellArray Destination bool array.
 * @param inputCellArrays Source arrays.
 * @param inputTupleShapes Source tuple dimensions.
 * @param newDestDims Destination dimensions.
 * @param direction Combine axis.
 * @param mirror True to mirror after combine.
 */
inline void RunCombineBoolAppend(IArray& destCellArray, const std::vector<const IArray*>& inputCellArrays, const std::vector<std::vector<usize>>& inputTupleShapes,
                                 const std::vector<usize>& newDestDims, Direction direction = Direction::Z, bool mirror = false)
{
  using DataArrayType = DataArray<bool>;
  std::vector<const DataArrayType*> castedArrays;
  castedArrays.reserve(inputCellArrays.size());
  std::transform(inputCellArrays.cbegin(), inputCellArrays.cend(), std::back_inserter(castedArrays),
                 [](const IArray* elem) -> const DataArrayType* { return dynamic_cast<const DataArrayType*>(elem); });
  CombineData<DataArrayType>(castedArrays, inputTupleShapes, *dynamic_cast<DataArrayType*>(&destCellArray), newDestDims, direction, mirror);
}

/**
 * @brief Creates a bool index-map copy dispatcher.
 * @param destCellArray Destination bool array.
 * @param inputCellArray Source bool array.
 * @param newToOldIndices Destination-to-source index map.
 * @pre destCellArray and inputCellArray identify DataArray<bool> objects.
 * @note The function does not invoke the dispatcher.
 */
inline void RunBoolCopyUsingIndexList(IArray& destCellArray, const IArray& inputCellArray, const nonstd::span<const int64>& newToOldIndices)
{
  using DataArrayType = DataArray<bool>;
  CopyUsingIndexList<DataArrayType>(*dynamic_cast<DataArrayType*>(&destCellArray), *dynamic_cast<const DataArrayType*>(&inputCellArray), newToOldIndices);
}

/**
 * @brief Creates a bool RectGrid-to-ImageGeom mapper.
 * @param destCellArray Destination bool array.
 * @param inputCellArray Source bool array.
 * @param origin ImageGeom origin.
 * @param imageGeoDims ImageGeom dimensions.
 * @param imageGeoSpacing ImageGeom spacing.
 * @param rectGridDims RectGrid dimensions.
 * @param xGridValues RectGrid X bounds.
 * @param yGridValues RectGrid Y bounds.
 * @param zGridValues RectGrid Z bounds.
 * @pre destCellArray and inputCellArray identify DataArray<bool> objects.
 * @note The function does not invoke the mapper.
 */
inline void RunBoolMapRectToImage(IArray& destCellArray, const IArray& inputCellArray, const FloatVec3& origin, const SizeVec3& imageGeoDims, const std::vector<float32>& imageGeoSpacing,
                                  const SizeVec3& rectGridDims, const Float32Array* xGridValues, const Float32Array* yGridValues, const Float32Array* zGridValues)
{
  using DataArrayType = DataArray<bool>;
  MapRectGridDataToImageData<DataArrayType>(*dynamic_cast<DataArrayType*>(&destCellArray), *dynamic_cast<const DataArrayType*>(&inputCellArray), origin, imageGeoDims, imageGeoSpacing, rectGridDims,
                                            xGridValues, yGridValues, zGridValues);
}

/**
 * @brief Selects an append dispatcher by destination array type.
 * @tparam ParallelRunnerT Parallel runner type.
 * @tparam ArgsT Forwarded append arguments.
 * @param destArray Destination array.
 * @param runner Parallel runner.
 * @param args Forwarded append arguments.
 *
 * Boolean arrays run directly because their specialized path does not use runner.
 */
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
      return RunAppendBoolAppend(destArray, std::forward<ArgsT>(args)...);
    }
  }

  ExecuteParallelFunction<AppendArray, NoBooleanType>(dataType, std::forward<ParallelRunnerT>(runner), destArray, std::forward<ArgsT>(args)...);
}

/**
 * @brief Selects a combine dispatcher by destination array type.
 * @tparam ParallelRunnerT Parallel runner type.
 * @tparam ArgsT Forwarded combine arguments.
 * @param destArray Destination array.
 * @param runner Parallel runner.
 * @param args Forwarded combine arguments.
 *
 * Boolean arrays run directly because their specialized path does not use runner.
 */
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
 * @brief Selects an index-map copy dispatcher by destination array type.
 * @tparam ParallelRunnerT Parallel runner type.
 * @tparam ArgsT Forwarded copy arguments.
 * @param destArray Destination array.
 * @param runner Parallel runner.
 * @param args Forwarded copy arguments.
 *
 * Large index maps require memory proportional to destination tuple count.
 * For bool arrays, the specialized wrapper does not invoke the dispatcher.
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

/**
 * @brief Selects a RectGrid mapping dispatcher by destination array type.
 * @tparam ParallelRunnerT Parallel runner type.
 * @tparam ArgsT Forwarded mapping arguments.
 * @param destArray Destination array.
 * @param runner Parallel runner.
 * @param args Forwarded mapping arguments.
 *
 * For bool arrays, the specialized wrapper does not invoke the mapper.
 */
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

/**
 * @namespace TransferGeometryElementData
 * @brief Contains geometry element-data transfer utilities.
 */
namespace TransferGeometryElementData
{
/**
 * @class CopyCellDataArray
 * @brief Copies selected source cells to a destination array.
 * @tparam T Array value type.
 */
template <typename T>
class CopyCellDataArray
{
public:
  /**
   * @brief Creates a cell-data copy operation.
   * @param oldCellArray Source array.
   * @param newCellArray Destination array.
   * @param newEdgesIndex Source index for each destination tuple.
   * @param shouldCancel Cancellation flag shared with transferElementData().
   * @pre Referenced arrays, index map, and cancellation flag outlive this operation.
   * @throws std::bad_cast If either array is not DataArray<T>.
   */
  CopyCellDataArray(const IDataArray& oldCellArray, IDataArray& newCellArray, const std::vector<usize>& newEdgesIndex, const std::atomic_bool& shouldCancel)
  : m_OldCellArray(dynamic_cast<const DataArray<T>&>(oldCellArray))
  , m_NewCellArray(dynamic_cast<DataArray<T>&>(newCellArray))
  , m_NewEdgesIndex(newEdgesIndex)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Destroys the cell-data copy operation.
   */
  ~CopyCellDataArray() = default;

  /**
   * @brief Copies the cell-data copy operation.
   * @param other Cell-data copy operation to copy.
   */
  CopyCellDataArray(const CopyCellDataArray& other) = default;

  /**
   * @brief Moves the cell-data copy operation.
   * @param other Cell-data copy operation to move.
   */
  CopyCellDataArray(CopyCellDataArray&& other) noexcept = default;
  CopyCellDataArray& operator=(const CopyCellDataArray&) = delete;
  CopyCellDataArray& operator=(CopyCellDataArray&&) noexcept = delete;

  /**
   * @brief Copies selected cell values.
   *
   * transferElementData() checks cancellation before it schedules each array.
   * This operation does not check cancellation after it starts.
   */
  void operator()() const
  {
    usize numComps = m_OldCellArray.getNumberOfComponents();
    const auto& oldCellData = m_OldCellArray.getDataStoreRef();

    auto& dataStore = m_NewCellArray.getDataStoreRef();
    std::fill(dataStore.begin(), dataStore.end(), static_cast<T>(-1));

    uint64 destTupleIndex = 0;
    for(const auto& srcIndex : m_NewEdgesIndex)
    {
      for(usize compIndex = 0; compIndex < numComps; compIndex++)
      {
        dataStore.setValue(destTupleIndex * numComps + compIndex, oldCellData.getValue(srcIndex * numComps + compIndex));
      }
      destTupleIndex++;
    }
  }

private:
  const DataArray<T>& m_OldCellArray;
  DataArray<T>& m_NewCellArray;
  const std::vector<usize>& m_NewEdgesIndex;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Transfers selected element arrays to a destination AttributeMatrix.
 * @param m_DataStructure Data structure that owns the arrays.
 * @param destCellDataAM Destination AttributeMatrix.
 * @param sourceDataPaths Source array paths.
 * @param newEdgesIndexList Destination-to-source index map.
 * @param m_ShouldCancel Cancellation flag.
 * @param m_MessageHandler Receives progress messages.
 *
 * The routine checks cancellation before it schedules each source array.
 */
SIMPLNX_EXPORT void transferElementData(DataStructure& m_DataStructure, AttributeMatrix& destCellDataAM, const std::vector<DataPath>& sourceDataPaths, const std::vector<usize>& newEdgesIndexList,
                                        const std::atomic_bool& m_ShouldCancel, const IFilter::MessageHandler& m_MessageHandler);

/**
 * @brief Creates actions for selected data arrays.
 * @param dataStructure Data structure that owns source arrays.
 * @param sourceAttrMatPtr Source AttributeMatrix.
 * @param selectedArrayPaths Selected source array paths.
 * @param reducedGeometryPathAttrMatPath Destination AttributeMatrix path.
 * @param resultOutputActions Receives created actions.
 * @pre sourceAttrMatPtr is not null.
 */
SIMPLNX_EXPORT void CreateDataArrayActions(const DataStructure& dataStructure, const AttributeMatrix* sourceAttrMatPtr, const MultiArraySelectionParameter::ValueType& selectedArrayPaths,
                                           const DataPath& reducedGeometryPathAttrMatPath, Result<OutputActions>& resultOutputActions);
} // namespace TransferGeometryElementData
} // namespace nx::core
