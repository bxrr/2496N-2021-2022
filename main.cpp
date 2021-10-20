#include "main.h"
#include "autons.h"
//autons has all includes through chains

#include <string.h>

using namespace pros;


// Globals
Chassis chas;

Piston frontPneu(FRONT_PNEUMATIC_PORT);
Piston backPneu(BACK_PNEUMATIC_PORT);
Piston frontSpecialPneu(FRONT_SPECIAL_PNUEMATIC_PORT);
Motor backLift(BACK_LIFT_PORT, E_MOTOR_GEARSET_18, false);
Motor frontLift(FRONT_LIFT_PORT, E_MOTOR_GEARSET_36, false);

Imu inert(INERT_PORT);
Controller con(E_CONTROLLER_MASTER);

Motor backLeft(BACK_LEFT_PORT);
Motor midLeft(MID_LEFT_PORT);
Motor frontLeft(FRONT_LEFT_PORT);
Motor backRight(BACK_RIGHT_PORT);
Motor midRight(MID_RIGHT_PORT);
Motor frontRight(FRONT_RIGHT_PORT);


int globalTime;		//time since code has initialized, used as a timer
bool disableAll = false;
double globalRotation = 0;


// misc functions ==============================================================


bool autonCurrentlySelecting = true;
int autonType = 1;
int autonColor = 1;
bool autonTypeSelected = false;
void autonSelector()
{
	static bool firstTime = true;
	static int localTime = 0;
	if(localTime > 150) {localTime = 0;}
	if(localTime == 50) {con.set_text(0,0, "Select Auton:");}

	if(localTime ==100)
	{
		if (autonType == 1){con.set_text(1,0, "Elevated Long        ");}
		else if (autonType == 2){con.set_text(1,0, "Elevated Short       ");}
		else if (autonType == 3){con.set_text(1,0, "De-elevated Long      ");}
		else if (autonType == 4){con.set_text(1,0, "De-elevated short     ");}
		else if (autonType == 5) {con.set_text(1,0, "Both               ");}
		else if (autonType == 6) {con.set_text(1,0, "Rush Neutral       ");}
		else if (autonType == 7){con.set_text(1,0, "Skills               ");}
		else if(autonType == 8) {con.set_text(1,0, "None                  ");}
	}

	if(autonTypeSelected)
	{
		if(localTime == 150)
		{
			if (autonColor == 1){con.set_text(2,0, "Red ");}
			else if (autonColor == 2){con.set_text(2,0, "Blue");}
		}
	}


	if (con.get_digital(E_CONTROLLER_DIGITAL_RIGHT))
	{
		con.clear();
		if(autonTypeSelected)
		{
			if(autonColor == 3){autonColor = 1;}
			else{autonColor++;}
		}
		else
		{
			if(autonType == 9) {autonType = 1;}
			else {autonType++;}
		}

		while (con.get_digital(E_CONTROLLER_DIGITAL_RIGHT)){}
	}

	else if (con.get_digital(E_CONTROLLER_DIGITAL_LEFT))
	{
		con.clear();
		if(autonTypeSelected)
		{
			if(autonColor == 0){autonColor = 2;}
			else{autonColor--;}
		}
		else
		{
			if(autonType == 0) {autonType = 8;}
			else {autonType--;}
		}
		while (con.get_digital(E_CONTROLLER_DIGITAL_LEFT)){}
	}


	if (con.get_digital(E_CONTROLLER_DIGITAL_A))
	{
		if(autonType >= 6) {autonCurrentlySelecting = false;}
		else
		{
			if(autonTypeSelected)
			{
				autonCurrentlySelecting = false;
				while(con.get_digital(E_CONTROLLER_DIGITAL_A)) {}
			}

			else
			{
				autonTypeSelected = true;
				while(con.get_digital(E_CONTROLLER_DIGITAL_A)) {}
			}
		}
	}

	localTime += 5;
	delay(5);
}





// auton functions =============================================================
double goalsPossessed = 0;
bool killAuton = false;

