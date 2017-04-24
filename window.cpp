#include "window.h"
#include <QApplication>
#include <QFile>
#include <QAudioInput>
#include <QTimer>
#include <QDebug>
#include <QLabel>
#include <QPixmap>
#include "camera_thread.h"
#include "opencv2/opencv.hpp"

#include <QMovie>
#include <QGroupBox>
#include <QTime>
#include <pocketsphinx.h>

using namespace cv;
#define MODELDIR "/Users/isabellahuang/sphinx4-5prealpha-src/sphinx4-data/src/main/resources/edu/cmu/sphinx/models"

Window::Window(QWidget *parent) : QWidget(parent)
{

    // ##########################
    // Speech recording box
    // ##########################
    speech_to_text_box = new QGroupBox(tr("Speech Input"), this);
    start_recording_button = new QPushButton("Start Recording", this);
//    confirm_object_button->setVisible(false);
    confirm_object_button = new QPushButton("Confirm object", this);

    //Put loading animation
    loading_gif_label = new QLabel("Raw", this);
    loading_gif_label->setAlignment(Qt::AlignCenter);
    loading_gif_label->setFixedHeight(80);
    loading_movie = new QMovie("/Users/isabellahuang/Downloads/ripple.gif");
    loading_movie->setScaledSize(QSize(0, 0));
    QPixmap mic_pix ("/Users/isabellahuang/Downloads/microphone.png");
    loading_gif_label->setPixmap(mic_pix.scaledToWidth(60));

    grid = new QGridLayout(this);
    title = new QLabel("Aries title", this);
    QPixmap pix ("/Users/isabellahuang/Downloads/logo.png");
    title->setPixmap(pix.scaledToWidth(250));
    title->setAlignment(Qt::AlignCenter);
    grid->addWidget(title, 0, 0);

    vbox = new QVBoxLayout(this);
    vbox->addWidget(start_recording_button);
    vbox->addWidget(loading_gif_label);
    vbox->addWidget(confirm_object_button);
    speech_to_text_box->setLayout(vbox);
    grid->addWidget(speech_to_text_box, 1, 0);
    confirm_object_button->setEnabled(false);

    // ##########################
    // Flight Status
    // ##########################
    search_again_button = new QPushButton("Search again", this);
    start_flight_button = new QPushButton("Start flight", this);
    confirm_camshift_button = new QPushButton("Confirm camshift", this);
    flight_status_label = new QLabel("Flight label", this);
    flight_status_label->setText("Idle");
    QFont font = flight_status_label->font();
    flight_status_label->setFont(font);
    flight_status_label->setAlignment(Qt::AlignCenter);
    flight_status_label->setStyleSheet("background: yellow");
    QVBoxLayout *vbox_flight_status = new QVBoxLayout(this);
    vbox_flight_status->addWidget(flight_status_label);
    vbox_flight_status->addWidget(start_flight_button);
    vbox_flight_status->addWidget(search_again_button);
    vbox_flight_status->addWidget(confirm_camshift_button);

    flight_status_box = new QGroupBox(tr("Flight Status"), this);
    flight_status_box->setLayout(vbox_flight_status);
    grid->addWidget(flight_status_box, 2, 0);

    // ##########################
    // Speech Recognition
    // ##########################
//    QAudioFormat format;
//    format.setSampleRate(16000);
//    format.setChannelCount(1);
//    format.setSampleSize(16);
//    format.setCodec("audio/pcm");
//    format.setByteOrder(QAudioFormat::LittleEndian);
//    format.setSampleType(QAudioFormat::SignedInt);

//    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
//    if (!info.isFormatSupported(format)) {
//        qWarning()<<"raw audio format not supported by backend, cannot play audio.";
//        return;
//    }

//    destinationFile.setFileName("/tmp/test.raw");
//    destinationFile.open( QIODevice::WriteOnly | QIODevice::Truncate );
//    audio = new QAudioInput(format, this);


    // ##########################
    // Video Streaming Window
    // ##########################
    stream_label = new QLabel("Video stream", this);
    stream_label->setText("Awaiting camera stream...");
    QVBoxLayout *vbox_stream = new QVBoxLayout(this);
    vbox_stream->addWidget(stream_label);
    stream_box = new QGroupBox(tr("Video Stream"), this);
    stream_box->setLayout(vbox_stream);
    grid->addWidget(stream_box, 1, 1, 4, 2);


    // Make the connection
    connect(audio, SIGNAL(stateChanged(QAudio::State)), this, SLOT(handleStateChanged(QAudio::State)));
    connect(start_recording_button, SIGNAL(clicked(bool)), this, SLOT(startRecording()));
    connect(start_flight_button, SIGNAL(clicked(bool)), this, SLOT(startFlightResponder()));
    connect(search_again_button, SIGNAL(clicked(bool)), this, SLOT(searchAgainResponder()));
    connect(confirm_camshift_button, SIGNAL(clicked(bool)), this, SLOT(confirmCamshiftResponder()));

}

