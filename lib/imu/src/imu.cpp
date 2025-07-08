#include <Arduino.h>
#include <I2Cdev.h>
#include <MPU6050_6Axis_MotionApps20.h>

#include <vars.h>
#include <Wire.h>
#include <imu.h>
#include <tools.h>
#include <oled.h>
#include <rgb.h>
#include <buzzer.h>


int imu_recalibrate_log_handler = imu_recalibrate_log_handler_moderate;

//For the Task that will govern the IMU behaviour --------------------------------

int task_imu_status = task_imu_not_needed;

int riding_mode_led_default_brightness = 100;
int riding_mode_normal_position_threshold_angle = 90;

//For the Core function of the IMU -----------------------------------------------
//#include "MPU6050.h" // not necessary if using MotionApps include file

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for SparkFun breakout and InvenSense evaluation board)
// AD0 high = 0x69
//MPU6050 mpu;
MPU6050 mpu(imu_addr_hex); 

/* =========================================================================
   NOTE: In addition to connection 3.3v, GND, SDA, and SCL, this sketch
   depends on the MPU-6050's INT pin being connected to the Arduino's
   external interrupt #0 pin. On the Arduino Uno and Mega 2560, this is
   digital I/O pin 2.
 * ========================================================================= */

// uncomment "OUTPUT_READABLE_QUATERNION" if you want to see the actual
// quaternion components in a [w, x, y, z] format (not best for parsing
// on a remote host such as Processing or something though)
//#define OUTPUT_READABLE_QUATERNION

// uncomment "OUTPUT_READABLE_EULER" if you want to see Euler angles
// (in degrees) calculated from the quaternions coming from the FIFO.
// Note that Euler angles suffer from gimbal lock (for more info, see
// http://en.wikipedia.org/wiki/Gimbal_lock)
//#define OUTPUT_READABLE_EULER

// uncomment "OUTPUT_READABLE_YAWPITCHROLL" if you want to see the yaw/
// pitch/roll angles (in degrees) calculated from the quaternions coming
// from the FIFO. Note this also requires gravity vector calculations.
// Also note that yaw/pitch/roll angles suffer from gimbal lock (for
// more info, see: http://en.wikipedia.org/wiki/Gimbal_lock)


//Uncomment this demo on readable Y P R values
//#define OUTPUT_READABLE_YAWPITCHROLL

// uncomment "OUTPUT_READABLE_REALACCEL" if you want to see acceleration
// components with gravity removed. This acceleration reference frame is
// not compensated for orientation, so +X is always +X according to the
// sensor, just without the effects of gravity. If you want acceleration
// compensated for orientation, us OUTPUT_READABLE_WORLDACCEL instead.


//TODO implement a demo showcasing both this and lower and decide which one to use 
//#define OUTPUT_READABLE_REALACCEL

// uncomment "OUTPUT_READABLE_WORLDACCEL" if you want to see acceleration
// components with gravity removed and adjusted for the world frame of
// reference (yaw is relative to initial orientation, since no magnetometer
// is present in this case). Could be quite handy in some cases.

//TODO implement a demo showcasing both this and upper and decide which one to use 
//#define OUTPUT_READABLE_WORLDACCEL

// uncomment "OUTPUT_TEAPOT" if you want output that matches the
// format used for the InvenSense teapot demo
//FOR THE TEapot DEMO on Ubuntu check the tutorial 

//for I2cDev https://github.com/postspectacular/toxiclibs.git



//Uncomment this for nice demo with plane
//#define OUTPUT_TEAPOT


#define INTERRUPT_PIN esp_imu_int_pin 
#define LED_PIN esp_built_in_led_pin 

bool blinkState = false;

bool imu_initialized = false;

int imu_run_mode = imu_run_mode_silent;


// MPU control/status vars
bool dmpReady = false;  // set true if DMP init was successful
uint8_t mpuIntStatus;   // holds actual interrupt status byte from MPU
uint8_t devStatus;      // return status after each device operation (0 = success, !0 = error)
uint16_t packetSize;    // expected DMP packet size (default is 42 bytes)
uint16_t fifoCount;     // count of all bytes currently in FIFO
uint8_t fifoBuffer[64]; // FIFO storage buffer

