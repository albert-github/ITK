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
#ifndef itkQuadEdgeMeshPoint_h
#define itkQuadEdgeMeshPoint_h

#include "itkPoint.h"
#include "itkConceptChecking.h"
#include "itkGeometricalQuadEdge.h"

namespace itk
{
/**
 * \class QuadEdgeMeshPoint
 *
 * \brief Wrapper around a itk::Point in order to add a reference
 * to an entry in the edge ring.
 * \ingroup ITKQuadEdgeMesh
 */
template <typename TCoordinate,
          unsigned int VPointDimension,
          typename TQuadEdge = GeometricalQuadEdge<unsigned long, unsigned long, bool, bool, true>>
class ITK_TEMPLATE_EXPORT QuadEdgeMeshPoint : public Point<TCoordinate, VPointDimension>
{
public:
  /** Standard type alias. */
  using Self = QuadEdgeMeshPoint;
  using Superclass = Point<TCoordinate, VPointDimension>;

  /** Types & values defined in superclass. */
  static constexpr unsigned int PointDimension = VPointDimension;

  using typename Superclass::ValueType;
  using typename Superclass::CoordinateType;
  using typename Superclass::RealType;
  using typename Superclass::Iterator;
  using typename Superclass::ConstIterator;
  using typename Superclass::VectorType;

  using ValueArrayType = ValueType[Self::PointDimension];

public:
  QuadEdgeMeshPoint() = default;
  QuadEdgeMeshPoint(const Self &) = default;
  QuadEdgeMeshPoint(QuadEdgeMeshPoint &&) = default;
  QuadEdgeMeshPoint &
  operator=(const QuadEdgeMeshPoint &) = default;
  QuadEdgeMeshPoint &
  operator=(QuadEdgeMeshPoint &&) = default;
  ~QuadEdgeMeshPoint() = default;

  QuadEdgeMeshPoint(const Superclass & r);

  QuadEdgeMeshPoint(const ValueType r[VPointDimension])
    : Superclass(r)
  {
    this->Initialize();
  }


  Self &
  operator=(const Superclass & r);

  Self &
  operator=(const ValueType r[VPointDimension]);

  /** Accessor on m_Edge */
  void
  SetEdge(TQuadEdge * inputEdge);

  /** Set the coordinates from a standard itk::Point */
  void
  SetPoint(const Superclass & point);

  /** Accessor on m_Edge */
  TQuadEdge *
  GetEdge();

  [[nodiscard]] [[nodiscard]] TQuadEdge *
  GetEdge() const;

  /** Return IsOriginalInternal of the edge.
   * @sa GeometricalQuadEdge::isOriginInternal
   */
  [[nodiscard]] [[nodiscard]] bool
  IsInternal() const;

  /** Return the valence of this QuadEdgeMeshPoint i.e. the number of edges constituting
   *  the Onext ring to which this point belongs.
   *  @return the valence when an entry in the Onext ring is present,
   *          and -1 otherwise.
   */
  [[nodiscard]] [[nodiscard]] int
  GetValence() const;

protected:
  /** Resets the state of m_Edge to nullptr. */
  void
  Initialize();

protected:
  TQuadEdge * m_Edge{}; /**< Entry edge for this point into an Onext ring */
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkQuadEdgeMeshPoint.hxx"
#endif

#endif
