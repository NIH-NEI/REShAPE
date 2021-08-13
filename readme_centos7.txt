
This package requires:
  - Centos 7 Linux OS
  - CUDA-enabled GPU
  - CUDA toolkit and drivers installed
  - GUI desktop (Gnome or a remote desktop)
  - a user with sudo access

1. Checkout from git to '~/REShAPE' or unzip archive into '~'. Check if Nvidia/CUDA drivers are properly
installed:
    sh ~/REShAPE/Standalone/deviceQuery
It should print a lengthy output with GPU/driver features and "Result = PASS" at the end. If it prints
"Result = FAIL", check your Nvidia/CUDA driver installation -- REShAPE won't work until this problem
is fixed.
2. cd to '~/REShAPE' and type
    python setup.py
3. Enter your system password when prompted for "sudo password"
4. Wait until setup is complete (it may take a couple of hours). If successful, create desktop icons by typing
    python setup_icons.py
5. Once the shortcuts appear on the desktop, click on the desktop (to focus on it) and press F5 to refresh.
   You should see now the proper names and icon images.

The main application is "REShAPE Controller".

    Troubleshooting.

If the automatic setup fails to install the Qt5 library, try the following steps:

1. Check if file "/opt/Qt/MaintenanceTool" exists and is executable. If so, run it with sudo:
    sudo /opt/Qt/MaintenanceTool
If this file does not exist, delete the directory /opt/QT:
    sudo rm -rf /opt/Qt
then run Qt online installer:
    sudo ~/REShAPE/thirdparty/Qt5/qt-unified-linux-x64-3.1.1-online.run
2. When the installer starts, follow on screen instructions (mostly, click the "Next" button).
On the license acceptance screen, check the "I have read and approve..." box, then click Next.
On the "Setup - Qt" screen, select "Add or remove components", then click Next.
On the "Select Components" screen, check "Archive" and click "Filter". After the tool refreshes the list,
expand it (by clicking on "> Qt"), then expand "Qt 5.14.2" and check "Desktop gcc 64 bit". Then click Next.
3. If successful, go back to the normal installation process starting from step 2 (run "python setup.py").
