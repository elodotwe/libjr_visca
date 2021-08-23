#include <jr_visca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bail(int line, char *message) {
    fprintf(stderr, "%d: %s\n", line, message);
    exit(-1);
}

void assertEqualsBuffer(uint8_t *a, uint8_t *b, int length, int line, char *message) {
    if (memcmp(a, b, length) != 0) {
        bail(line, message);
    }
}

void assertEqualsInt(int a, int b, int line, char *message) {
    if (a != b) {
        bail(line, message);
    }
}

int main() {
    printf("jr_visca_tester\n");
    uint8_t data[] = {1, 2, 3, 4, 5, 0xff};
    jr_viscaFrame frame;
    
    int result = jr_viscaDataToFrame(data, sizeof(data), &frame);

    assertEqualsInt(result, 6, __LINE__, "data to frame should consume 6 bytes");
    assertEqualsInt(frame.dataLength, 4, __LINE__, "frame.dataLength should be 4");
    uint8_t expectedFrameContents[] = {2, 3, 4, 5};
    assertEqualsBuffer(frame.data, expectedFrameContents, 4, __LINE__, "frame.data should be 2, 3, 4, 5");
    assertEqualsInt(frame.sender, 0, __LINE__, "sender should be 0");

    return 0;
}