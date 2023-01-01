#pragma once

#include <map>
#include <string>

// clang-format off
#include "Core/Filters/AlignSectionsFeatureCentroidFilter.hpp"
#include "Core/Filters/FillBadDataFilter.hpp"
#include "Core/Filters/FindNeighborhoodsFilter.hpp"
#include "Core/Filters/RemoveFlaggedFeaturesFilter.hpp"
#include "Core/Filters/ErodeDilateBadDataFilter.hpp"
// @@__HEADER__TOKEN__DO__NOT__DELETE__@@

namespace complex
{
  static const std::map<complex::Uuid, complex::Uuid> k_SIMPL_to_Core
  {
    // syntax std::make_pair {Dream3d UUID , Dream3dnx UUID}, // dream3d-class-name
    {complex::Uuid::FromString("886f8b46-51b6-5682-a289-6febd10b7ef0").value(), complex::FilterTraits<AlignSectionsFeatureCentroidFilter>::uuid}, // AlignSectionsFeatureCentroid
    {complex::Uuid::FromString("30ae0a1e-3d94-5dab-b279-c5727ab5d7ff").value(), complex::FilterTraits<FillBadDataFilter>::uuid}, // FillBadData
    {complex::Uuid::FromString("697ed3de-db33-5dd1-a64b-04fb71e7d63e").value(), complex::FilterTraits<FindNeighborhoodsFilter>::uuid}, // FindNeighborhoods
    {complex::Uuid::FromString("a8463056-3fa7-530b-847f-7f4cb78b8602").value(), complex::FilterTraits<RemoveFlaggedFeaturesFilter>::uuid}, // RemoveFlaggedFeatures
    {complex::Uuid::FromString("3adfe077-c3c9-5cd0-ad74-cf5f8ff3d254").value(), complex::FilterTraits<ErodeDilateBadDataFilter>::uuid}, // ErodeDilateBadData
    // @@__MAP__UPDATE__TOKEN__DO__NOT__DELETE__@@
  };

} // namespace complex
// clang-format on
