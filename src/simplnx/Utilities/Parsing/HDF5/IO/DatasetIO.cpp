#include "DatasetIO.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ChunkIndex.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ChunkShapePolicy.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5Support.hpp"
#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"
#include "simplnx/Utilities/Parsing/HDF5/ParallelChunkCodec.hpp"

#include <fmt/format.h>

#include "H5Dpublic.h"
#include "H5Fpublic.h"
#include "H5Spublic.h"
#include "H5Tpublic.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <numeric>
#include <span>
#include <vector>

using namespace nx::core;

namespace
{
std::string GetNameFromFilterType(H5Z_filter_t id)
{
  switch(id)
  {
  case H5Z_FILTER_DEFLATE:
    return "GZIP";
  case H5Z_FILTER_SHUFFLE:
    return "SHUFFLE";
  case H5Z_FILTER_FLETCHER32:
    return "FLETCHER32";
  case H5Z_FILTER_SZIP:
    return "SZIP";
  case H5Z_FILTER_NBIT:
    return "N-BIT";
  case H5Z_FILTER_SCALEOFFSET:
    return "SCALE-OFFSET";
  case H5Z_FILTER_ERROR:
  case H5Z_FILTER_NONE:
    return "NONE";
  default:
    return "UNKNOWN";
  }
}

/**
 * @brief Builds chunked deflate dataset-creation properties.
 * @param dims Specifies full dataset dimensions.
 * @param elementByteSize Specifies bytes per value.
 * @param compressionLevel Specifies gzip level from 0 through 9.
 * @return H5P_DEFAULT, an owned property-list ID, or an HDF5/configuration error.
 *
 * Level 0 and small arrays use H5P_DEFAULT, which the caller must not close.
 * Other successful results are caller-owned and require H5Pclose().
 *
 * BundleOuterSlabs targets approximately 1 MiB C-order chunks. Large slices use
 * row bands. Small outer slabs can share one chunk.
 */
nx::core::Result<hid_t> BuildChunkedDeflateDcpl(const std::vector<usize>& dims, usize elementByteSize, int32 compressionLevel)
{
  if(compressionLevel < 0 || compressionLevel > 9)
  {
    return nx::core::MakeErrorResult<hid_t>(-1300, fmt::format("Invalid gzip level {}; expected [0, 9]", compressionLevel));
  }
  if(compressionLevel == 0)
  {
    return {H5P_DEFAULT};
  }

  if(dims.empty())
  {
    return nx::core::MakeErrorResult<hid_t>(-1303, "BuildChunkedDeflateDcpl called with empty dims");
  }
  if(elementByteSize == 0)
  {
    return nx::core::MakeErrorResult<hid_t>(-1304, "BuildChunkedDeflateDcpl called with zero elementByteSize");
  }

  // Total byte size with saturating-overflow detection. In practice unreachable
  // (would require >2^64 bytes), but keeping the arithmetic honest is cheap.
  usize totalBytes = elementByteSize;
  for(auto d : dims)
  {
    const usize castD = static_cast<usize>(d);
    if(castD == 0)
    {
      return nx::core::MakeErrorResult<hid_t>(-1305, "BuildChunkedDeflateDcpl encountered a zero-valued dimension");
    }
    if(totalBytes > std::numeric_limits<usize>::max() / castD)
    {
      return nx::core::MakeErrorResult<hid_t>(-1306, "BuildChunkedDeflateDcpl total byte size overflowed usize");
    }
    totalBytes *= castD;
  }
  if(totalBytes < nx::core::HDF5::k_SmallArrayThresholdBytes)
  {
    return {H5P_DEFAULT};
  }

  // The chunk shape comes from the shared core policy. dims already includes the component
  // dimensions (writeSpan passes the full dataspace rank), so numComponents = 1 — the
  // component bytes are already folded into dims. BundleOuterSlabs reproduces the write-once
  // greedy outermost-first walk: ~1 MiB chunks, whole outer slabs bundled when they fit.
  const ShapeType chunk =
      nx::core::HDF5::computeChunkShape(dims, /*numComponents=*/1, elementByteSize, {.targetBytes = nx::core::HDF5::k_TargetChunkBytes, .regime = nx::core::HDF5::ChunkShapeRegime::BundleOuterSlabs});
  std::vector<hsize_t> chunkDims(chunk.size());
  for(usize i = 0; i < chunk.size(); ++i)
  {
    chunkDims[i] = static_cast<hsize_t>(chunk[i]);
  }

  // computeChunkShape above is pure CPU (no HDF5). The DCPL creation and configuration are
  // bare HDF5 C calls, so they self-lock as one leaf critical section. The error-string
  // construction below runs after the lock releases (each branch closes the DCPL before
  // unlocking).
  hid_t dcpl = H5I_INVALID_HID;
  int errorCode = 0;
  {
    std::lock_guard<std::mutex> hdf5Lock(nx::core::HDF5::Support::ApiLock());
    dcpl = H5Pcreate(H5P_DATASET_CREATE);
    if(dcpl < 0)
    {
      errorCode = -1300;
    }
    else if(H5Pset_chunk(dcpl, static_cast<int>(chunkDims.size()), chunkDims.data()) < 0)
    {
      H5Pclose(dcpl);
      errorCode = -1301;
    }
    else if(H5Pset_deflate(dcpl, static_cast<unsigned int>(compressionLevel)) < 0)
    {
      H5Pclose(dcpl);
      errorCode = -1302;
    }
  }

  switch(errorCode)
  {
  case -1300:
    return nx::core::MakeErrorResult<hid_t>(static_cast<int64>(dcpl), "H5Pcreate(H5P_DATASET_CREATE) failed");
  case -1301:
    return nx::core::MakeErrorResult<hid_t>(-1301, "H5Pset_chunk failed");
  case -1302:
    return nx::core::MakeErrorResult<hid_t>(-1302, fmt::format("H5Pset_deflate(level={}) failed", compressionLevel));
  default:
    return {dcpl};
  }
}
} // namespace

namespace nx::core::HDF5
{
constexpr int64 k_DimensionMismatchError = -5138;

DatasetIO::DatasetIO() = default;

DatasetIO::DatasetIO(hid_t parentId, const std::string& dataName)
: ObjectIO(parentId, dataName)
{
#if 0
  if(!tryOpeningDataset(datasetName, dataType))
  {
    tryCreatingDataset(datasetName, dataType);
  }
#endif
}

DatasetIO::DatasetIO(DatasetIO&& other) noexcept
: ObjectIO(std::move(other))
, m_CompressionLevel(other.m_CompressionLevel)
{
}

DatasetIO::~DatasetIO() noexcept
{
  close();
}

void DatasetIO::close()
{
  // Self-locks the bare H5Dclose leaf call. Invoked by ~DatasetIO, so destroying a
  // DatasetIO on any thread serializes its close against every other HDF5 C call. The id
  // is captured before the lock (isOpen() already guarantees it is open) so nothing
  // lock-taking runs inside the leaf scope.
  if(isOpen())
  {
    const hid_t selfId = getId();
    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      H5Dclose(selfId);
    }
    setId(0);
  }
}

hid_t DatasetIO::open() const
{
  if(isOpen())
  {
    return getId();
  }
  // Resolve the parent id and path (no HDF5 work) before locking the bare H5Dopen leaf.
  // The error-handler toggles set thread-global HDF5 error state, so they belong inside
  // the same leaf critical section as the call they guard.
  const hid_t parentId = getParentId();
  const std::string namePath = getNamePath();
  hid_t id = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    HDF_ERROR_HANDLER_OFF /* Does not matter what the 'id' is, we are accepting that value. */
        id = H5Dopen(parentId, namePath.c_str(), H5P_DEFAULT);
    HDF_ERROR_HANDLER_ON
  }
  setId(id);
  return id;
}

hid_t DatasetIO::createOrOpenDataset(IdType typeId, IdType dataspaceId, IdType propertiesId) const
{
  if(isOpen())
  {
    return getId();
  }

  // Resolve the parent id and path before the leaf-locked open-then-create sequence. The
  // error-handler toggles around H5Dopen set thread-global state, so they stay inside the
  // leaf critical section.
  const hid_t parentId = getParentId();
  const std::string namePath = getNamePath();
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    HDF_ERROR_HANDLER_OFF
    setId(H5Dopen(parentId, namePath.c_str(), H5P_DEFAULT));
    HDF_ERROR_HANDLER_ON
    if(!isOpen()) // dataset does not exist so create it
    {
      setId(H5Dcreate(parentId, namePath.c_str(), typeId, dataspaceId, H5P_DEFAULT, propertiesId, H5P_DEFAULT));
    }
  }

  return getId();
}

void DatasetIO::setCompressionLevel(int32 level) noexcept
{
  if(level < 0 || level > 9)
  {
    return;
  }
  m_CompressionLevel = level;
}

int32 DatasetIO::getCompressionLevel() const noexcept
{
  return m_CompressionLevel;
}

