#include <webots/Robot.hpp>
#include <webots/Accelerometer.hpp>
#include <webots/Camera.hpp>
#include <webots/Gyro.hpp>
#include <webots/LED.hpp>
#include <webots/Motor.hpp>
#include <webots/Device.hpp>
#include <webots/Speaker.hpp>
#include "LinuxDARwIn.h"
#include "keyboard.hpp"

#include <unistd.h>
#include <libgen.h>


webots::Robot::Robot() {
  initDarwinOP();
  initDevices();
  gettimeofday(&mStart, NULL);
  mPreviousStepTime = 0.0;
  mKeyboardEnable = false;
  mKeyboard = new Keyboard();

  // Load TimeStep from the file "config.ini"
  minIni ini("config.ini");
  LoadINISettings(&ini, "Robot Config");
  if(mTimeStep < 16)
    printf("The time step selected of %dms is very small and will probably not be respected.\n A time step of at least 16ms is recommended.\n", mTimeStep);
    
  mCM730->MakeBulkReadPacketWb(); // Create the BulkReadPacket to read the actuators states in Robot::step
  
  // Unactive all Joints in the Motion Manager //
  std::map<const std::string, int>::iterator motor_it;
  for(motor_it = Motor::mNamesToIDs.begin() ; motor_it != Motor::mNamesToIDs.end(); motor_it++ ) {
    ::Robot::MotionStatus::m_CurrentJoints.SetEnable((*motor_it).second, 0);
    ::Robot::MotionStatus::m_CurrentJoints.SetValue((*motor_it).second, ((Motor *) mDevices[(*motor_it).first])->getGoalPosition());
  }


  // Make each motors go to the start position slowly
  const int msgLength = 5; // id + Goal Position (L + H) + Moving speed (L + H)
  int value=0, changed_motors=0, n=0;
  int param[20*msgLength];
  
  for(motor_it = Motor::mNamesToIDs.begin() ; motor_it != Motor::mNamesToIDs.end(); motor_it++ ) {
    Motor *motor = static_cast <Motor *> (mDevices[(*motor_it).first]);
    int motorId = (*motor_it).second;
    if(motor->getTorqueEnable() && !(::Robot::MotionStatus::m_CurrentJoints.GetEnable(motorId))) {
      param[n++] = motorId;  // id
      value = motor->getGoalPosition(); // Start position
      param[n++] = ::Robot::CM730::GetLowByte(value);
      param[n++] = ::Robot::CM730::GetHighByte(value);
      value = 100; // small speed 100 => 11.4 rpm => 1.2 rad/s
      param[n++] = ::Robot::CM730::GetLowByte(value);
      param[n++] = ::Robot::CM730::GetHighByte(value);
      changed_motors++;
    }
  }
  mCM730->SyncWrite(::Robot::MX28::P_GOAL_POSITION_L, msgLength, changed_motors , param);
  usleep(2000000); // wait a moment to reach start position

  // Switch LED to GREEN
  mCM730->WriteWord(::Robot::CM730::ID_CM, ::Robot::CM730::P_LED_HEAD_L, 1984, 0);
  mCM730->WriteWord(::Robot::CM730::ID_CM, ::Robot::CM730::P_LED_EYE_L, 1984, 0);
}

webots::Robot::~Robot() {
}