/*
void drive(double targetEnc, int timeout = 4000, double maxspeed = .6, double errorRange = 3)  
{	
	/*drive function called with targetEnc required, timeout defaulted to 4 seconds,
	maxspeed defaulted to 60%, and max error range to exit at 3 encoders 
	
	// Timeout counter
	int time = 0;
	float slewMult = 0.05;	//slew to prevent jerk
	// Drive distance variables: uses motor encodings with distance error
	double leftStartPos = chas.getLeftPos();
	double rightStartPos = chas.getRightPos();
	double distError = 0.1;		
	double currentPos = 0.0;
	double baseSpeed = 0.0;
	float distKp = 1.5;
	bool withinRange = false;
	int withinRangeTime = 0;
	// Drive straight variables: uses the intertial with PID
	inert.set_heading(180);
	double initialRotation = inert.get_heading();
	double error = 0.0;
	float kP = (targetEnc >= 0) ? (1) : (-1);
	kP *= (1 + goalsPossessed/3);		//change kP based on how many goals(will change weight of robot)
	// Drive loop, might add a timeout thing if it's needed
	while(timeout > time && !killAuton)		//killAuton exits auton in case of crucial error to prevent damage
	{
		// Drive code: Distance error set to target encoding - average of left + right encodings
		distError = targetEnc - ((leftStartPos - chas.getLeftPos()) + (rightStartPos - chas.getRightPos()) / 2);
		baseSpeed = slewMult * ((abs(distError * distKp) > 127 * maxspeed) ? (targetEnc > 0 ? 127 * maxspeed : -127 * maxspeed) : (distError * distKp));
		if(distError < -3)
		{
			baseSpeed = (baseSpeed > -15) ? (-15) : (baseSpeed);
		}
		else if(distError > 3)
		{
			baseSpeed = (baseSpeed < 15) ? (15) : (baseSpeed);
		}
		// Drive straight code: Changes left side of the chassis' speed according to the intertial sensor's readings
		error = initialRotation - inert.get_heading();
		if(abs(error) > 25)	//kill auton if the robot jerks off course severely
		{
			killAuton = true;
			break;
		}
		// Apply speeds
		chas.spinLeft(baseSpeed + (error * kP)*(baseSpeed/80));
		chas.spinRight(baseSpeed - (error * kP)*(baseSpeed/80));
		//break if within error range for more than 500 ms
		if(abs(distError) < errorRange)
		{
			if(!withinRange)
		{
				withinRangeTime = time;
				withinRange = true;
			}
			else if(time >= withinRangeTime + 500) { break; }
		}
		else
		{
			withinRange = false;
		}
		// delay while loop
		delay(10);
		time += 10;
		currentPos = chas.getLeftPos();
		if(slewMult < 1) {slewMult += 0.05;}
	}
	// Stop robot after loop
	chas.changeBrake(chas.HOLD);
	chas.stop();
	globalRotation += inert.get_heading() - initialRotation;	//used for absolute rotate function
}
void rotate(double degrees, int timeout = 60000, double speedM = 1)
{
	// Timeout counter
	if(speedM > 1) {speedM = speedM/100;}
	int time = 0;
	// ensure the inertial sensor does not go from 359 degrees to 0
	if(degrees < 0)
		inert.set_heading(350);
	else if(degrees > 0)
		inert.set_heading(10);
    else
		return;
	double targetHeading = inert.get_heading() + degrees;
	double currentRotation = inert.get_heading();
	double initialRotation = inert.get_heading();
	double error = targetHeading - currentRotation;
	double lastError = error;
	double integral = 0.0;
	double derivative = 0.0;
	double speed = 0.0;
	double rotateStartI = goalsPossessed + (1 + goalsPossessed/4);
	float kP = (1.5*(90/degrees) > 2 ? (2) : (1.5*(90/degrees) < 1.5 ? (1.5) : 1.5*(90/degrees))) * (1 + goalsPossessed/6);
	float kI = 0.1;
	float kD = 2;
	bool integ = false;
	bool withinRange = false;
	int withinRangeTime = 0;
	int sameErrorCount = 0;
	double errorCheck;
	while(!killAuton)
	{
		currentRotation = inert.get_heading();
		if(time % 200 == 0)
		{
			errorCheck = error;
		}
		lastError = error;
		error = targetHeading - currentRotation;
		if(abs(error) <= rotateStartI)
		{
			integ = true;
		}
		else if(abs(errorCheck - error) < 0.0001 && error < 5)
		{
			sameErrorCount++;
		}
		else
		{
			sameErrorCount = 0;
		}
		if(integ) { integral += error; }
		derivative = lastError - error;
		speed = (error * kP) + (integral * kI) + (derivative * kD);
		chas.spinLeft(speed * speedM);
		chas.spinRight(-speed * speedM);
		if(abs(error) <= 0.5)
		{
			if(!withinRange)
		 	{
				withinRangeTime = time;
				withinRange = true;
			}
			else if(time >= withinRangeTime + 300) { break; }
		}
		else { withinRange = false; }
		if(time > timeout) {break;}
		// delay while loop
		delay(5);
		time += 5;
	}
	// Stop robot after loop
	chas.changeBrake(chas.HOLD);
	chas.stop();
	globalRotation += inert.get_heading() - initialRotation; 	//is used for absolute rotate
}
//absolute rotate
void rotateTo(double degrees, int timeout=100000, double speedM  = 1) 
{ 
	rotate(degrees - globalRotation, timeout, speedM); 
}
// curve
void curve(double degrees, double strength, double timeout)
{
	int time = 0;
	if(degrees > 0)
		inert.set_heading(10);
	else if(degrees < 0)
		inert.set_heading(350);
	else
		return;
	
	double startDeg = inert.get_heading();
	double error;
	float kP = 1.1;
	int withinRangeTime = 0;
	bool withinRange = false;
	while(time <= timeout)
	{
		error = degrees - (inert.get_heading() - startDeg);
		double baseSpeed = (error * kP)
		if(degrees > 0)
		{
			chas.spinLeft(baseSpeed + (strength * (error/2) / 100));
			chas.spinright(baseSpeed);
		}
		else
		{
			chas.spinLeft(baseSpeed);
			chas.spinright(baseSpeed + (strength * (error/2) / 100));
		}
		if(abs(error < 1.0))
		{
			if(!withinRange)
				withinRange = true;
				withinRangeTime = time
			else if(withinRangeTime + 400 <= time)
			{
				chas.stop()
				return;
			}
		}
		else
		{
			withinRange = false;
		}
		delay(5);
		time += 5;
	}
}
void redElevatedLong() // final b4 comp for all reds
{
	goalsPossessed = 0;
	drive(400, 1800, 1, 10);
	drive(111,230, 1);
	frontPneu.toggle();
	frontLift.move_absolute(-200,-127);
	goalsPossessed = -0.3;
	delay(300);
	drive(-215,1300);
	backLift.move_absolute(-2230,-127);
	rotateTo(-95,2000);
	drive(50, 500);
	drive(-88,1000, 0.4);
	delay(250);
	drive(150,1000);
	delay(250);
	rotate(18.5, 1000);
	backLift.move_absolute(-3000,-127);
	delay(250);
	drive(-250,1500);
	backLift.move_absolute(-2000,127);
	delay(250);
	rotate(-55, 1000);
	drive(200, 1000);
}
void redElevatedShort()
{
	
	drive(150);
	drive(20,500);
	frontPneu.toggle();
	frontLift.move_absolute(-200, -127);
	drive(-150);
}
void redDeElevatedLong()
{
	drive(415, 1800, 1, 10);
	drive(200,200);
	frontPneu.toggle();
	goalsPossessed = -0.3;
	drive(-100, 500, 1, 10);
	frontLift.move_absolute(-200,-127);
	drive(-305,1500);
	backLift.move_absolute(-1800, -127);
	rotateTo(-86,2000);
	drive(-30,800);
	backLift.move_absolute(-2442, -127);
	delay(1000);
	drive(120, 1200);
}
void redDeElevatedShort()
{
	drive(130);
	drive(25, 350);
	frontPneu.toggle();
	delay(200);
	frontPneu.toggle();
	drive(-150);
}
void redBoth() // 10/11
{
	drive(150, 1500);
	drive(-150, 800);
	rotateTo(-90, 1600);
	drive(175, 2000);
	rotateTo(-179, 1400);
	drive(-755, 4000, 0.73);
	backLift.move_absolute(-2160, -127);
	rotateTo(-160, 900);
	delay(400);
	drive(-90, 1300, 0.4);
	drive(100, 1000);
	backLift.move_absolute(-2750, -127);
	drive(-200, 1500);
	backLift.move_absolute(-1800, 127);
	delay(300);
	drive(210);
}
void blueElevatedLong()
{
	goalsPossessed = 0;
	drive(400, 1800, 1, 10);
	drive(111,230, 1);
	frontPneu.toggle();
	frontLift.move_absolute(-200,-127);
	goalsPossessed = -0.3;
	delay(300);
	drive(-215,1300);
	backLift.move_absolute(-2230,-127);
	rotateTo(-95,2000);
	drive(50, 500);
	drive(-88,1000, 0.4);
	delay(250);
	drive(150,1000);
	delay(250);
	rotate(18.5, 1000);
	backLift.move_absolute(-3000,-127);
	delay(250);
	drive(-250,1500);
	backLift.move_absolute(-2000,127);
	delay(250);
	rotate(-55, 1000);
	drive(200, 1000);
}
void blueElevatedShort()
{
	
	drive(150);
	drive(20,500);
	frontPneu.toggle();
	frontLift.move_absolute(-200, -127);
	drive(-150);
}
void blueDeElevatedLong()
{
	drive(415, 1800, 1, 10);
	drive(200,200);
	frontPneu.toggle();
	goalsPossessed = -0.3;
	drive(-100, 500, 1, 10);
	frontLift.move_absolute(-200,-127);
	drive(-305,1500);
	backLift.move_absolute(-1800, -127);
	rotateTo(-86,2000);
	drive(-30,800);
	backLift.move_absolute(-2442, -127);
	delay(1000);
	drive(120, 1200);
}
void blueDeElevatedShort()
{
	drive(130);
	drive(25, 350);
	frontPneu.toggle();
	delay(200);
	frontPneu.toggle();
	drive(-150);
}
void blueBoth()
{
	drive(150, 1500);
	drive(-150, 800);
	rotateTo(-90, 1600);
	drive(175, 2000);
	rotateTo(-179, 1400);
	drive(-755, 4000, 0.73);
	backLift.move_absolute(-2160, -127);
	rotateTo(-160, 900);
	delay(400);
	drive(-90, 1300, 0.4);
	drive(100, 1000);
	backLift.move_absolute(-2750, -127);
	drive(-200, 1500);
	backLift.move_absolute(-1800, 127);
	delay(300);
	drive(210);
}
void skillsFuture()
{
	backLift.move_absolute(-2450, -127);
	delay(1500);
	drive(-130, 2000);
	backLift.move_absolute(0, 127); 
	goalsPossessed++;
	delay(1500); // first alliance goal should be obtained by here
	drive(130, 2000);
	rotateTo(-90);
	drive(140, 2000);
	rotateTo(-179);
	drive(-700, 4000, 0.73); // drive far towards 2nd alliance goal
	backLift.move_absolute(-2450, -127);
	drive(-150, 2000, 0.5);
	backLift.move_absolute(-1000, 126);
	delay(750); // 2nd alliance goal should be obtained by here
	goalsPossessed++;
	drive(800), 1500;
	rotateTo(-270, 1500);
	drive(400, 1500);
	drive(150, 1500);
	frontPneu.toggle(); // 3rd alliance goal should be obtained here
	goalsPossessed++;
	frontLift.move_absolute(-2000, -127);
	drive(400, 2000);
	rotate(90);
	
}
void skills()
{
	
	drive(-220);
	rotateTo(90);
	backLift.move_absolute(-3000, -127);
	delay(1500);
	drive(-120);
	backLift.move_absolute(-1000, 127);
	goalsPossessed++;
	delay(500);
	drive(140);
	rotateTo(0, 3000);
	backLift.move_absolute(-800, 127);
	delay(800);
	backLift.move_absolute(300, 127);
	delay(1500);
	backLift.move_absolute(-3000, -127);
	delay(1500);
	drive(-250, 3000, 0.35);
	backLift.move_absolute(-1500, 127);
	//goalsPossessed++;
	delay(1000);
	backLift.move(20);
	drive(-150);
	rotateTo(26, 3000);
	drive(-300);
	rotateTo(180, 4000, 0.75);
	drive(120);
	rotateTo(105);
	drive(120, 1200);
	//frontPneu.toggle();
	rotateTo(91, 1200);
	goalsPossessed = 3;
	park();
}
void neutralRush()
{
	chas.spinTo(2100,127);
	frontPneu.toggle();
	frontLift.move_absolute(-600, -127);
	delay(200);
	drive(-400);
	rotate(-140,4000,0.4);
}
*/



