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
 * @brief Inputs to `BuildReadImageFilterArgs()`.
 *
 * Captures exactly the subset of per-slice options the stack filter forwards to the single-image
 * ReadImageFilter. Used by both `ReadImageStackFilter::preflightImpl` (first-slice preflight) and
 * `ReadImageStack::operator()` (per-slice execute) so the two paths stay in sync.
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
 * @brief Builds the `Arguments` object used to invoke `ReadImageFilter` on a single slice file.
 *
 * When `originSpacingProcessing` is `Postprocessed` the sub-filter is told NOT to apply the
 * origin/spacing override — the stack filter's final `UpdateImageGeomAction` handles that after
 * cropping and resampling.
 */
SIMPLNXCORE_EXPORT Arguments BuildReadImageFilterArgs(const ReadImageSubFilterConfig& config);

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
  usize resampleImagesChoice = 0; // 0=None, 1=Scaling, 2=ExactDims (kept as index until the resample filter's enum is shared too)
  float32 scaling = 100.0f;
  VectorUInt64Parameter::ValueType exactXYDimensions;
  bool changeDataType = false;
  DataType imageDataType = DataType::uint8;
  CropGeometryParameter::ValueType croppingOptions;
};

/**
 * @class ReadImageStack
 * @brief This algorithm reads a numbered sequence of 2D image files into a 3D DataArray
 * using ReadImageFilter as a sub-filter for per-slice reading. Supports resampling,
 * grayscale conversion, flip transforms, origin/spacing overrides, and Z-slice cropping.
 */
class SIMPLNXCORE_EXPORT ReadImageStack
{
public:
  ReadImageStack(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const ReadImageStackInputValues& inputValues);
  ~ReadImageStack() noexcept;

  ReadImageStack(const ReadImageStack&) = delete;
  ReadImageStack(ReadImageStack&&) noexcept = delete;
  ReadImageStack& operator=(const ReadImageStack&) = delete;
  ReadImageStack& operator=(ReadImageStack&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadImageStackInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
