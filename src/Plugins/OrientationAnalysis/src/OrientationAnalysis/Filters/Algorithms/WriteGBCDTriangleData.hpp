#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

/**
 * @brief Input values for the WriteGBCDTriangleData algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT WriteGBCDTriangleDataInputValues
{
  FileSystemPathParameter::ValueType OutputFile; ///< Path to the output ASCII file.
  DataPath SurfaceMeshFaceLabelsArrayPath;       ///< Path to FaceLabels (2-component int32, grain IDs per face side).
  DataPath SurfaceMeshFaceNormalsArrayPath;      ///< Path to FaceNormals (3-component float64).
  DataPath SurfaceMeshFaceAreasArrayPath;        ///< Path to FaceAreas (1-component float64).
  DataPath FeatureEulerAnglesArrayPath;          ///< Path to FeatureEulerAngles (3-component float32, feature-level).
};

/**
 * @class WriteGBCDTriangleData
 * @brief Writes grain boundary triangle data (Euler angles, normals, areas) to an ASCII file.
 *
 * @section ooc_summary OOC Optimization Summary
 * Three face-level arrays (labels, normals, areas) are read in chunks via copyIntoBuffer()
 * and formatted into a string buffer before writing. The feature-level Euler angles array
 * is cached entirely in memory (small, one tuple per grain). This avoids per-element OOC
 * access and reduces file I/O to one write per chunk.
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
