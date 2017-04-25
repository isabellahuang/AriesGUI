// This is a class that manages the settings used to detect certain objects
// So far the items are a pikachu doll, a watering can and a ball

enum ITEM {
    PIKA,
    PIN,
    WPOT,
    BALL};

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

/*
The VIO (very important object) class is a wrapper for all the objects that you 
wish to detect
Any new object you should classify should be added to this class.

How to use:
Declare an instance of VIO, passing in the appropriate enum corresponding to the object
that you wish to detect.

VIO would have these attributes

oid: Object ID, containing the enum of the object you wish to detect
cascade: the path to the xml that you wish to use for haar
minL: ??
maxL: ??

TODO: You'll have to copy and paste this into your camera_thread.cpp
*/
class VIO {
    // The items we can currently detect
    // TODO: ENSURE THIS WORKS
    String face_cascade_name = "trained_classifiers/haarcascade_frontalface_default.xml";
    String pika_cascade_name = "trained_classifiers/pikacascade7.xml";
    String ball_cascade_name = "trained_classifiers/ball_cascade7.xml";
    String pin_cascade_name = "trained_classifiers/pin_cascade<UNKNOWN>.xml";
    String pot_cascade_name = "trained_classifiers/pot_cascade9.xml";

    public:
        // getters
        int getMinL();
        int getMaxL();
        ITEM getOID();
        String getCascadeName();
        cv::Mat getColor(cv::Mat H);

        // setters
        VIO(ITEM id); // Constructor

    private:
        ITEM oid; //PIKA, BALL, WPOT
        String cascade; // one of the above cascade names
        int minL;
        int maxL; 
        // cv::Mat color;
};

VIO::getColor(cv::Mat H) {
    return (H > minL) | (H < maxL);
}

VIO::VIO(ITEM id) {
    switch(id) {
        case PIKA: 
            oid = id;
            cascade = pika_cascade_name;
            minL = YLWL;
            maxL = YLWH;
            break;

        case WPOT:
            oid = id
            cascade = pot_cascade_name;
            minL = GRNL;
            maxL = GRNH;
            break;

        case BALL:
            oid = id;
            cascade = ball_cascade_name;
            minL = BLUL;
            maxL = BLUH;
            break;

        case PIN:
            oid = id;
            cascade = pin_cascade_name;
            minL = -1;
            maxL = -1;
            break;
    }
}