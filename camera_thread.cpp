#include "camera_thread.h"
#include "src/ardrone/ardrone.h"
//#include "/users/isabellahuang/arya/cvdrone/src/ardrone/ardrone.h"

#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QCamera>
#include <QImage>
#include <chrono>

//cascade includes
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

//tracking includes
#include <opencv2/core/utility.hpp>
#include "opencv2/video/tracking.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include "opencv2/highgui.hpp"

#define PI 3.14159265

//Color definitions
//violet
#define VLTH 300
#define VLTL 250

//blue
#define BLUH 250
#define BLUL 170

//blue green
#define GBMH 180
#define GBML 145

//green
#define GRNH 145
#define GRNL 75

//yellow
#define YLWH 75
#define YLWL 25

//red
#define REDH 25
#define REDL 300

typedef std::chrono::high_resolution_clock Clock;

//Define Item Enums
enum ITEM {
    PIKA,
    BASK,
    WPOT,
    BALL};

enum State {
    PRE_TAKEOFF,
    TAKEOFF,
    START_TRACKING,
    YAW,
    YAW_CENTERING,
    HEIGHT_ADJUSTING,
    APPROACH,
    FLY_UP,
    HOVER_OVER,
};

using namespace cv;
using namespace std;

/** Function Headers */
Rect detectAndDisplay( Mat frame, ITEM inputItem );
double getRedFactor(Mat bgrMat, ITEM inputItem);

/** Global variables */
String face_cascade_name = "trained_classifiers/haarcascade_frontalface_default.xml";
String pika_cascade_name = "trained_classifiers/pikacascade7.xml";
String ball_cascade_name = "trained_classifiers/ball_cascade7.xml";
String basket_cascade_name = "trained_classifiers/basket_cascade8.xml";
String pot_cascade_name = "trained_classifiers/pot_cascade9.xml";

CascadeClassifier face_cascade;
CascadeClassifier pika_cascade;
CascadeClassifier select_cascade;


Rect detectAndDisplay(Mat frame)
{
    ITEM inputItem = PIKA;
    std::vector<Rect> faces;
    Mat frame_gray;
    cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
    equalizeHist(frame_gray, frame_gray);
    select_cascade.detectMultiScale( frame_gray, faces, 1.5, 2, 0|CASCADE_SCALE_IMAGE, Size(30, 30) );

    Rect maxObj;
    float maxFactor = 0;
    float currFactor;

    for( size_t i = 0; i < faces.size(); i++ ){
        Rect foundObj = faces[i];
        rectangle( frame, foundObj, Scalar( 255, 0, 0 ), 4, 8, 0 );
        Mat currImg = frame(foundObj);
        currFactor = getRedFactor(currImg, inputItem);
        if(currFactor > maxFactor){
            maxFactor = currFactor;
            maxObj = foundObj;
         }
    }
    rectangle( frame, maxObj, Scalar( 0, 255, 255 ), 4, 8, 0 );
    return maxObj;
}

double getRedFactor(Mat bgrMat, ITEM inputItem)
{
    cv::Mat hsv;
    int minL, maxL;
    std::vector<cv::Mat> hsvChannels;

    cv::cvtColor(bgrMat, hsv, CV_BGR2HSV);
    cv::split(hsv, hsvChannels);

    cv::Mat H = hsvChannels[0];
    cv::Mat S = hsvChannels[1];
    cv::Mat V = hsvChannels[2];

    cv::Mat color;
    switch(inputItem){
    case PIKA:
        //yellow
        minL = YLWL;
        maxL = YLWH;
        break;
    case BASK:
        //red
        minL = REDL;
        maxL = REDH;
        break;

    case WPOT:
        //green
        minL = GRNL;
        maxL = GRNH;
        break;

    case BALL:
        //blue
        minL = BLUL;
        maxL = BLUH;
        break;
    default:
        minL = REDL;
        maxL = REDH;
        break;

    }

    if( inputItem == BASK){
    //red we need to OR the two sections
        color =  (H > minL) | (H < maxL);
    }
    else{
    // other colors we need to AND the two sections
        color =  (H > minL) & (H < maxL);
    }

    double sumPixels  = cv::sum(color)[0] / 255;

    double colorFactor = sumPixels; // (yellow.cols * yellow.rows); //-- A factor that ranges between 0 to 1 according to the presence of red pixels

    return colorFactor;
}