// template <typename T>
// HighFive::DataSet createOrOpenDataset(DatasetIO& datasetIO, const HighFive::DataSpace& dims, HighFive::DataType& dataType)
//{
//   std::string name = datasetIO.getNamePath();
//   std::string datapath = datasetIO.getObjectPath();
//   try
//   {
//     auto& parentGroup = datasetIO.parentGroup()->groupRef();
//     if(parentGroup.exist(name))
//     {
//       return parentGroup.getDataSet(name);
//     }
//     else
//     {
//       return parentGroup.createDataSet(name, dims, dataType);
//     }
//   } catch(const std::exception& e)
//   {
//     throw e;
//   }
// }
//
// template <>
// HighFive::DataSet createOrOpenDataset<bool>(DatasetIO& datasetIO, const HighFive::DataSpace& dims)
//{
//   std::string name = datasetIO.getNamePath();
//   std::string datapath = datasetIO.getObjectPath();
//   try
//   {
//     auto& parentGroup = datasetIO.parentGroup()->groupRef();
//     if(parentGroup.exist(name))
//     {
//       return std::move(parentGroup.getDataSet(name));
//     }
//     else
//     {
//       return std::move(parentGroup.createDataSet<H5_BOOL_TYPE>(name, dims));
//     }
//   } catch(const std::exception& e)
//   {
//     throw e;
//   }
// }

DatasetIO& DatasetIO::operator=(DatasetIO&& rhs) noexcept
{
  m_CompressionLevel = rhs.m_CompressionLevel;
  moveObj(std::move(rhs));
  return *this;
}

#if 0
Result<> DatasetIO::findAndDeleteAttribute()
{
  hsize_t attributeNum = 0;
  //int32_t hasAttribute = H5Aiterate(getParentId(), H5_INDEX_NAME, H5_ITER_INC, &attributeNum, Support::FindAttr, const_cast<char*>(getNamePath().c_str()));

  /* The attribute already exists, delete it */
  if(hasAttribute())
  {
    herr_t error = H5Adelete(getId(), getNamePath().c_str());
    if(error < 0)
    {
      std::string ss = fmt::format("Error Deleting Attribute '{}' from Object '{}'", getNamePath(), getParentName());
      return MakeErrorResult(error, ss);
    }
  }
  return {};
}
#endif

// DataType DatasetIO::getType() const
//{
//   auto dataset = open();
//   return dataset.getDataType();
// }

hid_t DatasetIO::getTypeId() const
{
  // getId() self-locks (it may lazily open the dataset); resolve it before the leaf-locked
  // bare H5Dget_type.
  auto identifier = getId();
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  return H5Dget_type(identifier);
}

hid_t DatasetIO::getClassType() const
{
  // getTypeId() self-locks; resolve it before the leaf-locked bare H5Tget_class.
  auto typeId = getTypeId();
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  return H5Tget_class(typeId);
}

size_t DatasetIO::getTypeSize() const
{
  // getTypeId() self-locks; resolve it before the leaf-locked bare H5Tget_size.
  auto typeId = getTypeId();
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  return H5Tget_size(typeId);
}

#if 0
Result<nx::core::HDF5::Type> DatasetIO::getDataType() const
{
  auto datasetId = open();
  hid_t typeId = H5Dget_type(datasetId);
  auto type = getTypeFromId(typeId);
  Result<Type> result;
  switch(type)
  {
  case Type::float32:
    result = {Type::float32};
    break;
  case Type::float64:
    result = {Type::float64};
    break;
  case Type::int8:
    result = {Type::int8};
    break;
  case Type::int16:
    result = {Type::int16};
    break;
  case Type::int32:
    result = {Type::int32};
    break;
  case Type::int64:
    result = {Type::int64};
    break;
  case Type::uint8:
    result = {Type::uint8};
    break;
  case Type::uint16:
    result = {Type::uint16};
    break;
  case Type::uint32:
    result = {Type::uint32};
    break;
  case Type::uint64:
    result = {Type::uint64};
    break;
  default:
    result = {nonstd::make_unexpected(std::vector<Error>{Error{-20012, "The selected datatset is not a supported type for "
                                                                       "importing. Please select a different data set"}})};
    break;
  }
  H5Tclose(typeId);
  return result;
}
#else
Result<nx::core::DataType> DatasetIO::getDataType() const
{
  // open() and getTypeFromId() self-lock, so neither may run while the
  // non-recursive leaf lock below is held.
  auto datasetId = open();
  if(datasetId < 0)
  {
    return MakeErrorResult<nx::core::DataType>(-20013, fmt::format("The selected data set '{}' could not be opened.", getNamePath()));
  }

  hid_t typeId = -1;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    typeId = H5Dget_type(datasetId);
    const H5T_class_t classType = H5Tget_class(typeId);
    if(classType == H5T_COMPOUND)
    {
      H5Tclose(typeId);
      return MakeErrorResult<nx::core::DataType>(-20016, fmt::format("H5T_COMPOUND data type is not supported for importing '{}'.", getNamePath()));
    }
  }
  const Type type = getTypeFromId(typeId);
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    H5Tclose(typeId);
  }
  if(type == Type::unknown)
  {
    return MakeErrorResult<nx::core::DataType>(-20014, fmt::format("The selected data set '{}' typeid is unknown.", getNamePath()));
  }

  Result<DataType> result;
  switch(type)
  {
  case Type::float32:
    result = {DataType::float32};
    break;
  case Type::float64:
    result = {DataType::float64};
    break;
  case Type::int8:
    result = {DataType::int8};
    break;
  case Type::int16:
    result = {DataType::int16};
    break;
  case Type::int32:
    result = {DataType::int32};
    break;
  case Type::int64:
    result = {DataType::int64};
    break;
  case Type::uint8:
    result = {DataType::uint8};
    break;
  case Type::uint16:
    result = {DataType::uint16};
    break;
  case Type::uint32:
    result = {DataType::uint32};
    break;
  case Type::uint64:
    result = {DataType::uint64};
    break;
  default:
    result = {nonstd::make_unexpected(std::vector<Error>{Error{-20012, "The selected datat set is not a supported type for "
                                                                       "importing. Please select a different data set"}})};
    break;
  }
  return result;
}
#endif

size_t DatasetIO::getNumElements() const
{
  if(!exists())
  {
    return 0;
  }
  std::vector<usize> dims = getDimensions();
  hsize_t numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<hsize_t>(1), std::multiplies<>());
  return numElements;
}

size_t DatasetIO::getNumChunkElements() const
{
  std::vector<usize> dims = getChunkDimensions();
  usize numElements = std::accumulate(dims.cbegin(), dims.cend(), static_cast<usize>(1), std::multiplies<>());
  return numElements;
}

std::string DatasetIO::readAsString() const
{
  auto datasetId = open();
  if(!isValid())
  {
    return "";
  }

  // Inspect string type under one leaf lock. Call the self-locking vector reader
  // only after releasing this nonrecursive lock.
  bool isVariableString = false;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    const hid_t typeID = H5Dget_type(datasetId);
    isVariableString = (H5Tis_variable_str(typeID) == 1);
    H5Tclose(typeID);
  }

  if(isVariableString)
  {
    auto stringVec = readAsVectorOfStrings();
    if(stringVec.size() > 1)
    {
      std::cout << "Error Reading string dataset. There were multiple strings "
                   "and the program asked for a single string."
                << std::endl;
      return "";
    }
    return stringVec.empty() ? std::string{} : stringVec[0];
  }

  // Fixed-length string: read the raw bytes under one leaf lock.
  std::string data;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    const hid_t typeID = H5Dget_type(datasetId);
    const hsize_t size = H5Dget_storage_size(datasetId);
    std::vector<char> buffer(static_cast<size_t>(size + 1), 0x00); // Allocate and zero the array
    const herr_t error = H5Dread(datasetId, typeID, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer.data());
    H5Tclose(typeID);
    if(error < 0)
    {
      std::cout << "Error Reading string dataset." << std::endl;
      return "";
    }
    data.append(buffer.data()); // Append the read bytes to the result string
  }

  return data;
}

