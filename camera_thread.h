#ifndef CAMERA_THREAD_H
#define CAMERA_THREAD_H

#include <QThread>
#include <QString>
#include <QCamera>
#include <QVideoWidget>



class CameraThread : public QThread
{
    Q_OBJECT
private:
    QCamera *camera;
    QVideoWidget * video_widget;
    void run();
    QString m_lastTime;
    QImage myImage;
    bool keep_running;


signals:
    void sendTime(QPixmap time);
private slots:
    void timerHit();
    void endThread();

};


#endif // CAMERA_THREAD_H

