#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"

#include <vector>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ResampleImageGeomInputValues
{
  std::vector<float32> Spacing;
  DataPath SelectedImageGeometryPath;
  DataPath CellDataGroupPath;
  bool RemoveOriginalImageGeom;
  DataPath CreatedImageGeometryPath;
  bool RenumberFeatures;
  DataPath FeatureIdsArrayPath;
  DataPath CellFeatureAttributeMatrix;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ResampleImageGeom
{
public:
  ResampleImageGeom(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ResampleImageGeomInputValues* inputValues);
  ~ResampleImageGeom() noexcept;

  ResampleImageGeom(const ResampleImageGeom&) = delete;
  ResampleImageGeom(ResampleImageGeom&&) noexcept = delete;
  ResampleImageGeom& operator=(const ResampleImageGeom&) = delete;
  ResampleImageGeom& operator=(ResampleImageGeom&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ResampleImageGeomInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
