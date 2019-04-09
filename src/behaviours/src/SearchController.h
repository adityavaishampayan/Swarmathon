#ifndef SEARCH_CONTROLLER
#define SEARCH_CONTROLLER

#include <random_numbers/random_numbers.h>
#include "Controller.h"

/**
 * This class implements the search control algorithm for the rovers. The code
 * here should be modified and enhanced to improve search performance.
 */
class SearchController : virtual Controller {

public:

  SearchController();

  void Reset() override;

  // performs search pattern
  Result DoWork() override;
  bool ShouldInterrupt() override;
  bool HasWork() override;

  // sets the value of the current location
  //void UpdateData(geometry_msgs::Pose2D currentLocation, geometry_msgs::Pose2D centerLocation);
  void SetCurrentLocation(Point currentLocation);
  void SetCenterLocation(Point centerLocation);
  void SetSuccesfullPickup();
  void setRecruitmentLocation(Point p);
  void generateHilbertPoints(unsigned int degree);
  void setRoverName(string publishedName);
  void setRoverCount_Rank(int noOfRovers,int rank);
  void decrementPathIndex();
  std::vector<Point> hilbertWaypoints ;
  string roverName;
  std::vector<Point> currentPathPoints;

protected:

  void ProcessData();

private:

  random_numbers::RandomNumberGenerator* rng;
  Point currentLocation;
  Point centerLocation;
  Point searchLocation;
  int attemptCount = 0;
  //struct for returning data to ROS adapter
  Result result;

  // Search state
  // Flag to allow special behaviour for the first waypoint
  bool first_waypoint = true;
  bool succesfullPickup = false;
  long int pathPointIndex = 0;
  double hilbert2dScale = 0.406;
  int botIndex = 0;
  // std::vector<Point> hilbertWaypoints

  void updateCurrentPathPoints(string roverName);
  bool ranOnce = false;
  int totalRovers = 1;
  int myRoverIndex = 1;
  double lowerLeftHilbertPt = -6.5;
  bool pathUpdated = false;
  int getQuadrant(Point p);
  Point initCCWMove[4] = {
		  {1,1},
		  {-1,1},
		  {-1,-1},
		  {1,-1}
  };
  bool initCornerSent = false;

};

#endif /* SEARCH_CONTROLLER */