std::vector<std::string> DatasetIO::readAsVectorOfStrings() const
{
  auto datasetId = open();
  if(!isValid())
  {
    return {};
  }
  // auto dataset = openH5Dataset();

  std::vector<std::string> strings;

  // One leaf critical section spanning every bare H5 call below (type/space query,
  // memory-type build, vlen read, and reclaim). open()/isValid() were resolved
  // above and no other self-locking helper runs inside this scope.
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  hid_t typeID = H5Dget_type(datasetId);

  if(typeID >= 0)
  {
    hsize_t dims[1] = {0};
    /*
     * Get dataspace and allocate memory for read buffer.
     */
    hid_t dataspaceID = H5Dget_space(datasetId);
    int nDims = H5Sget_simple_extent_dims(dataspaceID, dims, nullptr);
    if(nDims != 1)
    {
      // H5Sclose(dataspaceID);
      // H5Tclose(typeID);
      std::cout << "H5DatasetReader.cpp::readVectorOfStrings(" << __LINE__ << ") Number of dims should be 1 but it was " << nDims << ". Returning early. Is your data file correct?" << std::endl;
      return {};
    }

    std::vector<char*> rData(dims[0], nullptr);

    /*
     * Create the memory datatype.
     */
    hid_t memtype = H5Tcopy(H5T_C_S1);
    herr_t status = H5Tset_size(memtype, H5T_VARIABLE);

    H5T_cset_t characterSet = H5Tget_cset(typeID);
    status = H5Tset_cset(memtype, characterSet);

    /*
     * Read the data.
     */
    status = H5Dread(datasetId, memtype, H5S_ALL, H5S_ALL, H5P_DEFAULT, rData.data());
    if(status < 0)
    {
      status = H5Dvlen_reclaim(memtype, dataspaceID, H5P_DEFAULT, rData.data());
      H5Sclose(dataspaceID);
      H5Tclose(typeID);
      H5Tclose(memtype);
      std::cout << "H5DatasetReader.cpp::readVectorOfStrings(" << __LINE__ << ") Error reading Dataset at locationID (" << getParentId() << ") with object name (" << getNamePath() << ")" << std::endl;
      return {};
    }
    /*
     * copy the data into the vector of strings
     */
    strings.resize(dims[0]);
    for(size_t i = 0; i < dims[0]; i++)
    {
      // printf("%s[%d]: %s\n", "VlenStrings", i, rData[i].p);
      strings[i] = std::string(rData[i]);
    }
    /*
     * Close and release resources.  Note that H5Dvlen_reclaim works
     * for variable-length strings as well as variable-length arrays.
     * Also note that we must still free the array of pointers stored
     * in rData, as H5Tvlen_reclaim only frees the data these point to.
     */
    status = H5Dvlen_reclaim(memtype, dataspaceID, H5P_DEFAULT, rData.data());
  }

  return strings;
}

template <typename T>
std::shared_ptr<AbstractDataStore<T>> DatasetIO::readAsDataStore() const
{
  auto dataset = open();
  size_t numElements = getNumElements();

  ShapeType tupleShape{numElements};
  ShapeType componentShape{1};

  // In-core branch: allocate a plain in-memory DataStore directly. The OOC
  // branch is intercepted upstream by the data store import handler.
  std::shared_ptr<AbstractDataStore<T>> dataStorePtr = std::make_shared<DataStore<T>>(tupleShape, componentShape, T{});
  dataStorePtr->readHdf5(*this);
  return dataStorePtr;
}

template <typename T>
std::shared_ptr<AbstractDataStore<T>> DatasetIO::readAsDataStore(const ShapeType& tupleShape, const ShapeType& componentShape) const
{
  auto dataset = open();
  size_t numElements = getNumElements();

  size_t numTuples = std::accumulate(tupleShape.begin(), tupleShape.end(), static_cast<size_t>(1), std::multiplies<>());
  size_t numComponents = std::accumulate(componentShape.begin(), componentShape.end(), static_cast<size_t>(1), std::multiplies<>());
  if(numTuples * numComponents != numElements)
  {
    return nullptr;
  }

  // In-core branch: allocate a plain in-memory DataStore directly. The OOC
  // branch is intercepted upstream by the data store import handler.
  std::shared_ptr<AbstractDataStore<T>> dataStorePtr = std::make_shared<DataStore<T>>(tupleShape, componentShape, T{});
  dataStorePtr->readHdf5(*this);
  return dataStorePtr;
}

template <typename T>
std::vector<T> DatasetIO::readAsVector() const
{
  // if(!isValid())
  //{
  //   return {};
  // }

  auto dataset = open();
  size_t numElements = getNumElements();

  T* data = new T[numElements];
  nonstd::span<T> span(data, numElements);

  Result<> result = readIntoSpan<T>(span);
  if(result.invalid())
  {
    delete[] data;
    return {};
  }

  std::vector<T> output(numElements);
  for(usize i = 0; i < numElements; i++)
  {
    output[i] = data[i];
  }
  delete[] data;
  return output;
}

template <class T>
nx::core::Result<> DatasetIO::readIntoSpan(nonstd::span<T> data) const
{
  const hid_t datasetId = open();
  if(datasetId <= 0)
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getNamePath()));
  }

  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, fmt::format("DatasetReader error: Unsupported span data type for dataset '{}' in file '{}'", getNamePath(), getFilePath().string()));
  }

  // getDimensions() manages its own HDF5 lock.
  const std::vector<usize> usizeDims = getDimensions();
  const hsize_t totalElements = std::accumulate(usizeDims.begin(), usizeDims.end(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());

  if(data.size() != totalElements)
  {
    return MakeErrorResult(-1006, "DatasetReader error: Span size does not match the number of elements to read.");
  }

  // H5Dread inflates under the process-wide HDF5 lock. Eligible chunked datasets
  // instead inflate on worker threads outside that lock.
  // Eligibility calls manage their own nonrecursive HDF5 lock.
  const std::vector<usize> usizeChunkDims = getChunkDimensions(); // empty when the dataset is not chunked
  if(!usizeChunkDims.empty())
  {
    // Model the complete flat array as one-component tuple space. Component shape
    // is not available or necessary for row-major scattering.
    std::vector<uint64> tupleShape(usizeDims.begin(), usizeDims.end());
    std::vector<uint64> chunkShape(usizeChunkDims.begin(), usizeChunkDims.end());

    // A GroupIO child can lack m_FilePath. Resolve the physical path from its
    // dataset handle for positional chunk reads. Failure keeps the codec ineligible.
    std::filesystem::path codecFilePath = getFilePath();
    if(codecFilePath.empty())
    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      const ssize_t nameLen = H5Fget_name(datasetId, nullptr, 0);
      if(nameLen > 0)
      {
        std::string nameBuffer(static_cast<usize>(nameLen) + 1, '\0');
        if(H5Fget_name(datasetId, nameBuffer.data(), nameBuffer.size()) > 0)
        {
          nameBuffer.resize(static_cast<usize>(nameLen));
          codecFilePath = nameBuffer;
        }
      }
    }
    HDF5::ParallelChunkCodec codec(codecFilePath, getNamePath(), tupleShape, chunkShape, /*componentShape=*/{}, sizeof(T), datasetId);
    if(codec.isEligible())
    {
      // An exception falls back to serial H5Dread. Successful codec output matches
      // the serial row-major result.
      try
      {
        const uint64 numChunks = HDF5::getNumberOfChunks(tupleShape, chunkShape);
        std::vector<uint64> allChunkIndices(numChunks);
        std::iota(allChunkIndices.begin(), allChunkIndices.end(), uint64{0});

        nonstd::span<std::byte> out(reinterpret_cast<std::byte*>(data.data()), data.size_bytes());
        // The codec locks metadata internally and inflates outside the lock.
        codec.inflateChunksIntoSpan(out, allChunkIndices);
        return {};
      } catch(const std::exception&)
      {
        // Use the serial fallback.
      }
    }
  }

  // The serial fallback locks only bare HDF5 calls. Error formatting stays outside
  // because path accessors can acquire the same nonrecursive lock.
  std::vector<hsize_t> memDims(usizeDims.begin(), usizeDims.end());

  hid_t fileSpaceId = H5I_INVALID_HID;
  hid_t memSpaceId = H5I_INVALID_HID;
  herr_t readStatus = 0;
  {
    std::lock_guard<std::mutex> hdf5Lock(HDF5::Support::ApiLock());
    fileSpaceId = H5Dget_space(datasetId);
    if(fileSpaceId >= 0)
    {
      memSpaceId = H5Screate_simple(static_cast<int>(memDims.size()), memDims.data(), nullptr);
      if(memSpaceId >= 0)
      {
        readStatus = H5Dread(datasetId, dataType, memSpaceId, fileSpaceId, H5P_DEFAULT, data.data());
        H5Sclose(memSpaceId);
      }
      H5Sclose(fileSpaceId);
    }
  }

  if(fileSpaceId < 0)
  {
    return MakeErrorResult(-1002, fmt::format("DatasetReader error: Unable to open the dataspace for dataset '{}' in file '{}'", getNamePath(), getFilePath().string()));
  }
  if(memSpaceId < 0)
  {
    return MakeErrorResult(-1007, "DatasetReader error: Unable to create memory dataspace.");
  }
  if(readStatus < 0)
  {
    return MakeErrorResult(-1008, fmt::format("DatasetReader error: Unable to read dataset '{}'", getNamePath()));
  }

  return {};
}

template <>
nx::core::Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool> data) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getNamePath()));
  }

  std::vector<H5_BOOL_TYPE> data2(data.begin(), data.end());
  nonstd::span<H5_BOOL_TYPE> span2(data2.data(), data2.size());
  auto result = readIntoSpan(span2);
  std::copy(span2.begin(), span2.end(), data.begin());
  return result;
}

