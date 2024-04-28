#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT RemoveFlaggedTrianglesInputValues
{
  DataPath TriangleGeometry;
  DataPath MaskArrayPath;
  DataPath ReducedTriangleGeometry;
};

/**
 * @class ConditionalSetValueFilter

 */
class SIMPLNXCORE_EXPORT RemoveFlaggedTriangles
{
public:
  RemoveFlaggedTriangles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedTrianglesInputValues* inputValues);
  ~RemoveFlaggedTriangles() noexcept = default;

  RemoveFlaggedTriangles(const RemoveFlaggedTriangles&) = delete;
  RemoveFlaggedTriangles(RemoveFlaggedTriangles&&) noexcept = delete;
  RemoveFlaggedTriangles& operator=(const RemoveFlaggedTriangles&) = delete;
  RemoveFlaggedTriangles& operator=(RemoveFlaggedTriangles&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedTrianglesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
