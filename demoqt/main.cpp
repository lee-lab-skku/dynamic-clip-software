/**************************************************************************************************************************************
Main Application Entry and demoqt Class Implementation

Description:
This C++ source file integrates various functionalities for a custom 3D printing application. It demonstrates handling of GUI events,
communication with hardware components (like SMC100C motorized stage controller and a custom light engine), and multithreading for
asynchronous operations. The 'demoqt' class serves as the central hub, connecting user interface actions with backend processes,
including initializing system components, starting print jobs, and managing advanced settings through dialogs.

Features:
- GUI management with Qt for selecting COM ports, directories, and adjusting printing parameters.
- Interaction with motorized stage and light engine hardware using provided APIs.
- Implementation of asynchronous processing using QtConcurrent and QThread to avoid blocking the main GUI thread.
- Dynamic adjustment of print settings based on user input, including DLP and CLIP methods.
- Extensive signal-slot connections for responsive and interactive user experience.

Implementation Details:
- Utilization of Qt widgets for constructing a user-friendly interface.
- Use of QtConcurrent for running long-running tasks (like a full print process) in the background.
- Direct integration with hardware APIs for real-time control and feedback.
- Custom slot implementations for handling various user actions, such as button clicks and combo box changes.
- Advanced settings dialog for detailed control over hardware parameters.

Author:
    Mats Grobe, 28/02/2024

Notes:
- This implementation assumes the presence of Qt framework and proper setup of Qt environment for GUI applications.
- Direct hardware communication requires proper setup and initialization, assuming the hardware is connected and configured correctly.
- The application is structured for demonstration and educational purposes, highlighting Qt's capabilities in handling GUI and hardware integration.
***************************************************************************************************************************************/



#include "demoqt.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    demoqt w;
    w.show();
    return a.exec();
}
