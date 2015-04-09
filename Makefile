all:	joystick

joystick: joystick.cpp
	g++ joystick.cpp -o joystick -lbluetooth