template <class T>
Result<> DatasetIO::readIntoSpan(nonstd::span<T> data, const std::optional<std::vector<uint64>>& start, const std::optional<std::vector<uint64>>& count) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} / {}", getFilePath().string(), getNamePath()));
  }

  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, "DatasetReader error: Unsupported span data type.");
  }

  // open() manages its own lock and must run before the leaf critical section.
  // Dataspaces close inside the lock. Error formatting stays outside it.
  const hid_t datasetId = open();

  int errorCode = 0;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    hid_t fileSpaceId = H5Dget_space(datasetId);
    if(fileSpaceId < 0)
    {
      errorCode = -1002;
    }
    else
    {
      std::vector<hsize_t> memDims;
      int rank = H5Sget_simple_extent_ndims(fileSpaceId);
      std::vector<hsize_t> dims(rank), maxDims(rank);
      H5Sget_simple_extent_dims(fileSpaceId, dims.data(), maxDims.data());
      if(start.has_value() && count.has_value())
      {
        // Select the explicit start and count.
#if defined(__APPLE__)
        std::vector<unsigned long long> startData(start->begin(), start->end());
        std::vector<unsigned long long> countVec(count->begin(), count->end());
        if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startData.data(), NULL, countVec.data(), NULL) < 0)
#else
        if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start->data(), NULL, count->data(), NULL) < 0)
#endif
        {
          errorCode = -1003;
        }
        memDims = std::vector<hsize_t>(count->begin(), count->end());
      }
      else if(start.has_value())
      {
        // Extend an explicit start through the remaining dataset.
        std::vector<hsize_t> countRemaining(rank);
        for(int i = 0; i < rank; ++i)
        {
          countRemaining[i] = dims[i] - start->at(i);
        }
#if defined(__APPLE__)
        std::vector<unsigned long long> startData(start->begin(), start->end());
        if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startData.data(), NULL, countRemaining.data(), NULL) < 0)
#else
        if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start->data(), NULL, countRemaining.data(), NULL) < 0)
#endif
        {
          errorCode = -1004;
        }
        memDims = countRemaining;
      }
      else if(count.has_value())
      {
        // Select the requested count from the dataset origin.
        std::vector<hsize_t> startZeros(rank, 0);
#if defined(__APPLE__)
        std::vector<unsigned long long> countVec(count->begin(), count->end());
        if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startZeros.data(), NULL, countVec.data(), NULL) < 0)
#else
        if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startZeros.data(), NULL, count->data(), NULL) < 0)
#endif
        {
          errorCode = -1005;
        }
        memDims = std::vector<hsize_t>(count->begin(), count->end());
      }
      else
      {
        // Select the complete dataset.
        memDims = dims;
      }

      if(errorCode == 0)
      {
        const hsize_t totalElements = std::accumulate(memDims.begin(), memDims.end(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());
        if(data.size() != totalElements)
        {
          errorCode = -1006;
        }
        else
        {
          hid_t memSpaceId = H5Screate_simple(memDims.size(), memDims.data(), NULL);
          if(memSpaceId < 0)
          {
            errorCode = -1007;
          }
          else
          {
            if(H5Dread(datasetId, dataType, memSpaceId, fileSpaceId, H5P_DEFAULT, data.data()) < 0)
            {
              errorCode = -1008;
            }
            H5Sclose(memSpaceId);
          }
        }
      }
      H5Sclose(fileSpaceId);
    }
  }

  switch(errorCode)
  {
  case 0:
    return {};
  case -1002:
    return MakeErrorResult(-1002, "DatasetReader error: Unable to open the dataspace.");
  case -1003:
  case -1004:
  case -1005:
    return MakeErrorResult(errorCode, "DatasetReader error: Unable to select hyperslab.");
  case -1006:
    return MakeErrorResult(-1006, "DatasetReader error: Span size does not match the number of elements to read.");
  case -1007:
    return MakeErrorResult(-1007, "DatasetReader error: Unable to create memory dataspace.");
  default:
    return MakeErrorResult(-1008, fmt::format("DatasetReader error: Unable to read dataset '{}'", getNamePath()));
  }
}

template <>
Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool> data, const std::optional<std::vector<uint64>>& start, const std::optional<std::vector<uint64>>& count) const
{
  if(!isValid())
  {
    return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getNamePath()));
  }
  if(start->size() != count->size())
  {
    return MakeErrorResult(-506,
                           fmt::format("Cannot read HDF5 data at {} called {}. Requested dimensions do not match: '{}', '{}'", getFilePath().string(), getNamePath(), start->size(), count->size()));
  }

  std::vector<H5_BOOL_TYPE> data2(data.begin(), data.end());
  nonstd::span<H5_BOOL_TYPE> span2(data2.data(), data2.size());
  auto result = readIntoSpan(span2, start, count);
  std::copy(span2.begin(), span2.end(), data.begin());
  return result;
}

#if 0
template <class T>
Result<> DatasetIO::readChunkIntoSpan(nonstd::span<T> data, nonstd::span<const usize> chunkOffset, nonstd::span<const usize> chunkDims) const
{
  if(!isValid())
  {
    return MakeErrorResult(-1000, "DatasetReader error: DatasetReader object is not valid.");
  }

  hid_t dataType = Support::HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1001, "DatasetReader error: Unsupported span data type.");
  }

  hid_t datasetId = getId();
  hid_t fileSpaceId = H5Dget_space(datasetId);
  if(fileSpaceId < 0)
  {
    return MakeErrorResult(-1002, "DatasetReader error: Unable to open the dataspace.");
  }

  hsize_t totalElements;
  std::vector<hsize_t> memDims;
  int rank = H5Sget_simple_extent_ndims(fileSpaceId);
  std::vector<hsize_t> dims(rank), maxDims(rank);
  H5Sget_simple_extent_dims(fileSpaceId, dims.data(), maxDims.data());
  if(start.has_value() && count.has_value())
  {
    // Both start and count are provided
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start->data(), NULL, count->data(), NULL) < 0)
    {
      return MakeErrorResult(-1003, "DatasetReader error: Unable to select hyperslab.");
    }
    memDims = count.value();
  }
  else if(start.has_value())
  {
    // Only start is provided
    std::vector<hsize_t> countRemaining(rank);
    for(int i = 0; i < rank; ++i)
    {
      countRemaining[i] = dims[i] - start->at(i);
    }
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start->data(), NULL, countRemaining.data(), NULL) < 0)
    {
      return MakeErrorResult(-1004, "DatasetReader error: Unable to select hyperslab.");
    }
    memDims = countRemaining;
  }
  else if(count.has_value())
  {
    // Only count is provided
    std::vector<hsize_t> startZeros(rank, 0);
    if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startZeros.data(), NULL, count->data(), NULL) < 0)
    {
      return MakeErrorResult(-1005, "DatasetReader error: Unable to select hyperslab.");
    }
    memDims = count.value();
  }
  else
  {
    // Neither start nor count is provided
    memDims = dims;
  }

  totalElements = std::accumulate(memDims.begin(), memDims.end(), static_cast<hsize_t>(1), std::multiplies<hsize_t>());

  if(data.size() != totalElements)
  {
    return MakeErrorResult(-1006, "DatasetReader error: Span size does not match the number of elements to read.");
  }

  hid_t memSpaceId = H5Screate_simple(memDims.size(), memDims.data(), NULL);
  if(memSpaceId < 0)
  {
    return MakeErrorResult(-1007, "DatasetReader error: Unable to create memory dataspace.");
  }

  if(H5Dread(datasetId, dataType, memSpaceId, fileSpaceId, H5P_DEFAULT, data.data()) < 0)
  {
    H5Sclose(memSpaceId);
    H5Sclose(fileSpaceId);
    return MakeErrorResult(-1008, fmt::format("DatasetReader error: Unable to read dataset '{}'", getNamePath()));
  }

  H5Sclose(memSpaceId);
  H5Sclose(fileSpaceId);

  return {};
}
#endif

// template <>
// Result<> DatasetIO::readChunkIntoSpan<bool>(nonstd::span<bool> data, nonstd::span<const usize> chunkOffset, nonstd::span<const usize> chunkDims) const
//{
//   if(!isValid())
//   {
//     return MakeErrorResult(-505, fmt::format("Cannot open HDF5 data at {} called {}", getFilePath().string(), getNamePath()));
//   }
//
//   // DataSet does not support bool data.
//   std::vector<uint8> data2(data.size());
//   auto dataset = openH5Dataset();
//   dataset.select({chunkOffset.data(), chunkDims.data()}).read(data2);
//   // Copy data back into span<bool>
//   std::copy(data2.begin(), data2.end(), data.begin());
//   return {};
// }

std::vector<nx::core::usize> DatasetIO::getChunkDimensions() const
{
  // Resolve self-locking accessors before one property-list leaf critical section.
  const hid_t selfId = open();
  const usize numDims = getDimensions().size();

  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  auto propertyListId = H5Dget_create_plist(selfId);
  H5D_layout_t layout = H5Pget_layout(propertyListId);
  std::vector<usize> result;
  if(layout == H5D_CHUNKED)
  {
    std::vector<hsize_t> chunk_dims_out(numDims);
    H5Pget_chunk(propertyListId, static_cast<int>(numDims), chunk_dims_out.data());
    result = std::vector<usize>(chunk_dims_out.begin(), chunk_dims_out.end());
  }
  if(propertyListId >= 0)
  {
    H5Pclose(propertyListId);
  }
  return result;
}

