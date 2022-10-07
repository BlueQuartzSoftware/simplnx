#pragma once

#include "complex/Common/Result.hpp"
#include "complex/DataStructure/DataObject.hpp"
#include "complex/Utilities/Parsing/HDF5/H5GroupReader.hpp"
#include "complex/complex_export.hpp"

namespace complex
{
class BaseGroup;
class DataMap;
class DataObject;

namespace H5
{
class GroupWriter;
class GroupReader;
class ObjectWriter;
} // namespace H5

namespace HDF5
{
class DataStructureReader;
class DataStructureWriter;

Result<> COMPLEX_EXPORT WriteObjectAttributes(DataStructureWriter& dataStructureWriter, H5::ObjectWriter& objectWriter, const DataObject* dataObject, bool importable);

Result<> COMPLEX_EXPORT ReadDataMap(DataStructureReader& reader, DataMap& dataMap, const H5::GroupReader& parentGroup, DataObject::IdType parentId, bool useEmptyDataStore = false);
Result<> COMPLEX_EXPORT ReadBaseGroup(DataStructureReader& reader, const H5::GroupReader& groupReader, BaseGroup* baseGroup, bool useEmptyDataStore = false);
Result<> COMPLEX_EXPORT WriteBaseGroup(DataStructureWriter& writer, H5::GroupWriter& h5Group, const BaseGroup* dataMap, bool importable = true);
Result<> COMPLEX_EXPORT WriteDataMap(DataStructureWriter& dataStructureWriter, H5::GroupWriter& h5Group, const DataMap& dataMap);
} // namespace HDF5
} // namespace complex
