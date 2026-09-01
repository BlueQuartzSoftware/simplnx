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
 * @struct WriteGBCDTriangleDataInputValues
 * @brief Identifies triangle arrays, feature orientations, and the output file.
 */
struct ORIENTATIONANALYSIS_EXPORT WriteGBCDTriangleDataInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath SurfaceMeshFaceNormalsArrayPath;
  DataPath SurfaceMeshFaceAreasArrayPath;
  DataPath FeatureEulerAnglesArrayPath;
};

/**
 * @class WriteGBCDTriangleData
 * @brief Writes grain boundary triangle data (Euler angles, normals, areas) to an ASCII file.
 *
 * Face labels, normals, and areas use 8,192-tuple pages. The complete feature
 * Euler array stays in memory because face labels can reference features in any
 * order. Therefore, staging memory scales with the feature count even though
 * triangle staging is bounded.
 *
 * Cancellation returns success and preserves the header and completed pages.
 * The current implementation does not inspect source bulk-read results or output
 * stream failures.
 */
class ORIENTATIONANALYSIS_EXPORT WriteGBCDTriangleData
{
public:
  /**
   * @brief Initializes a GBCD triangle-data writer.
   * @param dataStructure Provides triangle and feature arrays.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation between pages.
   * @param inputValues Identifies arrays and the output path.
   * @pre All arguments outlive this writer.
   */
  WriteGBCDTriangleData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteGBCDTriangleDataInputValues* inputValues);
  ~WriteGBCDTriangleData() noexcept;

  WriteGBCDTriangleData(const WriteGBCDTriangleData&) = delete;
  WriteGBCDTriangleData(WriteGBCDTriangleData&&) noexcept = delete;
  WriteGBCDTriangleData& operator=(const WriteGBCDTriangleData&) = delete;
  WriteGBCDTriangleData& operator=(WriteGBCDTriangleData&&) noexcept = delete;

  /**
   * @brief Writes triangle records in bounded pages.
   * @return Error when the output file cannot open.
   * @pre Face arrays have equal tuple counts with component counts 2, 3, and 1.
   * @pre Feature Euler angles have three components.
   * @pre Each nonnegative face label indexes the feature Euler array.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteGBCDTriangleDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