std::vector<nx::core::usize> DatasetIO::getDimensions() const
{
  // Resolve self-locking accessors before one dataspace leaf critical section.
  const hid_t selfId = getId();
  const hid_t classType = getClassType();

  std::vector<hsize_t> dims;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    auto dataspaceId = H5Dget_space(selfId);
    if(dataspaceId >= 0)
    {
      if(classType == H5T_STRING)
      {
        auto typeId = H5Dget_type(selfId);
        size_t typeSize = H5Tget_size(typeId);
        H5Tclose(typeId);
        dims = {typeSize};
      }
      else
      {
        size_t rank = H5Sget_simple_extent_ndims(dataspaceId);
        std::vector<hsize_t> hdims(rank, 0);
        /* Get dimensions */
        auto error = H5Sget_simple_extent_dims(dataspaceId, hdims.data(), nullptr);
        if(error < 0)
        {
          std::cout << "Error Getting Attribute dims" << std::endl;
          H5Sclose(dataspaceId);
          return std::vector<nx::core::usize>(dims.begin(), dims.end());
        }
        // Copy the dimensions into the dims vector
        dims.clear(); // Erase everything in the Vector
        dims.resize(rank);
        std::copy(hdims.cbegin(), hdims.cend(), dims.begin());
      }
      H5Sclose(dataspaceId);
    }
  }
  return std::vector<nx::core::usize>(dims.begin(), dims.end());
}

template <typename T>
Result<> DatasetIO::writeSpan(const DimsType& dims, nonstd::span<const T> values)
{
  Result<> returnError = {};
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1, fmt::format("DataType was unknown when writing dataset '{}' in file '{}'", getNamePath(), getFilePath().string()));
  }

  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });

  // Each wrapper call manages its own nonrecursive HDF5 lock. No lock spans
  // another public wrapper call.
  hid_t dataspaceId = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);
  }

  if(dataspaceId >= 0)
  {
    auto dcplResult = BuildChunkedDeflateDcpl(dims, sizeof(T), m_CompressionLevel);
    if(dcplResult.invalid())
    {
      returnError = MakeErrorResult(dcplResult.errors().front().code, fmt::format("Error building dataset creation property list for dataset '{}' in file '{}': {}", getNamePath(),
                                                                                  getFilePath().string(), dcplResult.errors().front().message));
    }
    else
    {
      const hid_t dcpl = dcplResult.value();
      auto datasetId = createOrOpenDataset<T>(dataspaceId, dcpl);
      ErrorType error = 0;
      if(datasetId >= 0)
      {
        // H5Dwrite compresses under the process-wide HDF5 lock. Eligible datasets
        // instead compress on workers and commit precompressed chunks serially.
        // Eligibility calls manage their own nonrecursive HDF5 lock.
        bool wroteViaCodec = false;
        const std::vector<usize> usizeChunkDims = getChunkDimensions(); // empty when contiguous (level 0 / small array)
        if(!usizeChunkDims.empty())
        {
          // Model the complete flat array as one-component tuple space.
          std::vector<uint64> tupleShape(dims.begin(), dims.end());
          std::vector<uint64> chunkShape(usizeChunkDims.begin(), usizeChunkDims.end());
          HDF5::ParallelChunkCodec codec(getFilePath(), getNamePath(), tupleShape, chunkShape, /*componentShape=*/{}, sizeof(T), datasetId);
          if(codec.isEligible())
          {
            const uint64 numChunks = HDF5::getNumberOfChunks(tupleShape, chunkShape);
            std::vector<uint64> allChunkIndices(numChunks);
            std::iota(allChunkIndices.begin(), allChunkIndices.end(), uint64{0});

            const nonstd::span<const std::byte> source(reinterpret_cast<const std::byte*>(values.data()), values.size_bytes());
            // The codec manages metadata and chunk-commit locks internally. A false
            // result selects the serial H5Dwrite fallback.
            std::string codecError;
            if(codec.deflateSpanIntoChunks(source, allChunkIndices, /*sourceStartTuple=*/0, &codecError))
            {
              wroteViaCodec = true;
            }
          }
        }

        if(!wroteViaCodec)
        {
          // The serial fallback locks only H5Dwrite. Error formatting stays outside.
          const void* data = static_cast<const void*>(values.data());
          {
            std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
            error = H5Dwrite(datasetId, dataType, H5S_ALL, H5S_ALL, H5P_DEFAULT, data);
          }
          if(error < 0)
          {
            returnError = MakeErrorResult(error, fmt::format("Error writing data to dataset '{}' in file '{}'", getNamePath(), getFilePath().string()));
          }
        }
      }
      else
      {
        returnError = MakeErrorResult(datasetId, fmt::format("Error creating dataset '{}' in file '{}'", getNamePath(), getFilePath().string()));
      }

      if(dcpl != H5P_DEFAULT)
      {
        std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
        H5Pclose(dcpl);
      }
    }
  }
  else
  {
    returnError = MakeErrorResult(dataspaceId, fmt::format("Error opening dataspace for dataset '{}' in file '{}'", getNamePath(), getFilePath().string()));
  }
  return returnError;
}

template <>
Result<> DatasetIO::writeSpan<bool>(const DimsType& dims, nonstd::span<const bool> values)
{
  try
  {
    // DataSet does not support bool data.
    const std::vector<H5_BOOL_TYPE> data2(values.begin(), values.end());
    nonstd::span<const H5_BOOL_TYPE> span2(data2.data(), data2.size());
    return writeSpan(dims, span2);
  } catch(const std::exception& e)
  {
    return MakeErrorResult(-853535, fmt::format("Failed to write to Dataset with error: '{}'", e.what()));
  }
  return {};
}

template <typename T>
Result<> DatasetIO::createEmptyDataset(const DimsType& dims)
{
  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1020, "createEmptyDataset error: Unsupported data type.");
  }

  // Each dataset-creation wrapper manages its own nonrecursive HDF5 lock.
  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  hid_t dataspaceId = H5I_INVALID_HID;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    dataspaceId = H5Screate_simple(static_cast<int>(hDims.size()), hDims.data(), nullptr);
  }
  if(dataspaceId < 0)
  {
    return MakeErrorResult(-1021, "createEmptyDataset error: Unable to create dataspace.");
  }

  // Use the full-span compression policy before later OOC hyperslab transfers.
  auto dcplResult = BuildChunkedDeflateDcpl(dims, sizeof(T), m_CompressionLevel);
  if(dcplResult.invalid())
  {
    {
      std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
      H5Sclose(dataspaceId);
    }
    return MakeErrorResult(dcplResult.errors().front().code, fmt::format("createEmptyDataset error: unable to build dataset creation property list for dataset '{}' in file '{}': {}", getNamePath(),
                                                                         getFilePath().string(), dcplResult.errors().front().message));
  }
  const hid_t dcpl = dcplResult.value();

  // createOrOpenDataset() locks itself. Close local properties in a later leaf scope.
  auto datasetId = createOrOpenDataset<T>(dataspaceId, dcpl);
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    H5Sclose(dataspaceId);
    if(dcpl != H5P_DEFAULT)
    {
      H5Pclose(dcpl);
    }
  }
  if(datasetId < 0)
  {
    return MakeErrorResult(-1022, "createEmptyDataset error: Unable to create dataset.");
  }

  return {};
}

template <typename T>
Result<> DatasetIO::writeSpanHyperslab(nonstd::span<const T> values, const std::vector<uint64>& start, const std::vector<uint64>& count)
{
  if(!isValid())
  {
    return MakeErrorResult(-506, fmt::format("Cannot open HDF5 data at {} / {}", getFilePath().string(), getNamePath()));
  }

  hid_t dataType = HdfTypeForPrimitive<T>();
  if(dataType == -1)
  {
    return MakeErrorResult(-1010, "writeSpanHyperslab error: Unsupported data type.");
  }

  // open() locks itself before the leaf hyperslab critical section.
  const hid_t datasetId = open();

  int errorCode = 0;
  herr_t writeError = 0;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    hid_t fileSpaceId = H5Dget_space(datasetId);
    if(fileSpaceId < 0)
    {
      errorCode = -1011;
    }
    else
    {
      // macOS uses a different hsize_t underlying type. Copy offsets to avoid pointer casts.
#if defined(__APPLE__)
      std::vector<unsigned long long> startVec(start.begin(), start.end());
      std::vector<unsigned long long> countVec(count.begin(), count.end());
      if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, startVec.data(), NULL, countVec.data(), NULL) < 0)
#else
      if(H5Sselect_hyperslab(fileSpaceId, H5S_SELECT_SET, start.data(), NULL, count.data(), NULL) < 0)
