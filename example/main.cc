//
// Created by keyi on 12/21/17.
//

#include "../src/h264.hh"

using namespace std;

int main(int , char * []) {
    MP4File mp4("test.mp4");
    h264 decoder(mp4);

}
