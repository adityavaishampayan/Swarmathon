#include "MapController.h"
#include <angles/angles.h>
#include <cmath>
#include <math.h>
#include <string>
#include "Tag.h"

#define PI 3.14159265


MapController::MapController() {
  rng = new random_numbers::RandomNumberGenerator();
  currentLocation.x = 0;
  currentLocation.y = 0;
  currentLocation.theta = 0;

  centerLocation.x = 0;
  centerLocation.y = 0;
  centerLocation.theta = 0;
  result.PIDMode = FAST_PID;

  result.fingerAngle = M_PI/2;
  result.wristAngle = M_PI/4;

  // Create a CV window to visualize generated Map
  cv::namedWindow("MapWindow", CV_WINDOW_AUTOSIZE );
}

void MapController::Reset() {
  result.reset = false;
  cv::destroyAllWindows();
}

/**
 * Take current location and convert into discrete grid point
 */
Point MapController::toGridPoint(Point _currentLocation) {
  Point gridPoint;
  gridPoint.x = round((centerLocation.x - _currentLocation.x) / gridSize);
  gridPoint.y = round((centerLocation.y - _currentLocation.y) / gridSize);
  return gridPoint;
}

bool MapController::currLocFound(Point _currentLocation) {
  bool updateFlag = false;
  _currentLocation = toGridPoint(_currentLocation);
  for (auto p : mapObj) {
    updateFlag = (p.location.x == _currentLocation.x) && (p.location.y == _currentLocation.y);
    if (updateFlag) break;
  }
  return updateFlag;
}

void MapController::SetSonarData(float left, float center, float right) {
  sonarLeft = left;
  sonarCenter = center;
  sonarRight = right;
}

void MapController::GetObjectPos(Point _currentLocation) {
  MapPoint mapPoint;
  if(sonarLeft < 0.0002) {
    Point obsPoint;
    obsPoint.x = _currentLocation.x + (sonarLeft * std::cos((PI / 4) + _currentLocation.theta));
    obsPoint.y = _currentLocation.y + (sonarLeft * std::sin((PI / 4) + _currentLocation.theta));
    if (!currLocFound(obsPoint)) {
      Point gridPoint = toGridPoint(obsPoint);
      mapPoint.location.x = gridPoint.x;
      mapPoint.location.y = gridPoint.y;
      mapPoint.id = 0;
      mapPoint.occType = OBSTACLE;
      mapObj.push_back(mapPoint);
    }
  }
  if(sonarCenter < 2) {
    Point obsPoint;
    obsPoint.x = _currentLocation.x + (sonarCenter * std::cos(_currentLocation.theta));
    obsPoint.y = _currentLocation.y + (sonarCenter * std::sin(_currentLocation.theta));
    if (!currLocFound(obsPoint)) {
      Point gridPoint = toGridPoint(obsPoint);
      mapPoint.location.x = gridPoint.x;
      mapPoint.location.y = gridPoint.y;
      mapPoint.id = 0;
      mapPoint.occType = OBSTACLE;
      mapObj.push_back(mapPoint);
    }
  }
  if(sonarRight < 0.0002) {
    Point obsPoint;
    obsPoint.x = _currentLocation.x + (sonarRight * std::cos(-(PI / 4) + _currentLocation.theta));
    obsPoint.y = _currentLocation.y + (sonarRight * std::sin(-(PI / 4) + _currentLocation.theta));
    if (!currLocFound(obsPoint)) {
      Point gridPoint = toGridPoint(obsPoint);
      mapPoint.location.x = gridPoint.x;
      mapPoint.location.y = gridPoint.y;
      mapPoint.id = 0;
      mapPoint.occType = OBSTACLE;
      mapObj.push_back(mapPoint);
    }
  }
}


/**
 * This code adds new grid points to map object vector
 */
Result MapController::DoWork() {
  if (!currLocFound(currentLocation)) {
    MapPoint mapPoint;
    Point gridPoint = toGridPoint(currentLocation);
    mapPoint.location.x = gridPoint.x;
    mapPoint.location.y = gridPoint.y;
    mapPoint.id = 0;
    mapPoint.occType = EMPTY;
    mapObj.push_back(mapPoint);
  }
  GetObjectPos(currentLocation);
  visualizeMap();
  // visuvalization();
  result.b = wait;
  return (result);
}

void MapController::SetCenterLocation(Point centerLocation) {

  float diffX = this->centerLocation.x - centerLocation.x;
  float diffY = this->centerLocation.y - centerLocation.y;
  this->centerLocation = centerLocation;
  // std::cout << "center loc x :" << centerLocation.x << " y: " << centerLocation.y << std::endl;
}

void MapController::SetCurrentLocation(Point currentLocation) {
  this->currentLocation = currentLocation;
}


void MapController::ProcessData() {
}

bool MapController::ShouldInterrupt(){
  ProcessData();

  return false;
}

bool MapController::HasWork() {
  return true;
}