// orientation/motion vars
Quaternion q;           // [w, x, y, z]         quaternion container
VectorInt16 aa;         // [x, y, z]            accel sensor measurements
VectorInt16 aaReal;     // [x, y, z]            gravity-free accel sensor measurements
VectorInt16 aaWorld;    // [x, y, z]            world-frame accel sensor measurements
VectorFloat gravity;    // [x, y, z]            gravity vector
float euler[3];         // [psi, theta, phi]    Euler angle container
float ypr[3];           // [yaw, pitch, roll]   yaw/pitch/roll container and gravity vector

// packet structure for InvenSense teapot demo
uint8_t teapotPacket[14] = { '$', 0x02, 0,0, 0,0, 0,0, 0,0, 0x00, 0x00, '\r', '\n' };

float imu_yaw = 0  ;
float imu_pitch = 0 ;
float imu_roll = 0 ;

int imu_acc_x ;
int imu_acc_y ;
int imu_acc_z ;

int imu_acc_comp_grav_x = 0;
int imu_acc_comp_grav_y = 0;
int imu_acc_comp_grav_z = 0;


bool imu_new_data = false;

//TODO all ints to maybe int_8?

//------------------------------------------------------------------------------------

//main_status will indicate if the bike is on riding or parking mode 

//IN PARKING MODE the imu is off to save battery 

//The imu will be triggered to RIDING MODE by the moving flag on the vibration sensor 

//In RIDING MODE we will make a silent task that will be running and handles 
//the imu_status based on the following combinations:

//Two parameters : one for acc and other for gyro

enum imu_acc_status
{
    imu_acc_static,
    imu_acc_moving,
    imu_acc_crash,
    imu_acc_fall,
};

int imu_acc_status = imu_acc_moving; 

enum imu_gyro_status 
{
    imu_gyro_riding,
    imu_gyro_falling,
    imu_gyro_upside_down
};
int imu_gyro_status;

//The combination of both variables will cascade into the imu_status cases:

//This is essentialy the biker status

enum imu_status
{
    imu_status_idle,   // <- norma position but not moving
    imu_status_riding, // <- normal position and moving   

    imu_status_abnormal_pos_static, // <- the acc was not enough to detect a fall (e.g. letting it on the ground or repairing upside-down) 
    imu_status_abnormal_pos_moving, // <- e.g. transporting the bike upside down or in weird position  

    imu_status_fall_detected,             
    imu_status_crash_detected, 

    imu_status_fall_not_responding, //Not moving or changing position after the fall and not dismissing    
    imu_status_crash_not_responding,//Not moving or changing position after the crash and not dismissing

    imu_status_emergency_activated,

    imu_status_emergency_dismissed_by_buttons,  //Any button in any position will dismiss but alert weird position
    imu_status_emergency_dismissed_by_position_and_button, //Returning to Riding Position and pushing the button will dismiss
    imu_status_emergency_dismissed_remotely,    //Either Bluetooth or LTE will dismiss  
};
int imu_status = imu_status_riding;


enum imu_task_log_mode
{
    imu_task_log_mode_silent,
    imu_task_log_mode_just_critical,
    imu_task_log_mode_moderate,
    imu_task_log_mode_verbose
};
int imu_task_log_mode = imu_task_log_mode_verbose;


#define imu_accident_detected_threshold  50
#define imu_accident_confirmed_threshold 100
#define imu_accident_dismiss_threshold   50


int gyro_upper_threshold =  90;
int gyro_lower_threshold = -90;

int acc_upper_threshold =  10000;
int acc_lower_threshold = -10000;


//---------------------------------------------------------------------------------------------------------

// ================================================================
// ===               INTERRUPT DETECTION ROUTINE                ===
// ================================================================

// indicates whether MPU interrupt pin has gone high
volatile bool mpuInterrupt = false;     

void dmpDataReady() 
{
    mpuInterrupt = false;
}


//TODO : later separate this into init , cal , etc ..   

