#include <jr_visca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bail(int line, char *message) {
    fprintf(stderr, "line %d: %s\n", line, message);
    exit(-1);
}

void _hex_print(char *buf, int buf_size) {
    for (int i = 0; i < buf_size; i++) {
        printf("%02hhx ", buf[i]);
    }
}

void assertEqualsBuffer(uint8_t *actual, uint8_t *expected, int length, int line, char *message) {
    if (memcmp(actual, expected, length) != 0) {
        printf("expected ");
        _hex_print(expected, length);
        printf(", actual ");
        _hex_print(actual, length);
        printf("\n");
        bail(line, message);
    }
}

void assertEqualsInt(int actual, int expected, int line, char *message) {
    if (actual != expected) {
        printf("expected %d, actual %d\n", expected, actual);
        bail(line, message);
    }
}

void assertEncodedMessage(int message, union jr_viscaMessageParameters messageParameters, uint8_t sender, uint8_t receiver, uint8_t *expectedData, int expectedDataLength, int line) {
    uint8_t actualData[JR_VISCA_MAX_ENCODED_MESSAGE_DATA_LENGTH];
    int actualEncodedLength = jr_viscaEncodeMessage(actualData, sizeof(actualData), message, messageParameters, sender, receiver);
    assertEqualsInt(actualEncodedLength, expectedDataLength, line, "data length should match");
    assertEqualsBuffer(actualData, expectedData, expectedDataLength, line, "data should be well formed");    
}

void testEncodeMessage() {
    {
        union jr_viscaMessageParameters messageParameters;
        messageParameters.panTiltPositionInqResponseParameters.panPosition = 0x1234;
        messageParameters.panTiltPositionInqResponseParameters.tiltPosition = 0xcdef;
        uint8_t expectedData[] = {0x90, 0x50, 0x01, 0x02, 0x03, 0x04, 0x0c, 0x0d, 0x0e, 0x0f, 0xff};
        assertEncodedMessage(JR_VISCA_MESSAGE_PAN_TILT_POSITION_INQ_RESPONSE, messageParameters, 1, 0, expectedData, sizeof(expectedData), __LINE__);
    }
}

void testAckDecode() {
    uint8_t encoded[] = {0x90, 0x42, 0xff};
    int message = 0;
    union jr_viscaMessageParameters parameters;
    uint8_t sender = 0;
    uint8_t receiver = 0;
    int result = jr_viscaDecodeMessage(encoded, sizeof(encoded), &message, &parameters, &sender, &receiver);
    assertEqualsInt(result, 3, __LINE__, "decode should consume the entire frame and not error out");
    assertEqualsInt(message, JR_VISCA_MESSAGE_ACK, __LINE__, "decoded message type should be ACK");
    uint8_t expectedSocket = 2;
    assertEqualsBuffer(&parameters.ackCompletionParameters.socketNumber, &expectedSocket, 1, __LINE__, "decoded ACK socket number wrong");
}

void testAckEncode() {
    union jr_viscaMessageParameters parameters;
    parameters.ackCompletionParameters.socketNumber = 3;
    uint8_t expectedData[] = {0x90, 0x43, 0xff};
    assertEncodedMessage(JR_VISCA_MESSAGE_ACK, parameters, 1, 0, expectedData, sizeof(expectedData), __LINE__);
}

int main() {
    printf("jr_visca_tester\n");

    testEncodeMessage();
    testAckDecode();
    testAckEncode();

    return 0;
}
