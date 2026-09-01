#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

#include <EbsdLib/Orientation/OrientationFwd.hpp>

#include <string>

namespace nx::core
{

/**
 * @struct ConvertOrientationsToVertexGeometryInputValues
 * @brief Identifies orientation-to-vertex conversion inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ConvertOrientationsToVertexGeometryInputValues
{
  ebsdlib::orientations::Type InputOrientationType;
  DataPath InputOrientationArrayPath;
  std::vector<DataPath> DataPathCopySources;
  bool ConvertToFundamentalZone;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath OutputVertexGeometryPath;
  std::string OutputVertexAttrMatrixName;
  std::string OutputSharedVertexListName;
};

/**
 * @class ConvertOrientationsToVertexGeometry
 * @brief Converts orientations to stereographic VertexGeom positions.
 *
 * The executor converts one bounded chunk to quaternions, optionally applies
 * its fundamental zone, then writes vertex positions. Crystal structures stay
 * local for repeated phase lookup.
 */
class ORIENTATIONANALYSIS_EXPORT ConvertOrientationsToVertexGeometry
{
public:
  /**
   * @brief Initializes orientation-to-vertex conversion.
   * @param dataStructure Provides selected arrays and output geometry.
   * @param mesgHandler Supplies the filter message handler.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies selected arrays and conversion options.
   * @pre dataStructure, mesgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ConvertOrientationsToVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                      ConvertOrientationsToVertexGeometryInputValues* inputValues);
  /**
   * @brief Destroys the orientation-to-vertex executor.
   */
  ~ConvertOrientationsToVertexGeometry() noexcept = default;

  ConvertOrientationsToVertexGeometry(const ConvertOrientationsToVertexGeometry&) = delete;
  ConvertOrientationsToVertexGeometry(ConvertOrientationsToVertexGeometry&&) noexcept = delete;
  ConvertOrientationsToVertexGeometry& operator=(const ConvertOrientationsToVertexGeometry&) = delete;
  ConvertOrientationsToVertexGeometry& operator=(ConvertOrientationsToVertexGeometry&&) noexcept = delete;

  /**
   * @brief Converts orientations to vertex positions.
   * @return Success, or a bulk-I/O error.
   *
   * Cancellation returns success with completed chunks preserved.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ConvertOrientationsToVertexGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
