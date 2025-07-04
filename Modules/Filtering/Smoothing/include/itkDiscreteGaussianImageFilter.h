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
#ifndef itkDiscreteGaussianImageFilter_h
#define itkDiscreteGaussianImageFilter_h

#include "itkGaussianOperator.h"
#include "itkImageToImageFilter.h"
#include "itkImage.h"
#include "itkZeroFluxNeumannBoundaryCondition.h"

namespace itk
{
/**
 * \class DiscreteGaussianImageFilter
 * \brief Blurs an image by separable convolution with discrete gaussian kernels.
 * This filter performs Gaussian blurring by separable convolution of an image
 * and a discrete Gaussian operator (kernel).
 *
 * The Gaussian operator used here was described by Tony Lindeberg in
 * \cite lindeberg1991. The Gaussian kernel used here was designed so
 * that smoothing and derivative operations commute after discretization.
 *
 * The variance or standard deviation (sigma) will be evaluated as pixel units
 * if SetUseImageSpacing is off (false) or as physical units if
 * SetUseImageSpacing is on (true, default). The variance can be set
 * independently in each dimension.
 *
 * When the Gaussian kernel is small, this filter tends to run faster than
 * itk::RecursiveGaussianImageFilter.
 *
 * \sa GaussianOperator
 * \sa Image
 * \sa Neighborhood
 * \sa NeighborhoodOperator
 * \sa RecursiveGaussianImageFilter
 *
 * \ingroup ImageEnhancement
 * \ingroup ImageFeatureExtraction
 * \ingroup ITKSmoothing
 *
 * \sphinx
 * \sphinxexample{Filtering/Smoothing/SmoothWithRecursiveGaussian,Computes the smoothing with Gaussian kernel}
 * \endsphinx
 */

template <typename TInputImage, typename TOutputImage = TInputImage>
class ITK_TEMPLATE_EXPORT DiscreteGaussianImageFilter : public ImageToImageFilter<TInputImage, TOutputImage>
{
public:
  ITK_DISALLOW_COPY_AND_MOVE(DiscreteGaussianImageFilter);

  /** Standard class type aliases. */
  using Self = DiscreteGaussianImageFilter;
  using Superclass = ImageToImageFilter<TInputImage, TOutputImage>;
  using Pointer = SmartPointer<Self>;
  using ConstPointer = SmartPointer<const Self>;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** \see LightObject::GetNameOfClass() */
  itkOverrideGetNameOfClassMacro(DiscreteGaussianImageFilter);

  /** Image type information. */
  using InputImageType = TInputImage;
  using OutputImageType = TOutputImage;

  /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
  using OutputPixelType = typename TOutputImage::PixelType;
  using OutputInternalPixelType = typename TOutputImage::InternalPixelType;
  using InputPixelType = typename TInputImage::PixelType;
  using InputInternalPixelType = typename TInputImage::InternalPixelType;

  /** Pixel value type for Vector pixel types **/
  using InputPixelValueType = typename NumericTraits<InputPixelType>::ValueType;
  using OutputPixelValueType = typename NumericTraits<OutputPixelType>::ValueType;

  /** Extract some information from the image types.  Dimensionality
   * of the two images is assumed to be the same. */
  static constexpr unsigned int ImageDimension = TOutputImage::ImageDimension;

  /** Type of the pixel to use for intermediate results */
  using RealOutputPixelType = typename NumericTraits<OutputPixelType>::RealType;
  using RealOutputImageType = Image<OutputPixelType, ImageDimension>;
  using RealOutputPixelValueType = typename NumericTraits<RealOutputPixelType>::ValueType;

  /** Typedef to describe the boundary condition. */
  using BoundaryConditionType = ImageBoundaryCondition<TInputImage>;
#ifndef ITK_FUTURE_LEGACY_REMOVE
  using InputBoundaryConditionPointerType [[deprecated("Please just use `BoundaryConditionType *` instead!")]] =
    BoundaryConditionType *;
#endif
  using InputDefaultBoundaryConditionType = ZeroFluxNeumannBoundaryCondition<TInputImage>;
  using RealBoundaryConditionPointerType = ImageBoundaryCondition<RealOutputImageType> *;
  using RealDefaultBoundaryConditionType = ZeroFluxNeumannBoundaryCondition<RealOutputImageType>;

  /** Typedef of double containers */
  using ArrayType = FixedArray<double, Self::ImageDimension>;
  using SigmaArrayType = ArrayType;
  using ScalarRealType = double;

  /** Type of kernel to be used in blurring */
  using KernelType = GaussianOperator<RealOutputPixelValueType, ImageDimension>;
  using RadiusType = typename KernelType::RadiusType;