void imu_init() 
{
    if(oled_enabled)
    {
        oled_demo_message_imu_booting();
        //oled_refresh();
    }

    pinMode(INTERRUPT_PIN, INPUT);

    //mpu.setRate(9);      // 1000/(1+9) should be 100Hz

    mpu.initialize();

    // verify connection
    Serial.println(mpu.testConnection() ? F("-IMU_MPU6050 connection successful") : F("IMU_MPU6050 connection failed"));

    // wait for ready (Unccomment if needed)
    //Serial.println(F("\nSend any character to begin DMP programming and demo: "));
    //while (Serial.available() && Serial.read()); // empty buffer
    //while (!Serial.available());                 // wait for data
    //while (Serial.available() && Serial.read()); // empty buffer again

    // load and configure the DMP
    Serial.println(F("Initializing DMP..."));

    devStatus = mpu.dmpInitialize();

    // supply your own gyro offsets here, scaled for min sensitivity
    mpu.setXGyroOffset(220);
    mpu.setYGyroOffset(76);
    mpu.setZGyroOffset(-85);
    mpu.setZAccelOffset(1788); // 1688 factory default for my test chip

    // make sure it worked (returns 0 if so)
    if (devStatus == 0) 
    {
        // Calibration Time: generate offsets and calibrate our MPU6050
        mpu.CalibrateAccel(6);
        mpu.CalibrateGyro(6);
        mpu.PrintActiveOffsets();
        // turn on the DMP, now that it's ready
        Serial.println(F("Enabling DMP..."));
        mpu.setDMPEnabled(true);

        // enable interrupt detection
        if(log_enabled) Serial.print(F("Enabling interrupt detection (external interrupt) "));
        if(log_enabled) Serial.print(digitalPinToInterrupt(INTERRUPT_PIN));
        Serial.println(F(")..."));
        attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), dmpDataReady, RISING);
        mpuIntStatus = mpu.getIntStatus();

        // set our DMP Ready flag so the main loop() function knows it's okay to use it
        //Serial.println(F("DMP ready! Waiting for first interrupt..."));
        Serial.println(F("DMP ready! IMU Initialized ..."));
        dmpReady = true;

        // get expected DMP packet size for later comparison
        packetSize = mpu.dmpGetFIFOPacketSize();

        imu_initialized = true;
    } 
    else 
    {
        // ERROR!
        // 1 = initial memory load failed
        // 2 = DMP configuration updates failed
        // (if it's going to break, usually the code will be 1)
        if(log_enabled) Serial.print(F("DMP Initialization failed (code "));
        if(log_enabled) Serial.print(devStatus);
        Serial.println(F(")"));
    }

    // configure LED for output
    pinMode(LED_PIN, OUTPUT);
}

void imu_demo() //Before launching Read down if uncomment teapot or yawpitchroll
{
    if(!imu_initialized)imu_init();

    if (imu_initialized)
    {
            //Uncomment above if Serial log is desired -> #define OUTPUT_READABLE_YAWPITCHROLL
        //and comment the #define OUTPUT_TEAPOT    


        // if programming failed, don't try to do anything
        if (!dmpReady) return;
        // read a packet from FIFO
        if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) 
        { 
            // Get the Latest packet 
            #ifdef OUTPUT_READABLE_QUATERNION
                // display quaternion values in easy matrix form: w x y z
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                if(log_enabled) Serial.print("quat\t");
                if(log_enabled) Serial.print(q.w);
                if(log_enabled) Serial.print("\t");
                if(log_enabled) Serial.print(q.x);
                if(log_enabled) Serial.print("\t");
                if(log_enabled) Serial.print(q.y);
                if(log_enabled) Serial.print("\t");
                Serial.println(q.z);
            #endif

            #ifdef OUTPUT_READABLE_EULER
                // display Euler angles in degrees
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetEuler(euler, &q);
                if(log_enabled) Serial.print("euler\t");
                if(log_enabled) Serial.print(euler[0] * 180/M_PI);
                if(log_enabled) Serial.print("\t");
                if(log_enabled) Serial.print(euler[1] * 180/M_PI);
                if(log_enabled) Serial.print("\t");
                Serial.println(euler[2] * 180/M_PI);
            #endif

            #ifdef OUTPUT_READABLE_YAWPITCHROLL
                // display Euler angles in degrees
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetGravity(&gravity, &q);
                mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
                if(log_enabled) Serial.print("ypr\t");
                if(log_enabled) Serial.print(ypr[0] * 180/M_PI);
                if(log_enabled) Serial.print("\t");
                if(log_enabled) Serial.print(ypr[1] * 180/M_PI);
                if(log_enabled) Serial.print("\t");
                Serial.println(ypr[2] * 180/M_PI);
            #endif

            #ifdef OUTPUT_READABLE_REALACCEL
                // display real acceleration, adjusted to remove gravity
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetAccel(&aa, fifoBuffer);
                mpu.dmpGetGravity(&gravity, &q);
                mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
                if(log_enabled) Serial.print("areal\t");
                if(log_enabled) Serial.print(aaReal.x);
                if(log_enabled) Serial.print("\t");
                if(log_enabled) Serial.print(aaReal.y);
                if(log_enabled) Serial.print("\t");
                Serial.println(aaReal.z);
            #endif

            #ifdef OUTPUT_READABLE_WORLDACCEL
                // display initial world-frame acceleration, adjusted to remove gravity
                // and rotated based on known orientation from quaternion
                mpu.dmpGetQuaternion(&q, fifoBuffer);
                mpu.dmpGetAccel(&aa, fifoBuffer);
                mpu.dmpGetGravity(&gravity, &q);
                mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
                mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
                if(log_enabled) Serial.print("aworld\t");
                if(log_enabled) Serial.print(aaWorld.x);
                if(log_enabled) Serial.print("\t");
                if(log_enabled) Serial.print(aaWorld.y);
                if(log_enabled) Serial.print("\t");
                Serial.println(aaWorld.z);
            #endif
        
            #ifdef OUTPUT_TEAPOT
                // display quaternion values in InvenSense Teapot demo format:
                teapotPacket[2] = fifoBuffer[0];
                teapotPacket[3] = fifoBuffer[1];
                teapotPacket[4] = fifoBuffer[4];
                teapotPacket[5] = fifoBuffer[5];
                teapotPacket[6] = fifoBuffer[8];
                teapotPacket[7] = fifoBuffer[9];
                teapotPacket[8] = fifoBuffer[12];
                teapotPacket[9] = fifoBuffer[13];
                Serial.write(teapotPacket, 14);
                teapotPacket[11]++; // packetCount, loops at 0xFF on purpose
            #endif

            // blink LED to indicate activity
            //blinkState = !blinkState;
            //digitalWrite(LED_PIN, blinkState);
        }
    }    
}


