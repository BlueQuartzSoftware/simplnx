#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT IterativeClosestPointInputValues
{
  bool ApplyTransformation;
  uint64 NumIterations;
  DataPath MovingVertexPath;
  DataPath TargetVertexPath;
  DataPath TransformArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT IterativeClosestPoint
{
public:
  IterativeClosestPoint(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IterativeClosestPointInputValues* inputValues);
  ~IterativeClosestPoint() noexcept;

  IterativeClosestPoint(const IterativeClosestPoint&) = delete;
  IterativeClosestPoint(IterativeClosestPoint&&) noexcept = delete;
  IterativeClosestPoint& operator=(const IterativeClosestPoint&) = delete;
  IterativeClosestPoint& operator=(IterativeClosestPoint&&) noexcept = delete;

  Result<> operator()();
  void updateProgress(const std::string& message);
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const IterativeClosestPointInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
