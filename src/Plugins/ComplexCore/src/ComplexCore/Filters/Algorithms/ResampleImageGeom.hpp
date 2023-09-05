#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

#include <vector>

namespace complex
{

struct COMPLEXCORE_EXPORT ResampleImageGeomInputValues
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
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT ResampleImageGeom
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

} // namespace complex
