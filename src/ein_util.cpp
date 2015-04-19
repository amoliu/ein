#include "ein_util.h"
#include <ros/console.h>

std::string operationStatusToString(operationStatusType mode) 
{
    string result;
  if (mode == UNKNOWN) {
    result = "Unknown";
  } else if (mode == FAILURE) {
    result = "Failure";
  } else if (mode == SUCCESS) {
    result = "Success";
  } else {
    cout << "Invalid operation status: " << mode << endl;
    assert(0);
  }
  return result;
}




string pickModeToString(pickMode mode) {
  string result;
  if (mode == STATIC_PRIOR) {
    result = "static prior";
  } else if (mode == LEARNING_SAMPLING) {
    result = "learning sampling";
  } else if (mode == LEARNING_ALGORITHMC) {
    result = "learning algorithm C";
  } else if (mode == STATIC_MARGINALS) {
    result = "static marginals";
  } else if (mode == MAPPING) {
    result = "mapping";
  } else {
    cout << "Invalid pick mode: " << mode << endl;
    assert(0);
  }
  return result;
}



void pushSpeedSign(shared_ptr<MachineState> ms, double speed) {

  if (speed == NOW_THATS_FAST) {
    ms->pushWord("setMovementSpeedNowThatsFast"); 
  } else if (speed == MOVE_EVEN_FASTER) {
    ms->pushWord("setMovementSpeedMoveEvenFaster"); 
  } else  if (speed == MOVE_FASTER) {
    ms->pushWord("setMovementSpeedMoveFaster"); 
  } else if (speed == MOVE_FAST) {
    ms->pushWord("setMovementSpeedMoveFast"); 
  } else if (speed == MOVE_MEDIUM) {
    ms->pushWord("setMovementSpeedMoveMedium"); 
  } else if (speed == MOVE_SLOW) {
    ms->pushWord("setMovementSpeedMoveSlow"); 
  } else if (speed == MOVE_VERY_SLOW) {
    ms->pushWord("setMovementSpeedMoveVerySlow"); 
  } else {
    ROS_ERROR_STREAM("Unknown speed: " << speed);
    assert(0);
  }

}


void guardedImshow(string name, Mat image, bool shouldIRender) {
  if ( !isSketchyMat(image) && shouldIRender ) {
    imshow(name, image);
  }
}

bool isSketchyMat(Mat sketchy) {
  return ( (sketchy.rows <= 1) || (sketchy.rows <= 1) );
}



gsl_matrix * boxMemoryToPolygon(BoxMemory b) {
  double min_x = b.top.px;
  double min_y = b.top.py;
  double max_x = b.bot.px;
  double max_y = b.bot.py;
  double width = max_x - min_x;
  double height = max_y - min_y;

  gsl_matrix *  polygon = gsl_matrix_alloc(2, 4);
  gsl_matrix_set(polygon, 0, 0, min_x);
  gsl_matrix_set(polygon, 1, 0, min_y);

  gsl_matrix_set(polygon, 0, 1, min_x + width);
  gsl_matrix_set(polygon, 1, 1, min_y);

  gsl_matrix_set(polygon, 0, 2, min_x + width);
  gsl_matrix_set(polygon, 1, 2, min_y + height);

  gsl_matrix_set(polygon, 0, 3, min_x);
  gsl_matrix_set(polygon, 1, 3, min_y + height);
  return polygon;
}


eePose rosPoseToEEPose(geometry_msgs::Pose pose) {
  eePose result;
  result.px = pose.position.x;
  result.py = pose.position.y;
  result.pz = pose.position.z;
  result.qx = pose.orientation.x;
  result.qy = pose.orientation.y;
  result.qz = pose.orientation.z;
  result.qw = pose.orientation.w;
  return result;
}