/*=========================================================================
 *
 *  Copyright NumFOCUS
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         https://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#ifndef itkSquaredEdgeLengthDecimationQuadEdgeMeshFilter_h
#define itkSquaredEdgeLengthDecimationQuadEdgeMeshFilter_h

#include "itkEdgeDecimationQuadEdgeMeshFilter.h"

namespace itk
{
/**
 * \class SquaredEdgeLengthDecimationQuadEdgeMeshFilter
 * \brief
 * \ingroup ITKQuadEdgeMeshFiltering
 */
template <typename TInput, typename TOutput, typename TCriterion>
class ITK_TEMPLATE_EXPORT SquaredEdgeLengthDecimationQuadEdgeMeshFilter
  : public EdgeDecimationQuadEdgeMeshFilter<TInput, TOutput, TCriterion>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(SquaredEdgeLengthDecimationQuadEdgeMeshFilter);

  using Self = SquaredEdgeLengthDecimationQuadEdgeMeshFilter;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;
  using Superclass = EdgeDecimationQuadEdgeMeshFilter<TInput, TOutput, TCriterion>;

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(SquaredEdgeLengthDecimationQuadEdgeMeshFilter);

  /** New macro for creation of through a Smart Pointer   */
  itkNewMacro(Self);

  using InputMeshType = TInput;
  using InputMeshPointer = typename InputMeshType::Pointer;

  using OutputMeshType = TOutput;
  using OutputMeshPointer = typename OutputMeshType::Pointer;
  using OutputPointIdentifier = typename OutputMeshType::PointIdentifier;
  using OutputPointType = typename OutputMeshType::PointType;
  using OutputQEType = typename OutputMeshType::QEType;
  using OutputEdgeCellType = typename OutputMeshType::EdgeCellType;
  using OutputCellsContainerIterator = typename OutputMeshType::CellsContainerIterator;

  using CriterionType = TCriterion;
  using MeasureType = typename CriterionType::MeasureType;

  using typename Superclass::PriorityType;
  using typename Superclass::PriorityQueueItemType;
  using typename Superclass::PriorityQueueType;
  using typename Superclass::PriorityQueuePointer;

  using typename Superclass::QueueMapType;
  using typename Superclass::QueueMapIterator;

  using typename Superclass::OperatorType;
  using typename Superclass::OperatorPointer;

protected:
  SquaredEdgeLengthDecimationQuadEdgeMeshFilter();
  ~SquaredEdgeLengthDecimationQuadEdgeMeshFilter() override = default;

  // keep the start of this documentation text on very first comment line,
  // it prevents a Doxygen bug
  /** Compute the measure value for iEdge.
   *
   * \param[in] iEdge
   * \return measure value, here the squared edge length
   */
  MeasureType
  MeasureEdge(OutputQEType * iEdge) override
  {
    const OutputPointIdentifier id_org = iEdge->GetOrigin();
    const OutputPointIdentifier id_dest = iEdge->GetDestination();

    const OutputPointType org = this->m_OutputMesh->GetPoint(id_org);
    const OutputPointType dest = this->m_OutputMesh->GetPoint(id_dest);

    return static_cast<MeasureType>(org.SquaredEuclideanDistanceTo(dest));
  }

  // keep the start of this documentation text on very first comment line,
  // it prevents a Doxygen bug
  /** Calculate the position of the remaining vertex from collapsing iEdge.
   *
   * \param[in] iEdge
   * \return the optimal point location
   */
  OutputPointType
  Relocate(OutputQEType * iEdge) override;
};
} // namespace itk

#include "itkSquaredEdgeLengthDecimationQuadEdgeMeshFilter.hxx"
#endif
