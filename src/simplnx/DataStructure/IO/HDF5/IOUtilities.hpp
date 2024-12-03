#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataObject.hpp"
#include "simplnx/simplnx_export.hpp"

#include "simplnx/Utilities/Parsing/HDF5/IO/GroupIO.hpp"

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
