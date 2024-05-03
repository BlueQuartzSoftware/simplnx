#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/SampleSurfaceMesh.hpp"

namespace nx::core
{
struct SIMPLNXCORE_EXPORT UncertainRegularGridSampleSurfaceMeshInputValues
{
  uint64 SeedValue;
  VectorUInt64Parameter::ValueType Dimensions;
  VectorFloat32Parameter::ValueType Spacing;
  VectorFloat32Parameter::ValueType Origin;
  VectorFloat32Parameter::ValueType Uncertainty;
  DataPath TriangleGeometryPath;
  DataPath SurfaceMeshFaceLabelsArrayPath;
  DataPath FeatureIdsArrayPath;
};

/**
 * @class ConditionalSetValueFilter

 */
class SIMPLNXCORE_EXPORT UncertainRegularGridSampleSurfaceMesh : public SampleSurfaceMesh
{
public:
  UncertainRegularGridSampleSurfaceMesh(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                        UncertainRegularGridSampleSurfaceMeshInputValues* inputValues);
  ~UncertainRegularGridSampleSurfaceMesh() noexcept override;

  UncertainRegularGridSampleSurfaceMesh(const UncertainRegularGridSampleSurfaceMesh&) = delete;
  UncertainRegularGridSampleSurfaceMesh(UncertainRegularGridSampleSurfaceMesh&&) noexcept = delete;
  UncertainRegularGridSampleSurfaceMesh& operator=(const UncertainRegularGridSampleSurfaceMesh&) = delete;
  UncertainRegularGridSampleSurfaceMesh& operator=(UncertainRegularGridSampleSurfaceMesh&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

protected:
  void generatePoints(std::vector<Point3Df>& points) override;

private:
  DataStructure& m_DataStructure;
  const UncertainRegularGridSampleSurfaceMeshInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
