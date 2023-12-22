#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeFeatureRectInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath FeatureDataAttributeMatrixPath;
  DataPath FeatureRectArrayPath;
};

class SIMPLNXCORE_EXPORT ComputeFeatureRect
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

} // namespace nx::core
