#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/neutrino.h>
#include <sys/syspage.h>
#include <hw/inout.h>
#include <sys/mman.h>
#include <limits.h>
#include "Global.h"

#define PULSE_TIME 10000
#define ECHO_WAIT 100000
#define EXIT_SUCCESS 0
#define BASE_ADDRESS 0x280
#define PORTA_ADDRESS 0x08
#define PORTB_ADDRESS 0x09
#define CTRL_ADDRESS 0x0B
#define PORT_LENGTH 1

#define IN_P_S 13397
#define IN_P_NS (IN_P_S / 1000000000.0)


// Converts time in seconds to inches
int time_to_inches(double time)
{
	unsigned int Dist;

	//Initialize maximum and minimum distance measured
	Min_Dist = UINT_MAX;
	Max_Dist = 0;

	// out of bounds
	if (time > 0.018)
	{
		return -1;
	}
	Dist = time * IN_P_S;
	if (Dist > Max_Dist)
	{
		Max_Dist = Dist;
	}
	if (Dist < Min_Dist)
	{
		Min_Dist = Dist;
	}
	return Dist;
}

// initial 10uSec pulse, returns timestamp of falling edge of trigger
void init_pulse(void)
{
	// Set output to 1
	out8(porta_data, 0xFF);
	// Wait 10uSec
	usleep(10);
	// Set output to 0
	out8(porta_data, 0x00);

}

void capture_loop(void)
{
	int Exit = 0;

	//timestamps for raising and falling edges of echo pulse
	uint64_t timestamp_TrigFalling, timestamp_EchoFalling;
	unsigned int echo_input;
	while (!Exit)
	{
		// Increment number of samples counter
		Num_Captures++;
		init_pulse();
		echo_input = in8(portb_data) & 0x01;

		//wait for echo to go high, indicating start
		while (!echo_input)
		{
			echo_input = in8(portb_data) & 0x01;
		}
		// Capture time of raising edge of echo
		timestamp_TrigFalling = ClockCycles();

		// now wait for echo to go low
		while (echo_input)
		{
			echo_input = in8(portb_data) & 0x01;
		}
		// Capture time of falling edge of echo
		timestamp_EchoFalling = ClockCycles();

		// calculate echo pulse width
		uint64_t diff = ( (timestamp_EchoFalling) - (timestamp_TrigFalling));

		// Convert from cycles to seconds - divided by two because it is a round trip
		double seconds_diff = ((double)diff / SYSPAGE_ENTRY(qtime)->cycles_per_sec) / 2;

		//calculate the distance using echo pulse width
		int inches = time_to_inches(seconds_diff);

		// Update console
		if (inches != -1)
		{
			printf("%d in\n",inches);
		}
		else
		{
			printf("Capture %d: * Out of range\n",Num_Captures);
		}
		usleep(100000);
	}
}

void init(void)
{
	/* Give this thread root permissions to access the hardware */
	privity_err = ThreadCtl( _NTO_TCTL_IO, NULL );
	if ( privity_err == -1 )
	{
		fprintf( stderr, "can't get root permissions\n" );
	}

	printf("Getting ctrl handle... ");
	ctrl_handle = mmap_device_io( PORT_LENGTH, BASE_ADDRESS + CTRL_ADDRESS );
	printf("Got %16X\n",(unsigned int)ctrl_handle);

	// Set port B to input, port A to output
	out8(ctrl_handle, 0x02);
	printf("Wrote to ctrl_handle\n");

	//data handles for portA and portB
	porta_data = mmap_device_io( PORT_LENGTH, BASE_ADDRESS + PORTA_ADDRESS );
	printf("DataA: %16X\n",(unsigned int)porta_data);
	portb_data = mmap_device_io( PORT_LENGTH, BASE_ADDRESS + PORTB_ADDRESS );
	printf("DataB: %16X\n",(unsigned int)portb_data);

}

int main(int argc, char **argv)
{
	init();
	char c;
	printf("Press Y to start measurement...\n");
	while(1)
	{
		c = getchar();

		// Check for lowercase or uppercase y
		if((c | 0x20) == 'y')
		{
			printf("Starting distance measurement software....Hit enter to stop...\n");
			capture_loop();
			printf("Number of captures : %d \nMaximum Distance measured : %d \nMinimum Distance measured : %d\n", Num_Captures, Max_Dist, Min_Dist);
			printf("Press Y to restart measurement.....\n");
		}
	}

	return 0;
}
