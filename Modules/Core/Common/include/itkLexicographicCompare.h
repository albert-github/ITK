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

#ifndef itkLexicographicCompare_h
#define itkLexicographicCompare_h

#include <algorithm>
#include <iterator> // For std::begin, std::end, and reverse_iterator.
#include "itkMacro.h"


/*
The  Functor was only used in one spot in the
LevelSet class,  It does not exist in Slicer, BRAINSTools, Remote modules,
ANTs, or any other project that I could find.
*/
namespace itk::Functor
{
/** \class LexicographicCompare
 * \brief Order Index instances lexicographically.
 *
 * This is a comparison functor suitable for storing Index instances
 * in an STL container.  The ordering is total and unique but has
 * little geometric meaning.
 * \ingroup ITKCommon
 */
class LexicographicCompare
{
public:
  template <class TAggregateType1, class TAggregateType2>
  bool
  operator()(const TAggregateType1 & lhs, const TAggregateType2 & rhs) const
  {
    return std::lexicographical_compare(std::begin(lhs), std::end(lhs), std::begin(rhs), std::end(rhs));
  }
};


/** \class CoLexicographicCompare
 * \brief Checks if one range of elements colexicographically comes before
 * another one.
 *
 * This is a comparison functor suitable for storing Index and Offset instances
 * in an STL container, and to check if those instances are in colexicographic
 * ("Colex") order. The ordering is total and unique and typically (for example
 * with Image<int,3>) correspond to the order in which the pixels, referred to
 * by the Index or Offset instances, are stored in the image buffer.
 * \ingroup ITKCommon
 */
class CoLexicographicCompare
{
public:
  /* Returns true when lhs comes before rhs. Each argument must be a
   * bidirectional range, for example an Index or an Offset
   */
  template <typename TBidirectionalRange1, typename TBidirectionalRange2>
  bool
  operator()(const TBidirectionalRange1 & lhs, const TBidirectionalRange2 & rhs) const
  {
    // Note that the begin and end arguments are passed in reverse order, as
    // each of them is converted to an std::reverse_iterator!
    return std::lexicographical_compare(std::make_reverse_iterator(std::end(lhs)),
                                        std::make_reverse_iterator(std::begin(lhs)),
                                        std::make_reverse_iterator(std::end(rhs)),
                                        std::make_reverse_iterator(std::begin(rhs)));
  }
};


} // namespace itk::Functor

#endif