void CameraThread::run() {
    std::cout << "Running camera thread" << endl;

    // Initiate AR drone and the loop
    keep_running = true;
    ARDrone ardrone;
    if (!ardrone.open()) {
        keep_running = false;
    }
//    ardrone.landing();
    qInfo() << "Battery percentage:" << ardrone.getBatteryPercentage();
    ardrone.setFlatTrim();

    // Load Haar cascade
//    VideoCapture capture(0);
    Rect trackWindow;
    int hsize = 16;
    float hranges[] = {0,180};
    const float* phranges = hranges;
    select_cascade.load("/users/isabellahuang/AriesGUI/trained_classifiers/pikacascade7.xml");

    // Initiate OpenCV structures
    Mat temp;
    Mat frame;
    RotatedRect trackBox; // For camshift

    // Initiate flags for states
    start_haar = false;
    int initial_counter = 0;
    State current_state = PRE_TAKEOFF;
    confirmed_camshift = false;

    // Timing
    std::clock_t start_time;
    start_time = std::clock();

    std::clock_t start_approach_calibration_time;
    std::clock_t start_yaw_control_time;

    std::clock_t start_takeoff_time;
    std::clock_t start_fly_up_time;

    double takeoff_duration;
    double yaw_control_duration;

    // Yaw centering control
    double x_offset_yaw = 0.0;
    double height_offset = 0.0;
    double flyup_width_offset = 0.0;
    double prev_x_offset_yaw = 0.0;
    float kp_yaw = 1.f/500.f;
    float kd_yaw = 1.f/500.f;
    float ki_yaw = 1.f/500.f;
    float prev_vr = 0.0;

    // Control loop gains for translational stability
    // Up down movement
    float kp_z = 1.f/1000.f;
    float kd_z = 1.f/500.f;
    float ki_z = 1.f/600.f;

    // Control gains for forward approach stability
    float kp_a = 1.f/600.f;
    float kd_a = 1.f/500.f;
    float ki_a = 1.f/600.f;

    // Keep track of total offset
    double integral_x_offset_yaw = 0.0;
    double integral_height_offset = 0.0;
    double integral_flyup_width_offset = 0.0;

    double pre_flyup_width = 100;

    // The main loop
    try {
    while (keep_running) {

        // Get image from drone
        frame = ardrone.getImage();

        // Set intended velocities to zero
        double vx = 0.0, vy = 0.0, vz = 0.0, vr = 0.0;

        // Start takeoff
        if (current_state == PRE_TAKEOFF) {
            if (ardrone.onGround()) {
//                ardrone.takeoff();
                cout << "Take off" << endl;
                current_state = TAKEOFF;
                start_takeoff_time = std::clock();
            }
        }

        // Give the drone 8 seconds from takeoff to start tracking
//        if (!ardrone.onGround() && current_state == TAKEOFF) {
        if (current_state == TAKEOFF) {

            takeoff_duration = (std::clock() - start_takeoff_time) / (double) CLOCKS_PER_SEC;
            if (takeoff_duration > 8) {
                std::cout << "Start tracking" << endl;
                start_haar = true;
                current_state = START_TRACKING;
            }
        }

        // Run the Haar cascade
        if (start_haar) {
             cout << "Starting haar" << endl;
            while (true) {
                frame = ardrone.getImage();
                if (frame.empty()) break;
                trackWindow = detectAndDisplay(frame);
                if (trackWindow.height != 0 && trackWindow.width != 0) break;
            }
            start_haar = false;
        }

        // Do camshift if the trackWindow is present
        if (trackWindow.height != 0 && trackWindow.width != 0) {
            Mat image, hsv, hue, mask, hist, histimg = Mat::zeros(200, 320, CV_8UC3), backproj;
            if (frame.empty()) break;
            cvtColor(frame, hsv, COLOR_BGR2HSV);
            inRange(hsv, Scalar(0, 60, 32), Scalar(180, 255, 255), mask);
            int ch[] = {0, 0};
            hue.create(hsv.size(), hsv.depth());
            mixChannels(&hsv, 1, &hue, 1, ch, 1);
            Mat roi(hue, trackWindow), maskroi(mask, trackWindow);
            calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
            normalize(hist, hist, 0, 255, NORM_MINMAX);
            calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
            backproj &= mask;
            trackBox = CamShift(backproj, trackWindow,
                        TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));
            if( trackWindow.area() <= 1 )
            {
                int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
                trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                           trackWindow.x + r, trackWindow.y + r) &
                      Rect(0, 0, cols, rows);
            }
            ellipse(frame, trackBox, Scalar(0,0,255), 3, LINE_AA );
        }

        // Start yaw centering once the object is confirmed
        if (confirmed_camshift && current_state == START_TRACKING) {
            current_state = YAW_CENTERING;
            cout << "Starting yaw centering" << endl;
            start_yaw_control_time = std::clock();
        }

        // Calculate yaw required to stabilize object
        if (current_state == YAW_CENTERING || current_state == APPROACH) {
            x_offset_yaw = trackBox.center.x - 320;
            // The offset ranges from -320 to 320
            integral_x_offset_yaw += x_offset_yaw/250.0;
            vr = (kp_yaw * x_offset_yaw) + (kd_yaw * std::abs(prev_x_offset_yaw - x_offset_yaw)) + (ki_yaw * integral_x_offset_yaw);
            vr = -1.0 * vr;
            prev_vr = vr;
            prev_x_offset_yaw = x_offset_yaw;
//            cout << integral_x_offset_yaw << " " << x_offset_yaw << endl;
            yaw_control_duration = (std::clock() - start_yaw_control_time) / (double) CLOCKS_PER_SEC;
        }

        // Start approach
//        if (current_state == YAW_CENTERING && fabs(integral_x_offset_yaw) < 90 && fabs(x_offset_yaw) < 3 && fabs(prev_vr) < 0.2) {
        if (current_state == YAW_CENTERING && yaw_control_duration > 20) {
            // This means the object has been stabilized enough to commence approach calibration
            std::cout << "STARTING HEIGHT ADJUSTMENT" <<  std::endl;
            current_state = APPROACH;
        }

        // Approach control (height and x)
        if (current_state == APPROACH) {

           // Height control
           height_offset = 180 - trackBox.center.y;
           integral_height_offset += height_offset / 250;
           double z_p = height_offset * kp_z;
           double z_i = integral_height_offset * ki_z;
           z_i = 0.0;
           vz = z_p + z_i;

           // Forward flight control
           flyup_width_offset = pre_flyup_width - trackBox.size.width;
           integral_flyup_width_offset += flyup_width_offset / 1000;

           double a_p = flyup_width_offset * kp_a;
           double a_i = integral_flyup_width_offset * ki_a;
           a_i = 0.0;
           vx = a_p + a_i;

           if (flyup_width_offset < 0) {
               current_state == FLY_UP;
               cout << "Flying up" << endl;
           }

        }

        if (current_state == FLY_UP) {
            vz = 0.1;
        }

        // Command movements
        if (!ardrone.onGround()) {
            std::cout << vx << " " << vy << " " << vz << " " << vr << std::endl;
//            ardrone.move3D(vx, vz, vz, vr);
        }

        // Send frame to GUI to display image
        if (!frame.empty()) {
            cvtColor(frame, temp, CV_BGR2RGB);
            QImage dest((const uchar *)temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
            dest.bits();

            QPixmap pix = QPixmap::fromImage(dest);
            emit sendTime(pix) ;

            if (!keep_running) {
                break;
            }
        }


    } // while keep running loop end


    }

    catch (...) {
        ardrone.landing();
    }

    ardrone.landing();
    exec();

}

void CameraThread::timerHit() {
    QString newTime= QDateTime::currentDateTime().toString("ddd MMMM d yy, hh:mm:ss");
    if(m_lastTime != newTime ){
        m_lastTime = newTime;

        QPixmap pix ("/Users/isabellahuang/Downloads/logo.png");
        emit sendTime(pix) ;

    }
}

void CameraThread::endThread() {
    keep_running = false;
    qInfo() << "Ending thread";
}

void CameraThread::searchAgainSlot() {
    start_haar = true;
}

void CameraThread::confirmCamshiftSlot() {
    qInfo() << "Confirming camshift";
    confirmed_camshift = true;
}


