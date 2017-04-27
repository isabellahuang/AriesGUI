#include <string>
#include <iostream>
#include <vector>
#include <algorithm>    // std::any_of
#include <array>        // std::array
#include <sstream>
#include <iterator>
#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))

// MAKE SURE YOU PUT THIS IN YOUR .PRO FILE
// CONFIG += c++11

enum ITEM {
    PIKA,
    PIN,
    WPOT,
    BALL};

const std::string pika_words[] = {"mouse", "doll", "plush", "yellow", "toy", "cartoon"};
const std::string pin_words[] = {"orange", "pin", "bowling", "bowl"};
const std::string wpot_words[] = {"watering", "can", "green", "pail", "pale", "bucket", "water"};
const std::string ball_words[] = {"ball", "volleyball", "basketball", "blue", "round", "circular"};

// unsigned int split(const std::string &txt, std::vector<std::string> &strs, char ch);
ITEM find_object(std::string input);

// unsigned int split(const std::string &txt, std::vector<std::string> &strs, char ch)
// {
//     unsigned int pos = txt.find( ch );
//     unsigned int initialPos = 0;
//     strs.clear();

//     // Decompose statement
//     while( pos != std::string::npos ) {
//         strs.push_back( txt.substr( initialPos, pos - initialPos + 1 ) );
//         initialPos = pos + 1;

//         pos = txt.find( ch, initialPos );
//     }

//     // Add the last one
//     strs.push_back( txt.substr( initialPos, std::min<int>( pos, txt.size() ) - initialPos + 1 ) );

//     return strs.size();
// }


ITEM find_object(std::string input){
    // Split the string into tokens

    std::istringstream iss(input);

    std::vector<std::string> v{std::istream_iterator<std::string>{iss},
                      std::istream_iterator<std::string>{}};

    int i = 0; 
    for (i = 0; i < ARRAY_SIZE(pika_words); i++) {
        if (std::find(v.begin(), v.end(), pika_words[i]) != v.end()){
            return PIKA;
        }
    }

    for (i = 0; i < ARRAY_SIZE(pin_words); i++) {
        if (std::find(v.begin(), v.end(), pin_words[i]) != v.end()){
            return PIN;
        }
        
    }

    for (i = 0; i < ARRAY_SIZE(wpot_words); i++) {
        if (std::find(v.begin(), v.end(), wpot_words[i]) != v.end()){
            return WPOT;
        }        
    }

    for (i = 0; i < ARRAY_SIZE(ball_words); i++) {        
        if (std::find(v.begin(), v.end(), ball_words[i]) != v.end()){
            return BALL;
        }        
    }
    std::cout << "uhoh";    
    return PIKA;
}

int main()
{
    std::string input = "Get me the yellow doll";
    ITEM thing = find_object(input);
    std::cout << thing;

    return 0;
}