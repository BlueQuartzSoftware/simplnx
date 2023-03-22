#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT ComputeFeatureRectInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath FeatureRectArrayPath;
};

class COMPLEXCORE_EXPORT ComputeFeatureRect
{
public:
  ComputeFeatureRect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeatureRectInputValues* inputValues);
  ~ComputeFeatureRect() noexcept;

  ComputeFeatureRect(const ComputeFeatureRect&) = delete;
  ComputeFeatureRect(ComputeFeatureRect&&) noexcept = delete;
  ComputeFeatureRect& operator=(const ComputeFeatureRect&) = delete;
  ComputeFeatureRect& operator=(ComputeFeatureRect&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ComputeFeatureRectInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
