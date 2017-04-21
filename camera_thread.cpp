#include "camera_thread.h"
#include "src/ardrone/ardrone.h"
//#include "/users/isabellahuang/arya/cvdrone/src/ardrone/ardrone.h"

#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QCamera>
#include <QImage>

#include "opencv2/opencv.hpp"
using namespace cv;

void CameraThread::run() {
    keep_running = true;

//    ARDrone ardrone;
//    if (!ardrone.open()) {
//        keep_running = false;
//    }
//    qInfo() << "Battery percentage:" << ardrone.getBatteryPercentage();


    VideoCapture cap(0);

    while(keep_running) {
        Mat frame;

        cap >> frame;

//        frame = ardrone.getImage();
        cv::Mat temp;


        cvtColor(frame, temp, CV_BGR2RGB);

        QImage dest((const uchar *)temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
        dest.bits();
        myImage = dest;

        QPixmap pix = QPixmap::fromImage(myImage);
        emit sendTime(pix) ;
        this->msleep(100);

    }

    exec();

//    QTimer timer;
//    connect(&timer, SIGNAL(timeout()), this, SLOT(timerHit()), Qt::DirectConnection);
//    timer.setInterval(0.1);
//    timer.start();
//    exec();
//    timer.stop();
}

void CameraThread::timerHit() {
    QString newTime= QDateTime::currentDateTime().toString("ddd MMMM d yy, hh:mm:ss");
    if(m_lastTime != newTime ){
        m_lastTime = newTime;
//        VideoCapture cap(0);

//        Mat frame;
//        cap >> frame;

//        cv::Mat temp;
//        cvtColor(frame, temp, CV_BGR2RGB);
//        QImage dest((const uchar *)temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
//        dest.bits();
//        myImage = dest;


//        QPixmap pix = QPixmap::fromImage(myImage);
        QPixmap pix ("/Users/isabellahuang/Downloads/logo.png");
        emit sendTime(pix) ;

    }
}

void CameraThread::endThread() {
    keep_running = false;
}