// driver control functions ====================================================

enum arcadeTypes {MANUAL, AUTO};
void arcadeDrive(arcadeTypes arcadeType)			//fully manual arcade drive
{
	
	if(arcadeType == MANUAL)
	{

		if(abs(con.get_analog(E_CONTROLLER_ANALOG_LEFT_Y)) > 10 || abs(con.get_analog(E_CONTROLLER_ANALOG_RIGHT_X)) > 10)
		{
			double turnStick = (chas.reverseStatus()) ? (-con.get_analog(E_CONTROLLER_ANALOG_RIGHT_X)) : (con.get_analog(E_CONTROLLER_ANALOG_RIGHT_X));
			chas.spinLeft(con.get_analog(E_CONTROLLER_ANALOG_LEFT_Y) + turnStick);
			chas.spinRight(con.get_analog(E_CONTROLLER_ANALOG_LEFT_Y) - turnStick);
		}
		else
		{
		chas.stop();
		}

	}

	else
	{
		static int autoStraightVal;
		static double inertialStart;					//starting inertial heading when driving straight
		static bool autoStraight = false; 				//boolean checking whether to implement autostraight assist
		double kPauto = chas.getVelocity() > 0 ? 4 : -4;		//kP is the magnitude of the effect of autostraight, reverse for when chassis is negative
		kPauto = chas.reverseStatus() ? -kPauto : kPauto;
		if(abs(con.get_analog(E_CONTROLLER_ANALOG_LEFT_Y)) > 10 || abs(con.get_analog(E_CONTROLLER_ANALOG_RIGHT_X)) > 10)
		{
				double turnStick = (chas.reverseStatus()) ? (-con.get_analog(E_CONTROLLER_ANALOG_RIGHT_X)) : (con.get_analog(E_CONTROLLER_ANALOG_RIGHT_X));
				if(abs(turnStick) < 5) 		//if not turning(initiate autostraight)
				{
					if(autoStraight) 			//if autostraight enabled, calculate error
					{
						autoStraightVal = (inert.get_heading() - inertialStart) * kPauto;

						if(autoStraightVal > 10) {autoStraight = false;} 		//if there is a big unexpected jerk, ie if the robot is hit by an external object, reset autostraight
					}

					else
					{
						autoStraight = true;					//since driving straight, set autostraight to true
						inert.set_heading(180);					//reset inertial valu
						inertialStart = inert.get_heading();	//set initial inertial value, this part of the loop will not be executed again until autostraight is reset
					}
				}

				else
				{
					autoStraight = false;			//disable autostraight calculations because turning is true
					autoStraightVal = 0;			//disable effect of autostraight
				}

				//chas controls(take the vertical stick +- the turn stick and apply autostraight effect, if autoStraightVal = 0, then autostraight is disabled)
				chas.spinLeft(con.get_analog(E_CONTROLLER_ANALOG_LEFT_Y) + turnStick - (autoStraightVal) * (chas.getVelocity()/127));
				chas.spinRight(con.get_analog(E_CONTROLLER_ANALOG_LEFT_Y) - turnStick + (autoStraightVal) * (chas.getVelocity()/127));
		}
		else
		{
			chas.stop();
			autoStraight = false;
		}
	}
}



