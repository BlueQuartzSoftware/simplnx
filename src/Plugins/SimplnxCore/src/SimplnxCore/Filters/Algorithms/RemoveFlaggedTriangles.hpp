#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{
namespace detail
{
const std::string k_CopySelectedTriangleData("Copy Selected Triangle Data");
const std::string k_CopyAllTriangleData("Copy All Triangle Data");

const nx::core::ChoicesParameter::ChoicesType k_TriangleDataHandlingChoices = {k_CopySelectedTriangleData, k_CopyAllTriangleData};
const nx::core::ChoicesParameter::ValueType k_CopySelectedTriangleArraysIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_CopyAllTriangleArraysIdx = 1ULL;

const std::string k_CopySelectedVertexData("Copy Selected Vertex Data");
const std::string k_CopyAllVertexData("Copy All Vertex Data");

const nx::core::ChoicesParameter::ChoicesType k_VertexDataHandlingChoices = {k_CopySelectedVertexData, k_CopyAllVertexData};
const nx::core::ChoicesParameter::ValueType k_CopySelectedVertexArraysIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_CopyAllVertexArraysIdx = 1ULL;
} // namespace detail

struct SIMPLNXCORE_EXPORT RemoveFlaggedTrianglesInputValues
{
  DataPath triangle_geometry;
  DataPath mask_array_path;
  DataPath reduced_triangle_geometry;
  // These variables are associated with the Edge Data Handling
  nx::core::ChoicesParameter::ValueType triangle_data_handling{};
  MultiArraySelectionParameter::ValueType selected_triangle_data;
  DataPath triangle_attribute_matrix_path;
  // These variables are associated with the Vertex Data Handling
  nx::core::ChoicesParameter::ValueType vertex_data_handling{};
  MultiArraySelectionParameter::ValueType selected_vertex_data;
  DataPath vertex_attribute_matrix_path;
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