int webots::Robot::step(int ms) {
  double actualTime = getTime() * 1000;
  int stepDuration = actualTime - mPreviousStepTime;
  std::map<const std::string, int>::iterator motor_it;
  
  // -------- Update speed of each motors, according to acceleration limit if set --------  //
  for(motor_it = Motor::mNamesToIDs.begin() ; motor_it != Motor::mNamesToIDs.end(); motor_it++  ) {
    Motor *motor = static_cast <Motor *> (mDevices[(*motor_it).first]);
    motor->updateSpeed(stepDuration);
  }
  
  // -------- Bulk Read to read the actuators states (position, speed and load) and body sensors -------- //
  if(!(::Robot::MotionManager::GetInstance()->GetEnable())) // If MotionManager is enable, no need to execute the BulkRead, the MotionManager has allready done it.
    mCM730->BulkRead();

  // Motors
  for(motor_it = Motor::mNamesToIDs.begin() ; motor_it != Motor::mNamesToIDs.end(); motor_it++) {
    Motor *motor = static_cast <Motor *> (mDevices[(*motor_it).first]);
    int motorId = (*motor_it).second;
    motor->setPresentPosition( mCM730->m_BulkReadData[motorId].ReadWord(::Robot::MX28::P_PRESENT_POSITION_L));
    motor->setPresentSpeed( mCM730->m_BulkReadData[motorId].ReadWord(::Robot::MX28::P_PRESENT_SPEED_L));
    motor->setPresentLoad( mCM730->m_BulkReadData[motorId].ReadWord(::Robot::MX28::P_PRESENT_LOAD_L));

    int limit = mCM730->m_BulkReadData[motorId].ReadWord(::Robot::MX28::P_TORQUE_LIMIT_L);
    if (limit == 0) {
      fprintf(stderr, "Alarm detected on id = %d\n", motorId);
      exit(EXIT_FAILURE);
    }
  }
  
  int values[3];

  // Gyro
  values[0] = mCM730->m_BulkReadData[::Robot::CM730::ID_CM].ReadWord(::Robot::CM730::P_GYRO_X_L);
  values[1] = mCM730->m_BulkReadData[::Robot::CM730::ID_CM].ReadWord(::Robot::CM730::P_GYRO_Y_L);
  values[2] = mCM730->m_BulkReadData[::Robot::CM730::ID_CM].ReadWord(::Robot::CM730::P_GYRO_Z_L);
  ((Gyro *)mDevices["Gyro"])->setValues(values);
  
  // Accelerometer
  values[0] = mCM730->m_BulkReadData[::Robot::CM730::ID_CM].ReadWord(::Robot::CM730::P_ACCEL_X_L);
  values[1] = mCM730->m_BulkReadData[::Robot::CM730::ID_CM].ReadWord(::Robot::CM730::P_ACCEL_Y_L);
  values[2] = mCM730->m_BulkReadData[::Robot::CM730::ID_CM].ReadWord(::Robot::CM730::P_ACCEL_Z_L);
  ((Accelerometer *)mDevices["Accelerometer"])->setValues(values);
  // Led states
  values[0] = mCM730->m_BulkReadData[::Robot::CM730::ID_CM].ReadWord(::Robot::CM730::P_LED_HEAD_L);
  values[1] = mCM730->m_BulkReadData[::Robot::CM730::ID_CM].ReadWord(::Robot::CM730::P_LED_EYE_L);
  values[2] = mCM730->m_BulkReadData[::Robot::CM730::ID_CM].ReadByte(::Robot::CM730::P_LED_PANNEL);
  ((LED *)mDevices["HeadLed"])->setColor(values[0]);
  ((LED *)mDevices["EyeLed"])->setColor(values[1]);
  LED::setBackPanel(values[2]);

  // -------- Sync Write to actuators --------  //
  const int msgLength = 9; // id + P + Empty + Goal Position (L + H) + Moving speed (L + H) + Torque Limit (L + H)
  
  int param[20*msgLength];
  int n=0;
  int changed_motors=0;
  int value;
  
  for(motor_it = Motor::mNamesToIDs.begin() ; motor_it != Motor::mNamesToIDs.end(); motor_it++ ) {
    Motor *motor = static_cast <Motor *> (mDevices[(*motor_it).first]);
    int motorId = (*motor_it).second;
    if(motor->getTorqueEnable() && !(::Robot::MotionStatus::m_CurrentJoints.GetEnable(motorId))) {
      param[n++] = motorId;
      param[n++] = motor->getPGain();
      param[n++] = 0; // Empty
      value = motor->getGoalPosition();
      param[n++] = ::Robot::CM730::GetLowByte(value);
      param[n++] = ::Robot::CM730::GetHighByte(value);
      value = motor->getMovingSpeed();
      param[n++] = ::Robot::CM730::GetLowByte(value);
      param[n++] = ::Robot::CM730::GetHighByte(value);
      value = motor->getTorqueLimit();
      param[n++] = ::Robot::CM730::GetLowByte(value);
      param[n++] = ::Robot::CM730::GetHighByte(value);
      changed_motors++;
    }
  }
  mCM730->SyncWrite(::Robot::MX28::P_P_GAIN, msgLength, changed_motors , param);

  // -------- Keyboard Reset ----------- //
  if(mKeyboardEnable == true)
    mKeyboard->resetKeyPressed();

  // -------- Timing management -------- //
  if(stepDuration < ms) { // Step to short -> wait remaining time
    usleep((ms - stepDuration) * 1000);
    mPreviousStepTime = actualTime;
    return 0;
  }
  else { // Step to long -> return step duration
    mPreviousStepTime = actualTime;
    return stepDuration;
  }
}

std::string webots::Robot::getName() const {
  return "darwin-op";
}