void tankDrive()
{
	if(abs(con.get_analog(E_CONTROLLER_ANALOG_LEFT_Y)) > 10 || abs(con.get_analog(E_CONTROLLER_ANALOG_RIGHT_Y)) > 10)
	{
		chas.spinLeft(con.get_analog(E_CONTROLLER_ANALOG_LEFT_Y));
		chas.spinRight(con.get_analog(E_CONTROLLER_ANALOG_RIGHT_Y));
	}
	else
	{
		chas.stop();
	}
}

void reverseToggle()
{
	if(con.get_digital(E_CONTROLLER_DIGITAL_Y))
		chas.reverseControls();
	else
		chas.reverseReleased();
}

//brake type control
bool disableAuto = false;
void brakeType()
{
	bool brakeinitiate = true;
	static int startTime = 0;
	if(con.get_digital(E_CONTROLLER_DIGITAL_UP))
	{
		if(chas.getBrakeMode() == 0) {chas.changeBrake(chas.HOLD);}
		else {chas.changeBrake(chas.COAST);}
		disableAuto = true;
		startTime = globalTime;
		while(con.get_digital(E_CONTROLLER_DIGITAL_UP)) {}
	}
	//disable automatic brake mode selection if manual selection has been made within 10 sec
	if(!(startTime == 0) && !disableAll) {globalTime - startTime < 8000 ? disableAuto = true : disableAuto = false;}
}

