#include <msp430g2553.h>
#include <stdio.h>
#include <stdint.h>
#include "cc1101_def.h"
#include "cc1101.h"
#include "task.h"
int main(void) {
	WDTCTL = WDTPW | WDTHOLD;

	BCSCTL1 = CALBC1_16MHZ;
	DCOCTL = CALDCO_16MHZ;

	radio_init();
	task();
	return 0;
}