//WARNING YPR Different from demo, adjusted to board

void imu_run()
{
    if (!imu_initialized)
    {
        if(log_enabled) Serial.print("INITIALIZING IMU...");
        imu_init();
        wait(500);
    }

    if(!imu_initialized) //Not Necessary , just doorkeeper
    {
        if(log_enabled) Serial.print("\nERROR : IMU Not INIT OR FLAG ILLEGALLY MODIFIED\n");
        return;
    }

    //the imu_mode will trigger different outputs for different purposes

    // if programming failed, don't try to do anything
    if (!dmpReady) 
    {
        if(log_enabled) Serial.print("\n---DMP Not Ready!----\n");
        return;
    }
    // read a packet from FIFO
        
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) 
    { 
        // Get the Latest packet 
        
        if(imu_run_mode==imu_run_mode_print_values) //Printing Values
        {
            //if(log_enabled) Serial.print("\n---EULER ANGLES!----\n");
            // display Euler angles in degrees
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);
            if(log_enabled) Serial.print("ypr\t");
            if(log_enabled) Serial.print(ypr[0] * 180/M_PI);
            if(log_enabled) Serial.print("\t");
            if(log_enabled) Serial.print(ypr[1] * 180/M_PI);
            if(log_enabled) Serial.print("\t");
            Serial.println(ypr[2] * 180/M_PI);
        }
            
        else if(imu_run_mode == imu_run_mode_simulation) //Simulation Mode
        {
            // display quaternion values in InvenSense Teapot demo format:
            teapotPacket[2] = fifoBuffer[0];
            teapotPacket[3] = fifoBuffer[1];
            teapotPacket[4] = fifoBuffer[4];
            teapotPacket[5] = fifoBuffer[5];
            teapotPacket[6] = fifoBuffer[8];
            teapotPacket[7] = fifoBuffer[9];
            teapotPacket[8] = fifoBuffer[12];
            teapotPacket[9] = fifoBuffer[13];
            Serial.write(teapotPacket, 14);
            teapotPacket[11]++; // packetCount, loops at 0xFF on purposevoid rgb_leds_follow_imu_pos_y(int brightness,int max_val)
        }

        if(imu_run_mode==imu_run_mode_silent) //Quiet Mode and Refreshing Flag and data
        {
            //This is for DEMO POS
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);

            //For DEMO_IMU_POS
            mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

            mpu.dmpGetAccel(&aa, fifoBuffer);

            //TODO TEST WHICH ACCEL IS BETTER FOR OUR CASE
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
            mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
                                        
            //For DEMO_IMU_POS   
            //THIS IS DIFFERENT FROM DEMO, ADJUSTED TO BOARD!
            imu_yaw = (ypr[0] * 180/M_PI);
            imu_roll = (ypr[1] * 180/M_PI);//This was swapped for Down
            imu_pitch =(ypr[2] * 180/M_PI);//This was swapped for UP

            imu_acc_x = aaReal.x;
            imu_acc_y = aaReal.y;
            imu_acc_z = aaReal.z;

            imu_acc_comp_grav_x = aaWorld.x;
            imu_acc_comp_grav_y = aaWorld.y;
            imu_acc_comp_grav_z = aaWorld.z;

            imu_new_data = true;
        }

        //TODO Later implement demos on Acceleration with this

        #ifdef OUTPUT_READABLE_REALACCEL
            // display real acceleration, adjusted to remove gravity
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
            if(log_enabled) Serial.print("areal\t");
            if(log_enabled) Serial.print(aaReal.x);
            if(log_enabled) Serial.print("\t");
            if(log_enabled) Serial.print(aaReal.y);
            if(log_enabled) Serial.print("\t");
            Serial.println(aaReal.z);
        #endif

        #ifdef OUTPUT_READABLE_WORLDACCEL
            // display initial world-frame acceleration, adjusted to remove gravity
            // and rotated based on known orientation from quaternion
            mpu.dmpGetQuaternion(&q, fifoBuffer);
            mpu.dmpGetAccel(&aa, fifoBuffer);
            mpu.dmpGetGravity(&gravity, &q);
            mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
            mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);
            if(log_enabled) Serial.print("aworld\t");
            if(log_enabled) Serial.print(aaWorld.x);
            if(log_enabled) Serial.print("\t");
            if(log_enabled) Serial.print(aaWorld.y);
            if(log_enabled) Serial.print("\t");
            Serial.println(aaWorld.z);
        #endif
    
        // blink LED to indicate activity
        //blinkState = !blinkState;
        //digitalWrite(LED_PIN, blinkState);
    } 
    else
    {
        //wait(10);
        //IS RUNNING OK BUT NO NEW DATA AVAILABLE  
        //if(log_enabled) Serial.print("\nERROR\n");  
    }
}


