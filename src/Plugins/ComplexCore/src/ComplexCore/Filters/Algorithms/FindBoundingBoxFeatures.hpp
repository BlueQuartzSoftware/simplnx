#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT FindBoundingBoxFeaturesInputValues
{
  bool CalcByPhase;
  DataPath ImageGeometryPath;
  DataPath CentroidsArrayPath;
  DataPath SurfaceFeaturesArrayPath;
  DataPath PhasesArrayPath;
  DataPath BiasedFeaturesArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT FindBoundingBoxFeatures
{
public:
  FindBoundingBoxFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBoundingBoxFeaturesInputValues* inputValues);
  ~FindBoundingBoxFeatures() noexcept;

  FindBoundingBoxFeatures(const FindBoundingBoxFeatures&) = delete;
  FindBoundingBoxFeatures(FindBoundingBoxFeatures&&) noexcept = delete;
  FindBoundingBoxFeatures& operator=(const FindBoundingBoxFeatures&) = delete;
  FindBoundingBoxFeatures& operator=(FindBoundingBoxFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  void findBoundingBoxFeatures();
  void findBoundingBoxFeatures2D();

private:
  DataStructure& m_DataStructure;
  const FindBoundingBoxFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
