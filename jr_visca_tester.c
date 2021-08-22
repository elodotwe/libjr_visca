#include <jr_visca.h>
#include <stdio.h>

int main() {
    printf("jr_visca_tester\n");
    uint8_t data[] = {1, 2, 3, 4, 5, 0xff};
    jr_viscaFrame frame;
    printf("%d\n", jr_viscaDataToFrame(data, sizeof(data), &frame));

    return 0;
}