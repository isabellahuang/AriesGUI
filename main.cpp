#include <QApplication>
#include <QPushButton>
#include <QProgressBar>
#include <QSlider>
#include <QFile>
#include <QAudioInput>
#include <QAudioRecorder>
#include "window.h"
#include "camera_thread.h"
#include "opencv2/opencv.hpp"
#include <QImage>
#include <QLabel>
#include <QObject>
#include <QBluetoothDeviceDiscoveryAgent>

using namespace cv;


int main(int argc, char **argv) {

    QBluetoothDeviceDiscoveryAgent *discoveryAgent = new QBluetoothDeviceDiscoveryAgent();


    QApplication app(argc, argv);
    // Create a container window
    Window window;
    window.show();


    CameraThread thread;
//    QObject::connect(&thread, SIGNAL(sendTime(QString)), &window, SLOT(valueChanged(QString)), Qt::QueuedConnection);
    QObject::connect(&thread, SIGNAL(sendTime(QPixmap)), &window, SLOT(valueChanged(QPixmap)), Qt::QueuedConnection);
    QObject::connect(&app, SIGNAL(aboutToQuit()), &thread, SLOT(endThread()));
    QObject::connect(&window, SIGNAL(startFlight()), &thread, SLOT(start()), Qt::QueuedConnection);
    QObject::connect(&window, SIGNAL(searchAgain()), &thread, SLOT(searchAgainSlot()), Qt::QueuedConnection);
    QObject::connect(&window, SIGNAL(confirmCamshift()), &thread, SLOT(confirmCamshiftSlot()), Qt::QueuedConnection);

//    QObject::connect(&window, SIGNAL(start_flight_button.clicked()), &thread, SLOT(endThread()));


//    thread.start();

    qDebug() << "Hello from the GUI THread" << app.thread()->currentThreadId();



    app.exec();
    thread.quit();
    thread.wait();

    return 0;
}