void autoBrakeMode()	//automatically sets brake mode
{
	static bool getStartTime = true;
	static int startTime;
	//disable automatic brake mode selection if manual selection has been made within 10 sec
	if(!disableAuto)
	{
		//set brake type to hold if robot is on platform and is at risk of sliding off
		if(abs(int(inert.get_pitch())) > 10 && globalTime > 3000)
		{
			if (abs(con.get_analog(E_CONTROLLER_ANALOG_RIGHT_X)) > 10|| abs(con.get_analog(E_CONTROLLER_ANALOG_LEFT_Y)) > 10)
			{
				chas.changeBrake(chas.HOLD);
			}
			else
			{
				chas.changeBrake(chas.S_HOLD, inert.get_pitch());
			}
			getStartTime = true;		//reset the 2 second timer
		}
		//set brake type to coast w/ 2 second delay
		else
		{
			if(chas.getBrakeMode() == 1 && globalTime > 3000)
			{
				if(getStartTime)
				{
					startTime = globalTime;
					getStartTime = false;
				}
				if (globalTime - startTime > 2000)
				{
					chas.changeBrake(chas.COAST);
					chas.stop();
					getStartTime = true;
				}
			}
		}
	}
}


void park(bool pistonUsed = false)
{
	bool parking = false;		
	while(true)
	{
		//prevent the back lift from falling down if holding mobile goal
		backLift.move(25);	

		//prevent the front lift from inhibiting the park, if being used
		if(abs(inert.get_pitch()) > 1 && pistonUsed) {frontLift.move(-30);}

		if(abs(inert.get_pitch() > 21))		//move from step 1 to 2
		{
			parking = true;
		}

		if(abs(inert.get_pitch()) < 15 && !parking)		//step 1(initialize)
		{
			chas.spinLeft(127);
			chas.spinRight(127);
		}

		else if(abs(inert.get_pitch()) < 21 && parking)	//step 3(finalize park)
		{
			chas.stop();
			chas.changeBrake(chas.HOLD);
			backLift.move(0);
			frontLift.move(0);
		}

		else
		{	
			//step 2(moving up the platform)
			chas.changeBrake(chas.S_HOLD, inert.get_pitch(), 4.7 + 0.3 * goalsPossessed);
		}
	}
}


