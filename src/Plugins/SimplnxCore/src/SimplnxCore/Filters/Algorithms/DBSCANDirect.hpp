#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
struct DBSCANInputValues;

/**
 * @class DBSCANDirect
 * @brief In-core algorithm for DBSCAN. Uses direct per-element getValue()/operator[]
 * access for grid construction and canMerge distance computation. Selected by
 * DispatchAlgorithm when all input arrays are backed by in-memory DataStore.
 */
class SIMPLNXCORE_EXPORT DBSCANDirect
{
public:
  DBSCANDirect(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, const DBSCANInputValues* inputValues);
  ~DBSCANDirect() noexcept;

  DBSCANDirect(const DBSCANDirect&) = delete;
  DBSCANDirect(DBSCANDirect&&) noexcept = delete;
  DBSCANDirect& operator=(const DBSCANDirect&) = delete;
  DBSCANDirect& operator=(DBSCANDirect&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const DBSCANInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
