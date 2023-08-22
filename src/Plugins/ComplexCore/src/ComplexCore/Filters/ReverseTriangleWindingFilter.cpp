#include "ReverseTriangleWindingFilter.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/Geometry/TriangleGeom.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Utilities/ParallelDataAlgorithm.hpp"

using namespace complex;

namespace
{
/**
 * @brief The ReverseWindingImpl class implements a threaded algorithm that reverses the node
 * windings for a set of triangles
 */
class ReverseWindingImpl
{
public:
  explicit ReverseWindingImpl(TriangleGeom::SharedFaceList& triangles)
  : m_Triangles(triangles)
  {
  }
  ~ReverseWindingImpl() = default;

  void generate(usize start, usize end) const
  {

    for(size_t i = start; i < end; i++)
    {
      // Swap the indices
      TriangleGeom::MeshIndexType nId0 = m_Triangles[i * 3 + 0];
      TriangleGeom::MeshIndexType nId2 = m_Triangles[i * 3 + 2];

      m_Triangles[i * 3 + 0] = nId2;
      m_Triangles[i * 3 + 2] = nId0;
    }
  }

  void operator()(const Range& range) const
  {
    generate(range.min(), range.max());
  }

private:
  TriangleGeom::SharedFaceList& m_Triangles;
};
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ReverseTriangleWindingFilter::name() const
{
  return FilterTraits<ReverseTriangleWindingFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReverseTriangleWindingFilter::className() const
{
  return FilterTraits<ReverseTriangleWindingFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReverseTriangleWindingFilter::uuid() const
{
  return FilterTraits<ReverseTriangleWindingFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReverseTriangleWindingFilter::humanName() const
{
  return "Reverse Triangle Winding";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReverseTriangleWindingFilter::defaultTags() const
{
  return {className(), "Surface Meshing", "Connectivity Arrangement"};
}

//------------------------------------------------------------------------------
Parameters ReverseTriangleWindingFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Required Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeomPath_Key, "Triangle Geometry", "The DataPath to then input Triangle Geometry", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReverseTriangleWindingFilter::clone() const
{
  return std::make_unique<ReverseTriangleWindingFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReverseTriangleWindingFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReverseTriangleWindingFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(filterArgs.value<DataPath>(k_TriGeomPath_Key));

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, triangleGeom.getNumberOfFaces());
  dataAlg.execute(ReverseWindingImpl(triangleGeom.getFacesRef()));

  return {};
}
} // namespace complex