// lifts
void liftControl()
{
	static bool front = true;
	static double backSpeed = 0;
	static bool firstTime = false;
	static bool changeVelocity = true;
	static int startTime = 0;
	// Check if 'X' is being pressed and set back lift to coast if it is
	if(con.get_digital(E_CONTROLLER_DIGITAL_X))
		backLift.set_brake_mode(E_MOTOR_BRAKE_COAST);
	else
		backLift.set_brake_mode(E_MOTOR_BRAKE_HOLD);

	// Check if driver is currently controlling pneumatics or lifts
	if(con.get_digital(E_CONTROLLER_DIGITAL_L2) == false)
	{

		// Check L1 to see if driver wants to control front lift or back lift

		if(con.get_digital(E_CONTROLLER_DIGITAL_L1)) {front = false;}
		else {front = true;}
		// Move lift up
		if(con.get_digital(E_CONTROLLER_DIGITAL_R1))
		{
			if(front)
			{
				frontLift.move(-127);
				backLift.move(0);
			}
			else
			{
				frontLift.move(0);
				/*
				if(backLift.get_position() <= 5)
				{
					backLift.move(0);
				}
				else
				{
					backLift.move(127);
				}
				*/
				backLift.move(127);
			}
			firstTime = true;
			changeVelocity = true;
			startTime = 0;
		}
		// Move lift down
		else if(con.get_digital(E_CONTROLLER_DIGITAL_R2))
		{
			if(front)
			{
				frontLift.move(127);
				backLift.move(0);
			}
			else
			{
				backLift.move(-127);
				frontLift.move(0);
			}
			firstTime = true;
			changeVelocity = true;
			startTime = 0;
		}
		// No buttons are being pressed so stop both lifts
		else
		{
			if(firstTime) 
			{
				startTime = globalTime;
				firstTime = false;
			}
			if(globalTime - startTime <= 200) {backSpeed = 0;}
			else if(globalTime - startTime > 200 && changeVelocity) 
			{
				backSpeed = -backLift.get_actual_velocity();
				changeVelocity = false;
			}
			
			if(globalTime - startTime > 700 && abs(backLift.get_actual_velocity()) > 1) {changeVelocity = true;}
			frontLift.move(0);
			backLift.move(backSpeed);
		}
	}
}

