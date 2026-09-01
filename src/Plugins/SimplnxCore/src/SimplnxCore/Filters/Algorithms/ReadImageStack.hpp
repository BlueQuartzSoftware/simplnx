#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOEnums.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace nx::core
{

/**
 * @struct ReadImageSubFilterConfig
 * @brief Stores options forwarded to one ReadImageFilter execution.
 *
 * Preflight and execution use this shared subset so their per-slice configuration stays equal.
 */
struct SIMPLNXCORE_EXPORT ReadImageSubFilterConfig
{
  std::filesystem::path filePath;
  DataPath imageGeometryPath;
  std::string cellDataName;
  std::string imageDataArrayName;
  bool changeOrigin = false;
  bool changeSpacing = false;
  VectorFloat32Parameter::ValueType origin;
  VectorFloat32Parameter::ValueType spacing;
  OriginSpacingProcessing originSpacingProcessing = OriginSpacingProcessing::Postprocessed;
  bool changeDataType = false;
  DataType imageDataType = DataType::uint8;
  CropGeometryParameter::ValueType croppingOptions;
};

/**
 * @brief Builds arguments for one delegated ReadImageFilter execution.
 * @param config Specifies file, output, type, crop, and spatial settings.
 * @return Arguments for one input slice.
 *
 * Preprocessed spatial overrides go to the delegated reader. Deferred overrides
 * stay at stack level so they apply after crop and resampling.
 */
SIMPLNXCORE_EXPORT Arguments BuildReadImageFilterArgs(const ReadImageSubFilterConfig& config);

/**
 * @struct ReadImageStackInputValues
 * @brief Stores file-list, output, transform, crop, resample, and conversion settings.
 *
 * resampleImagesChoice maps 0 to none, 1 to scaling, and 2 to exact dimensions.
 */
struct SIMPLNXCORE_EXPORT ReadImageStackInputValues
{
  std::vector<std::string> fileList;
  DataPath imageGeometryPath;
  std::string imageDataArrayName;
  std::string cellDataName;
  bool changeOrigin = false;
  VectorFloat32Parameter::ValueType origin;
  bool changeSpacing = false;
  VectorFloat32Parameter::ValueType spacing;
  OriginSpacingProcessing originSpacingProcessing = OriginSpacingProcessing::Postprocessed;
  ImageFlipTransform imageTransform = ImageFlipTransform::None;
  bool convertToGrayScale = false;
  VectorFloat32Parameter::ValueType colorWeights;
  usize resampleImagesChoice = 0;
  float32 scaling = 100.0f;
  VectorUInt64Parameter::ValueType exactXYDimensions;
  bool changeDataType = false;
  DataType imageDataType = DataType::uint8;
  CropGeometryParameter::ValueType croppingOptions;
};

/**
 * @class ReadImageStack
 * @brief Reads an ordered image-file sequence into a three-dimensional DataArray.
 *
 * Each file uses a temporary DataStructure and delegated filters. The completed
 * slice then moves to the destination through one checked slice transfer.
 *
 * Flip operations use one or two row buffers. No complete duplicate stack is created.
 */
class SIMPLNXCORE_EXPORT ReadImageStack
{
public:
  /**
   * @brief Creates an image-stack reader.
   * @param dataStructure Receives the destination stack.
   * @param mesgHandler Receives per-file messages.
   * @param shouldCancel Stops after the current slice transfer when true.
   * @param inputValues Specifies validated stack settings. The caller must keep
   * this object alive for the reader lifetime.
   */
  ReadImageStack(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadImageStackInputValues& inputValues);
  /**
   * @brief Destroys the non-owning reader.
   */
  ~ReadImageStack() noexcept;

  ReadImageStack(const ReadImageStack&) = delete;
  ReadImageStack(ReadImageStack&&) noexcept = delete;
  ReadImageStack& operator=(const ReadImageStack&) = delete;
  ReadImageStack& operator=(ReadImageStack&&) noexcept = delete;

  /**
   * @brief Reads and appends all selected slices in file order.
   * @return Validation, delegated-filter, or bulk-I/O error, or accumulated warnings.
   *
   * Cancellation returns success after the current slice and retains earlier slices.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadImageStackInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
