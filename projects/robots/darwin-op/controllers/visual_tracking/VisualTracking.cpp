#include "VisualTracking.hpp"
#include <webots/Motor.hpp>
#include <webots/LED.hpp>
#include <webots/Camera.hpp>
#include <DARwInOPVisionManager.hpp>

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <fstream>

using namespace webots;
using namespace managers;
using namespace std;

static const char *motorNames[NMOTORS] = {
  "ShoulderR" /*ID1 */, "ShoulderL" /*ID2 */, "ArmUpperR" /*ID3 */, "ArmUpperL" /*ID4 */,
  "ArmLowerR" /*ID5 */, "ArmLowerL" /*ID6 */, "PelvYR"    /*ID7 */, "PelvYL"    /*ID8 */,
  "PelvR"     /*ID9 */, "PelvL"     /*ID10*/, "LegUpperR" /*ID11*/, "LegUpperL" /*ID12*/,
  "LegLowerR" /*ID13*/, "LegLowerL" /*ID14*/, "AnkleR"    /*ID15*/, "AnkleL"    /*ID16*/,
  "FootR"     /*ID17*/, "FootL"     /*ID18*/, "Neck"      /*ID19*/, "Head"      /*ID20*/
};

VisualTracking::VisualTracking(): Robot() {
  mTimeStep = getBasicTimeStep();
  
  mEyeLED = getLED("EyeLed");
  mHeadLED = getLED("HeadLed");
  mCamera = getCamera("Camera");
  mCamera->enable(2*mTimeStep);
  
  for (int i=0; i<NMOTORS; i++)
    mMotors[i] = getMotor(motorNames[i]);
  
  mVisionManager = new DARwInOPVisionManager(mCamera->getWidth(), mCamera->getHeight(), 355, 15, 60, 15, 0.1, 30);
}

VisualTracking::~VisualTracking() {
}

void VisualTracking::myStep() {
  int ret = step(mTimeStep);
  if (ret == -1)
    exit(EXIT_SUCCESS);
}

// function containing the main feedback loop
void VisualTracking::run() {
  double horizontal = 0.0;
  double vertical = 0.0;
  int width  = mCamera->getWidth();
  int height = mCamera->getHeight();

  cout << "---------------Visual Tracking---------------" << endl;
  cout << "This example illustrates the possibilities of Vision Manager." << endl;
  cout << "Move the red ball by holding ctrl + shift keys and select it (left mouse click)." << endl;
	
  // First step to update sensors values
  myStep();

  while (true) {
    double x, y;
    bool ballInFieldOfView = mVisionManager->getBallCenter(x, y, mCamera->getImage());
    // Eye led indicate if ball has been found
    if(ballInFieldOfView)
      mEyeLED->set(0x00FF00);
    else
      mEyeLED->set(0xFF0000);
    // Move the head in direction of the ball if it has been found
    if(ballInFieldOfView) {
      double dh = 0.1*((x / width ) - 0.5);
      horizontal -= dh;
      double dv = 0.1*((y / height ) - 0.5);
      vertical -= dv;
      // clamp horizontal to [-2;2]
      if (horizontal > 1.81) horizontal = 1.81;
      else if (horizontal < -1.81) horizontal = -1.81;
      // clamp vertical to [-1;1]
      if (vertical > 0.94) vertical = 0.94;
      else if (vertical < -0.36) vertical = -0.36;      
      mMotors[18]->setPosition(horizontal);
      mMotors[19]->setPosition(vertical);
    }
    
    // step
    myStep();
  }
}