// Pneumatics
void pneumaticControl()
{
	// Check if l2 is pressed down, meaning driver wants to control the pneumatic
	if(con.get_digital(E_CONTROLLER_DIGITAL_L2))
	{
		// firstPress1 boolean is used to make sure the front pneumatic
		// doesn't repeatedly toggle on and off when R1 is held
		static bool firstPress1 = true;
		if(con.get_digital(E_CONTROLLER_DIGITAL_R1))
		{
			// Check if this is R1's initial press, and toggle the pneumatic if it is.
			if(firstPress1)
			{
				backPneu.toggle();
				firstPress1 = false;
			}
		}
		// Allow the pneumatic to be toggled again after the button has been released
		else
			firstPress1 = true;

		// firstPress2 boolean is used to make sure the back pneumatic
		// doesn't repeatedly toggle on and off when the R2 is held
		static bool firstPress2 = true;
		if(con.get_digital(E_CONTROLLER_DIGITAL_R2))
		{
			// Check if this is R2's initial press, and toggle the pneumatic if it is.
			if(firstPress2)
			{
				frontPneu.toggle();
				firstPress2 = false;
			}
		}

		else
			firstPress2 = true;

		static bool firstPress3 = true;
		if(con.get_digital(E_CONTROLLER_DIGITAL_L1))
		{
			// Check if this is R2's initial press, and toggle the pneumatic if it is.
			if(firstPress3)
			{
				frontSpecialPneu.toggle();
				firstPress3 = false;
			}
		}

		// Allow the pneumatic to be toggled again after the button has been released
		else
			firstPress3 = true;
	}
}

void killAllAuto()
{
	if(disableAll) {disableAuto = true;}
	if(con.get_digital(E_CONTROLLER_DIGITAL_LEFT) && con.get_digital(E_CONTROLLER_DIGITAL_RIGHT))
	{
		con.clear();
		disableAll = !disableAll;
		while(con.get_digital(E_CONTROLLER_DIGITAL_LEFT) && con.get_digital(E_CONTROLLER_DIGITAL_RIGHT)) {}
	}
}

