#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/Arguments.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

class AbstractGeometryGrid;

class COMPLEXCORE_EXPORT SegmentFeatures
{

public:
  SegmentFeatures(DataStructure& data, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);

  virtual ~SegmentFeatures();

  SegmentFeatures(const SegmentFeatures&) = delete;            // Copy Constructor Not Implemented
  SegmentFeatures(SegmentFeatures&&) = delete;                 // Move Constructor Not Implemented
  SegmentFeatures& operator=(const SegmentFeatures&) = delete; // Copy Assignment Not Implemented
  SegmentFeatures& operator=(SegmentFeatures&&) = delete;      // Move Assignment Not Implemented

  /**
   * @brief execute
   * @param gridGeom
   * @return
   */
  Result<> execute(complex::AbstractGeometryGrid* gridGeom);

  /**
   * @brief Returns the seed for the specified values.
   * @param data
   * @param args
   * @param gnum
   * @param nextSeed
   * @return int64
   */
  virtual int64_t getSeed(int32_t gnum, int64 nextSeed) const;

  /**
   * @brief Determines the grouping for the specified values.
   * @param data
   * @param args
   * @param referencePoint
   * @param neighborPoint
   * @param gnum
   * @return bool
   */
  virtual bool determineGrouping(int64_t referencePoint, int64_t neighborPoint, int32_t gnum) const;

  /* from http://www.newty.de/fpt/functor.html */
  /**
   * @brief The CompareFunctor class serves as a functor superclass for specific implementations
   * of performing scalar comparisons
   */
  class CompareFunctor
  {
  public:
    virtual ~CompareFunctor() = default;

    virtual bool operator()(int64 index, int64 neighIndex, int32 gnum) // call using () operator
    {
      return false;
    }
  };

protected:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

private:
};

} // namespace complex