void MapController::setTagData(vector<Tag> tags){
  MapPoint mapPoint;
  Point cubePoint;
  for (auto tag : tags) {
    posX = tag.getPositionX();
    posY = tag.getPositionY();
    posZ = tag.getPositionZ();
    // std::tuple<float, float, float> orientation = tag.calcRollPitchYaw();
    // using a^2 + b^2 = c^2 to find the distance to the block
    // 0.195 is the height of the camera lens above the ground in cm.
    //
    // a is the linear distance from the robot to the block, c is the
    // distance from the camera lens, and b is the height of the
    // camera above the ground.

    blockDistanceFromCamera = std::hypot(std::hypot(posX, posY), posZ);

    if ( (blockDistanceFromCamera*blockDistanceFromCamera - 0.195*0.195) > 0 )
    {
      blockDistance = std::sqrt(blockDistanceFromCamera*blockDistanceFromCamera - 0.195*0.195);
      blockYawError = std::atan((posX + cameraOffsetCorrection)/blockDistance);
      auto tagDistFromBot = std::hypot(blockDistance, (posX + cameraOffsetCorrection));
      cubePoint.x = currentLocation.x + (std::cos(currentLocation.theta + blockYawError) * tagDistFromBot);
      cubePoint.y = currentLocation.y + (std::sin(currentLocation.theta + blockYawError) * tagDistFromBot);
      cubePoint.theta = currentLocation.theta + blockYawError;
      std::cout << "Tag Pos : X : " << cubePoint.x << " Y : " << cubePoint.y << std::endl ;
      //std::cout << "Tag Pos : R : " << get<0>(orientation) << " P : " << get<1>(orientation) << " Y : " << get<2>(orientation) << std::endl ;
      if (!currLocFound(cubePoint)) {
        Point gridPoint = toGridPoint(cubePoint);
        mapPoint.location.x = gridPoint.x;
        mapPoint.location.y = gridPoint.y;
        mapPoint.location.theta = cubePoint.theta;
        mapPoint.id = 0;
        switch((int)tag.getID()){
          case 1:
            mapPoint.occType = BOUNDARY;
            break;
          case 256:
            mapPoint.occType = COLLECTIONCENTER;
            break;
          default:
            mapPoint.occType = CUBE;
            break;
        }
        mapObj.push_back(mapPoint);
    }

    }
  }
}

std::vector<int> MapController::getMapSize() {
  int minX = -1000;
  int minY = -1000;
  int maxX = -1000;
  int maxY = -1000;
  for(auto pts : mapObj) {
    minX = (pts.location.x < minX) ? pts.location.x : minX;
    minY = (pts.location.y < minY) ? pts.location.y : minY;
    maxX = (pts.location.x > maxX) ? pts.location.x : maxX;
    maxY = (pts.location.y > maxY) ? pts.location.y : maxY;
  }
  std::vector<int> mapSize{minX, minY, maxX, maxY};
  return(mapSize);
}

void MapController::visualizeMap() {
  /*
  for(auto p : hilbertWaypoints) {
    std::cout << "Hil X: " << round(p.x/gridSize) << " Y : " << round(p.y/gridSize) << std::endl;
  }
  Point loc = toGridPoint(currentLocation);
  std:: cout << "------------ Current X : " << loc.x << " Y : " << loc.y << std::endl;
  */

  // auto mapDims = getMapSize();
  cv::Mat mapCVMat(200, 200, CV_8UC3, cv::Scalar(0,0,0));

  for (auto p : mapObj) {
    // std::cout << "point x : " << p.location.x << " y : " << p.location.y << " type : " << p.occType << endl;

    switch(p.occType){
      case EMPTY:
        cv::circle(mapCVMat, cv::Point(p.location.x + 100,
          p.location.y + 100), 1, cv::Scalar(105, 105, 105), cv::FILLED);
        break;
      case OBSTACLE:
        cv::circle(mapCVMat, cv::Point(p.location.x + 100,
          p.location.y + 100), 1, cv::Scalar(0, 0, 255), cv::FILLED);
        break;
      case CUBE:
        cv::circle(mapCVMat, cv::Point(p.location.x + 100,
          p.location.y + 100), 1, cv::Scalar(255, 255, 255), cv::FILLED);
        break;
      case BOUNDARY:
        cv::circle(mapCVMat, cv::Point(p.location.x + 100,
          p.location.y + 100), 1, cv::Scalar(0, 69, 255), cv::FILLED);
        break;
      case COLLECTIONCENTER:
        cv::circle(mapCVMat, cv::Point(p.location.x + 100,
          p.location.y + 100), 1, cv::Scalar(0, 215, 255), cv::FILLED);
        break;
      default:
        cv::circle(mapCVMat, cv::Point(p.location.x + 100,
          p.location.y + 100), 1, cv::Scalar(25, 25, 25), cv::FILLED);
        break;
    }
  }

  Point curLoc;
  curLoc.x = currentLocation.x;
  curLoc.y = currentLocation.y;
  Point gridPoint = toGridPoint(curLoc);
  cv::circle(mapCVMat, cv::Point(gridPoint.x + 100,
    gridPoint.y + 100), 1, cv::Scalar(255, 255, 0), cv::FILLED);

  cv::resize(mapCVMat, mapCVMat,
    cv::Size(mapCVMat.cols * 5,mapCVMat.rows * 5),
    0, 0, CV_INTER_LINEAR);
  cv::imshow("MapWindow", mapCVMat);
  cv::waitKey(30);

/*
  std::cout << "size : " << mapObj.size()<< std::endl;
  for(int i =0 ; i < mapSize[0] ; i++) {
    std::string row = "";
    for(int j=0 ; j < mapSize[1] ; j++) {
      row  = row + mapDisp[i][j];
    }
    std::cout << row <<std::endl;
  }
*/

}