double webots::Robot::getTime() const {
  struct timeval end;
  
  gettimeofday(&end, NULL);

  long seconds  = end.tv_sec  - mStart.tv_sec;
  long useconds = end.tv_usec - mStart.tv_usec;
  long mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;

  return (double) mtime/1000.0;
}

int webots::Robot::getMode() const {
  return 1;
}

double webots::Robot::getBasicTimeStep() const {
  return mTimeStep;
}

webots::Device *webots::Robot::getDevice(const std::string &name) const {
  std::map<const std::string, webots::Device *>::const_iterator it = mDevices.find(name);
  if (it != mDevices.end())
    return (*it).second;
  return NULL;

}

webots::Accelerometer *webots::Robot::getAccelerometer(const std::string &name) const {
  webots::Device *device = getDevice(name);
  if (device) {
    webots::Accelerometer *accelerometer = dynamic_cast<webots::Accelerometer *> (device);
    if (accelerometer)
      return accelerometer;
  }
  return NULL;
}

webots::Camera *webots::Robot::getCamera(const std::string &name) const {
  webots::Device *device = getDevice(name);
  if (device) {
    webots::Camera *camera = dynamic_cast<webots::Camera *> (device);
    if (camera)
      return camera;
  }
  return NULL;
}

webots::Gyro *webots::Robot::getGyro(const std::string &name) const {
  webots::Device *device = getDevice(name);
  if (device) {
    webots::Gyro *gyro = dynamic_cast<webots::Gyro *> (device);
    if (gyro)
      return gyro;
  }
  return NULL;
}

webots::Motor *webots::Robot::getMotor(const std::string &name) const {
  webots::Device *device = getDevice(name);
  if (device) {
    webots::Motor *motor = dynamic_cast<webots::Motor *> (device);
    if (motor)
      return motor;
  }
  return NULL;
}

webots::LED *webots::Robot::getLED(const std::string &name) const {
  webots::Device *device = getDevice(name);
  if (device) {
    webots::LED *led = dynamic_cast<webots::LED *> (device);
    if (led)
      return led;
  }
  return NULL;
}

webots::Speaker *webots::Robot::getSpeaker(const std::string &name) const {
  webots::Device *device = getDevice(name);
  if (device) {
    webots::Speaker *speaker = dynamic_cast<webots::Speaker *> (device);
    if (speaker)
      return speaker;
  }
  return NULL;
}

void webots::Robot::initDevices() {
  mDevices["Accelerometer"] = new webots::Accelerometer("Accelerometer", this);
  mDevices["Camera"]        = new webots::Camera       ("Camera",        this);
  mDevices["Gyro"]          = new webots::Gyro         ("Gyro",          this);
  mDevices["EyeLed"]        = new webots::LED          ("EyeLed",        this);
  mDevices["HeadLed"]       = new webots::LED          ("HeadLed",       this);
  mDevices["BackLedRed"]    = new webots::LED          ("BackLedRed",    this);
  mDevices["BackLedGreen"]  = new webots::LED          ("BackLedGreen",  this);
  mDevices["BackLedBlue"]   = new webots::LED          ("BackLedBlue",   this);
  mDevices["ShoulderR"]     = new webots::Motor        ("ShoulderR",     this);
  mDevices["ShoulderL"]     = new webots::Motor        ("ShoulderL",     this);
  mDevices["ArmUpperR"]     = new webots::Motor        ("ArmUpperR",     this);
  mDevices["ArmUpperL"]     = new webots::Motor        ("ArmUpperL",     this);
  mDevices["ArmLowerR"]     = new webots::Motor        ("ArmLowerR",     this);
  mDevices["ArmLowerL"]     = new webots::Motor        ("ArmLowerL",     this);
  mDevices["PelvYR"]        = new webots::Motor        ("PelvYR",        this);
  mDevices["PelvYL"]        = new webots::Motor        ("PelvYL",        this);
  mDevices["PelvR"]         = new webots::Motor        ("PelvR",         this);
  mDevices["PelvL"]         = new webots::Motor        ("PelvL",         this);
  mDevices["LegUpperR"]     = new webots::Motor        ("LegUpperR",     this);
  mDevices["LegUpperL"]     = new webots::Motor        ("LegUpperL",     this);
  mDevices["LegLowerR"]     = new webots::Motor        ("LegLowerR",     this);
  mDevices["LegLowerL"]     = new webots::Motor        ("LegLowerL",     this);
  mDevices["AnkleR"]        = new webots::Motor        ("AnkleR",        this);
  mDevices["AnkleL"]        = new webots::Motor        ("AnkleL",        this);
  mDevices["FootR"]         = new webots::Motor        ("FootR",         this);
  mDevices["FootL"]         = new webots::Motor        ("FootL",         this);
  mDevices["Neck"]          = new webots::Motor        ("Neck",          this);
  mDevices["Head"]          = new webots::Motor        ("Head",          this);
  mDevices["Speaker"]       = new webots::Speaker      ("Speaker",       this);
}

