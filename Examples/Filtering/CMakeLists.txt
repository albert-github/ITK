project(FilteringExamples)

add_executable(CompositeFilterExample CompositeFilterExample.cxx)
target_link_libraries(CompositeFilterExample ${ITK_LIBRARIES})
add_executable(FlipImageFilter FlipImageFilter.cxx)
target_link_libraries(FlipImageFilter ${ITK_LIBRARIES})
add_executable(FFTImageFilter FFTImageFilter.cxx)
target_link_libraries(FFTImageFilter ${ITK_LIBRARIES})
if(ITK_USE_FFTWF)
  add_executable(FFTDirectInverse2 FFTDirectInverse2.cxx)
  target_link_libraries(FFTDirectInverse2 ${ITK_LIBRARIES})
endif()
add_executable(GradientMagnitudeImageFilter GradientMagnitudeImageFilter.cxx)
target_link_libraries(GradientMagnitudeImageFilter ${ITK_LIBRARIES})
add_executable(
  GradientMagnitudeRecursiveGaussianImageFilter
  GradientMagnitudeRecursiveGaussianImageFilter.cxx
)
target_link_libraries(
  GradientMagnitudeRecursiveGaussianImageFilter
  ${ITK_LIBRARIES}
)
add_executable(
  GradientRecursiveGaussianImageFilter
  GradientRecursiveGaussianImageFilter.cxx
)
target_link_libraries(GradientRecursiveGaussianImageFilter ${ITK_LIBRARIES})
add_executable(
  GrayscaleFunctionDilateImageFilter
  GrayscaleFunctionDilateImageFilter.cxx
)
target_link_libraries(GrayscaleFunctionDilateImageFilter ${ITK_LIBRARIES})
add_executable(BinaryThresholdImageFilter BinaryThresholdImageFilter.cxx)
target_link_libraries(BinaryThresholdImageFilter ${ITK_LIBRARIES})
add_executable(BinaryMedianImageFilter BinaryMedianImageFilter.cxx)
target_link_libraries(BinaryMedianImageFilter ${ITK_LIBRARIES})
add_executable(OrientImageFilter OrientImageFilter.cxx)
target_link_libraries(OrientImageFilter ${ITK_LIBRARIES})
add_executable(OtsuThresholdImageFilter OtsuThresholdImageFilter.cxx)
target_link_libraries(OtsuThresholdImageFilter ${ITK_LIBRARIES})
add_executable(
  OtsuMultipleThresholdImageFilter
  OtsuMultipleThresholdImageFilter.cxx
)
target_link_libraries(OtsuMultipleThresholdImageFilter ${ITK_LIBRARIES})
add_executable(ThresholdImageFilter ThresholdImageFilter.cxx)
target_link_libraries(ThresholdImageFilter ${ITK_LIBRARIES})
add_executable(CastingImageFilters CastingImageFilters.cxx)
target_link_libraries(CastingImageFilters ${ITK_LIBRARIES})
add_executable(CannyEdgeDetectionImageFilter CannyEdgeDetectionImageFilter.cxx)
target_link_libraries(CannyEdgeDetectionImageFilter ${ITK_LIBRARIES})
add_executable(MedianImageFilter MedianImageFilter.cxx)
target_link_libraries(MedianImageFilter ${ITK_LIBRARIES})
add_executable(MeanImageFilter MeanImageFilter.cxx)
target_link_libraries(MeanImageFilter ${ITK_LIBRARIES})
add_executable(
  MathematicalMorphologyBinaryFilters
  MathematicalMorphologyBinaryFilters.cxx
)
target_link_libraries(MathematicalMorphologyBinaryFilters ${ITK_LIBRARIES})
add_executable(
  MathematicalMorphologyGrayscaleFilters
  MathematicalMorphologyGrayscaleFilters.cxx
)
target_link_libraries(MathematicalMorphologyGrayscaleFilters ${ITK_LIBRARIES})
add_executable(
  BinaryMinMaxCurvatureFlowImageFilter
  BinaryMinMaxCurvatureFlowImageFilter.cxx
)
target_link_libraries(BinaryMinMaxCurvatureFlowImageFilter ${ITK_LIBRARIES})
add_executable(DiscreteGaussianImageFilter DiscreteGaussianImageFilter.cxx)
target_link_libraries(DiscreteGaussianImageFilter ${ITK_LIBRARIES})
add_executable(GaussianBlurImageFunction GaussianBlurImageFunction.cxx)
target_link_libraries(GaussianBlurImageFunction ${ITK_LIBRARIES})
add_executable(
  LaplacianRecursiveGaussianImageFilter1
  LaplacianRecursiveGaussianImageFilter1.cxx
)
target_link_libraries(LaplacianRecursiveGaussianImageFilter1 ${ITK_LIBRARIES})
add_executable(ScaleSpaceGenerator2D ScaleSpaceGenerator2D.cxx)
target_link_libraries(ScaleSpaceGenerator2D ${ITK_LIBRARIES})
add_executable(BinomialBlurImageFilter BinomialBlurImageFilter.cxx)
target_link_libraries(BinomialBlurImageFilter ${ITK_LIBRARIES})
add_executable(
  CurvatureAnisotropicDiffusionImageFilter
  CurvatureAnisotropicDiffusionImageFilter.cxx
)
target_link_libraries(CurvatureAnisotropicDiffusionImageFilter ${ITK_LIBRARIES})
add_executable(
  VectorGradientAnisotropicDiffusionImageFilter
  VectorGradientAnisotropicDiffusionImageFilter.cxx
)
target_link_libraries(
  VectorGradientAnisotropicDiffusionImageFilter
  ${ITK_LIBRARIES}
)
add_executable(
  VectorCurvatureAnisotropicDiffusionImageFilter
  VectorCurvatureAnisotropicDiffusionImageFilter.cxx
)
target_link_libraries(
  VectorCurvatureAnisotropicDiffusionImageFilter
  ${ITK_LIBRARIES}
)
add_executable(RGBToGrayscale RGBToGrayscale.cxx)
target_link_libraries(RGBToGrayscale ${ITK_LIBRARIES})
add_executable(BilateralImageFilter BilateralImageFilter.cxx)
target_link_libraries(BilateralImageFilter ${ITK_LIBRARIES})
add_executable(ResampleImageFilter2 ResampleImageFilter2.cxx)
target_link_libraries(ResampleImageFilter2 ${ITK_LIBRARIES})
add_executable(
  GradientAnisotropicDiffusionImageFilter
  GradientAnisotropicDiffusionImageFilter.cxx
)
target_link_libraries(GradientAnisotropicDiffusionImageFilter ${ITK_LIBRARIES})
add_executable(CurvatureFlowImageFilter CurvatureFlowImageFilter.cxx)
target_link_libraries(CurvatureFlowImageFilter ${ITK_LIBRARIES})
add_executable(GradientVectorFlowImageFilter GradientVectorFlowImageFilter.cxx)
target_link_libraries(GradientVectorFlowImageFilter ${ITK_LIBRARIES})
add_executable(SigmoidImageFilter SigmoidImageFilter.cxx)
target_link_libraries(SigmoidImageFilter ${ITK_LIBRARIES})
add_executable(
  RGBCurvatureAnisotropicDiffusionImageFilter
  RGBCurvatureAnisotropicDiffusionImageFilter.cxx
)
target_link_libraries(
  RGBCurvatureAnisotropicDiffusionImageFilter
  ${ITK_LIBRARIES}
)
add_executable(
  RGBGradientAnisotropicDiffusionImageFilter
  RGBGradientAnisotropicDiffusionImageFilter.cxx
)
target_link_libraries(
  RGBGradientAnisotropicDiffusionImageFilter
  ${ITK_LIBRARIES}
)
add_executable(
  SignedDanielssonDistanceMapImageFilter
  SignedDanielssonDistanceMapImageFilter.cxx
)
target_link_libraries(SignedDanielssonDistanceMapImageFilter ${ITK_LIBRARIES})
add_executable(
  SecondDerivativeRecursiveGaussianImageFilter
  SecondDerivativeRecursiveGaussianImageFilter.cxx
)
target_link_libraries(
  SecondDerivativeRecursiveGaussianImageFilter
  ${ITK_LIBRARIES}
)
add_executable(DerivativeImageFilter DerivativeImageFilter.cxx)
target_link_libraries(DerivativeImageFilter ${ITK_LIBRARIES})
add_executable(
  DanielssonDistanceMapImageFilter
  DanielssonDistanceMapImageFilter.cxx
)
target_link_libraries(DanielssonDistanceMapImageFilter ${ITK_LIBRARIES})
add_executable(AntiAliasBinaryImageFilter AntiAliasBinaryImageFilter.cxx)
target_link_libraries(AntiAliasBinaryImageFilter ${ITK_LIBRARIES})
add_executable(ResampleImageFilter ResampleImageFilter.cxx)
target_link_libraries(ResampleImageFilter ${ITK_LIBRARIES})
add_executable(
  SmoothingRecursiveGaussianImageFilter
  SmoothingRecursiveGaussianImageFilter.cxx
)
target_link_libraries(SmoothingRecursiveGaussianImageFilter ${ITK_LIBRARIES})
add_executable(
  SmoothingRecursiveGaussianImageFilter2
  SmoothingRecursiveGaussianImageFilter2.cxx
)
target_link_libraries(SmoothingRecursiveGaussianImageFilter2 ${ITK_LIBRARIES})
add_executable(
  LaplacianRecursiveGaussianImageFilter2
  LaplacianRecursiveGaussianImageFilter2.cxx
)
target_link_libraries(LaplacianRecursiveGaussianImageFilter2 ${ITK_LIBRARIES})
add_executable(MorphologicalImageEnhancement MorphologicalImageEnhancement.cxx)
target_link_libraries(MorphologicalImageEnhancement ${ITK_LIBRARIES})
add_executable(
  MinMaxCurvatureFlowImageFilter
  MinMaxCurvatureFlowImageFilter.cxx
)
target_link_libraries(MinMaxCurvatureFlowImageFilter ${ITK_LIBRARIES})
add_executable(
  FFTImageFilterFourierDomainFiltering
  FFTImageFilterFourierDomainFiltering.cxx
)
target_link_libraries(FFTImageFilterFourierDomainFiltering ${ITK_LIBRARIES})
add_executable(LaplacianImageFilter LaplacianImageFilter.cxx)
target_link_libraries(LaplacianImageFilter ${ITK_LIBRARIES})
add_executable(ResampleImageFilter3 ResampleImageFilter3.cxx)
target_link_libraries(ResampleImageFilter3 ${ITK_LIBRARIES})
add_executable(ResampleImageFilter4 ResampleImageFilter4.cxx)
target_link_libraries(ResampleImageFilter4 ${ITK_LIBRARIES})
add_executable(
  VotingBinaryHoleFillingImageFilter
  VotingBinaryHoleFillingImageFilter.cxx
)
target_link_libraries(VotingBinaryHoleFillingImageFilter ${ITK_LIBRARIES})
add_executable(
  VotingBinaryIterativeHoleFillingImageFilter
  VotingBinaryIterativeHoleFillingImageFilter.cxx
)
target_link_libraries(
  VotingBinaryIterativeHoleFillingImageFilter
  ${ITK_LIBRARIES}
)
add_executable(ResampleImageFilter5 ResampleImageFilter5.cxx)
target_link_libraries(ResampleImageFilter5 ${ITK_LIBRARIES})
add_executable(ResampleImageFilter6 ResampleImageFilter6.cxx)
target_link_libraries(ResampleImageFilter6 ${ITK_LIBRARIES})
add_executable(ResampleImageFilter8 ResampleImageFilter8.cxx)
target_link_libraries(ResampleImageFilter8 ${ITK_LIBRARIES})
add_executable(ResampleImageFilter9 ResampleImageFilter9.cxx)
target_link_libraries(ResampleImageFilter9 ${ITK_LIBRARIES})
add_executable(ResampleVolumesToBeIsotropic ResampleVolumesToBeIsotropic.cxx)
target_link_libraries(ResampleVolumesToBeIsotropic ${ITK_LIBRARIES})
add_executable(SubsampleVolume SubsampleVolume.cxx)
target_link_libraries(SubsampleVolume ${ITK_LIBRARIES})
add_executable(SpatialObjectToImage1 SpatialObjectToImage1.cxx)
target_link_libraries(SpatialObjectToImage1 ${ITK_LIBRARIES})
add_executable(SpatialObjectToImage2 SpatialObjectToImage2.cxx)
target_link_libraries(SpatialObjectToImage2 ${ITK_LIBRARIES})
add_executable(SpatialObjectToImage3 SpatialObjectToImage3.cxx)
target_link_libraries(SpatialObjectToImage3 ${ITK_LIBRARIES})
add_executable(
  LaplacianSharpeningImageFilter
  LaplacianSharpeningImageFilter.cxx
)
target_link_libraries(LaplacianSharpeningImageFilter ${ITK_LIBRARIES})
add_executable(VectorIndexSelection VectorIndexSelection.cxx)
target_link_libraries(VectorIndexSelection ${ITK_LIBRARIES})
add_executable(VesselnessMeasureImageFilter VesselnessMeasureImageFilter.cxx)
target_link_libraries(VesselnessMeasureImageFilter ${ITK_LIBRARIES})
add_executable(
  DiffusionTensor3DReconstructionImageFilter
  DiffusionTensor3DReconstructionImageFilter.cxx
)
target_link_libraries(
  DiffusionTensor3DReconstructionImageFilter
  ${ITK_LIBRARIES}
)
add_executable(FFTDirectInverse FFTDirectInverse.cxx)
target_link_libraries(FFTDirectInverse ${ITK_LIBRARIES})
add_executable(
  ZeroCrossingBasedEdgeDetectionImageFilter
  ZeroCrossingBasedEdgeDetectionImageFilter.cxx
)
target_link_libraries(
  ZeroCrossingBasedEdgeDetectionImageFilter
  ${ITK_LIBRARIES}
)
add_executable(WarpImageFilter1 WarpImageFilter1.cxx)
target_link_libraries(WarpImageFilter1 ${ITK_LIBRARIES})
add_executable(SurfaceExtraction SurfaceExtraction.cxx)
target_link_libraries(SurfaceExtraction ${ITK_LIBRARIES})
add_executable(
  DigitallyReconstructedRadiograph1
  DigitallyReconstructedRadiograph1.cxx
)
target_link_libraries(DigitallyReconstructedRadiograph1 ${ITK_LIBRARIES})
add_executable(ResampleImageFilter7 ResampleImageFilter7.cxx)
target_link_libraries(ResampleImageFilter7 ${ITK_LIBRARIES})

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
