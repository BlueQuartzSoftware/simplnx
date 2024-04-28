#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT LabelTriangleGeometryInputValues
{
  DataPath TriangleGeomPath;
  DataPath RegionIdsPath;
  DataPath TriangleAMPath;
  DataPath NumTrianglesPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT LabelTriangleGeometry
{
public:
  LabelTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LabelTriangleGeometryInputValues* inputValues);
  ~LabelTriangleGeometry() noexcept;

  LabelTriangleGeometry(const LabelTriangleGeometry&) = delete;
  LabelTriangleGeometry(LabelTriangleGeometry&&) noexcept = delete;
  LabelTriangleGeometry& operator=(const LabelTriangleGeometry&) = delete;
  LabelTriangleGeometry& operator=(LabelTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const LabelTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
