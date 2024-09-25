#include "ReverseTriangleWindingFilter.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/TriangleGeom.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

using namespace nx::core;

namespace
{
/**
 * @brief The ReverseWindingImpl class implements a threaded algorithm that reverses the node
 * windings for a set of triangles
 */
class ReverseWindingImpl
{
public:
  using TriStore = AbstractDataStore<TriangleGeom::SharedFaceList::value_type>;
  explicit ReverseWindingImpl(TriStore& triangles)
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
  TriStore& m_Triangles;
};
} // namespace

namespace nx::core
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
  params.insertSeparator(Parameters::Separator{"Input Triangle Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriGeomPath_Key, "Triangle Geometry", "The DataPath to then input Triangle Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Triangle}));

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
  return {};
}

//------------------------------------------------------------------------------
Result<> ReverseTriangleWindingFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  auto& triangleGeom = dataStructure.getDataRefAs<TriangleGeom>(filterArgs.value<DataPath>(k_TriGeomPath_Key));

  ParallelDataAlgorithm dataAlg;
  dataAlg.setRange(0, triangleGeom.getNumberOfFaces());
  dataAlg.execute(ReverseWindingImpl(triangleGeom.getFaces()->getDataStoreRef()));

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SurfaceDataContainerNameKey = "SurfaceDataContainerName";
} // namespace SIMPL
} // namespace

Result<Arguments> ReverseTriangleWindingFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReverseTriangleWindingFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceDataContainerNameKey, k_TriGeomPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
