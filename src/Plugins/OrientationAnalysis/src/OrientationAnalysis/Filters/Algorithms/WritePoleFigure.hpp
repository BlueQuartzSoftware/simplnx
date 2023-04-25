#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{
namespace write_pole_figure
{
const std::string k_ImageAttrMatName("CellData");
const std::string k_ImageDataName("Image");
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
  bool UseGoodVoxels;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath GoodVoxelsArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath MaterialNameArrayPath;

  bool SaveAsImageGeometry;
  bool WriteImageToDisk;
  DataPath OutputImageGeometryPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
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

  std::vector<unsigned char> m_FiraSansRegular;
  std::vector<unsigned char> m_LatoRegular;
  std::vector<unsigned char> m_LatoBold;
};

} // namespace complex
