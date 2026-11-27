#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT IterativeClosestPointInputValues
{
  bool apply_transformation{};
  uint64 num_iterations{};
  DataPath moving_vertex_path;
  DataPath target_vertex_path;
  DataPath transform_array_path;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT IterativeClosestPoint
{
public:
  IterativeClosestPoint(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, IterativeClosestPointInputValues* inputValues);
  ~IterativeClosestPoint() noexcept = default;

  IterativeClosestPoint(const IterativeClosestPoint&) = delete;
  IterativeClosestPoint(IterativeClosestPoint&&) noexcept = delete;
  IterativeClosestPoint& operator=(const IterativeClosestPoint&) = delete;
  IterativeClosestPoint& operator=(IterativeClosestPoint&&) noexcept = delete;

  Result<> operator()();
  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const IterativeClosestPointInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
