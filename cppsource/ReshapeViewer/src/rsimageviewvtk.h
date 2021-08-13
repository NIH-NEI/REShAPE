#ifndef rsimageviewvtk_h
#define rsimageviewvtk_h

#include <vtkSmartPointer.h>
#include <vtkObjectFactory.h>
#include <vtkImageData.h>
#include <vtkImageActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkInteractorStyleImage.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkCamera.h>
#include <itkImageToVTKImageFilter.h>
#include <itkImageRegionIteratorWithIndex.h>

#include "rsimageview.h"

template <class TITKImage, class TType>
void ConvertITKImageToVTKImage(typename TITKImage::Pointer input_img,
	vtkSmartPointer<vtkImageData> output_img)
{
	if (input_img->GetImageDimension() == 3)
	{
		output_img->SetDimensions(input_img->GetLargestPossibleRegion().GetSize()[0],
			input_img->GetLargestPossibleRegion().GetSize()[1],
			input_img->GetLargestPossibleRegion().GetSize()[2]);
		output_img->SetSpacing(input_img->GetSpacing()[0], input_img->GetSpacing()[1], input_img->GetSpacing()[2]);
		output_img->SetOrigin(input_img->GetOrigin()[0], input_img->GetOrigin()[1], input_img->GetOrigin()[2]);
	}
	else
	{
		output_img->SetDimensions(input_img->GetLargestPossibleRegion().GetSize()[0],
			input_img->GetLargestPossibleRegion().GetSize()[1], 1);
		output_img->SetSpacing(input_img->GetSpacing()[0], input_img->GetSpacing()[1], 1.0);
		output_img->SetOrigin(input_img->GetOrigin()[0], input_img->GetOrigin()[1], 0);
	}

	output_img->AllocateScalars(VTK_UNSIGNED_CHAR, 3);

	typedef itk::ImageRegionIteratorWithIndex<TITKImage>   TIteratorType;
	TIteratorType it(input_img, input_img->GetLargestPossibleRegion());

	for (it.GoToBegin(); !it.IsAtEnd(); ++it)
	{
		TType * pixel;
		if (input_img->GetImageDimension() == 3)
			pixel = static_cast<TType *>(output_img->GetScalarPointer(it.GetIndex()[0], it.GetIndex()[1], it.GetIndex()[2]));
		else
			pixel = static_cast<TType *>(output_img->GetScalarPointer(it.GetIndex()[0], it.GetIndex()[1], 0));

		pixel[2] = pixel[1] = pixel[0] = it.Get();
	}
}

// Define interaction style
class rsMouseInteractorStyle : public vtkInteractorStyleImage
{
private:

public:
	static rsMouseInteractorStyle* New();
	vtkTypeMacro(rsMouseInteractorStyle, vtkInteractorStyleImage);

	rsMouseInteractorStyle()
	{
	}

	virtual void OnLeftButtonDown() override;
};

class rsImageViewVTK : public rsImageView
{
private:
	bool ImageAddFlag = false;
	QVTKWidget *imageWidget = NULL;
protected:
	vtkSmartPointer<vtkImageData> ImageData;
	vtkSmartPointer<vtkImageActor> ImageActor;

	vtkSmartPointer<rsMouseInteractorStyle> ImageStyle;
	vtkSmartPointer<vtkRenderer> ImageRender;
	vtkSmartPointer<vtkRenderWindowInteractor> WinInteractor;
	vtkSmartPointer<vtkRenderWindow> RenderWin;

	void ChangeCameraOrientation(vtkSmartPointer<vtkRenderer>);
public:
	rsImageViewVTK();

	virtual void ResetView(bool camera_flag = true) override; //used for initialization
	virtual void InitializeView() override;
	virtual void SetGrayImage(ImageType2D::Pointer) override;
	virtual void ImageDataModified() override;
	virtual byte *GetPixelDataPointer(int x, int y) override;
	virtual QWidget *GetImageWidget(QWidget *parent = NULL) override;

};

#endif
