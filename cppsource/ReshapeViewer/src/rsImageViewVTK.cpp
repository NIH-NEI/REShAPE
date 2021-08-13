#include <QVTKWidget.h>
#include "rsimageviewvtk.h"

vtkStandardNewMacro(rsMouseInteractorStyle);

void rsMouseInteractorStyle::OnLeftButtonDown()
{
	if (this->CurrentRenderer == nullptr)
		return;
	this->StartPan();
}

rsImageViewVTK::rsImageViewVTK()
{
	ImageData = vtkSmartPointer<vtkImageData>::New();
	ImageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
	ImageActor = vtkSmartPointer<vtkImageActor>::New();

	ImageRender = vtkSmartPointer<vtkRenderer>::New();
	ImageRender->SetBackground(0.0f, 0.0f, 0.0f);

	ImageStyle = vtkSmartPointer<rsMouseInteractorStyle>::New();
	RenderWin = vtkSmartPointer<vtkRenderWindow>::New();
	RenderWin->AddRenderer(ImageRender);
	WinInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	WinInteractor->SetInteractorStyle(ImageStyle);
	WinInteractor->SetRenderWindow(RenderWin);

	WinInteractor->Initialize();
}

void rsImageViewVTK::InitializeView()
{
	ImageData->Initialize();
	ImageData->Modified();
}

void rsImageViewVTK::ChangeCameraOrientation(vtkSmartPointer<vtkRenderer> img_ren)
{
	img_ren->ResetCamera();
	double* fp = img_ren->GetActiveCamera()->GetFocalPoint();
	double* p = img_ren->GetActiveCamera()->GetPosition();
	double dist = sqrt((p[0] - fp[0])*(p[0] - fp[0]) + (p[1] - fp[1])*(p[1] - fp[1])
		+ (p[2] - fp[2])*(p[2] - fp[2]));
	img_ren->GetActiveCamera()->SetPosition(fp[0], fp[1], fp[2] - dist);
	img_ren->GetActiveCamera()->SetViewUp(0.0, -1.0, 0.0);
}

void rsImageViewVTK::ResetView(bool camera_flag)
{
	if (camera_flag)
		ChangeCameraOrientation(ImageRender);
	RenderWin->Render();
}

void rsImageViewVTK::SetGrayImage(ImageType2D::Pointer img)
{
	particles.resize(0);
	ImageData->Initialize();
	ConvertITKImageToVTKImage<ImageType2D, unsigned char>(img, ImageData);
	ImageData->Modified();

	if (!ImageAddFlag)
	{
		ImageActor->SetInputData(ImageData);
		ImageRender->AddActor(ImageActor);
		ImageAddFlag = true;
	}
}

void rsImageViewVTK::ImageDataModified()
{
	ImageData->Modified();
}

byte * rsImageViewVTK::GetPixelDataPointer(int x, int y)
{
	return static_cast<byte *>(ImageData->GetScalarPointer(x, y, 0));
}

QWidget * rsImageViewVTK::GetImageWidget(QWidget *parent)
{
	if (!imageWidget) {
		imageWidget = new QVTKWidget(parent);
		imageWidget->setAttribute(Qt::WA_DeleteOnClose, true);
		// ImageWidget = new QVTKOpenGLSimpleWidget(centralwidget);
		imageWidget->SetRenderWindow(RenderWin);
	}
	return (QWidget *) imageWidget;
}