  /** The variance for the discrete Gaussian kernel.  Sets the variance
   * independently for each dimension, but
   * see also SetVariance(const double v). The default is 0.0 in each
   * dimension. If UseImageSpacing is true, the units are the physical units
   * of your image.  If UseImageSpacing is false then the units are
   * pixels. */
  /** @ITKStartGrouping */
  itkSetMacro(Variance, ArrayType);
  itkGetConstMacro(Variance, const ArrayType);
  /** @ITKEndGrouping */

  /** The algorithm will size the discrete kernel so that the error
   * resulting from truncation of the kernel is no greater than
   * MaximumError. The default is 0.01 in each dimension. */
  /** @ITKStartGrouping */
  itkSetMacro(MaximumError, ArrayType);
  itkGetConstMacro(MaximumError, const ArrayType);
  /** @ITKEndGrouping */

  /** Set the kernel to be no wider than MaximumKernelWidth pixels,
   *  even if MaximumError demands it. The default is 32 pixels. */
  /** @ITKStartGrouping */
  itkGetConstMacro(MaximumKernelWidth, unsigned int);
  itkSetMacro(MaximumKernelWidth, unsigned int);
  /** @ITKEndGrouping */

  /** Set the number of dimensions to smooth. Defaults to the image
   * dimension. Can be set to less than ImageDimension, smoothing all
   * the dimensions less than FilterDimensionality.  For instance, to
   * smooth the slices of a volume without smoothing in Z, set the
   * FilterDimensionality to 2. */
  /** @ITKStartGrouping */
  itkGetConstMacro(FilterDimensionality, unsigned int);
  itkSetMacro(FilterDimensionality, unsigned int);
  /** @ITKEndGrouping */

  /** Set/get the boundary condition. */
  /** @ITKStartGrouping */
  itkSetMacro(InputBoundaryCondition, BoundaryConditionType *);
  itkGetConstMacro(InputBoundaryCondition, BoundaryConditionType *);
  itkSetMacro(RealBoundaryCondition, RealBoundaryConditionPointerType);
  itkGetConstMacro(RealBoundaryCondition, RealBoundaryConditionPointerType);
  /** @ITKEndGrouping */

  /** Convenience Set methods for setting all dimensional parameters
   *  to the same values. */
  void
  SetVariance(const typename ArrayType::ValueType v)
  {
    m_Variance.Fill(v);
    this->Modified();
  }

  void
  SetMaximumError(const typename ArrayType::ValueType v)
  {
    m_MaximumError.Fill(v);
    this->Modified();
  }

  void
  SetVariance(const double * v)
  {
    ArrayType dv;

    for (unsigned int i = 0; i < ImageDimension; ++i)
    {
      dv[i] = v[i];
    }
    this->SetVariance(dv);
  }

  void
  SetVariance(const float * v)
  {
    ArrayType dv;

    for (unsigned int i = 0; i < ImageDimension; ++i)
    {
      dv[i] = v[i];
    }
    this->SetVariance(dv);
  }

  /** Set the standard deviation of the Gaussian used for smoothing.
   * Sigma is measured in the units of image spacing. */
  void
  SetSigma(const ArrayType & sigma)
  {
    ArrayType variance;
    for (unsigned int i = 0; i < ImageDimension; ++i)
    {
      variance[i] = sigma[i] * sigma[i];
    }
    this->SetVariance(variance);
  }

  /** SetSigmaArray is preserved for interface
   *  backwards compatibility. */
  /** @ITKStartGrouping */
  void
  SetSigmaArray(const ArrayType & sigmas)
  {
    this->SetSigma(sigmas);
  }
  void
  SetSigma(double sigma)
  {
    this->SetVariance(sigma * sigma);
  }
  /** @ITKEndGrouping */

  /** Get the Sigma value. */
  ArrayType
  GetSigmaArray() const
  {
    ArrayType sigmas;
    for (unsigned int i = 0; i < ImageDimension; ++i)
    {
      sigmas[i] = std::sqrt(m_Variance[i]);
    }
    return sigmas;
  }

  /** Get the Sigma scalar. If the Sigma is anisotropic, we will just
   * return the Sigma along the first dimension. */
  [[nodiscard]] double
  GetSigma() const
  {
    return std::sqrt(m_Variance[0]);
  }

  void
  SetMaximumError(const double * v)
  {
    ArrayType dv;

    for (unsigned int i = 0; i < ImageDimension; ++i)
    {
      dv[i] = v[i];
    }
    this->SetMaximumError(dv);
  }

  void
  SetMaximumError(const float * v)
  {
    ArrayType dv;

    for (unsigned int i = 0; i < ImageDimension; ++i)
    {
      dv[i] = v[i];
    }
    this->SetMaximumError(dv);
  }

  /** Get the radius of the generated directional kernel */
  [[nodiscard]] unsigned int
  GetKernelRadius(const unsigned int dimension) const;

  /** Get the radius of the separable kernel in each direction */
  ArrayType
  GetKernelRadius() const;

