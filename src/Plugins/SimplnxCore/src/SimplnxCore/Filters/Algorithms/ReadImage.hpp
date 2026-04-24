#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOEnums.hpp"

#include <filesystem>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadImageInputValues
{
  std::filesystem::path inputFilePath;
  DataPath imageGeometryPath;
  DataPath imageDataArrayPath;
  std::string cellDataName;
  bool changeOrigin = false;
  bool centerOrigin = false;
  FloatVec3 origin;
  bool changeSpacing = false;
  FloatVec3 spacing;
  OriginSpacingProcessing originSpacingProcessing = OriginSpacingProcessing::Postprocessed;
  bool changeDataType = false;
  DataType imageDataType = DataType::uint8;
  CropGeometryParameter::ValueType croppingOptions;
};

/**
 * @class ReadImage
 * @brief This algorithm reads a single 2D image file into a pre-allocated DataArray
 * using the IImageIO abstraction layer.
 */
class SIMPLNXCORE_EXPORT ReadImage
{
public:
  ReadImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadImageInputValues& inputValues);
  ~ReadImage() noexcept;

  ReadImage(const ReadImage&) = delete;
  ReadImage(ReadImage&&) noexcept = delete;
  ReadImage& operator=(const ReadImage&) = delete;
  ReadImage& operator=(ReadImage&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadImageInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