//As a little Workaround I'll run the teapot demo as a function 
//so I can stop it and pass to graph inmmediately and without killing tasks or changing the imu_mode
//Processing must be waiting for ttyUSB0 already and with toxin libs 
//(Attached to this repo under /test/imu-test-files/teapot)
//https://maker.pro/arduino/tutorial/how-to-interface-arduino-and-the-mpu-6050-sensor


void imu_graph_demo()
{

    if (!imu_initialized)
    {
        if(log_enabled) Serial.print("INITIALIZING IMU...");
        imu_init();
        wait(500);
    }

    if(!imu_initialized) //Not Necessary , just doorkeeper
    {
        if(log_enabled) Serial.print("\nERROR : IMU Not INIT OR FLAG ILLEGALLY MODIFIED\n");
        return;
    }

    //the imu_mode will trigger different outputs for different purposes

    // if programming failed, don't try to do anything
    if (!dmpReady) 
    {
        if(log_enabled) Serial.print("\n---DMP Not Ready!----\n");
        return;
    }
    
    // read a packet from FIFO
    
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) 
    { 
        
        //GET DATA

        //if(log_enabled) Serial.print("\n---EULER ANGLES!----\n");
        // display Euler angles in degrees
        mpu.dmpGetQuaternion(&q, fifoBuffer);
        mpu.dmpGetGravity(&gravity, &q);
        mpu.dmpGetYawPitchRoll(ypr, &q, &gravity);

        // display initial world-frame acceleration, adjusted to remove gravity
        // and rotated based on known orientation from quaternion
        mpu.dmpGetAccel(&aa, fifoBuffer);
        mpu.dmpGetLinearAccel(&aaReal, &aa, &gravity);
        mpu.dmpGetLinearAccelInWorld(&aaWorld, &aaReal, &q);

        
        //LOGGING
        
        //GYROSCOPE

        //Scale Delimiter
        Serial.print(gyro_upper_threshold);
        Serial.print("\t");
        
        //IMU_GYRO VARS

        Serial.print(ypr[2] * 180/M_PI); //X
        Serial.print("\t");
        
        Serial.print(ypr[1] * 180/M_PI); //Y
        Serial.print("\t");
        
        Serial.print(ypr[0] * 180/M_PI); //Z
        Serial.print("\t");
        
        //Scale Delimiter
        Serial.print(gyro_lower_threshold);
        //Serial.println();
        Serial.print("\t");

        //ACCELERATION

        //Scale Delimiter
        Serial.print(acc_upper_threshold);
        Serial.print("\t");
        //IMU_ACC VARS
        Serial.print(aaWorld.x); //X
        Serial.print("\t");
        Serial.print(aaWorld.y); //Y
        Serial.print("\t");
        Serial.print(aaWorld.z); //Z
        Serial.print("\t");
        //Scale Delimiter
        Serial.print(acc_lower_threshold);
        Serial.println();

        // blink LED to indicate activity
        blinkState = !blinkState;
        digitalWrite(LED_PIN, blinkState);

    } 

    else
    {
        //wait(10);
        //IS RUNNING OK BUT NO NEW DATA AVAILABLE  
        //if(log_enabled) Serial.print("\nERROR\n");  
    }
    
}