void webots::Robot::initDarwinOP() {
  char exepath[1024] = {0};
  if(readlink("/proc/self/exe", exepath, sizeof(exepath)) != -1)  {
      if(chdir(dirname(exepath)))
          fprintf(stderr, "chdir error!! \n");
  }
  
  mLinuxCM730 = new ::Robot::LinuxCM730("/dev/ttyUSB0");
  mCM730 = new ::Robot::CM730(mLinuxCM730);
  
  if(mCM730->Connect() == false) {
    printf("Fail to connect CM-730!\n");
    exit(EXIT_FAILURE);
  }
  
  // Read firmware version
  int firm_ver = 0;
  if(mCM730->ReadByte(::Robot::JointData::ID_HEAD_PAN, ::Robot::MX28::P_VERSION, &firm_ver, 0)  != ::Robot::CM730::SUCCESS) {
    fprintf(stderr, "Can't read firmware version from Dynamixel ID %d!\n", ::Robot::JointData::ID_HEAD_PAN);
    exit(EXIT_FAILURE);
  }
  
  if(firm_ver < 27) {
    fprintf(stderr, "Firmware version of Dynamixel is too old, please update them.\nMore information available in the User guide.\n");
    exit(EXIT_FAILURE);
  }

  ::Robot::MotionManager::GetInstance()->Initialize(mCM730);

  // Switch LED to RED
  mCM730->WriteWord(::Robot::CM730::ID_CM, ::Robot::CM730::P_LED_HEAD_L, 63, 0);
  mCM730->WriteWord(::Robot::CM730::ID_CM, ::Robot::CM730::P_LED_EYE_L, 63, 0);
}

void webots::Robot::LoadINISettings(minIni* ini, const std::string &section) {
  double value = INVALID_VALUE;

  if((value = ini->getd(section, "time_step", INVALID_VALUE)) != INVALID_VALUE)
    mTimeStep = value;
  else
    printf("Can't read time step from 'config.ini'\n");

  if((value = ini->getd(section, "camera_width", INVALID_VALUE)) != INVALID_VALUE)
    ::Robot::Camera::WIDTH = value;
  else
    printf("Can't read camera width from 'config.ini'\n");

  if((value = ini->getd(section, "camera_height", INVALID_VALUE)) != INVALID_VALUE)
    ::Robot::Camera::HEIGHT = value;
  else
    printf("Can't read camera height from 'config.ini'\n");

  if(!(::webots::Camera::checkResolution(::Robot::Camera::WIDTH, ::Robot::Camera::HEIGHT))) {
    printf("The resolution of %dx%d selected is not supported by the camera.\nPlease use one of the resolution recommended.\n", ::Robot::Camera::WIDTH, ::Robot::Camera::HEIGHT);
    ::Robot::Camera::WIDTH = 320;
    ::Robot::Camera::HEIGHT = 240;
    printf("WARNING : Camera resolution reseted to 320x240 pixels.");
  }
}

void webots::Robot::keyboardEnable(int ms) {
  if(mKeyboardEnable == false) {
    // Starting keyboard listenning in a thread 
    int error = 0;
  
    mKeyboard->createWindow();

    // create and start the thread
    if((error = pthread_create(&this->mKeyboardThread, NULL, this->KeyboardTimerProc, this))!= 0) {
      printf("Keyboard thread error = %d\n",error);
      exit(-1);
    }
  }
  
  mKeyboardEnable = true;
}

void *::webots::Robot::KeyboardTimerProc(void *param) {
  ((Robot*)param)->mKeyboard->startListenKeyboard();
  return NULL;
}

void webots::Robot::keyboardDisable() {
  if(mKeyboardEnable == true) {
    int error=0;
    // End the thread
    if((error = pthread_cancel(this->mKeyboardThread))!= 0) {
      printf("Keyboard thread error = %d\n",error);
      exit(-1);
    }
  mKeyboard->closeWindow();
  mKeyboard->resetKeyPressed();
  }

  mKeyboardEnable = false;
}

int webots::Robot::keyboardGetKey() {
  if(mKeyboardEnable == true) {
    return mKeyboard->getKeyPressed();
  }
  else
    return 0;
}
