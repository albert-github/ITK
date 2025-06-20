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
#ifndef itkLaplacianSharpeningImageFilter_h
#define itkLaplacianSharpeningImageFilter_h

#include "itkNumericTraits.h"
#include "itkImageToImageFilter.h"

namespace itk
{
/**
 * \class LaplacianSharpeningImageFilter
 * \brief This filter sharpens an image using a Laplacian.
 * LaplacianSharpening highlights regions of rapid intensity change
 * and therefore highlights or enhances the edges.  The result is an
 * image that appears more in focus.
 *
 * \par The LaplacianSharpening at each pixel location is computed by
 * convolution with the itk::LaplacianOperator.
 *
 * \par Inputs and Outputs
 * The input to this filter is a scalar-valued itk::Image of arbitrary
 * dimension. The output is a scalar-valued itk::Image.
 * \sa Image
 * \sa Neighborhood
 * \sa NeighborhoodOperator
 * \sa NeighborhoodIterator
 * \sa LaplacianOperator
 *
 * \ingroup ImageFeatureExtraction
 * \ingroup ITKImageFeature
 *
 * \sphinx
 * \sphinxexample{Filtering/ImageFeature/SharpenImage,Sharpen Image}
 * \endsphinx
 */
template <typename TInputImage, typename TOutputImage>
class ITK_TEMPLATE_EXPORT LaplacianSharpeningImageFilter : public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(LaplacianSharpeningImageFilter);

  /** Standard "Self" & Superclass type alias.   */
  using Self = LaplacianSharpeningImageFilter;
  using Superclass = ImageToImageFilter<TInputImage, TOutputImage>;

  /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
  using OutputPixelType = typename TOutputImage::PixelType;
  using OutputInternalPixelType = typename TOutputImage::InternalPixelType;
  using RealType = typename NumericTraits<OutputPixelType>::RealType;
  using InputPixelType = typename TInputImage::PixelType;
  using InputInternalPixelType = typename TInputImage::InternalPixelType;
  static constexpr unsigned int InputImageDimension = TInputImage::ImageDimension;
  static constexpr unsigned int ImageDimension = TOutputImage::ImageDimension;

  /** Image type alias support */
  using InputImageType = TInputImage;
  using OutputImageType = TOutputImage;
  using InputImagePointer = typename InputImageType::Pointer;

  /** Smart pointer type alias support   */
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(LaplacianSharpeningImageFilter);

  /** Method for creation through the object factory.  */
  itkNewMacro(Self);

  /** Set/Get whether or not the filter will use the spacing information of the input image in its calculations. Use
   * this option if derivatives are required in physical space. */
  /** @ITKStartGrouping */
  itkBooleanMacro(UseImageSpacing);
  itkSetMacro(UseImageSpacing, bool);
  itkGetConstMacro(UseImageSpacing, bool);
  /** @ITKEndGrouping */
  itkConceptMacro(SameDimensionCheck, (Concept::SameDimension<InputImageDimension, ImageDimension>));

protected:
  LaplacianSharpeningImageFilter()
    : m_UseImageSpacing(true)
  {}

  ~LaplacianSharpeningImageFilter() override = default;

  /** This filter must produce all of its output at once. */
  void
  EnlargeOutputRequestedRegion(DataObject * output) override;

  /** Standard pipeline method. While this class does not implement a
   * ThreadedGenerateData(), its GenerateData() delegates all
   * calculations to an NeighborhoodOperatorImageFilter.  Since the
   * NeighborhoodOperatorImageFilter is multithreaded, this filter is
   * multithreaded by default.   */
  void
  GenerateData() override;

  void
  PrintSelf(std::ostream &, Indent) const override;

private:
  bool m_UseImageSpacing{};
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkLaplacianSharpeningImageFilter.hxx"
#endif

#endif
