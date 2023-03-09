#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ExportGBCDTriangleDataInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshFaceAreasArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT ExportGBCDTriangleData
{
public:
  ExportGBCDTriangleData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExportGBCDTriangleDataInputValues* inputValues);
  ~ExportGBCDTriangleData() noexcept;

  ExportGBCDTriangleData(const ExportGBCDTriangleData&) = delete;
  ExportGBCDTriangleData(ExportGBCDTriangleData&&) noexcept = delete;
  ExportGBCDTriangleData& operator=(const ExportGBCDTriangleData&) = delete;
  ExportGBCDTriangleData& operator=(ExportGBCDTriangleData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExportGBCDTriangleDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
