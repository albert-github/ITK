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
#ifndef itkFlatStructuringElement_h
#define itkFlatStructuringElement_h

#include "itkNeighborhood.h"
#include <vector>
#include "itkVector.h"
#include "itkImage.h"

namespace itk
{
/** \class FlatStructuringElement
 * \brief A class to support a variety of flat structuring elements,
 * including versions created by decomposition of lines.
 *
 * FlatStructuringElement provides several static methods, which can
 * be used to create a structuring element with a particular shape,
 * size, etc. Currently, those methods enable the creation of the following
 * structuring elements: ball, box, cross, annulus, or polygon.
 * Polygons are available as fast approximations of balls using line
 * decompositions. Boxes also use line decompositions.
 *
 * "Flat" refers to binary as opposed to grayscale structuring elements. Flat
 * structuring elements can be used for both binary and grayscale images.
 *
 * A Neighborhood has an N-dimensional \em radius.  The radius is defined
 * separately for each dimension as the number of pixels that the neighborhood
 * extends outward from the center pixel.  For example, a 2D Neighborhood
 * object with a radius of 2x3 has sides of length 5x7.
 * However, in the case of balls and annuli, this definition is slightly
 * different from the parametric definition of those objects.
 * For example, an ellipse of radius 2x3 has a diameter of 4x6, not 5x7.
 * To have a diameter of 5x7, the radius would need to increase by 0.5
 * in each dimension.
 * Thus, the "radius" of the neighborhood and the "radius" of the
 * object should be distinguished.
 *
 * To accomplish this, the "ball" and "annulus" structuring elements
 * have an optional flag called "radiusIsParametric" (off by default).
 * Setting this flag to true will use the parametric definition of the object
 * and will generate structuring elements with more accurate areas,
 * which can be especially important when morphological operations are
 * intended to remove or retain objects of particular sizes.
 * When the mode is turned off (default), the radius is the same, but the object
 * diameter is set to (radius*2)+1, which is the size of the neighborhood region.
 * Thus, the original ball and annulus structuring
 * elements have a systematic bias in the radius of +0.5 voxels in each dimension
 * relative to the parametric definition of the radius.
 * Thus, we recommend turning this mode on for more accurate structuring elements,
 * but this mode is turned off by default for backward compatibility.
 *
 * As an example, a 3D ball of radius 5 should have an area of 523.
 * With this mode turned on, the number of "on" pixels is 515 (error 1.6%),
 * but with it turned off, the area is 739 (error 41%).
 * For a 3D annulus of radius 5 and thickness 2, the area should be 410.
 * With this mode turned on, the area is 392 (error 4.5%),
 * but when turned off it is 560 (error 36%).
 * This same trend holds for balls and annuli of any radius or dimension.
 * For more detailed experiments with this mode, please refer to the results of the
 * test itkFlatStructuringElementTest.cxx or the wiki example.
 *
 * \ingroup ITKMathematicalMorphology
 *
 * \sphinx
 * \sphinxexample{Filtering/MathematicalMorphology/ErodeBinaryImageUsingFlatStruct,Erode Binary Image Using Flat
 * Structure Element} \endsphinx \sphinx
 * \sphinxexample{Filtering/MathematicalMorphology/GenerateStructureElementsWithAccurateArea,Generate Structuring
 * Elements With Accurate Area} \endsphinx
 */

template <unsigned int VDimension>
class ITK_TEMPLATE_EXPORT FlatStructuringElement : public Neighborhood<bool, VDimension>
{
public:
  /** Standard class type aliases. */
  using Self = FlatStructuringElement<VDimension>;
  using Superclass = Neighborhood<bool, VDimension>;

  /** External support for pixel type. */
  using typename Superclass::PixelType;

  /** Iterator type alias support Note the naming is intentional, i.e.,
   * AllocatorType::iterator and AllocatorType::const_iterator, because the
   * allocator may be a vnl object or other type, which uses this form. */
  using typename Superclass::Iterator;
  using typename Superclass::ConstIterator;

  /** Size and value type alias support */
  using typename Superclass::SizeType;
  using typename Superclass::OffsetType;

  /** Radius type alias support */
  using typename Superclass::RadiusType;