#endif
      {
        errorCode = -1012;
      }
      else
      {
        // Match the memory dataspace to the selected extent.
        std::vector<hsize_t> memDims(count.begin(), count.end());
        hid_t memSpaceId = H5Screate_simple(static_cast<int>(memDims.size()), memDims.data(), nullptr);
        if(memSpaceId < 0)
        {
          errorCode = -1013;
        }
        else
        {
          writeError = H5Dwrite(datasetId, dataType, memSpaceId, fileSpaceId, H5P_DEFAULT, values.data());
          if(writeError < 0)
          {
            errorCode = -1014;
          }
          H5Sclose(memSpaceId);
        }
      }
      H5Sclose(fileSpaceId);
    }
  }

  switch(errorCode)
  {
  case 0:
    return {};
  case -1011:
    return MakeErrorResult(-1011, "writeSpanHyperslab error: Unable to open the dataspace.");
  case -1012:
    return MakeErrorResult(-1012, "writeSpanHyperslab error: Unable to select hyperslab.");
  case -1013:
    return MakeErrorResult(-1013, "writeSpanHyperslab error: Unable to create memory dataspace.");
  default:
    return MakeErrorResult(-1014, fmt::format("writeSpanHyperslab error: H5Dwrite failed with error {}", writeError));
  }
}

template <typename T>
nx::core::Result<ChunkedDataInfo> DatasetIO::initChunkedDataset(const DimsType& h5Dims, const DimsType& chunkDims) const
{
  ChunkedDataInfo dataInfo;
  std::vector<hsize_t> h5DimsVec(h5Dims.begin(), h5Dims.end());

  // Create the dataspace before the separately locked dataset open or create call.
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    dataInfo.dataspaceId = H5Screate_simple(h5Dims.size(), h5DimsVec.data(), nullptr);
  }
  if(dataInfo.dataspaceId < 0)
  {
    return MakeErrorResult<ChunkedDataInfo>(-120, "Failed to open HDF5 Dataspace");
  }

  dataInfo.dataType = HdfTypeForPrimitive<T>();
  if(dataInfo.dataType == -1)
  {
    return MakeErrorResult<ChunkedDataInfo>(-100, "DataType was unknown");
  }

  // dataInfo.chunkProp = CreateH5DatasetChunkProperties(chunkDims);
  // dataInfo.datasetId = createOrOpenDataset(dataInfo.dataType, dataInfo.dataspaceId, dataInfo.chunkProp);
  dataInfo.datasetId = createOrOpenDataset(dataInfo.dataType, dataInfo.dataspaceId);
  if(dataInfo.datasetId < 0)
  {
    return MakeErrorResult<ChunkedDataInfo>(-110, "Failed to open HDF5 Dataset");
  }

  // dataInfo.transferProp = H5Pcreate(H5P_DATASET_XFER);
  // if(dataInfo.transferProp < 0)
  //{
  //   return MakeErrorResult<ChunkedDataInfo>(-130, "Failed to create HDF5 transfer properties");
  // }

  setId(dataInfo.datasetId);
  return {dataInfo};
}

hid_t DatasetIO::CreateH5DatasetChunkProperties(const DimsType& chunkDims)
{
  std::vector<hsize_t> hDims(chunkDims.size());
  std::transform(chunkDims.begin(), chunkDims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
  auto cparms = H5Pcreate(H5P_DATASET_CREATE);
  auto status = H5Pset_chunk(cparms, hDims.size(), hDims.data());
  if(status < 0)
  {
    return H5P_DEFAULT;
  }
  return cparms;
}

nx::core::Result<> DatasetIO::closeChunkedDataset(const ChunkedDataInfo& datasetInfo) const
{
  // herr_t error = H5Pclose(datasetInfo.transferProp);
  // if(error < 0)
  //{
  //   return MakeErrorResult(error, "Error Closing Transfer Property");
  // }
  /*herr_t error = H5Dclose(datasetInfo.datasetId);
  if(error < 0)
  {
    return MakeErrorResult(error, "Error Closing DataSet");
  }*/
  // else
  //{
  //   setId(-1);
  // }

  /*error = H5Pclose(datasetInfo.chunkProp);
  if(error < 0)
  {
    return MakeErrorResult(error, "Error Closing Chunk Property");
  }*/
  herr_t error = 0;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    error = H5Sclose(datasetInfo.dataspaceId);
  }
  if(error < 0)
  {
    return MakeErrorResult(error, "Error Closing Dataspace");
  }

  return {};
}

template <typename T>
nx::core::Result<> DatasetIO::readChunk(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<T> values, const DimsType& chunkShape, nonstd::span<const usize> offset) const
{
  if(chunkShape.size() != dims.size())
  {
    std::string ss = fmt::format("Dimension mismatch when writing DataStore chunk. Num Shape Dimensions: {} Num Chunk Dimensions: {}", dims.size(), chunkShape.size());
    return MakeErrorResult(k_DimensionMismatchError, ss);
  }

  Result<> returnError = {};
  herr_t error = 0;
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = chunkInfo.dataType;
  if(dataType == -1)
  {
    return MakeErrorResult(-100, "DataType was unknown");
  }
  std::vector<hsize_t> hDims(dims.size());
  std::transform(dims.begin(), dims.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  hid_t dataspaceId = chunkInfo.dataspaceId;
  if(dataspaceId >= 0)
  {
    // auto result = findAndDeleteAttribute();
    // if(result.invalid())
    //{
    //   returnError = MakeErrorResult(result.errors()[0].code, "Error Removing Existing Attribute");
    // }
    // else
    {
      /* Create the attribute. */
      auto h5Id = chunkInfo.datasetId;
      if(h5Id >= 0)
      {
        /* Write the attribute data. */
        void* data = static_cast<void*>(values.data());
        std::vector<hsize_t> offsetVec(offset.begin(), offset.end());
        std::vector<hsize_t> chunkShapeVec(chunkShape.begin(), chunkShape.end());
        {
          std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
          const hid_t plistId = H5Dget_create_plist(h5Id);
          if(plistId > 0)
          {
            H5Pclose(plistId);
          }
          else
          {
            std::cout << "Error Writing Chunk: No PList ID found" << std::endl;
          }

          // Select hyperslab
          error = H5Sselect_hyperslab(dataspaceId, H5S_SELECT_SET, offsetVec.data(), NULL, chunkShapeVec.data(), NULL);

          // Create memory dataspace for the hyperslab
          const hid_t memspaceId = H5Screate_simple(rank, chunkShapeVec.data(), NULL);

          // Read hyperslab from the dataset
          error = H5Dread(h5Id, HdfTypeForPrimitive<T>(), memspaceId, dataspaceId, H5P_DEFAULT, data);

          H5Sclose(memspaceId);
        }
        if(error < 0)
        {
          returnError = MakeErrorResult(error, "Error Writing Dataset Chunk");
        }
      }
      else
      {
        returnError = MakeErrorResult(h5Id, "Error Creating Dataset Chunk");
      }
    }
    /* Close the dataspace after reading all required chunks. */
  }
  else
  {
    returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace");
  }
  return returnError;
}

template <>
Result<> DatasetIO::readChunk<bool>(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<bool> values, const DimsType& chunkShape, nonstd::span<const usize> offset) const
{
  std::vector<H5_BOOL_TYPE> h5ValuesVec(values.begin(), values.end());
  nonstd::span<H5_BOOL_TYPE> h5Values(h5ValuesVec.data(), h5ValuesVec.size());

  auto result = readChunk(chunkInfo, dims, h5Values, chunkShape, offset);
  if(result.invalid())
  {
    return result;
  }
  std::copy(h5Values.begin(), h5Values.end(), values.begin());
  return {};
}

template <typename T>
Result<> DatasetIO::writeChunk(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<const T> values, const DimsType& chunkShape, const DimsType& trueChunkDims,
                               nonstd::span<const usize> offset)
{
  if(chunkShape.size() != dims.size())
  {
    std::string ss = fmt::format("Dimension mismatch when writing DataStore chunk. Num Shape Dimensions: {} Num Chunk Dimensions: {}", dims.size(), chunkShape.size());
    return MakeErrorResult(k_DimensionMismatchError, ss);
  }

  Result<> returnError = {};
  herr_t error = 0;
  int32_t rank = static_cast<int32_t>(dims.size());
  hid_t dataType = chunkInfo.dataType;
  if(dataType == -1)
  {
    return MakeErrorResult(-100, "DataType was unknown");
  }
  // std::vector<hsize_t> hDims(chunkShape.size());
  // std::transform(chunkShape.begin(), chunkShape.end(), hDims.begin(), [](DimsType::value_type x) { return static_cast<hsize_t>(x); });
  // hid_t dataspaceId = H5Screate_simple(rank, hDims.data(), nullptr);
  hid_t dataspaceId = chunkInfo.dataspaceId;
  if(dataspaceId >= 0)
  {
    /* Create the attribute. */
    auto h5Id = chunkInfo.datasetId;
    if(h5Id >= 0)
    {
      /* Write the attribute data. */
      const void* data = static_cast<const void*>(values.data());

      // Select hyperslab
      std::vector<hsize_t> offsetVec(offset.begin(), offset.end());
      std::vector<hsize_t> chunkShapeVec(chunkShape.begin(), chunkShape.end());
      std::vector<hsize_t> trueChunkShapeVec(trueChunkDims.begin(), trueChunkDims.end());

      // Open chunk handles permit one leaf lock around selection, write, and cleanup.
      {
        std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
        error = H5Sselect_hyperslab(dataspaceId, H5S_SELECT_SET, offsetVec.data(), NULL, trueChunkShapeVec.data(), NULL);

        // Create memory dataspace for the hyperslab
        hid_t memspace_id = H5Screate_simple(rank, chunkShapeVec.data(), NULL);

        if(chunkShape != trueChunkDims)
        {
          std::vector<hsize_t> chunkOffset(rank, 0);
          std::vector<hsize_t> count(rank, 1);

          error = H5Sselect_hyperslab(memspace_id, H5S_SELECT_SET, chunkOffset.data(), nullptr, count.data(), trueChunkShapeVec.data());
        }

        // Write hyperslab into the dataset
        error = H5Dwrite(h5Id, HdfTypeForPrimitive<T>(), memspace_id, dataspaceId, H5P_DEFAULT, data);

        H5Sclose(memspace_id);
      }

      if(error < 0)
      {
        returnError = MakeErrorResult(error, "Error Writing Dataset Chunk");
      }
    }
    else
    {
      returnError = MakeErrorResult(h5Id, "Error Creating Dataset Chunk");
    }
  }
  else
  {
    returnError = MakeErrorResult(dataspaceId, "Error Opening Dataspace");
  }
  return returnError;
}

template <>
nx::core::Result<> DatasetIO::writeChunk<bool>(const ChunkedDataInfo& chunkInfo, const DimsType& dims, nonstd::span<const bool> values, const DimsType& chunkShape, const DimsType& trueChunkDims,
                                               nonstd::span<const usize> offset)
{
  std::vector<H5_BOOL_TYPE> h5ValuesVec(values.begin(), values.end());
  nonstd::span<const H5_BOOL_TYPE> h5Values(h5ValuesVec.data(), h5ValuesVec.size());

  return writeChunk(chunkInfo, dims, h5Values, chunkShape, trueChunkDims, offset);
}

nx::core::Result<> DatasetIO::writeString(const std::string& text)
{
  // if(!isValid())
  //{
  //   return MakeErrorResult(-100, "Cannot Write to Invalid DatasetIO");
  // }

  herr_t error = 0;
  Result<> returnError = {};

  // Pure member access occurs before one leaf lock around all HDF5 string calls.
  const hid_t parentId = getParentId();
  const std::string namePath = getNamePath();
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());

    /* create a string data type */
    hid_t typeId;
    if((typeId = H5Tcopy(H5T_C_S1)) >= 0)
    {
      size_t size = text.size() + 1;
      if(H5Tset_size(typeId, size) >= 0)
      {
        if(H5Tset_strpad(typeId, H5T_STR_NULLTERM) >= 0)
        {
          /* Create the data space for the dataset. */
          hid_t dataspaceId;
          if((dataspaceId = H5Screate(H5S_SCALAR)) >= 0)
          {
            /* Create or open the dataset. */
            hid_t id = H5Dcreate(parentId, namePath.c_str(), typeId, dataspaceId, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            if(id >= 0)
            {
              if(!text.empty())
              {
                error = H5Dwrite(id, typeId, H5S_ALL, H5S_ALL, H5P_DEFAULT, text.c_str());
                if(error < 0)
                {
                  returnError = MakeErrorResult(error, "Error Writing String Data");
                }
              }
            }
            else
            {
              returnError = {};
            }
            H5Dclose(id);
          }
          if(H5Sclose(dataspaceId) < 0)
          {
            returnError = MakeErrorResult(error, "Error closing Dataspace");
          }
        }
      }
      // if(H5Sclose(typeId) < 0)
      //{
      //   returnError = MakeErrorResult(error, "Error closing DataType");
      // }
    }
  }
  return returnError;
}