// opControl/auton functions
void printInfo()
{
	chas.getLeftPos();
	chas.getRightPos();
	if(globalTime < 2000)
	{
		if(globalTime % 50 == 0)
		{ con.set_text(0,0, "Selected:       "); }
	}

	else
	{

		static int counter = 0;
		if(counter == 10)
		{
			//print whether the chassis controls are reversed or not
			/*
			if(chas.reverseStatus() == false) {con.set_text(0, 0, "Chas: FORWARD");}
			else {con.set_text(0,0, "Chas: REVERSE");}
			*/
			
			con.print(0,0,"inert: %.2f", inert.get_heading());
		}
		if(counter == 20)
		{

			//print the brake type for the chassis
			if(disableAuto)
			{
				if(chas.getBrakeMode() == 0) {con.set_text(1,0, "Brake Mode: COAST");}
				else if(chas.getBrakeMode() == 1) {con.set_text(1,0, "Brake Mode : HOLD");}
			}
			else
			{
				if(chas.getBrakeMode() == 0) {con.set_text(1,0, "Brake M(A): COAST");}
				else if(chas.getBrakeMode() == 1) {con.set_text(1,0, "Brake M(A) : HOLD");}
			}

		}
		if(counter == 30)
		{
			//prints the temperature of the chassis

			if(disableAll) {con.print(2, 0, "ALL AUTO DISABLED");}
			else
			{
				if((chas.leftTemp() + chas.rightTemp()) /2 > 53)
				{
					con.print(2, 0, "Chassis(HOT): %.0f°C", ((chas.leftTemp() + chas.rightTemp()) / 2));
				}
				else
				{
					con.print(2, 0, "Chassis: %.0f°C", ((chas.leftTemp() + chas.rightTemp()) / 2));
				}
			}

			//con.print(2, 0, "Inert: %.2f", inert.get_pitch());
			counter = 0;
		}

		counter++;
	}
}

//autonomous functions
//drive(enc, timeout, pct power)
//rotate(degrees, timeout)

//autonomous(will be called by competition)
void autonomous()
{	
	/*
	KEY:
	elevated = side with platform elevated
	de-elevated = side with platform not elevated
	both = starts from de-elevated side but utilizes both side, most likely use in quals
	rush: rushes the neutral goal, will win 99% of the time, no win point, most likely use in elims
	long = gets at least 2 goals
	short = only gets one win point from alliance mobile goal
	*/

	midRight.move(127);

	while(!killAuton)
	{

		if(autonType == 1)
		{
			if(autonColor == 1) {redElevatedLong();}
			else {blueElevatedLong();}
		}

		if(autonType == 2)	//only gets one win point on elevated side
		{
			if(autonColor == 1) {redElevatedShort();}
			else {blueElevatedShort();}
		}

		if(autonType == 3)
		{
			if(autonColor == 1) {redDeElevatedLong();}
			else {blueDeElevatedLong();}
		}

		if(autonType == 4)
		{
			if(autonColor == 1) {redDeElevatedShort();}
			else {blueDeElevatedShort();}
		}

		if(autonType == 5)
		{
			if(autonColor == 1) {redBoth();}
			else {blueBoth();}
		}


		if(autonType == 6) {neutralRush();}
		if(autonType == 7) {skills();}

		break;
	}	
}

// main control functions ======================================================
void initialize()
{
	lcd::initialize();
	frontLift.set_brake_mode(E_MOTOR_BRAKE_HOLD);
	backLift.set_brake_mode(E_MOTOR_BRAKE_HOLD);
	chas.changeBrake(chas.COAST);
	disableAuto = false;
}



void disabled() {
	chas.changeBrake(chas.HOLD);
	backLift.set_brake_mode(E_MOTOR_BRAKE_HOLD);
	frontLift.set_brake_mode(E_MOTOR_BRAKE_HOLD);
}

void competition_initialize() {}

void opcontrol()
{
	lcd::set_text(0, "aayush the goat");
	con.clear();
	while(autonCurrentlySelecting) { autonSelector(); }

	chas.changeBrake(chas.COAST);
	backLift.set_brake_mode(E_MOTOR_BRAKE_HOLD);

	while (true)
	{

		// Drive loop (there's an arcadeDrive() function and tankDrive() function.

		arcadeDrive(MANUAL);
		liftControl();
		pneumaticControl();
		reverseToggle();
		brakeType();
		autoBrakeMode();
		killAllAuto();
		printInfo();

		if(con.get_digital(E_CONTROLLER_DIGITAL_DOWN)) { autonomous(); }
		
		delay(5);
		globalTime += 5;
	}
}