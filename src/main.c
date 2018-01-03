#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include "task.h"
int main(void) {
	WDTCTL = WDTPW | WDTHOLD;

	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

	task();
	return 0;
}