nx::core::Result<> DatasetIO::writeVectorOfStrings(const std::vector<std::string>& text)
{
  // if(!isValid())
  //{
  //   return MakeErrorResult(-100, "Cannot Write to Invalid DatasetIO");
  // }

  // Pure member access occurs before one leaf lock around all HDF5 string calls.
  const hid_t parentId = getParentId();
  const std::string namePath = getNamePath();
  hid_t dataspaceID = -1;
  hid_t memSpace = -1;
  hid_t datatype = -1;
  herr_t error = -1;
  Result<> returnError = {};

  std::array<hsize_t, 1> dims = {text.size()};
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    if((dataspaceID = H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr)) >= 0)
    {
      dims[0] = 1;

      if((memSpace = H5Screate_simple(static_cast<int>(dims.size()), dims.data(), nullptr)) >= 0)
      {
        datatype = H5Tcopy(H5T_C_S1);
        H5Tset_size(datatype, H5T_VARIABLE);

        auto datasetId = H5Dcreate(parentId, namePath.c_str(), datatype, dataspaceID, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
        setId(datasetId);
        if(datasetId >= 0)
        {
          // Select the "memory" to be written out - just 1 record.
          hsize_t dataset_offset[] = {0};
          hsize_t dataset_count[] = {1};
          H5Sselect_hyperslab(memSpace, H5S_SELECT_SET, dataset_offset, nullptr, dataset_count, nullptr);
          hsize_t pos = 0;
          for(const auto& element : text)
          {
            // Select the file position, 1 record at position 'pos'
            hsize_t element_count[] = {1};
            hsize_t element_offset[] = {pos};
            pos++;
            H5Sselect_hyperslab(dataspaceID, H5S_SELECT_SET, element_offset, nullptr, element_count, nullptr);
            const char* strPtr = element.c_str();
            error = H5Dwrite(datasetId, datatype, memSpace, dataspaceID, H5P_DEFAULT, &strPtr);
            if(error < 0)
            {
              std::cout << "Error Writing String Data: " __FILE__ << "(" << __LINE__ << ")" << std::endl;
              returnError = MakeErrorResult(error, "Error Writing String Data");
            }
          }
          // H5Dclose(datasetId);
        }
        H5Tclose(datatype);
        H5Sclose(memSpace);
      }

      H5Sclose(dataspaceID);
    }
  }

  return returnError;
}

#if 0
usize DatasetIO::getNumAttributes() const
{
  if(!exists())
  {
    return 0;
  }
  return openH5Dataset().getNumberAttributes();
}

std::vector<std::string> DatasetIO::getAttributeNames() const
{
  if(!exists())
  {
    return {};
  }
  return openH5Dataset().listAttributeNames();
}

void DatasetIO::deleteAttribute(const std::string& name)
{
  try
  {
    if(!exists())
    {
      return;
    }
    openH5Dataset().deleteAttribute(name);
  }
  catch (const std::exception& e)
  {
    deleteH5Attribute(name);
  }
}

Result<> DatasetIO::deleteH5Attribute(const std::string& name)
{
  auto parentId = parentGroupRef().getId();
  hsize_t attributeNum = 0;
  int32_t hasAttribute = H5Aiterate(parentId, H5_INDEX_NAME, H5_ITER_INC, &attributeNum, Support::FindAttr, const_cast<char*>(getNamePath().c_str()));

  /* The attribute already exists, delete it */
  if(hasAttribute == 1)
  {
    herr_t error = H5Adelete(parentId, getNamePath().c_str());
    if(error < 0)
    {
      std::string ss = fmt::format("Error Deleting Attribute '{}' from Object '{}'", getNamePath(), getParentName());
      return MakeErrorResult(error, ss);
    }
  }
  return {};
}
#endif

bool DatasetIO::exists() const
{
  if(getId() > 0)
  {
    return true;
  }
  return false;
}

std::string DatasetIO::getFilterName() const
{
  // Resolve getId() before one leaf lock around property-list inspection and cleanup.
  const hid_t selfId = getId();
  std::string filterNames;
  {
    std::lock_guard<std::mutex> hdf5Lock(Support::ApiLock());
    const hid_t cpListId = H5Dget_create_plist(selfId);
    const int numFilters = H5Pget_nfilters(cpListId);
    for(int j = 0; j < numFilters; ++j)
    {
      unsigned int flags;
      unsigned int filterConfig;
      size_t cdNElements = 0;
      char name[1024];
      H5Z_filter_t filter = H5Pget_filter2(cpListId, j, &flags, &cdNElements, nullptr, std::size(name) / sizeof(*name), name, &filterConfig);
      std::vector<unsigned int> cdValues(cdNElements);
      filter = H5Pget_filter2(cpListId, j, &flags, &cdNElements, cdValues.data(), std::size(name) / sizeof(*name), name, &filterConfig);
      if(j != 0)
      {
        filterNames += ", ";
      }
      filterNames += GetNameFromFilterType(filter);
    }
    if(cpListId >= 0)
    {
      H5Pclose(cpListId);
    }
  }
  if(filterNames.empty())
  {
    filterNames = "NONE";
  }
  return filterNames;
}

