#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT WriteGBCDTriangleDataInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshFaceAreasArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
};

/**
 * @class
 */
class ORIENTATIONANALYSIS_EXPORT WriteGBCDTriangleData
{
public:
  WriteGBCDTriangleData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteGBCDTriangleDataInputValues* inputValues);
  ~WriteGBCDTriangleData() noexcept;

  WriteGBCDTriangleData(const WriteGBCDTriangleData&) = delete;
  WriteGBCDTriangleData(WriteGBCDTriangleData&&) noexcept = delete;
  WriteGBCDTriangleData& operator=(const WriteGBCDTriangleData&) = delete;
  WriteGBCDTriangleData& operator=(WriteGBCDTriangleData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteGBCDTriangleDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
