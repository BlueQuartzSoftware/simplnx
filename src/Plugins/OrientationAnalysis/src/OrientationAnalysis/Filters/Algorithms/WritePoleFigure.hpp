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

#include <EbsdLib/Core/EbsdLibConstants.h>

namespace nx::core
{
namespace write_pole_figure
{
const std::string k_ImageAttrMatName("Cell Data");
const std::string k_ImageDataName("Image");
const std::string k_MetaDataName("MetaData");

} // namespace write_pole_figure

/**
 * @struct WritePoleFigureInputValues
 * @brief Defines pole-figure generation, output, and input-array settings.
 */
struct ORIENTATIONANALYSIS_EXPORT WritePoleFigureInputValues
{
  StringParameter::ValueType Title;
  ChoicesParameter::ValueType GenerationAlgorithm;
  int32 LambertSize;
  int32 NumColors;
  float32 DiscreteMarkerRadius;
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

  bool FlipFinalImage;
  DataPath IntensityGeometryDataPath;
  std::string IntensityPlot1Name;
  std::string IntensityPlot2Name;
  std::string IntensityPlot3Name;

  ebsdlib::HexConvention HexConvention = ebsdlib::HexConvention::XParallelA;
};

/**
 * @class WritePoleFigure
 * @brief Generates pole figure images from Euler angle data using Lambert or discrete projection.
 *
 * The algorithm streams phase and Euler inputs in 65,536-tuple pages. It scans
 * the inputs twice for each phase and materializes all selected Euler angles for
 * that phase in an EbsdLib array. Therefore, page buffers are bounded but total
 * projection memory scales with the largest phase.
 *
 * Mask access remains per tuple through MaskCompareUtilities. An out-of-core mask
 * can cause repeated element access during both scans of every phase. The current
 * algorithm does not inspect cancellation or bulk-I/O Result values.
 *
 * The ImageFormat setting is retained by the input interface, but disk output is
 * currently always PNG. Output geometry and intensity arrays receive bulk writes.
 */
class ORIENTATIONANALYSIS_EXPORT WritePoleFigure
{
public:
  /**
   * @brief Initializes a pole-figure generator.
   * @param dataStructure Provides source arrays and output geometries.
   * @param mesgHandler Receives phase progress and crystal-structure warnings.
   * @param shouldCancel Supplies a retained flag that operator() does not inspect.
   * @param inputValues Defines generation and output settings.
   * @pre All arguments outlive this generator.
   */
  WritePoleFigure(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WritePoleFigureInputValues* inputValues);
  ~WritePoleFigure() noexcept;

  WritePoleFigure(const WritePoleFigure&) = delete;
  WritePoleFigure(WritePoleFigure&&) noexcept = delete;
  WritePoleFigure& operator=(const WritePoleFigure&) = delete;
  WritePoleFigure& operator=(WritePoleFigure&&) noexcept = delete;

  /**
   * @enum ImageFormatType
   * @brief Defines image-format choice values retained by the filter interface.
   */
  enum ImageFormatType
  {
    TifImageType = 0,
    BmpImageType = 1,
    PngImageType = 2,
    JpgImageType = 3,
    PdfImageType = 4,
  };

  using EnumType = ChoicesParameter::ValueType;

  /**
   * @enum LayoutType
   * @brief Selects the composite pole-figure layout.
   */
  enum class LayoutType : EnumType
  {
    Horizontal = 0,
    Vertical = 1,
    Square = 2,
  };

  /**
   * @enum Algorithm
   * @brief Selects Lambert or discrete pole-figure generation.
   */
  enum class Algorithm : EnumType
  {
    LambertProjection = 0,
    Discrete = 1,
    Unknown = 2,
  };

  /**
   * @brief Generates requested intensity, geometry, and PNG outputs.
   * @return Directory, mask, array-creation, or PNG writer errors that the current implementation inspects.
   * @pre Cell phases, Euler angles, and an optional mask have equal tuple counts.
   * @pre Euler tuples have three components and phase IDs index ensemble arrays.
   * @pre Image, Lambert, color, and marker settings satisfy EbsdLib requirements.
   *
   * Some dynamic intensity-array creation failures and all bulk-I/O failures are
   * not returned by the current implementation.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WritePoleFigureInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
