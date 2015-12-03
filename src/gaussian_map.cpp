#include "gaussian_map.h"
#include "ein_words.h"

void GaussianMap::reallocate() {
  if (cells == NULL) {
    cells = new GaussianMapCell[width*height];
  } else {
    delete cells;
    cells = new GaussianMapCell[width*height];
  }
}

GaussianMap::GaussianMap(int w, int h, double cw) {
  width = w;
  height = h;
  cell_width = cw;
  cells = NULL;
  reallocate();
}

int GaussianMap::safeAt(int x, int y) {
  return ( (cells != NULL) && (x >= 0) && (x < width) && (y >= 0) && (y < height) );
}

GaussianMapCell *GaussianMap::refAtCell(int x, int y) {
  return (cells + x + width*y);
}

GaussianMapCell GaussianMap::valAtCell(int x, int y) {
  return *(cells + x + width*y);
}

GaussianMapCell GaussianMap::bilinValAtCell(double x, double y) {
  // XXX

}

GaussianMapCell GaussianMap::bilinValAtMeters(double x, double y) {
  // XXX
  // call bilinValAtCell

}

// XXX write to file
void GaussianMap::saveToFile(string filename) {
}

// XXX read from file
void GaussianMap::loadFromFile(string filename) {
}

// XXX 
Mat GaussianMap::rgbToMat() {
}

// XXX 
Mat GaussianMap::zToMat() {
}

// XXX extract a bounding box at specified corners top left (x1,y1) bottom right (x2,y2)
shared_ptr<GaussianMap> GaussianMap::copyBox(int x1, int y1, int x2, int y2) {
}

void GaussianMap::invalidateBox(int x1, int y1, int x2, int y2) {
}

void GaussianMap::invalidate() {
}




Scene::Scene(shared_ptr<MachineState> _ms) {
  ms = _ms;
}

// XXX 
void Scene::composePredictedMap() {
}
// XXX 
void Scene::measureDiscrepancy() {
}
// XXX 
void Scene::assignScore() {
}
// XXX 
void Scene::assignScoreRegion() {
}

// XXX 
void Scene::proposeRegion() {
}
// XXX 
void Scene::proposeObject() {
}

// XXX 
void Scene::tryToAddObjectToScene() {
}

// XXX 
void Scene::removeSpaceObjects() {
}

// XXX 
void Scene::addSpaceObjects() {
}

// XXX 
void Scene::reregisterBackground() {
}

// XXX 
void Scene::reregisterObject(int i) {
}




// XXX word
void TransitionTable::setPrescene(shared_ptr<Scene> s) {
}

// XXX word
void TransitionTable::setPostscene(shared_ptr<Scene> s) {
}

// XXX word
void TransitionTable::setPerformedAction() {
}

// XXX word
void TransitionTable::recordTransitionSceneObject() {
}
// XXX word
void setStateLabelsFromClassLabels() {
}

// XXX word
// XXX word setter
// XXX word getter
//   the word should take a compound word of words, which may be compound words, 
//   and stores their reprs as strings; this makes it easier to serialize
void TransitionTable::setActions(std::vector<string> * actions) {
}

// XXX word
// XXX word setter
// XXX word getter
void TransitionTable::setActionProbabilities(std::vector<double> * actions) {
}

// XXX 
void TransitionTable::initCounts() {
}





/*


// XXX accessors for sceneObjects

// XXX word to add objects to scene until evidence is accounted for

// XXX words to save and load current background and current object maps
// XXX word to remove object from scene and invalidate region around it
// XXX word to invalidate entire observedMap
// XXX word to search clearance area for the cell with lowest number of counts, push its eePose




// XXX sample action word and put string word onto stack
// XXX evaluate string word 
// XXX use string word to set current action 

// XXX calls to init class maps
//  XXX add calls to writeClassToFolder

// XXX word to update observed_map based on current wrist camera image
// XXX map the table by using image stream buffer 
//   XXX  word to update observed_map  based on stream buffer, images and IR
// XXX when training from crops, render from the stream buffer into the observed_map then crop out

// XXX word to fly on a path over all the sceneObjects, streaming images
// XXX word to densely explore map streaming images and IR 
*/





namespace ein_words {

WORD(SceneSaveBackgroundMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
}
END_WORD
REGISTER_WORD(SceneSaveBackgroundMap)

WORD(SceneLoadBackgroundMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
}
END_WORD
REGISTER_WORD(SceneLoadBackgroundMap)

WORD(SceneSaveObservedMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
}
END_WORD
REGISTER_WORD(SceneSaveObservedMap)

WORD(SceneLoadObservedMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
}
END_WORD
REGISTER_WORD(SceneLoadObservedMap)

WORD(SceneInitObservedMap)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
}
END_WORD
REGISTER_WORD(SceneInitObservedMap)

WORD(SceneUpdateObservedFromSnout)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
}
END_WORD
REGISTER_WORD(SceneUpdateObservedFromSnout)

WORD(SceneUpdateDiscrepancy)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
}
END_WORD
REGISTER_WORD(SceneUpdateDiscrepancy)

WORD(SceneGrabCenterCropAsClass)
virtual void execute(std::shared_ptr<MachineState> ms) {
//XXX 
}
END_WORD
REGISTER_WORD(SceneGrabCenterCropAsClass)

WORD(SceneDensityFromDiscrepancy)
virtual void execute(std::shared_ptr<MachineState> ms) {
// this enables denisty based models to use the new channel
//XXX 
}
END_WORD
REGISTER_WORD(SceneDensityFromDiscrepancy)

/* 
WORD()
virtual void execute(std::shared_ptr<MachineState> ms) {
}
END_WORD
REGISTER_WORD()

WORD(TransitionTableInit)
virtual void execute(std::shared_ptr<MachineState> ms) {
// zero it out
}
END_WORD
REGISTER_WORD(TransitionTableInit)

WORD(TransitionTableCount)
virtual void execute(std::shared_ptr<MachineState> ms) {
}
END_WORD
REGISTER_WORD(TransitionTableCount)


WORD(PlanWithTransitionTable)
virtual void execute(std::shared_ptr<MachineState> ms) {

// takes a desired state and outputs the action that best takes the prescene to that state

}
END_WORD
REGISTER_WORD(PlanWithTransitionTable)

*/

}



