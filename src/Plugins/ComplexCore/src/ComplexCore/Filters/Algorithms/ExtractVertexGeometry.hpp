#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT ExtractVertexGeometryInputValues
{
  ChoicesParameter::ValueType ArrayHandling;
  bool UseMask;
  DataPath MaskArrayPath;
  DataPath InputGeometryPath;
  MultiArraySelectionParameter::ValueType IncludedDataArrayPaths;
  DataPath VertexGeometryPath;
  std::string VertexAttrMatrixName;
  std::string SharedVertexListName;
};

class COMPLEXCORE_EXPORT ExtractVertexGeometry
{
public:
  ExtractVertexGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ExtractVertexGeometryInputValues* inputValues);
  ~ExtractVertexGeometry() noexcept;

  ExtractVertexGeometry(const ExtractVertexGeometry&) = delete;
  ExtractVertexGeometry(ExtractVertexGeometry&&) noexcept = delete;
  ExtractVertexGeometry& operator=(const ExtractVertexGeometry&) = delete;
  ExtractVertexGeometry& operator=(ExtractVertexGeometry&&) noexcept = delete;

  enum class ArrayHandlingType : ChoicesParameter::ValueType
  {
    MoveArrays,
    CopyArrays
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ExtractVertexGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
