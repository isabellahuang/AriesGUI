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
#define GRNH 115
#define GRNL 80

//yellow
#define YLWH 45
#define YLWL 25

//red
#define REDH 25
#define REDL 300

typedef std::chrono::high_resolution_clock Clock;

//Define Item Enums
enum ITEM {
    PIKA,
    PIN,
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
    FLY_UP_2,
    HOVER_OVER,
    FIND_LANDING,
    LAND
};

using namespace cv;
using namespace std;

/** Function Headers */
Rect detectAndDisplay( Mat frame, ITEM inputItem );
double getRedFactor(Mat bgrMat, ITEM inputItem);
Rect2d update_box(cv::Mat input_image, ITEM inputItem);
RotatedRect camshiftBottom(Mat frame, Rect &trackWindowBottom);
RotatedRect camshiftFront(Mat frame, Rect &trackWindow);


/** Global variables */
String face_cascade_name = "trained_classifiers/haarcascade_frontalface_default.xml";
String pika_cascade_name = "/users/isabellahuang/AriesGUI/trained_classifiers/pikacascade7.xml";
String ball_cascade_name = "trained_classifiers/ball_cascade7.xml";
String basket_cascade_name = "trained_classifiers/basket_cascade8.xml";
String pot_cascade_name = "trained_classifiers/pot_cascade9.xml";

CascadeClassifier face_cascade;
CascadeClassifier pika_cascade;
CascadeClassifier select_cascade;


