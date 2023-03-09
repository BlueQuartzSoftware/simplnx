#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/complex_export.hpp"

#include "complex/Utilities/Parsing/HDF5/Readers/GroupReader.hpp"
#include "complex/Utilities/Parsing/HDF5/Writers/GroupWriter.hpp"

namespace complex
{
class BaseGroup;
class DataMap;
class DataObject;

namespace HDF5
{
class DataStructureReader;
class DataStructureWriter;

class GroupWriter;
class GroupReader;
class ObjectWriter;

/**
 * @brief Attempts to write the DataObject attributes to HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureWriter
 * @param objectWriter
 * @param dataObject
 * @param importable
 * @return Result<>
 */
Result<> COMPLEX_EXPORT WriteObjectAttributes(DataStructureWriter& dataStructureWriter, ObjectWriter& objectWriter, const DataObject* dataObject, bool importable);

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
Result<> COMPLEX_EXPORT ReadDataMap(DataStructureReader& dataStructureReader, DataMap& dataMap, const GroupReader& parentGroup, DataObject::IdType parentId, bool useEmptyDataStore = false);

/**
 * @brief Attempts to read the BaseGroup from HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureReader
 * @param groupReader
 * @param baseGroup
 * @param useEmptyDataStore = false
 * @return Result<>
 */
Result<> COMPLEX_EXPORT ReadBaseGroup(DataStructureReader& dataStructureReader, const GroupReader& groupReader, BaseGroup* baseGroup, bool useEmptyDataStore = false);

/**
 * @brief Attempts to write the BaseGroup to HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureWriter
 * @param groupWriter
 * @param dataMap
 * @param importable = true
 * @return Result<>
 */
Result<> COMPLEX_EXPORT WriteBaseGroup(DataStructureWriter& dataStructureWriter, GroupWriter& groupWriter, const BaseGroup* dataMap, bool importable = true);

/**
 * @brief Attempts to write the DataMap to HDF5.
 * Returns a Result<> with any errors or warnings encountered during the process.
 * @param dataStructureWriter
 * @param groupWriter
 * @param dataMap
 * @return Result<>
 */
Result<> COMPLEX_EXPORT WriteDataMap(DataStructureWriter& dataStructureWriter, GroupWriter& groupWriter, const DataMap& dataMap);
} // namespace HDF5
} // namespace complex
