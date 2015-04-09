#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/joystick.h>
#include <cstdlib>
#include <iostream>
#include <signal.h>
#include <sys/socket.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#define JOY_DEV "/dev/input/js0"

#define FWD 0
#define BCK	1
#define LEFT 2
#define RIGHT 3
#define DIR 4

using namespace std;

int s;

int processed_values[4] = {0,0,0,0};

void my_handler(int signal){
           printf("Caught CTRL+C\n");
           close(s);
           exit(1); 
}

void apply_values(){
	//Send data to robot
	char buffer[12], aux_1, aux_2;
	int speed, dir;
	strcpy (buffer,"DIR");
  	
	if (processed_values[FWD] != 0){
		aux_1 = '0';
		speed = processed_values[FWD];
	}
	
	if (processed_values[BCK] != 0){
		aux_1 = '-';
		speed = processed_values[BCK];
	}
	
	if(processed_values[FWD] == 0 && processed_values[BCK] == 0){
		aux_1 = '0';
		speed = 0;
	}
	
	if (processed_values[LEFT] != 0){
		aux_2 = '-';
		dir = processed_values[LEFT];
	}
	
	if (processed_values[RIGHT] != 0){
		aux_2 = '0';
		dir = processed_values[RIGHT];
	}
	
	if(processed_values[LEFT] == 0 && processed_values[RIGHT] == 0){
		aux_2 = '0';
		dir = 0;
	}
	
	sprintf (buffer, "DIR%c%03d%c%03d\n", aux_1, speed, aux_2, dir);
	
	cout << buffer << endl;
	
	write(s, buffer, 12);
	
	cout << "FWD [" << processed_values[FWD] << "] | BCK ["<< processed_values[BCK] << "] | LEFT [" << processed_values[LEFT] << "] | RIGHT [" << processed_values[RIGHT] << "]" << endl;
}

void input_processing (int cmd, int input){
	if (cmd == DIR){
		if (input > 0){
			cmd = LEFT;
			input_processing(cmd, 0);
			cmd = RIGHT;
		}else if(input < 0){
			cmd = RIGHT;
			input_processing(cmd, 0);
			cmd = LEFT;
			input = abs(input);
		}else{
			cmd = LEFT;
			input_processing(cmd, input);
			cmd = RIGHT;
		}
	}
	if (processed_values[cmd] == 0 && input != 0 || processed_values[cmd] != 0 && input == 0 || processed_values[cmd] != 0 && input != 0){
		switch(cmd){
			case FWD:
				processed_values[cmd] = input/328;
				break;
			case BCK:
				processed_values[cmd] = input/328;
				break;
			case LEFT:	
				processed_values[cmd] = input/328;
				break;
			case RIGHT:
				processed_values[cmd] = input/328;
				break;
		}
		
		apply_values();
	}
	
	
}


int main()
{
	int joy_fd, *axis=NULL, num_of_axis=0, num_of_buttons=0, x;
	char *button=NULL, name_of_joystick[80];
	struct js_event js;


	// Bluetooth code
	
	struct sockaddr_rc addr = { 1 };
    int status;
    char dest[18] = "20:13:05:15:31:05";
	
	struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);	
	
    // allocate a socket
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( dest, &addr.rc_bdaddr );

    // connect to server
    status = connect(s, (struct sockaddr *)&addr, sizeof(addr));

    // send a message
    if( status == 0 ) {
    	printf("Connected!\n");
    }else if( status < 0 ){
    	printf("Ups.. can't connect!");
   	}
    	
   	
   	// Bluetooth code END
   	

	if( ( joy_fd = open( JOY_DEV , O_RDONLY)) == -1 )
	{
		cout << "Couldn't open joystick" << endl;
		return -1;
	}

	ioctl( joy_fd, JSIOCGAXES, &num_of_axis );
	ioctl( joy_fd, JSIOCGBUTTONS, &num_of_buttons );
	ioctl( joy_fd, JSIOCGNAME(80), &name_of_joystick );

	axis = (int *) calloc( num_of_axis, sizeof( int ) );
	button = (char *) calloc( num_of_buttons, sizeof( char ) );

	printf("Joystick detected: %s\n\t%d axis\n\t%d buttons\n"
		, name_of_joystick
		, num_of_axis
		, num_of_buttons );

	fcntl( joy_fd, F_SETFL, O_NONBLOCK );	/* use non-blocking mode */

	cout << "Ready!\n" << endl;

	while( 1 ) 	/* infinite loop */
	{

			/* read the joystick state */
		read(joy_fd, &js, sizeof(struct js_event));
		
			/* see what to do with the event */
		switch (js.type & ~JS_EVENT_INIT)
		{
			case JS_EVENT_AXIS:
				axis   [ js.number ] = js.value;
				break;
			case JS_EVENT_BUTTON:
				button [ js.number ] = js.value;
				break;
		}
		
		// Detect conflits and process input
		
		if (axis[12] != 0 && axis[13] != 0){
			// Rumbleeeeeee
			input_processing(BCK, 0);
			input_processing(FWD, 0);
		}else{
			input_processing(BCK, axis[12]);
			input_processing(FWD, axis[13]);
		}
		
		input_processing(DIR, axis[0]);
	
	}



	close( joy_fd );	/* too bad we never get here */
	close(s);
	return 0;
}