void imu_teapot_demo()
{

    if (!imu_initialized)
    {
        if(log_enabled) Serial.print("INITIALIZING IMU...");
        imu_init();
        wait(500);
    }

    if(!imu_initialized) //Not Necessary , just doorkeeper
    {
        if(log_enabled) Serial.print("\nERROR : IMU Not INIT OR FLAG ILLEGALLY MODIFIED\n");
        return;
    }

    //the imu_mode will trigger different outputs for different purposes

    // if programming failed, don't try to do anything
    if (!dmpReady) 
    {
        if(log_enabled) Serial.print("\n---DMP Not Ready!----\n");
        return;
    }
    // read a packet from FIFO
    
    if (mpu.dmpGetCurrentFIFOPacket(fifoBuffer)) 
    { 
        // Get the Latest packet 
                    
        // display quaternion values in InvenSense Teapot demo format:
        teapotPacket[2] = fifoBuffer[0];
        teapotPacket[3] = fifoBuffer[1];
        teapotPacket[4] = fifoBuffer[4];
        teapotPacket[5] = fifoBuffer[5];
        teapotPacket[6] = fifoBuffer[8];
        teapotPacket[7] = fifoBuffer[9];
        teapotPacket[8] = fifoBuffer[12];
        teapotPacket[9] = fifoBuffer[13];
        Serial.write(teapotPacket, 14);
        teapotPacket[11]++;

        // blink LED to indicate activity
        blinkState = !blinkState;
        digitalWrite(LED_PIN, blinkState);
    } 
    else
    {
        //wait(10);
        //IS RUNNING OK BUT NO NEW DATA AVAILABLE  
        //if(log_enabled) Serial.print("\nERROR\n");  
    }
    
}



//----------- MAIN IMU_TASK

//current_status will indicate if the bike is on riding or parking mode 

//IN PARKING MODE the imu is off to save battery 

//In RIDING MODE we will make a silent task that will be running and handles 
//the imu_status based on the following combinations:

//Two parameters : one for acc and other for gyro

// imu_acc_static
// imu_acc_moving
// imu_acc_crash
// imu_acc_fall

// imu_gyro_riding
// imu_gyro_falling
// imu_gyro_upside_down

//The combination of both variables will cascade into the imu_status cases:

// imu_status_idle     <- riding position but not moving
// imu_status_riding    

// imu_status_abnormal_pos_static <- the acc was not enough to detect a fall (e.g. letting it on the ground or repairing upside-down) 
// imu_status_abnormal_pos_moving <- e.g. transporting the bike upside down or in weird position  

// imu_status_fall_detected                 
// imu_status_crash_detected

// imu_status_accident_not_responding

//The imu will be triggered by the moving flag on the 

//task_imu


//IMU Task---------------------------------------------------------------------------------------------------- 
//Main Task to keep IMU alive

//It can hold an I2C token to reserve the full rail to read faster in cases of detected risk
//TODO Ideally later will have its separate i2c rail by software or hw will see 

TaskHandle_t task_imu_handle = NULL;

