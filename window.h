#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QAudioInput>
#include <QFile>
#include <QLabel>
#include <QRadioButton>
#include <QCamera>
#include <QVideoWidget>
#include <QMovie>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>
#include <QBluetoothUuid>
#include <QLowEnergyService>

class Window : public QWidget
{
    Q_OBJECT
public:
    explicit Window(QWidget *parent = 0);
signals:
    void counterReached();
    void stateChanged(QAudio::State newState);
    void startFlight();
    void searchAgain();
    void confirmCamshift();

private slots:
    void stopRecording();
    void startRecording();
    void startFlightResponder();
    void searchAgainResponder();
    void confirmCamshiftResponder();
    void handleStateChanged(QAudio::State newState);
    void valueChanged(QPixmap newValue);
    void delay( int millisecondsToWait );
    QString sphinx_test();
    void addDevice(const QBluetoothDeviceInfo&);
    void deviceConnected();
    void disconnectFromDevice();
    void deviceDisconnected();
    void errorReceived(QLowEnergyController::Error);
    void serviceDiscovered(const QBluetoothUuid &gatt);
    void serviceScanDone();
    void serviceStateChanged(QLowEnergyService::ServiceState);
    void dropMarker();
    void changeStatusSlot(QString status);


private:
    int m_counter;
    QPushButton *m_button;
    QPushButton *search_again_button;
    QPushButton *start_flight_button;
    QPushButton *start_recording_button;
    QPushButton *confirm_object_button;
    QPushButton *confirm_camshift_button;
    QAudioInput* audio;
    QFile destinationFile;
    QLabel *stream_label;
    QLabel *speech_to_text;
    QLabel *speech_to_text_title;
    QLabel *confirm_object_title;
    QRadioButton *radio_yes;
    QRadioButton *radio_no;
    QLabel *flight_status_label;
    QLabel *loading_gif_label;
    QCamera *camera;
    QVideoWidget * video_widget;
    QMovie *loading_movie;
    QGroupBox *speech_to_text_box;
    QGroupBox *flight_status_box;
    QGroupBox *stream_box;
    QVBoxLayout *vbox;
    QGridLayout *grid;
    QLabel *title;
    QMediaPlayer *player;
    QAudioOutput *audio_output;
    QFile inputFile;
    QPushButton *drop_marker_button;


    // Bluetooth stuff
    QBluetoothDeviceDiscoveryAgent *m_deviceDiscoveryAgent;
    QLowEnergyController *m_control;
    bool foundDevice = false;
    int uart_command = 0;
    QLowEnergyService *m_service;





public slots:
};

#endif // WINDOW_H
