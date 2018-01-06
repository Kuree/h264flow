//
// Created by keyi on 12/21/17.
//

#include "../src/h264.hh"

using namespace std;

int main(int , char * []) {
    auto bs = make_shared<BitStream>("news.264");
    h264 decoder(bs);
    decoder.load_frame(4);
}
