#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace nx::core
{
class BaseGroup;
class DataMap;
class DataObject;

namespace HDF5
{
class DataStructureReader;
class DataStructureWriter;

class GroupIO;
class ObjectIO;

/**
 * @brief Appends each new write diagnostic without copying the accumulated collections.
 * @param accumulated Receives diagnostics and retains failure validity, including empty error collections.
 * @param next Result from the next write operation.
 */
#if SIMPLNX_BUILD_TESTS
// Scoped by tests around a single writer call; consumed only at a growing diagnostic vector.
SIMPLNX_EXPORT void SetWriteDiagnosticAllocationFailure(bool enabled);
SIMPLNX_EXPORT bool WriteDiagnosticAllocationFailureConsumed();
SIMPLNX_EXPORT void FailWriteDiagnosticAllocation();
#endif

/**
 * @brief Appends each new write diagnostic without copying the accumulated collections.
 * @param accumulated Receives diagnostics and retains failure validity, including empty error collections.
 * @param next Result from the next write operation.
 */
inline void AppendWriteResult(Result<>& accumulated, Result<> next)
{
  const bool appendErrors = accumulated.invalid() && next.invalid();
  if(accumulated.valid() && next.invalid())
  {
    // Both vector moves transfer their buffers; no diagnostic elements are allocated here.
    auto earlierWarnings = std::move(accumulated.warnings());
    accumulated.m_Expected = nonstd::make_unexpected(std::move(next.errors()));
    accumulated.warnings() = std::move(next.warnings());
    if(!earlierWarnings.empty())
    {
      try
      {
        // Build separately so a failed prepend leaves the failed operation's warnings intact.
        static_assert(std::is_nothrow_move_constructible_v<Warning>);
        WarningCollection combined;
        if(earlierWarnings.size() > combined.max_size() - accumulated.warnings().size())
        {
          throw std::length_error("Too many write warnings");
        }
#if SIMPLNX_BUILD_TESTS
        FailWriteDiagnosticAllocation();
#endif
        combined.reserve(earlierWarnings.size() + accumulated.warnings().size());
        for(auto& warning : earlierWarnings)
        {
          combined.push_back(std::move(warning));
        }
        for(auto& warning : accumulated.warnings())
        {
          combined.push_back(std::move(warning));
        }
        accumulated.warnings().swap(combined);
      } catch(const std::bad_alloc&)
      {
      } catch(const std::length_error&)
      {
      }
    }
    return;
  }

  auto appendDiagnostics = [&] {
    if(appendErrors)
    {
      for(auto& error : next.errors())
      {
#if SIMPLNX_BUILD_TESTS
        if(accumulated.invalid() && accumulated.errors().size() == accumulated.errors().capacity())
        {
          FailWriteDiagnosticAllocation();
        }
#endif
        accumulated.errors().push_back(std::move(error));
      }
    }
    for(auto& warning : next.warnings())
    {
#if SIMPLNX_BUILD_TESTS
      if(accumulated.invalid() && accumulated.warnings().size() == accumulated.warnings().capacity())
      {
        FailWriteDiagnosticAllocation();
      }
#endif
      accumulated.warnings().push_back(std::move(warning));
    }
  };

  if(accumulated.valid())
  {
    appendDiagnostics();
    return;
  }
  // Keep the first write failure if diagnostic enrichment cannot allocate.
  try
  {
    appendDiagnostics();
  } catch(const std::bad_alloc&)
  {
  } catch(const std::length_error&)
  {
  }
}
/**
 * @brief Attempts to write the DataObject attributes to HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureWriter
 * @param objectWriter
 * @param dataObject
 * @param importable
 * @return Result<>
 */
Result<> SIMPLNX_EXPORT WriteObjectAttributes(DataStructureWriter& dataStructureWriter, ObjectIO& objectWriter, const DataObject* dataObject, bool importable);

/**
 * @brief Attempts to read the DataMap from HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureReader
 * @param dataMap
 * @param parentGroup
 * @param parentId
 * @param useEmptyDataStore = false
 * @return Result<>
 */
Result<> SIMPLNX_EXPORT ReadDataMap(DataStructureReader& dataStructureReader, DataMap& dataMap, const GroupIO& parentGroup, std::optional<DataObject::IdType> parentId, bool useEmptyDataStore = false);

/**
 * @brief Attempts to read the BaseGroup from HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureReader
 * @param groupReader
 * @param baseGroup
 * @param useEmptyDataStore = false
 * @return Result<>
 */
Result<> SIMPLNX_EXPORT ReadBaseGroup(DataStructureReader& dataStructureReader, const GroupIO& groupReader, BaseGroup* baseGroup, bool useEmptyDataStore = false);

/**
 * @brief Attempts to write the BaseGroup to HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureWriter
 * @param groupWriter
 * @param dataMap
 * @param importable = true
 * @return Result<>
 */
Result<> SIMPLNX_EXPORT WriteBaseGroup(DataStructureWriter& dataStructureWriter, GroupIO& groupWriter, const BaseGroup* dataMap, bool importable = true);

/**
 * @brief Attempts to write the DataMap to HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureWriter
 * @param groupWriter
 * @param dataMap
 * @return Result<>
 */
Result<> SIMPLNX_EXPORT WriteDataMap(DataStructureWriter& dataStructureWriter, GroupIO& groupWriter, const DataMap& dataMap);
} // namespace HDF5
} // namespace nx::core
