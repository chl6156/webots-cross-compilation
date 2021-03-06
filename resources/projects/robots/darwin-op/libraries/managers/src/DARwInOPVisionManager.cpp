#include "DARwInOPVisionManager.hpp"

using namespace Robot;
using namespace managers;
using namespace std;


DARwInOPVisionManager::DARwInOPVisionManager(int width, int height, int hue, int hueTolerance, int minSaturation, int minValue, int minPercent, int maxPercent) {
  mfinder = new ColorFinder(hue, hueTolerance, minSaturation, minValue, minPercent, maxPercent);
  mbuffer = new FrameBuffer(width, height);
}

DARwInOPVisionManager::~DARwInOPVisionManager() {
  delete mfinder;
  delete mbuffer;
}

bool DARwInOPVisionManager::getBallCenter(double &x, double &y, const unsigned char * image)
{
  
  Point2D pos;
  
  // Put the image in mbuffer
  mbuffer->m_BGRAFrame->m_ImageData = (unsigned char *)image;
  // Convert the image from BGRA format to HSV format
  ImgProcess::BGRAtoHSV(mbuffer);
  // Extract position of the ball from HSV verson of the image
  pos = mfinder->GetPosition(mbuffer->m_HSVFrame);
  
  if (pos.X==-1 && pos.Y==-1) {
    x = 0.0;
    y = 0.0;
    return false;
  } else {
    x = pos.X;
    y = pos.Y;
    return true;
  }
}

bool DARwInOPVisionManager::isDetected(int x, int y) {
  
  if(x > mfinder->m_result->m_Width || y > mfinder->m_result->m_Height)
    return false;
  
  int i = y * mfinder->m_result->m_Width + x;
  
  if(mfinder->m_result->m_ImageData[i] == 1)
    return true;
  else
    return false;
}
