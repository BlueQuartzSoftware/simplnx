#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{
namespace write_pole_figure
{
const std::string k_ImageAttrMatName("Cell Data");
const std::string k_ImageDataName("Image");
const std::string k_MetaDataName("MetaData");

} // namespace write_pole_figure

struct ORIENTATIONANALYSIS_EXPORT WritePoleFigureInputValues
{
  StringParameter::ValueType Title;
  ChoicesParameter::ValueType GenerationAlgorithm;
  int32 LambertSize;
  int32 NumColors;
  ChoicesParameter::ValueType ImageFormat;
  ChoicesParameter::ValueType ImageLayout;
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType ImagePrefix;
  int32 ImageSize;
  bool UseMask;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath MaskArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath MaterialNameArrayPath;

  bool SaveAsImageGeometry;
  bool WriteImageToDisk;
  DataPath OutputImageGeometryPath;

  bool SaveIntensityData;
  bool NormalizeToMRD;
  DataPath IntensityGeometryDataPath;
  std::string IntensityPlot1Name;
  std::string IntensityPlot2Name;
  std::string IntensityPlot3Name;
};

/**
 * @class WritePoleFigure
 * @brief Generates pole figure images from Euler angle data using Lambert or discrete projection.
 *
 * Supports multiple output formats (TIFF, BMP, PNG, JPG, PDF), layouts (horizontal,
 * vertical, square), and optional intensity data output. The projection computation
 * uses EbsdLib's ModifiedLambertProjection or a discrete stereographic projection.
 *
 * @note The OOC changes in this file are limited to type cleanup (size_t -> usize,
 * float -> float32) and removing an unused include. The algorithm itself operates on
 * EbsdLib's internal arrays which are always in-memory.
 */
class ORIENTATIONANALYSIS_EXPORT WritePoleFigure
{
public:
  WritePoleFigure(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WritePoleFigureInputValues* inputValues);
  ~WritePoleFigure() noexcept;

  WritePoleFigure(const WritePoleFigure&) = delete;
  WritePoleFigure(WritePoleFigure&&) noexcept = delete;
  WritePoleFigure& operator=(const WritePoleFigure&) = delete;
  WritePoleFigure& operator=(WritePoleFigure&&) noexcept = delete;

  enum ImageFormatType
  {
    TifImageType = 0,
    BmpImageType = 1,
    PngImageType = 2,
    JpgImageType = 3,
    PdfImageType = 4,
  };

  using EnumType = ChoicesParameter::ValueType;

  enum class LayoutType : EnumType
  {
    Horizontal = 0,
    Vertical = 1,
    Square = 2,
  };

  enum class Algorithm : EnumType
  {
    LambertProjection = 0, //!<
    Discrete = 1,          //!<
    Unknown = 2,           //!
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WritePoleFigureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
