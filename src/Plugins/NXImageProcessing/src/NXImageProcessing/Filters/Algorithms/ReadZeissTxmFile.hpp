#pragma once

#include "simplnx/Utilities/ImageIO/ReaderGeometryOverrides.hpp"

#include "NXImageProcessing/NXImageProcessing_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"

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

struct NXIMAGEPROCESSING_EXPORT ReadZeissTxmFileInputValues
{
  DataPath image_geometry_path;
  std::string cell_attribute_matrix_name;
  std::string density_array_name;
  std::filesystem::path txm_data_file;
  CropGeometryParameter::ValueType cropping_options;
  image_io::GeometryOverrides geometry_overrides;
};

/**
 * @brief
 */
struct ZeissTxmHeaderMetadata
{
  CreateImageGeometryAction::DimensionType dimensions = {0, 0, 0};
  CreateImageGeometryAction::SpacingType spacing = {0.0f, 0.0f, 0.0f};
  CreateImageGeometryAction::OriginType origin = {0.0f, 0.0f, 0.0f};
  IGeometry::LengthUnit units = IGeometry::LengthUnit::Micrometer;
  std::string data_file_path;
  ZeissTxmDataType data_type = ZeissTxmDataType::FLOAT_TYPE;
  void clear()
  {
    dimensions = {0, 0, 0};
    spacing = {0.0f, 0.0f, 0.0f};
    origin = {0.0f, 0.0f, 0.0f};
    units = IGeometry::LengthUnit::Micrometer;
    data_file_path = {};
    data_type = ZeissTxmDataType::FLOAT_TYPE;
  }
};

/**
 * @brief
 */
struct NXIMAGEPROCESSING_EXPORT ReadZeissTxmFilterFileCache
{
  fs::path input_file;
  fs::file_time_type time_stamp;
  ZeissTxmHeaderMetadata meta_data;
  void flush()
  {
    input_file.clear();
    meta_data.clear();
    time_stamp = fs::file_time_type();
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
class NXIMAGEPROCESSING_EXPORT ReadZeissTxmFile
{

public:
  ReadZeissTxmFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadZeissTxmFileInputValues* inputValues);
  ~ReadZeissTxmFile() noexcept = default;

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