void create_task_imu() //once created it will automatically run
{
    if(log_enabled) Serial.print("\n--creating task_imu--");
    wait(100);
    
    task_imu_i2c_declare();
    wait(100);

    xTaskCreate
    (
        task_imu,           //Function Name (must be a while(1))
        "task_imu", //Logging Name
        4096,                //Stack Size
        NULL,                //Passing Parameters
        5,                   //Task Priority
        &task_imu_handle
    );   

    task_imu_status = task_imu_running;

    if(log_enabled) Serial.print("-- done --\n");
}

void task_imu_i2c_declare()
{
    if(log_enabled)Serial.print("\ntask_imu_i2c_declared\n");
    wait(100);
    //This Taks will use the following I2C_Devs
    
    imu_needed++;
    rgb_needed++;
    //temp_needed++;
    //lux_needed++;
    //rtc_needed++;
    //fuelgauge_needed++;
    oled_needed++;
}

void task_imu_i2c_release()
{
    if(log_enabled)Serial.print("\ntask_imu_i2c_released\n");
    wait(100);
    
    //This Taks will release the following I2C_Devs
    
    imu_needed--;
    rgb_needed--;
    //temp_needed--;
    //lux_needed--;
    //rtc_needed--;
    //fuelgauge_needed--;
    oled_needed--;
}

