#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadVtkStructuredPointsInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  bool ReadPointData;
  bool ReadCellData;
  DataPath PointGeometryPath;
  DataPath CellGeometryPath;
  std::string PointAttributeMatrixName;
  std::string CellAttributeMatrixName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SIMPLNXCORE_EXPORT ReadVtkStructuredPoints
{
public:
  ReadVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadVtkStructuredPointsInputValues* inputValues);
  ~ReadVtkStructuredPoints() noexcept;

  ReadVtkStructuredPoints(const ReadVtkStructuredPoints&) = delete;
  ReadVtkStructuredPoints(ReadVtkStructuredPoints&&) noexcept = delete;
  ReadVtkStructuredPoints& operator=(const ReadVtkStructuredPoints&) = delete;
  ReadVtkStructuredPoints& operator=(ReadVtkStructuredPoints&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReadVtkStructuredPointsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