// declare readAsVector
template SIMPLNX_EXPORT std::vector<int8_t> DatasetIO::readAsVector<int8_t>() const;
template SIMPLNX_EXPORT std::vector<int16_t> DatasetIO::readAsVector<int16_t>() const;
template SIMPLNX_EXPORT std::vector<int32_t> DatasetIO::readAsVector<int32_t>() const;
template SIMPLNX_EXPORT std::vector<int64_t> DatasetIO::readAsVector<int64_t>() const;
template SIMPLNX_EXPORT std::vector<uint8_t> DatasetIO::readAsVector<uint8_t>() const;
template SIMPLNX_EXPORT std::vector<uint16_t> DatasetIO::readAsVector<uint16_t>() const;
template SIMPLNX_EXPORT std::vector<uint32_t> DatasetIO::readAsVector<uint32_t>() const;
template SIMPLNX_EXPORT std::vector<uint64_t> DatasetIO::readAsVector<uint64_t>() const;
#ifdef __APPLE__
template SIMPLNX_EXPORT std::vector<size_t> DatasetIO::readAsVector<size_t>() const;
#endif
template SIMPLNX_EXPORT std::vector<float> DatasetIO::readAsVector<float>() const;
template SIMPLNX_EXPORT std::vector<double> DatasetIO::readAsVector<double>() const;
#ifdef _WIN32
template SIMPLNX_EXPORT std::vector<bool> DatasetIO::readAsVector<bool>() const;
#endif

template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int8_t>> DatasetIO::readAsDataStore<int8_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int16_t>> DatasetIO::readAsDataStore<int16_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int32_t>> DatasetIO::readAsDataStore<int32_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int64_t>> DatasetIO::readAsDataStore<int64_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint8_t>> DatasetIO::readAsDataStore<uint8_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint16_t>> DatasetIO::readAsDataStore<uint16_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint32_t>> DatasetIO::readAsDataStore<uint32_t>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint64_t>> DatasetIO::readAsDataStore<uint64_t>() const;
#ifdef __APPLE__
// template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<size_t>> DatasetIO::readAsDataStore<size_t>() const;
#endif
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<bool>> DatasetIO::readAsDataStore<bool>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<float>> DatasetIO::readAsDataStore<float>() const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<double>> DatasetIO::readAsDataStore<double>() const;

template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int8_t>> DatasetIO::readAsDataStore<int8_t>(const ShapeType&, const ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int16_t>> DatasetIO::readAsDataStore<int16_t>(const ShapeType&, const ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int32_t>> DatasetIO::readAsDataStore<int32_t>(const ShapeType&, const ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<int64_t>> DatasetIO::readAsDataStore<int64_t>(const ShapeType&, const ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint8_t>> DatasetIO::readAsDataStore<uint8_t>(const ShapeType&, const ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint16_t>> DatasetIO::readAsDataStore<uint16_t>(const ShapeType&, const ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint32_t>> DatasetIO::readAsDataStore<uint32_t>(const ShapeType&, const ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<uint64_t>> DatasetIO::readAsDataStore<uint64_t>(const ShapeType&, const ShapeType&) const;
#ifdef __APPLE__
// template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<size_t>> DatasetIO::readAsDataStore<size_t>(const ShapeType&, const ShapeType&) const;
#endif
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<bool>> DatasetIO::readAsDataStore<bool>(const ShapeType&, const ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<float>> DatasetIO::readAsDataStore<float>(const ShapeType&, const ShapeType&) const;
template SIMPLNX_EXPORT std::shared_ptr<AbstractDataStore<double>> DatasetIO::readAsDataStore<double>(const ShapeType&, const ShapeType&) const;

template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<size_t>(nonstd::span<size_t>) const;
#else
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>) const;
#endif
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>) const;

template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int8_t>(nonstd::span<int8_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int16_t>(nonstd::span<int16_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int32_t>(nonstd::span<int32_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<int64_t>(nonstd::span<int64_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint8_t>(nonstd::span<uint8_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint16_t>(nonstd::span<uint16_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint32_t>(nonstd::span<uint32_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<uint64_t>(nonstd::span<uint64_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<size_t>(nonstd::span<size_t>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#endif
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<float>(nonstd::span<float>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<double>(nonstd::span<double>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#ifdef _WIN32
template SIMPLNX_EXPORT Result<> DatasetIO::readIntoSpan<bool>(nonstd::span<bool>, const std::optional<std::vector<uint64>>&, const std::optional<std::vector<uint64>>&) const;
#endif

#if 0
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<int8_t>(nonstd::span<int8_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<int16_t>(nonstd::span<int16_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<int32_t>(nonstd::span<int32_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<int64_t>(nonstd::span<int64_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<uint8_t>(nonstd::span<uint8_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<uint16_t>(nonstd::span<uint16_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<uint32_t>(nonstd::span<uint32_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<uint64_t>(nonstd::span<uint64_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<bool>(nonstd::span<bool>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<char>(nonstd::span<char>, nonstd::span<const usize>, nonstd::span<const usize>) const;
#ifdef __APPLE__
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<size_t>(nonstd::span<size_t>, nonstd::span<const usize>, nonstd::span<const usize>) const;
#endif
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<float>(nonstd::span<float>, nonstd::span<const usize>, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunkIntoSpan<double>(nonstd::span<double>, nonstd::span<const usize>, nonstd::span<const usize>) const;
#endif

template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<int8_t>(const DimsType&, nonstd::span<const int8_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<int16_t>(const DimsType&, nonstd::span<const int16_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<int32_t>(const DimsType&, nonstd::span<const int32_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<int64_t>(const DimsType&, nonstd::span<const int64_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<uint8_t>(const DimsType&, nonstd::span<const uint8_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<uint16_t>(const DimsType&, nonstd::span<const uint16_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<uint32_t>(const DimsType&, nonstd::span<const uint32_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<uint64_t>(const DimsType&, nonstd::span<const uint64_t>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<float>(const DimsType&, nonstd::span<const float>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<double>(const DimsType&, nonstd::span<const double>);
#ifdef _WIN32
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpan<bool>(const DimsType&, nonstd::span<const bool>);
#endif

template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int8_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int16_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int32_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<int64_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint8_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint16_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint32_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<uint64_t>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<float>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<double>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<bool>(const DimsType&, const DimsType&) const;
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<char>(const DimsType&, const DimsType&) const;
#ifdef _WIN32
template SIMPLNX_EXPORT Result<ChunkedDataInfo> DatasetIO::initChunkedDataset<bool>(const DimsType&, const DimsType&) const;
#endif

template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<int8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int8_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<int16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int16_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<int32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int32_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<int64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<int64_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<uint8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint8_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<uint16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint16_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<uint32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint32_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<uint64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<uint64_t>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<float>(const ChunkedDataInfo&, const DimsType&, nonstd::span<float>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<double>(const ChunkedDataInfo&, const DimsType&, nonstd::span<double>, const DimsType&, nonstd::span<const usize>) const;
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<char>(const ChunkedDataInfo&, const DimsType&, nonstd::span<char>, const DimsType&, nonstd::span<const usize>) const;
#ifdef _WIN32
template SIMPLNX_EXPORT Result<> DatasetIO::readChunk<bool>(const ChunkedDataInfo&, const DimsType&, nonstd::span<bool>, const DimsType&, nonstd::span<const usize>) const;
#endif

template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int8_t>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int16_t>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int32_t>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<int64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const int64_t>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint8_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint8_t>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint16_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint16_t>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint32_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint32_t>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<uint64_t>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const uint64_t>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<float>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const float>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<double>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const double>, const DimsType&, const DimsType&, nonstd::span<const usize>);
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<char>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const char>, const DimsType&, const DimsType&, nonstd::span<const usize>);
#ifdef _WIN32
template SIMPLNX_EXPORT Result<> DatasetIO::writeChunk<bool>(const ChunkedDataInfo&, const DimsType&, nonstd::span<const bool>, const DimsType&, const DimsType&, nonstd::span<const usize>);
#endif

template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<int8>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<int16>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<int32>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<int64>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<uint8>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<uint16>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<uint32>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<uint64>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<float32>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<float64>(const DimsType&);
template SIMPLNX_EXPORT Result<> DatasetIO::createEmptyDataset<bool>(const DimsType&);

template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<int8>(nonstd::span<const int8>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<int16>(nonstd::span<const int16>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<int32>(nonstd::span<const int32>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<int64>(nonstd::span<const int64>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<uint8>(nonstd::span<const uint8>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<uint16>(nonstd::span<const uint16>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<uint32>(nonstd::span<const uint32>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<uint64>(nonstd::span<const uint64>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<float32>(nonstd::span<const float32>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<float64>(nonstd::span<const float64>, const std::vector<uint64>&, const std::vector<uint64>&);
template SIMPLNX_EXPORT Result<> DatasetIO::writeSpanHyperslab<bool>(nonstd::span<const bool>, const std::vector<uint64>&, const std::vector<uint64>&);

} // namespace nx::core::HDF5
