#pragma once

#include "ImageProcessing/ImageProcessing_export.hpp"

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

/**
 * @struct ReadImageInputValues
 * @brief Stores file, output, spatial, type-conversion, and crop settings.
 */
struct IMAGEPROCESSING_EXPORT ReadImageInputValues
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
  // ReadImageStack sets this programmatic hint to read only page 0 with Z equal to 1.
  // The hint applies to multi-page TIFF files and is not a user parameter.
  bool readSinglePageOnly = false;
};

/**
 * @class ReadImage
 * @brief Reads image files into a preallocated DataArray through IImageIO.
 *
 * The algorithm streams raster and multi-page TIFF data one page at a time.
 * The shared NRRD reader streams .nrrd and .nhdr volumes. Each Z slice uses one bulk destination write.
 */
class IMAGEPROCESSING_EXPORT ReadImage
{
public:
  /**
   * @brief Creates a single-image reader.
   * @param dataStructure Receives decoded pixels.
   * @param mesgHandler Receives file and conversion messages.
   * @param shouldCancel Stops the decoder callback when true.
   * @param inputValues Specifies validated file and output settings. The caller
   * must keep this object alive for the reader lifetime.
   */
  ReadImage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadImageInputValues& inputValues);
  /**
   * @brief Destroys the non-owning reader.
   */
  ~ReadImage() noexcept;

  ReadImage(const ReadImage&) = delete;
  ReadImage(ReadImage&&) noexcept = delete;
  ReadImage& operator=(const ReadImage&) = delete;
  ReadImage& operator=(ReadImage&&) noexcept = delete;

  /**
   * @brief Reads, crops, converts, and writes decoded row segments.
   * @return Decoder, metadata, crop, or destination-write error, or success after cancellation.
   *
   * Cancellation returns success and retains destination segments written before the callback stops.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadImageInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
