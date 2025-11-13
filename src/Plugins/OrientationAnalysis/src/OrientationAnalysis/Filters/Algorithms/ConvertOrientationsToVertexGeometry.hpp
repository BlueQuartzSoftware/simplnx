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
 * @brief
 */
struct ORIENTATIONANALYSIS_EXPORT ConvertOrientationsToVertexGeometryInputValues
{
  ebsdlib::orientations::Type InputOrientationType;
  DataPath InputOrientationArrayPath;
  std::vector<DataPath> CopyVertexArrayPaths;
  bool ConvertToFundamentalZone;
  DataPath CellPhasesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath OutputVertexGeometryPath;
  std::string OutputVertexAttrMatrixName;
  std::string OutputSharedVertexListName;
};

/**
 * @brief
 */
class ORIENTATIONANALYSIS_EXPORT ConvertOrientationsToVertexGeometry
{
public:
  ConvertOrientationsToVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                      ConvertOrientationsToVertexGeometryInputValues* inputValues);
  ~ConvertOrientationsToVertexGeometry() noexcept = default;

  ConvertOrientationsToVertexGeometry(const ConvertOrientationsToVertexGeometry&) = delete;
  ConvertOrientationsToVertexGeometry(ConvertOrientationsToVertexGeometry&&) noexcept = delete;
  ConvertOrientationsToVertexGeometry& operator=(const ConvertOrientationsToVertexGeometry&) = delete;
  ConvertOrientationsToVertexGeometry& operator=(ConvertOrientationsToVertexGeometry&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ConvertOrientationsToVertexGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