void Window::startFlightResponder() {
    qInfo() << "Emitting start flight";
    start_flight_button->setEnabled(false);

    emit startFlight();
}

void Window::searchAgainResponder() {
    qInfo() << "Emitting search again";
    emit searchAgain();
}

void Window::confirmCamshiftResponder() {
    qInfo() << "Emitting search again";
    emit confirmCamshift();
}

void Window::startRecording() {
    qInfo() << "Started recording" ;
    QAudioFormat format;
    format.setSampleRate(16000);
    format.setChannelCount(1);
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);

    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(format)) {
        qWarning()<<"raw audio format not supported by backend, cannot play audio.";
        return;
    }

    destinationFile.setFileName("/tmp/test.raw");
    destinationFile.open( QIODevice::WriteOnly | QIODevice::Truncate );
    audio = new QAudioInput(format, this);
    audio->start(&destinationFile);

    loading_gif_label->setMovie(loading_movie);


    loading_movie->setScaledSize(QSize(35, 35));
    loading_movie->start();
    confirm_object_button->setEnabled(false);


    QTimer::singleShot(3000, this, SLOT(stopRecording()));
}

void Window::stopRecording()
{
    loading_movie->stop();
    loading_gif_label->clear();
    loading_gif_label->setText("Done recording");

    audio->stop();
    destinationFile.close();
    QString test_result = sphinx_test();

    qInfo() << "Finished recording";

    loading_gif_label->setText(test_result);

    start_recording_button->setText("Record again");
    confirm_object_button->setEnabled(true);

    // Hardcoded stuff: remove
//    flight_status_label->setText("Awaiting confirmation...");

//    delay(3000);

//    // Set to take off and then in flight
//    flight_status_label->setText("Taking Off");
//    flight_status_label->setStyleSheet("background: orange");


    delete audio;
}


void Window::valueChanged(QPixmap newValue) {
    stream_label->setPixmap(newValue.scaledToWidth(800));
}

void Window::handleStateChanged(QAudio::State newState)
{
    switch (newState) {
        case QAudio::StoppedState:
            if (audio->error() != QAudio::NoError) {
                // Error handling
            } else {
                // Finished recording
            }
            break;

        case QAudio::ActiveState:
            // Started recording - read from IO device
            break;

        default:
            // ... other cases as appropriate
            break;
    }
}

void Window::delay( int millisecondsToWait )
{
    QTime dieTime = QTime::currentTime().addMSecs( millisecondsToWait );
    while( QTime::currentTime() < dieTime )
    {
        QCoreApplication::processEvents( QEventLoop::AllEvents, 100 );
    }
}

QString Window::sphinx_test() {
    ps_decoder_t *ps;
    cmd_ln_t *config;
    FILE *fh;
    char const *hyp, *uttid;
    QString text_result = "Failed";
    int16 buf[512];
    int rv;
    int32 score;

    config = cmd_ln_init(NULL, ps_args(), TRUE,
                 "-hmm", MODELDIR "/en-us/en-us",
                 "-lm", MODELDIR "/en-us/en-us.lm.bin",
                 "-dict", MODELDIR "/en-us/cmudict-en-us.dict",
                 NULL);
    if (config == NULL) {
        fprintf(stderr, "Failed to create config object, see log for details\n");
        return text_result;
    }

    ps = ps_init(config);
    if (ps == NULL) {
        fprintf(stderr, "Failed to create recognizer, see log for details\n");
        return text_result;
    }

    fh = fopen("/tmp/test.raw", "rb");

    if (fh == NULL) {
        fprintf(stderr, "Unable to open input file goforward.raw\n");
        return text_result;
    }

    rv = ps_start_utt(ps);

    while (!feof(fh)) {
        size_t nsamp;
        nsamp = fread(buf, 2, 512, fh);
        rv = ps_process_raw(ps, buf, nsamp, FALSE, FALSE);
    }

    rv = ps_end_utt(ps);
    hyp = ps_get_hyp(ps, &score);
    text_result = QString::fromUtf8(hyp);
    printf("Recognized: %s\n", hyp);

    fclose(fh);
    ps_free(ps);
    cmd_ln_free_r(config);

    return text_result;
}