  /** External slice iterator type alias support */
  using typename Superclass::SliceIteratorType;

  /** External support for dimensionality. */
  static constexpr unsigned int NeighborhoodDimension = VDimension;

  using LType = Vector<float, VDimension>;
  using DecompType = std::vector<LType>;

  /** ImageType used in constructors */
  using ImageType = typename itk::Image<PixelType, VDimension>;

  /** Default destructor. */
  ~FlatStructuringElement() override = default;

  /** Default constructor. */
  FlatStructuringElement() = default;

  /** Various constructors */

  /**
   * Create a box structuring element. The structuring element is
   * is decomposable.
   */
  static Self
  Box(RadiusType radius);

  /** Create a ball structuring element */
  static Self
  Ball(RadiusType radius, bool radiusIsParametric = false);

  /** Create a cross structuring element */
  static Self
  Cross(RadiusType radius);

  /** Create an annulus structuring element */
  static Self
  Annulus(RadiusType radius, unsigned int thickness = 1, bool includeCenter = false, bool radiusIsParametric = false);

  /**
   * Create a polygon structuring element. The structuring element is
   * is decomposable.
   * lines is the number of elements in the decomposition
   */
  static Self
  Polygon(RadiusType radius, unsigned int lines);

  /**
   * Returns whether the structuring element is decomposable or not. If the
   * structuring is decomposable, the set of lines associated with the
   * structuring may be used by an algorithm instead of the standard buffer.
   */
  /** @ITKStartGrouping */
  [[nodiscard]] bool
  GetDecomposable() const
  {
    return m_Decomposable;
  }
  void
  SetDecomposable(bool v)
  {
    m_Decomposable = v;
  }
  /** @ITKEndGrouping */
  /** Return the lines associated with the structuring element */
  /** @ITKStartGrouping */
  [[nodiscard]] const DecompType &
  GetLines() const
  {
    return (m_Lines);
  }
  void
  AddLine(LType l)
  {
    m_Lines.push_back(l);
  }
  /** @ITKEndGrouping */
  [[nodiscard]] bool
  CheckParallel(LType NewVec) const;

  /**
   * Fill the buffer of the structuring element based on the lines
   * associated to the structuring element
   */
  void
  ComputeBufferFromLines();

  /**
   * The RadiusIsParametric mode ensures that the area of the foreground
   * corresponds to the radius that was specified.
   * This defaults to "off" for backward compatibility.
   */
  /** @ITKStartGrouping */
  [[nodiscard]] bool
  GetRadiusIsParametric() const
  {
    return m_RadiusIsParametric;
  }
  void
  SetRadiusIsParametric(bool flag)
  {
    m_RadiusIsParametric = flag;
  }
  itkBooleanMacro(RadiusIsParametric);
  /** @ITKEndGrouping */
  /** Create a FlatStructureElement from a bool
   *  image. Image must be odd in all dimensions.*/
  static Self
  FromImage(const ImageType * image);

protected:
  void
  PrintSelf(std::ostream & os, Indent indent) const override;

private:
  bool m_Decomposable{};

  DecompType m_Lines{};

  template <unsigned int VDimension3>
  struct StructuringElementFacet
  {
    Vector<float, VDimension3> P1, P2, P3;
  };
  using FacetType = StructuringElementFacet<VDimension>;

  template <typename TStructuringElement, typename TRadius>
  static void
  GeneratePolygon(TStructuringElement & res, TRadius radius, unsigned int lines);
  static void
  GeneratePolygon(itk::FlatStructuringElement<2> & res, itk::Size<2> radius, unsigned int lines);
  static void
  GeneratePolygon(itk::FlatStructuringElement<3> & res, itk::Size<3> radius, unsigned int lines);

  using LType2 = Vector<float, 2>;

  using LType3 = Vector<float, 3>;
  using FacetType3 = StructuringElementFacet<3>;

  bool m_RadiusIsParametric{};

  /** Check for correct odd size image.
   *  Return image size. Called in constructor FromImage.*/
  static RadiusType
  CheckImageSize(const ImageType * image);
};
} // namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkFlatStructuringElement.hxx"
#endif

#endif
