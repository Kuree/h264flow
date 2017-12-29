//
// Created by keyi on 12/21/17.
//

#include "../src/h264.hh"

using namespace std;

int main(int , char * []) {
    auto mp4 = make_shared<MP4File>("news.mp4");
    h264 decoder(mp4);
    decoder.load_frame(1);
}
