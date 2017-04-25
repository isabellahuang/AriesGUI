// This is a class that manages the settings used to detect certain objects
// So far the items are a pikachu doll, a watering can and a ball

enum ITEM {
    PIKA,
    BASK,
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


class Object_Manager {
    // The items we can currently detect
    // TODO: ENSURE THIS WORKS
    String face_cascade_name = "trained_classifiers/haarcascade_frontalface_default.xml";
    String pika_cascade_name = "trained_classifiers/pikacascade7.xml";
    String ball_cascade_name = "trained_classifiers/ball_cascade7.xml";
    String basket_cascade_name = "trained_classifiers/basket_cascade8.xml";
    String pot_cascade_name = "trained_classifiers/pot_cascade9.xml";
    public:
        // getters
        int getMinL();
        int getMaxL();
        ITEM getOID();
        String getCascadeName();
        cv::Mat getColor(cv::Mat H);

        // setters
        Object_Manager(ITEM id); // Constructor

    private:
        ITEM oid; //PIKA, BALL, WPOT
        String cascade; // one of the above cascade names
        int minL;
        int maxL; 
        // cv::Mat color;
};

Object_Manager::getColor(cv::Mat H) {
    return (H > minL) | (H < maxL);
}

Object_Manager::Object_Manager(ITEM id) {
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
    }
}