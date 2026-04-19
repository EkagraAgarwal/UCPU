#include "Display.h"

void Visualizer::begin() {
    matrix.begin();
}

void Visualizer::drawDepth(float depthMM) {
    int displayHeight = map((int)depthMM, 0, 40, 0, 8);
    uint8_t frame[8][12] = {0};

    for (int row = 7; row >= (8 - displayHeight); row--) {
        for (int col = 0; col < 12; col++) {
            frame[row][col] = 1;
        }
    }
    matrix.renderBitmap(frame, 8, 12);
}

void Visualizer::showBootMessage() {
    // Optional: Add a custom animation or scrolling text here
}
