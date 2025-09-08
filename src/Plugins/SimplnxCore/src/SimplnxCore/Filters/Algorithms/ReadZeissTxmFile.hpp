#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include "oless/pole.h"

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace read_zeiss_txm
{

using StoragePtrType = std::shared_ptr<POLE::Storage>;
using StreamPtrType = std::shared_ptr<POLE::Stream>;

enum class ZeissTxmDataType : int32
{
  UCHAR_TYPE = 3,
  INT16_TYPE = 5,
  FLOAT_TYPE = 10,
};

struct SIMPLNXCORE_EXPORT ReadZeissTxmFileInputValues
{
  DataPath ImageGeometryPath;
  std::string CellAttributeMatrixName;
  std::string DensityArrayName;
  std::filesystem::path TxmDataFile;
  bool UseSubVolume;
  usize SubVolumeStartSlice;
  usize SubVolumeEndSlice;
};

/**
 * @brief
 */
struct ZeissTxmHeaderMetadata
{
  CreateImageGeometryAction::DimensionType Dimensions = {0, 0, 0};
  CreateImageGeometryAction::SpacingType Spacing = {0.0f, 0.0f, 0.0f};
  CreateImageGeometryAction::OriginType Origin = {0.0f, 0.0f, 0.0f};
  IGeometry::LengthUnit Units = IGeometry::LengthUnit::Micrometer;
  std::string DataFilePath = {};
  ZeissTxmDataType DataType = ZeissTxmDataType::FLOAT_TYPE;
  void clear()
  {
    Dimensions = {0, 0, 0};
    Spacing = {0.0f, 0.0f, 0.0f};
    Origin = {0.0f, 0.0f, 0.0f};
    Units = IGeometry::LengthUnit::Micrometer;
    DataFilePath = {};
    DataType = ZeissTxmDataType::FLOAT_TYPE;
  }
};

/**
 * @brief
 */
struct SIMPLNXCORE_EXPORT ReadZeissTxmFilterFileCache
{
  fs::path inputFile;
  fs::file_time_type timeStamp;
  ZeissTxmHeaderMetadata metaData;
  void flush()
  {
    inputFile.clear();
    metaData.clear();
    timeStamp = fs::file_time_type();
  }
};

/**
 * @brief
 * @tparam T
 * @param storage
 * @param path
 * @return
 */
template <typename T>
T ReadValue(const StoragePtrType& storage, const std::string& path)
{
  T value = 0;
  if(const StreamPtrType stream = std::make_shared<POLE::Stream>(storage.get(), path); !stream->fail())
  {
    stream->read(reinterpret_cast<unsigned char*>(&value), sizeof(T));
  }
  return value;
}

/**
 * @brief Reads selected meta-data from the .txm or .txrm file
 * @param inputFilePath The file path to the input file
 * @return
 */
Result<ZeissTxmHeaderMetadata> ReadHeaderMetaData(const std::string& inputFilePath);

/**
 * @class ReadZeissTxmFile
 */
class SIMPLNXCORE_EXPORT ReadZeissTxmFile
{

public:
  ReadZeissTxmFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadZeissTxmFileInputValues* inputValues);
  ~ReadZeissTxmFile() noexcept;

  ReadZeissTxmFile(const ReadZeissTxmFile&) = delete;
  ReadZeissTxmFile(ReadZeissTxmFile&&) noexcept = delete;
  ReadZeissTxmFile& operator=(const ReadZeissTxmFile&) = delete;
  ReadZeissTxmFile& operator=(ReadZeissTxmFile&&) noexcept = delete;

  Result<> operator()() const;

  const std::atomic_bool& getCancel() const;

private:
  DataStructure& m_DataStructure;
  const ReadZeissTxmFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace read_zeiss_txm