Rect detectAndDisplay(Mat frame, ITEM inputItem)
{
    std::vector<Rect> faces;
    Mat frame_gray;
    cvtColor(frame, frame_gray, COLOR_BGR2GRAY);
    equalizeHist(frame_gray, frame_gray);
    select_cascade.detectMultiScale( frame_gray, faces, 1.1, 2, 0, Size(30, 30) );

    Rect maxObj;
    float maxFactor = 0;
    float currFactor;
    float currRatio;
    float maxRatio;

    for( size_t i = 0; i < faces.size(); i++ ){
        Rect foundObj = faces[i];
//        rectangle( frame, foundObj, Scalar( 255, 0, 0 ), 4, 8, 0 );
        Mat currImg = frame(foundObj);
        currFactor = getRedFactor(currImg, inputItem);
        currRatio = 1.0 * currFactor / (currImg.cols * currImg.rows);
        if(currRatio > 0.1 && currFactor > maxFactor){
            maxFactor = currFactor;
            maxObj = foundObj;
            maxRatio = currRatio;
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
    case PIN:
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

    if( inputItem == PIN){
    //red we need to OR the two sections
        color =  (H > minL) | (H < maxL);
    }
    else{
    // other colors we need to AND the two sections
        color =  (H > minL) & (H < maxL);
    }

    double sumPixels  = cv::sum(color)[0] / 255;

    double colorFactor = sumPixels;  // (yellow.cols * yellow.rows); //-- A factor that ranges between 0 to 1 according to the presence of red pixels

    return colorFactor;
}

void CameraThread::run() {
    std::cout << "Running camera thread" << endl;

    // Initiate AR drone and the loop
    keep_running = true;
    start_flight = false;
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
    Rect trackWindowBottom;
    select_cascade.load("/users/isabellahuang/AriesGUI/trained_classifiers/pikacascade7.xml");

    // Initiate OpenCV structures
    Mat temp;
    Mat frame;
    RotatedRect trackBox; // For camshift
    RotatedRect trackBoxBottom;

    // Initiate flags for states
    start_haar = false;
    State current_state = PRE_TAKEOFF;
    confirmed_camshift = false;

    // Timing
    std::clock_t start_time;
    start_time = std::clock();

    std::clock_t start_yaw_control_time;
    std::clock_t start_takeoff_time;
    std::clock_t start_fly_up_time;
    std::clock_t start_hover_over_time;
    std::clock_t start_find_landing_time;

    double takeoff_duration;
    double yaw_control_duration;
    double flyup_duration;
    double hover_over_duration;
    double find_landing_duration;

    // Yaw centering control
    double x_offset_yaw = 0.0;
    double side_offset = 0.0;
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

    // Side side movement
    float kp_y = 1.f/600.f;

    // Control gains for forward approach stability
    float kp_a = 1.f/500.f;
    float kd_a = 1.f/500.f;
    float ki_a = 1.f/600.f;

    // Keep track of total offset
    double integral_x_offset_yaw = 0.0;
    double integral_height_offset = 0.0;
    double integral_flyup_width_offset = 0.0;

    double pre_flyup_width = 105;
    ITEM inputItem = PIKA;

    Rect2d bottom_cam_rect;

    static int m = 0;
    ardrone.setCamera(++m % 4);

    // The main loop
    try {
    while (keep_running) {

        // Get image from drone
        frame = ardrone.getImage();

        // Camshift bottom
        if (trackWindowBottom.height != 0 && trackWindowBottom.width != 0) {
            cout << "Camshifting" << endl;
            trackBoxBottom = camshiftFront(frame, trackWindowBottom);
            ellipse(frame, trackBoxBottom, Scalar(0,0,255), 3, LINE_AA );
        }

        else if (current_state == FLY_UP_2 || current_state == HOVER_OVER || true){
            trackWindowBottom = update_box(frame, inputItem);
//            cout << "Updating box" << endl;
            cv::rectangle(frame, trackWindowBottom, cv::Scalar(0, 255, 0));
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

        continue;

        // Set intended velocities to zero
        double vx = 0.0, vy = 0.0, vz = 0.0, vr = 0.0;

        // Start takeoff
        if (current_state == PRE_TAKEOFF && start_flight) {
            emit changeStatusSignal("Taking off");

            start_hover_over_time = clock();

            if (ardrone.onGround()) {
//                ardrone.takeoff();
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
            frame = ardrone.getImage();
            trackWindow = detectAndDisplay(frame, inputItem);
            if (trackWindow.height != 0 && trackWindow.width != 0) {
                start_haar = false;
                cout << "Done haar" << endl;
            }
        }

        // Do camshift if the trackWindow is present
        if (trackWindow.height != 0 && trackWindow.width != 0) {
            cout << "Camshifting" << endl;
            trackBox = camshiftFront(frame, trackWindow);
            ellipse(frame, trackBox, Scalar(0,0,255), 3, LINE_AA );
        }


        // Start yaw centering once the object is confirmed
        if (confirmed_camshift && current_state == START_TRACKING) {
            current_state = YAW_CENTERING;
            cout << "Starting yaw centering" << endl;
            start_yaw_control_time = std::clock();
        }

        // Calculate yaw required to stabilize object
        if (current_state == YAW_CENTERING) {
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

           // Side side control
           side_offset = trackBox.center.x - 320;
           vy = -1.0 * side_offset * kp_y;

           // Forward flight control
           flyup_width_offset = pre_flyup_width - trackBox.size.width;
           integral_flyup_width_offset += flyup_width_offset / 1000;

           double a_p = flyup_width_offset * kp_a;
           double a_i = integral_flyup_width_offset * ki_a;
           a_i = 0.0;
           vx = a_p + a_i;

           if (flyup_width_offset < 0) {
               current_state = FLY_UP;
               cout << "Flying up" << endl;
           }
        }

        if (current_state == FLY_UP) {

            // Height control
            vz = 0.1;

            // Side side control
            side_offset = trackBox.center.x - 320;
            vy = -1.0 * side_offset * kp_y;

            // Forward flight control
            flyup_width_offset = pre_flyup_width - trackBox.size.width;
            integral_flyup_width_offset += flyup_width_offset / 1000;

            double a_p = flyup_width_offset * kp_a;
            double a_i = integral_flyup_width_offset * ki_a;
            a_i = 0.0;
            vx = a_p + a_i;

            if (trackBox.size.width < 50) {
//                current_state = HOVER_OVER;
                current_state = FLY_UP_2;
                start_fly_up_time = clock();

                static int mode = 0;
                ardrone.setCamera(++mode % 4);
                start_hover_over_time = clock();
            }
            cout << trackBox.size.width << endl;
        }


        if(current_state == FLY_UP_2){
            flyup_duration = (std::clock() - start_fly_up_time) / (double) CLOCKS_PER_SEC;
            double flyup_height_offset = 2.0 - ardrone.getAltitude();

            if (flyup_duration < 3) {
                vx = 0.3;
                vz = 0.2;
            }
            else {
                vx = 0.1;
                vz = flyup_height_offset / 7;

            }
            cout << "Flying up_2 " <<flyup_height_offset << " "<< vz << endl;
            if (flyup_height_offset < 0.5) {
                current_state = HOVER_OVER;
            }
        }

        if (current_state == HOVER_OVER) {
            hover_over_duration = (std::clock() - start_hover_over_time) / (double) CLOCKS_PER_SEC;
            if (hover_over_duration > 10) {
                current_state = FIND_LANDING;
                start_find_landing_time = clock();
            }
        }

        if (current_state == FIND_LANDING) {
            find_landing_duration = (std::clock() - start_find_landing_time) / (double) CLOCKS_PER_SEC;
            vx = 0.2;
            if (find_landing_duration > 5) {
                cout << "DRONE IS LANDING" << endl;
                ardrone.landing();
            }
        }

        // Command movements
        if (!ardrone.onGround()) {
            if (vx != 0 || vy != 0 || vz != 0 || vr != 0) {
                std::cout << vx << " " << vy << " " << vz << " " << vr << std::endl;

            }
            cout << ardrone.getAltitude() << endl;

            ardrone.move3D(vx, vy, vz, vr);
        }


//        // Camshift bottom
//        if (trackWindowBottom.height != 0 && trackWindowBottom.width != 0 && false) {
//            cout << "Camshifting" << endl;
//            trackBoxBottom = camshiftBottom(frame, trackWindowBottom);
//            ellipse(frame, trackBoxBottom, Scalar(0,0,255), 3, LINE_AA );
//        }

//        else if (current_state == FLY_UP_2 || current_state == HOVER_OVER){
//            trackWindowBottom = update_box(frame, inputItem);
//            cout << trackWindowBottom.height << " " << trackWindowBottom.width << endl;
//            cv::rectangle(frame, trackWindowBottom, cv::Scalar(0, 255, 0));
//        }

//        // Send frame to GUI to display image
//        if (!frame.empty()) {
//            cvtColor(frame, temp, CV_BGR2RGB);
//            QImage dest((const uchar *)temp.data, temp.cols, temp.rows, temp.step, QImage::Format_RGB888);
//            dest.bits();

//            QPixmap pix = QPixmap::fromImage(dest);
//            emit sendTime(pix) ;

//            if (!keep_running) {
//                break;
//            }
//        }


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

void CameraThread::startFlightFlag() {
    start_flight = true;
    qInfo() << "Starting flight";
}

void CameraThread::searchAgainSlot() {
    start_haar = true;
}

void CameraThread::confirmCamshiftSlot() {
    qInfo() << "Confirming camshift";
    confirmed_camshift = true;
}


cv::Rect2d update_box(cv::Mat input_image, ITEM inputItem) {
    // HSV image
    // image setup
    int minH, maxH, minS, maxS, minV, maxV;
    std::string filename_pin("/users/isabellahuang/AriesGUI/thresholds/pin_thresholds.xml");
    std::string filename_pika("/users/isabellahuang/AriesGUI/thresholds/pika_thresholds.xml");
    std::string filename_wpot("/users/isabellahuang/AriesGUI/thresholds/wpot_thresholds.xml");
    std::string filename_ball("/users/isabellahuang/AriesGUI/thresholds/ball_thresholds.xml");
    std::string filename_select;

    switch(inputItem){
        case PIKA:
        filename_select = filename_pika;
        break;
        case PIN:
        filename_select = filename_pin;
        break;
        case WPOT:
        filename_select = filename_wpot;
        break;
        case BALL:
        filename_select = filename_ball;
        break;
        default:
        filename_select = filename_pika;
        break;
    }

    cv::FileStorage fs(filename_select, cv::FileStorage::READ);

    // If there is a save file then read it
    if (fs.isOpened()) {
        maxH = fs["H_MAX"];
        minH = fs["H_MIN"];
        maxS = fs["S_MAX"];
        minS = fs["S_MIN"];
        maxV = fs["V_MAX"];
        minV = fs["V_MIN"];
        fs.release();
    }

    // Create a window
    cv::namedWindow("binalized");
    cv::createTrackbar("H max", "binalized", &maxH, 255);
    cv::createTrackbar("H min", "binalized", &minH, 255);
    cv::createTrackbar("S max", "binalized", &maxS, 255);
    cv::createTrackbar("S min", "binalized", &minS, 255);
    cv::createTrackbar("V max", "binalized", &maxV, 255);
    cv::createTrackbar("V min", "binalized", &minV, 255);
    cv::resizeWindow("binalized", 0, 0);
    cv::Scalar lower(minH, minS, minV);
    cv::Scalar upper(maxH, maxS, maxV);
    cv::Mat hsv;
    cv::cvtColor(input_image, hsv, cv::COLOR_BGR2HSV_FULL);

    // Binalize
    cv::Mat binalized;
    cv::inRange(hsv, lower, upper, binalized);

    // De-noising
    cv::Mat kernel = getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::morphologyEx(binalized, binalized, cv::MORPH_CLOSE, kernel);
    //cv::imshow("morphologyEx", binalized);

    // Detect contours
    std::vector< std::vector<cv::Point> > contours;
    cv::findContours(binalized.clone(), contours, cv::RETR_CCOMP, cv::CHAIN_APPROX_SIMPLE);

    // Find largest contour
    int contour_index = -1;
    double max_area = 0.0;
    for (size_t i = 0; i < contours.size(); i++) {
        double area = fabs(cv::contourArea(contours[i]));
        if (area > max_area) {
            contour_index = i;
            max_area = area;
        }
    }
    cv::Rect2d rect;

    // Object detected
    float colorFactor = 0;
    if (contour_index >= 0) {
        // Show result
        rect = cv::boundingRect(contours[contour_index]);
        if (rect.width > 0 && rect.height > 0) {
            Mat cfRoi(input_image, rect);
            colorFactor = getRedFactor(cfRoi, inputItem)/(cfRoi.cols*cfRoi.rows);

        }
    }

    cout << colorFactor << endl;
    if (colorFactor < 0.25) {
        rect.width = 0;
        rect.height = 0;
    }

    return rect;
}

// Camshift on bottom camera
RotatedRect camshiftBottom(Mat frame, Rect &trackWindowBottom) {
    if (trackWindowBottom.height != 0 && trackWindowBottom.width != 0) {
        Mat image, hsv, hue, mask, hist, histimg = Mat::zeros(200, 320, CV_8UC3), backproj;
        cvtColor(frame, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(0, 60, 32), Scalar(180, 255, 255), mask);
        int ch[] = {0, 0};
        hue.create(hsv.size(), hsv.depth());
        mixChannels(&hsv, 1, &hue, 1, ch, 1);
        Mat roi(hue, trackWindowBottom), maskroi(mask, trackWindowBottom);
        int hsize = 16;
        float hranges[] = {0,180};
        const float* phranges = hranges;
        calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
        normalize(hist, hist, 0, 255, NORM_MINMAX);
        calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
        backproj &= mask;
        RotatedRect trackBox = CamShift(backproj, trackWindowBottom,
                    TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));
        if( trackWindowBottom.area() <= 1 )
        {
            int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
            trackWindowBottom = Rect(trackWindowBottom.x - r, trackWindowBottom.y - r,
                       trackWindowBottom.x + r, trackWindowBottom.y + r) &
                  Rect(0, 0, cols, rows);
        }
        return trackBox;
    }
}

// Camshift on front camera
RotatedRect camshiftFront(Mat frame, Rect &trackWindow) {
    if (trackWindow.height != 0 && trackWindow.width != 0) {
        Mat image, hsv, hue, mask, hist, histimg = Mat::zeros(200, 320, CV_8UC3), backproj;
        cvtColor(frame, hsv, COLOR_BGR2HSV);
        inRange(hsv, Scalar(0, 60, 32), Scalar(180, 255, 255), mask);
        int ch[] = {0, 0};
        hue.create(hsv.size(), hsv.depth());
        mixChannels(&hsv, 1, &hue, 1, ch, 1);
        Mat roi(hue, trackWindow), maskroi(mask, trackWindow);
        int hsize = 16;
        float hranges[] = {0,180};
        const float* phranges = hranges;
        calcHist(&roi, 1, 0, maskroi, hist, 1, &hsize, &phranges);
        normalize(hist, hist, 0, 255, NORM_MINMAX);
        calcBackProject(&hue, 1, 0, hist, backproj, &phranges);
        backproj &= mask;
        RotatedRect trackBox = CamShift(backproj, trackWindow,
                    TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));
        if( trackWindow.area() <= 1 )
        {
            int cols = backproj.cols, rows = backproj.rows, r = (MIN(cols, rows) + 5)/6;
            trackWindow = Rect(trackWindow.x - r, trackWindow.y - r,
                       trackWindow.x + r, trackWindow.y + r) &
                  Rect(0, 0, cols, rows);
        }
        return trackBox;
    }
}

