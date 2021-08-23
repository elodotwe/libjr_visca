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

void assertHappyPathFrame(jr_viscaFrame frame, int result) {
    assertEqualsInt(result, 6, __LINE__, "data to frame should consume 6 bytes");
    assertEqualsInt(frame.dataLength, 4, __LINE__, "frame.dataLength should be 4");
    uint8_t expectedFrameContents[] = {2, 3, 4, 5};
    assertEqualsBuffer(frame.data, expectedFrameContents, 4, __LINE__, "frame.data should be 2, 3, 4, 5");
    assertEqualsInt(frame.sender, 0, __LINE__, "sender should be 0");
    assertEqualsInt(frame.receiver, 1, __LINE__, "receiver should be 1");
}

void testDataToFrame() {
    {
        // Happy path
        uint8_t data[] = {1, 2, 3, 4, 5, 0xff};
        jr_viscaFrame frame;
        int result = jr_viscaDataToFrame(data, sizeof(data), &frame);
        assertHappyPathFrame(frame, result);
    }

    {
        // More than one frame present in buffer
        uint8_t data[] = {1, 2, 3, 4, 5, 0xff, 10, 20, 30, 40};
        jr_viscaFrame frame;
        int result = jr_viscaDataToFrame(data, sizeof(data), &frame);
        assertHappyPathFrame(frame, result);
    }

    {
        // Partial frame in buffer
        uint8_t data[] = {1, 2, 3, 4, 5};
        jr_viscaFrame frame;
        int result = jr_viscaDataToFrame(data, sizeof(data), &frame);

        assertEqualsInt(result, 0, __LINE__, "no frame is present, so result should be 0");
    }

    {
        // 0 bytes in buffer
        uint8_t data[] = {};
        jr_viscaFrame frame;
        int result = jr_viscaDataToFrame(data, sizeof(data), &frame);
        assertEqualsInt(result, 0, __LINE__, "no frame is present, so result should be 0");
    }

    {
        // Truncated frame (header + terminator); probably technically illegal but we'll tolerate it.
        uint8_t data[] = {0xa3, 0xff};
        jr_viscaFrame frame;
        int result = jr_viscaDataToFrame(data, sizeof(data), &frame);
        assertEqualsInt(result, 2, __LINE__, "should consume the header and terminator (2)");
        assertEqualsInt(frame.dataLength, 0, __LINE__, "frame.dataLength should be 0");
        assertEqualsInt(frame.sender, 2, __LINE__, "sender should be 2");
        assertEqualsInt(frame.receiver, 3, __LINE__, "receiver should be 3");
    }

    {
        // Bare terminator (no header or contents); we don't currently have a way to describe this in `jr_viscaFrame` so we'll call it an error.
        uint8_t data[] = {0xff};
        jr_viscaFrame frame;
        int result = jr_viscaDataToFrame(data, sizeof(data), &frame);
        assertEqualsInt(result, -1, __LINE__, "bare terminator should cause error");
    }
}

int main() {
    printf("jr_visca_tester\n");
    testDataToFrame();

    return 0;
}