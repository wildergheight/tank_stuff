//outputs
#define motor_dig1 14
#define motor_dig2 27

#define actuator_in1 5
#define actuator_in2 4
#define actuator_en 15

#define relay 23

//analog output

#define motor_an1 13
#define motor_an2 12

//inputs

#define ch1 22
#define ch2 19
#define ch3 18

#define actuator_sf 21

//analog input
#define actuator_fb 2



void setup() {
// Check home assistant power pin

// If off, deep sleep for time (10 seconds?)

// If on, turn relay pin ON

}

void loop() {

// Take in reciever RC PWM

// Send PWM out to Motor Driver Pins (convert for digital direction and analog magnitude)

// Check home assistant for actuator value

// If Extend/Retract, set actuator pins as such

// Check home assistant for power pin

// If off, deep sleep

}