void task_imu(void * parameters)
{   

    if(log_enabled)Serial.println("Running imu_task ");
    wait(100);  
    
    //To check the amount of time on emergency

    int emergency_threshold = 5000;
    unsigned long counter = 0;

    task_imu_status = task_imu_running;

    
    rgb_leds_on('r',riding_mode_led_default_brightness);


    while(1)
    {
        if(task_imu_status == task_imu_not_needed) //Destroying Task upon request 
        {

            if(log_enabled && imu_task_log_mode > imu_task_log_mode_just_critical)
            {
                Serial.print(" ----- Destroying task_imu ----\n ");   
            }
            wait(100);

            task_imu_i2c_release();
            wait(100);

            //wipe leds ready for next order
            rgb_leds_off();

            vTaskDelete(task_imu_handle);//Delete itself
        }
      
        else //IMU RUNNING NORMALLY
        {            
            if(log_enabled && imu_task_log_mode > imu_task_log_mode_just_critical)
            {
                Serial.print("\n--- TASK_IMU MAIN_CORE STARTED ---\n");
            }

            int imu_accident_counter = 0;
            int imu_accident_dismiss_counter = 0;
            
            //Here main core
            while(task_imu_status != task_imu_not_needed)
            {

                //wait if there is backend activity
                //Firebase looks like is running good in parallel

                /*
                if(firestore_update_in_progress)
                {
                    if(log_enabled)Serial.print("\n----IMU waiting for Firestore Token as well---");
                    while(firestore_update_in_progress)
                    {
                        wait(100);
                        //Wait until the firestore task finish updating
                    } 
                    if(log_enabled)Serial.print("\n---Resuming IMU---");
                }
                */

                //Running the rgb_leds_demo_imu_pos for roll (y) will return true if there is an accident suspected
                if(rgb_leds_follow_imu_pos_y(riding_mode_led_default_brightness,riding_mode_normal_position_threshold_angle,imu_log_mode_moderate))
                {
                    if(imu_accident_dismiss_counter > 0 ) imu_accident_dismiss_counter = 0;

                    if(imu_accident_counter < imu_accident_confirmed_threshold) 
                    {
                        imu_accident_counter++;    

                        if(log_enabled && imu_task_log_mode > imu_log_mode_silent )
                        {
                            Serial.printf("\n IMU_ACCIDENT_COUNTER : %d ", imu_accident_counter);
                        }
                    }

                    if(imu_accident_counter == imu_accident_detected_threshold)
                    {
                        if(log_enabled)Serial.print("\n\n---Accident Detected");
                        

                        buzzer_one_tone(2730,1000,0,1);

                        if(oled_enabled)
                        {
                            oled_token = oled_taken;
                            oled_imu_message_accident_detected();
                            wait(2000);
                            oled_token = oled_free;
                        }

                        accident_detected = true;

                    }

                    else if (imu_accident_counter == imu_accident_confirmed_threshold)
                    {
                        if(log_enabled)Serial.print("\n\n---Accident Confirmed");

                        
                        buzzer_one_tone(2730,500,100,3);       

                        if(oled_enabled)
                        {
                            oled_token = oled_taken;
                            oled_imu_message_accident_confirmed();
                            wait(2000);
                            oled_token = oled_free;

                        }                 

                        accident_confirmed = true;

                        //if the accident was confirmed apart from putting the bike back we will press the button

                        imu_accident_counter++;
                    }

                    //After passing the confirmation we will stop counting and wait with the imu
                    //TODO maybe later deactivate?

                }
                else //normal position
                {

                    //Clear the accumulator    
                    if(imu_accident_counter > 1 && !accident_detected) imu_accident_counter = 0;

                    if( imu_accident_counter > 1 && accident_detected)
                    {
                        //returning to correct position for at least 10 cycles
                        imu_accident_dismiss_counter++;

                        if(log_enabled && imu_task_log_mode > imu_log_mode_silent)
                        {
                            Serial.printf("\n DISMISSING ACCIDENT IN : %d " , imu_accident_dismiss_threshold - imu_accident_dismiss_counter );
                        }
                        
                        if(imu_accident_dismiss_counter > imu_accident_dismiss_threshold )
                        {
                            if(!accident_confirmed) //If the accident was just detected can be immediately dismissed
                            {
                                if(log_enabled)Serial.print("\n\n------CRASH DETECTION MODE DISMISSED BY POSITION---------");

                                buzzer_ok();

                                rgb_leds_blink_once('g',255,100);
                                                                
                                accident_detected = false;
                                imu_accident_counter = 0;

                                accident_dismissed = true;                                 
                            }
                            //If the accident is confirmed then BTN 1 needs to be pressed to dismiss
                            else if(accident_confirmed)
                            {
                                if(log_enabled)Serial.print("\n\n------POSITION SET TO NORMAL---------");
                                if(log_enabled)Serial.print("\n\n------PRESS BTN_1 TO DISMISS---------");

                                buzzer_one_tone(800,100,0,1);
                                if(oled_enabled)
                                {
                                    oled_token = oled_taken;
                                    oled_imu_message_press_btn1_to_dismiss();
                                    wait(1000);
                                    oled_token = oled_free;
                                } 

                                //While we are still in a normal position we will offer the option to dismiss with the button
                                while(!rgb_leds_follow_imu_pos_y(riding_mode_led_default_brightness,
                                                                 riding_mode_normal_position_threshold_angle,
                                                                 imu_log_mode_moderate)
                                                                 && !accident_dismissed)
                                {
                                    wait(100);

                                    if(digitalRead(esp_btn_1_pin))
                                    {
                                        if(log_enabled)Serial.print("\n\n------BTN_1 PRESSED---------");
                                        
                                        buzzer_ok();

                                        rgb_leds_blink_once('g',255,100);

                                        accident_dismissed = true; 
                                        if(log_enabled)Serial.print("\n------WAITING FOR BTN_1 RELEASE------");
                                        while(digitalRead(esp_btn_1_pin))
                                        {
                                            wait(100);
                                        }
                                        if(log_enabled)Serial.print("\n------BTN_1 RELEASED------");
                                        break;
                                    }
                                }
                            }  
                            //Flag triggered Locally or Remotely
                            if(accident_dismissed)
                            {
                                if(log_enabled)Serial.print("\n\n------CRASH DETECTION MODE DISMISSED ---------");    

                                if(oled_enabled)
                                {
                                    oled_token = oled_taken;
                                    oled_imu_message_accident_dismissed();
                                    wait(1000);
                                    oled_token = oled_free;
                                } 


                                accident_detected = false;
                                accident_confirmed = false;
                                imu_accident_counter=0;
                                
                                imu_accident_dismiss_counter = 0;

                                //TODO later turn off this flag also after waiting for some time
                                task_imu_status = task_imu_not_needed;//turn off the imu and reset all flags
                                //From here we are Back to Riding mode but waiting to move
                            }                                                    
                        }            
                    }                                      
                }

                //else do nothing if riding normally
                wait(100);
            }
        }
    }  
}


void recalibrate_imu_via_i2c_manager(int imu_recalibrate_log_handler_mode)
{
    if(imu_recalibrate_log_handler_mode > imu_task_log_mode_silent)
    {
        if(log_enabled)Serial.print("\n--Recalibrating IMU via I2C Manager--\n");
    }
    
    imu_initialized = false;

    while(!imu_initialized)
    {
        wait(100);
    }

    if(imu_recalibrate_log_handler_mode > imu_task_log_mode_silent)
    {
        if(log_enabled)Serial.print("\n ... IMU Recalibrated via I2C Manager-- \n");
    }    
    
}



 