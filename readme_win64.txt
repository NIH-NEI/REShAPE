Setting up REShAPE on a Windows machine.

Pre-requisites:
  - Python 2.7
  - CUDA-enabled GPU
  - CUDA toolkit 8.0 (or higher) and drivers installed
  - Matlab R2017a (or higher, R2019b is preferred) with the following toolboxes:
      - Image Processing Toolbox
      - Parallel Computing Toolbox
      - Statistics and Machine Learning Toolbox
  - Fiji (one of the latest versions) installed in C:\Fiji or C:\Fiji.app
  - Microsoft Visual Studio (2017 or higher) w. C/C++ compiler
  - CMake 3 (https://cmake.org)

Step 1. Download and install Qt5 open source using Qt installer from https://www.qt.io/download-qt-installer/
You may need to create an account, but it is free. Only one of the "Qt 5.x, MSVC 2017 64-bit" options is needed.
REShAPE has been tested with Qt version 5.14.2, but newer versions might work as well.

Step 2. Download and build VTK 8.2 from https://vtk.org/download/
To build VTK, run CMake with the following options:
	VTK_Group_Qt [BOOL] - checked
	Qt5_DIR [PATH] - point to Qt5's Cmake dir, something like {Qt5_install_dir}/5.14.2/msvc2017_64/lib/cmake/Qt5
	VTK_LEGACY_SILENT [BOOL] - checked
After "Configure" and "Generate", click "Open Project", switch to "Release" and build the "ALL_BUILD" target.

Step 3. Download and build ITK 4.13 from https://itk.org/download/
To build ITK, run CMake with the following options:
	Module_ITKV3Compatibility [BOOL] - checked
	ITKV3_COMPATIBILITY [BOOL] - checked
	Module_ITKVtkGlue [BOOL] - checked
	Qt5_DIR [PATH] - point to Qt5's Cmake dir, something like {Qt5_install_dir}/5.14.2/msvc2017_64/lib/cmake/Qt5
	VTK_DIR [PATH] - point to VTK build dir
After "Configure" and "Generate", click "Open Project", switch to "Release" and build the "ALL_BUILD" target.

Step 4. Check out REShAPE source into a separate directory (e.g. C:\GitHubNIH\REShAPE)
and build the binary (-ies). The only binary required to run REShAPE is ReshapeImaging, found in
{C:\GitHubNIH\REShAPE}\cppsource\ReshapeImaging . To build it, run CMake, point the "source code" to
{C:/AVProjects/REShAPE}/cppsource/ReshapeImaging, and "where to build the binaries", to
{C:/AVProjects/REShAPE}/cppsource/ReshapeImaging/build , then specify the following options:
	Qt5_DIR [PATH] - point to Qt5's Cmake dir, something like {Qt5_install_dir}/5.14.2/msvc2017_64/lib/cmake/Qt5
	VTK_DIR [PATH] - point to VTK 8 build dir
	ITK_DIR [PATH] - point to ITK 4 build dir
	CMAKE_INSTALL_PREFIX [PATH] - point to {C:/AVProjects/REShAPE}/Imaging
Click "Configure", "Generate", then "Open Project". Switch to "Release", build the "ALL_BUILD" target.
If success, also build the "INSTALL" target.

Step 5 (optional). Build ReshapeViewer and RsCircles. These are separate utilities, not required to
run REShAPE. If you want to build them anyway, just repeat the build process at step 4, using their
respective directories as CMake's "source code". For "RsCircles", the "VTK_DIR" and "ITK_DIR"
CMake parameters are not needed.

Step 6. Run REShAPE.
Go to {C:\GitHubNIH\REShAPE} and type "python reshapectl.py". On the first start,
REShAPE runs a self-configuration, which tries to find Matlab, Fiji, etc. and create a config file.
If everything goes smooth, this may be all you need. To test installation, set the "Input directory"
to {C:\GitHubNIH\REShAPE}/TestData and click "Go For It".

If something goes wrong, check the (automatically generated) REShAPE config file located in
C:\Users\<your-windows-username>\.reshape\config.json
It should look like this:
======================================== config.json ==========================================
{
  "matlab_ver": "R2019b", 
  "environ": {
    "+INCLUDE": "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.17763.0\\ucrt;C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.17763.0\\um;C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.17763.0\\shared", 
    "+PATH": "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Community\\VC\\Tools\\MSVC\\14.16.27023\\bin\\Hostx64\\x64"
  }, 
  "matlab": "C:\\Program Files\\MATLAB\\R2019b\\bin\\matlab.exe", 
  "REShAPE": "REShAPE_Auto_0.0.1.ijm", 
  "matlab_opts": [
    "-wait", 
    "-automation"
  ], 
  "fiji": "C:\\Fiji\\ImageJ-win64.exe", 
  "mex": {
    "enableCudnn": false, 
    "verbose": 1, 
    "EnableImreadJpeg": true, 
    "cudaMethod": "nvcc", 
    "mexFlags": [
      "-R2018a"
    ], 
    "enableGpu": true, 
    "cudaRoot": "C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v11.0"
  }, 
  "cuda_ver": 11.0, 
  "cuda": "C:\\Program Files\\NVIDIA GPU Computing Toolkit\\CUDA\\v11.0"
}
======================================== end of config.json =====================================

Check if variables "matlab", "fiji", "cuda" and "cudaRoot" are properly set. If you have Matlab R2019a or higher
and you get errors compiling MatConvNet, try to change "cudaMethod": "nvcc" -> "cudaMethod": "mex" .


Step 7 (optional). Setup and run REShAPE Visualizations.

If you also want to use REShAPE Visualizations, run "python setup-win64.py" (in {C:\GitHubNIH\REShAPE}) first -
it will install extra Python libraries required for Visualisations. To run Visualisations itself, type:
python reshapevis.py