  /** Get the size of the separable kernel in each direction.
   *  size[i] = radius[i] * 2 + 1 */
  ArrayType
  GetKernelSize() const;

  /** Set/Get whether or not the filter will use the spacing of the input
   * image in its calculations. Use On to take the image spacing information
   * into account and to specify the Gaussian variance in real world units;
   * use Off to ignore the image spacing and to specify the Gaussian variance
   * in voxel units. Default is On. */
  /** @ITKStartGrouping */
  itkSetMacro(UseImageSpacing, bool);
  itkGetConstMacro(UseImageSpacing, bool);
  itkBooleanMacro(UseImageSpacing);
  /** @ITKEndGrouping */

#if !defined(ITK_FUTURE_LEGACY_REMOVE)
  /** Use the image spacing information in calculations. Use this option if you
   *  want to specify Gaussian variance in real world units.  Default is
   *  ImageSpacingOn.
   *  \deprecated Use DiscreteGaussianImageFilter::UseImageSpacingOn instead. */
  void
  SetUseImageSpacingOn()
  {
    this->SetUseImageSpacing(true);
  }

  /** Ignore the image spacing. Use this option if you want to specify Gaussian
      variance in pixels.  Default is ImageSpacingOn.
      \deprecated Use DiscreteGaussianImageFilter::UseImageSpacingOff instead. */
  void
  SetUseImageSpacingOff()
  {
    this->SetUseImageSpacing(false);
  }
#endif

  /** \brief Set/Get number of pieces to divide the input for the
   * internal composite pipeline. The upstream pipeline will not be
   * effected.
   *
   * The default value is $ImageDimension^2$.
   *
   * This parameter was introduced to reduce the memory used by images
   * internally, at the cost of performance.
   */
  itkLegacyMacro(unsigned int GetInternalNumberOfStreamDivisions() const;)
  itkLegacyMacro(void SetInternalNumberOfStreamDivisions(unsigned int);)

  itkConceptMacro(OutputHasNumericTraitsCheck, (Concept::HasNumericTraits<OutputPixelValueType>));

protected:
  DiscreteGaussianImageFilter()
    : m_MaximumKernelWidth(32)
    , m_FilterDimensionality(ImageDimension)
    , m_UseImageSpacing(true)
  {
    m_Variance.Fill(0.0);
    m_MaximumError.Fill(0.01);

    m_InputBoundaryCondition = &m_InputDefaultBoundaryCondition;
    m_RealBoundaryCondition = &m_RealDefaultBoundaryCondition;
  }

  ~DiscreteGaussianImageFilter() override = default;
  void
  PrintSelf(std::ostream & os, Indent indent) const override;

  /** DiscreteGaussianImageFilter needs a larger input requested region
   * than the output requested region (larger by the size of the
   * Gaussian kernel).  As such, DiscreteGaussianImageFilter needs to
   * provide an implementation for GenerateInputRequestedRegion() in
   * order to inform the pipeline execution model.
   * \sa ImageToImageFilter::GenerateInputRequestedRegion() */
  void
  GenerateInputRequestedRegion() override;

  /** Standard pipeline method. While this class does not implement a
   * ThreadedGenerateData(), its GenerateData() delegates all
   * calculations to an NeighborhoodOperatorImageFilter.  Since the
   * NeighborhoodOperatorImageFilter is multithreaded, this filter is
   * multithreaded by default. */
  void
  GenerateData() override;

  /** Build a directional kernel to match user specifications */
  void
  GenerateKernel(const unsigned int dimension, KernelType & oper) const;

  /** Get the variance, optionally adjusted for pixel spacing */
  ArrayType
  GetKernelVarianceArray() const;

private:
  /** The variance of the gaussian blurring kernel in each dimensional
    direction. */
  ArrayType m_Variance{};

  /** The maximum error of the gaussian blurring kernel in each dimensional
   * direction. For definition of maximum error, see GaussianOperator.
   * \sa GaussianOperator */
  ArrayType m_MaximumError{};

  /** Maximum allowed kernel width for any dimension of the discrete Gaussian
      approximation */
  unsigned int m_MaximumKernelWidth{};

  /** Number of dimensions to process. Default is all dimensions */
  unsigned int m_FilterDimensionality{};

  /** Flag to indicate whether to use image spacing */
  bool m_UseImageSpacing{};

  /** Pointer to a persistent boundary condition object used
   ** for the image iterator. */
  BoundaryConditionType * m_InputBoundaryCondition{};

  /** Default boundary condition */
  InputDefaultBoundaryConditionType m_InputDefaultBoundaryCondition{};

  /** Boundary condition use for the intermediate filters */
  RealBoundaryConditionPointerType m_RealBoundaryCondition{};

  /** Default boundary condition use for the intermediate filters */
  RealDefaultBoundaryConditionType m_RealDefaultBoundaryCondition{};
};
} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#  include "itkDiscreteGaussianImageFilter.hxx"
#endif

#endif
