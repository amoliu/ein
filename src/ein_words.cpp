
// Preprocessor macro to automatically create class structure for words
// extracted from the case statement of evil (despair)
// WORD(gName)
// // code here
// CODE(123)

#define WORD(gName) \
class gName: public Word \
{ \
public: \
  virtual string name() { \
    string str = #gName; \
    str[0] = tolower(str[0]); \
    return str; \
  } 

#define CODE(code) \
  virtual int character_code() { \
    return code; \
  } 

#define END_WORD };



namespace ein_words
{

#include "ein_bandit.cpp"
#include "ein_render.cpp"
#include "ein_movement.cpp"
#include "ein_servo.cpp"
#include "ein_vision_cycle.cpp"
#include "ein_scanning.cpp"

WORD(ClearStack)
CODE('r') 
virtual void execute() {
  pilot_call_stack.resize(0);
}
END_WORD


 
WORD(PauseAndClearVariables)
CODE('c') 
virtual void execute() {
  holding_pattern = 0;
  auto_pilot = 0;
  go_on_lock = 0;
  execute_stack = 0;
  lastPtheta = INFINITY;
}
END_WORD


WORD(Beep)
CODE(1245308)     // capslock + numlock + |
virtual void execute() {
  cout << "\a"; cout.flush();
}
END_WORD

WORD(WaitUntilAtCurrentPosition)
CODE(131154)    // capslock + r
virtual void execute() {
  double dx = (currentEEPose.px - trueEEPose.position.x);
  double dy = (currentEEPose.py - trueEEPose.position.y);
  double dz = (currentEEPose.pz - trueEEPose.position.z);
  double distance = dx*dx + dy*dy + dz*dz;
  
  double qx = (fabs(currentEEPose.qx) - fabs(trueEEPose.orientation.x));
  double qy = (fabs(currentEEPose.qy) - fabs(trueEEPose.orientation.y));
  double qz = (fabs(currentEEPose.qz) - fabs(trueEEPose.orientation.z));
  double qw = (fabs(currentEEPose.qw) - fabs(trueEEPose.orientation.w));
  double angleDistance = qx*qx + qy*qy + qz*qz + qw*qw;
  
  if ((distance > w1GoThresh*w1GoThresh) || (angleDistance > w1AngleThresh*w1AngleThresh))
    pilot_call_stack.push_back(131154); // w1 wait until at current position
}
END_WORD




WORD(AssumeWholeFoodsCounter1)
CODE(196672)  // capslock + @
virtual void execute() {
  currentEEPose = wholeFoodsCounter1;
}
END_WORD

WORD(AssumeWholeFoodsPantry1)
CODE(196643)   // capslock + #
virtual void execute() {
  currentEEPose = wholeFoodsPantry1;
}
END_WORD


WORD(ChangeToCounterTable)
CODE(1179735) // capslock + numlock + w
virtual void execute() {
  currentTableZ = counterTableZ;
}
END_WORD
  
WORD(ChangeToPantryTable)
CODE(1179717)    // capslock + numlock + e
virtual void execute() {
  currentTableZ = pantryTableZ;
}
END_WORD


WORD(ExecuteStack)
CODE('y')
virtual void execute() {
  execute_stack = 1;
}
END_WORD

WORD(PauseStackExecution)
CODE('Y') 
virtual void execute()  {
  cout << "STACK EXECUTION PAUSED, press 'y' to continue." << endl;
  execute_stack = 0;
}
END_WORD



WORD(PrintState)
CODE('u')
virtual void execute() {
  cout << endl;
  cout << "Current EE Position (x,y,z): " << currentEEPose.px << " " << currentEEPose.py << " " << currentEEPose.pz << endl;
  cout << "Current EE Orientation (x,y,z,w): " << currentEEPose.qx << " " << currentEEPose.qy << " " << currentEEPose.qz << " " << currentEEPose.qw << endl;
  cout << "True EE Position (x,y,z): " << trueEEPose.position.x << " " << trueEEPose.position.y << " " << trueEEPose.position.z << endl;
  cout << "True EE Orientation (x,y,z,w): " << trueEEPose.orientation.x << " " << trueEEPose.orientation.y << " " << trueEEPose.orientation.z << " " << trueEEPose.orientation.w << endl;
  cout <<
    "eePose = {.px = " << trueEEPose.position.x << ", .py = " << trueEEPose.position.y << ", .pz = " << trueEEPose.position.z << "," << endl <<
    "		      .ox = 0, .oy = 0, .oz = 0," << endl <<
    "		      .qx = " << trueEEPose.orientation.x << ", .qy = " << trueEEPose.orientation.y << ", .qz = " << trueEEPose.orientation.z << ", .qw = " << trueEEPose.orientation.w << "};" << endl;
  cout << "mostRecentUntabledZ: " << mostRecentUntabledZ << endl;
  cout << "currentPickMode: " << pickModeToString(currentPickMode) << endl;
  cout << "currentBoundingBoxMode: " << pickModeToString(currentBoundingBoxMode) << endl;
  cout << "gradientServoTakeClosest: " << gradientTakeClosest << endl;
  cout << "synchronicTakeClosest: " << synchronicTakeClosest << endl;
  cout << "focusedClass: " << focusedClass;
  if (focusedClass != -1) {
    cout << " " << classLabels[focusedClass];
  }
  cout << endl;
  
  cout << "targetClass: " << targetClass;
  if (targetClass != -1) {
    cout << " " << classLabels[targetClass];
  }
  
  cout << endl;
  cout << endl;
}
END_WORD

WORD(DecrementTargetClass)
CODE(196438)     // capslock + pagedown
virtual void execute() {
  cout << "targetClass-- " << endl;
  if (numClasses > 0) {
    int newTargetClass = (targetClass - 1 + numClasses) % numClasses;
    changeTargetClass(newTargetClass);
  }
}
END_WORD

WORD(IncrementTargetClass)
CODE(196437)// capslock + pageup
virtual void execute()
{
  cout << "targetClass++ " << endl;
  if (numClasses > 0) {
    int newTargetClass = (targetClass + 1) % numClasses;
    changeTargetClass(newTargetClass);
  }
}
END_WORD


WORD(GraspGear1)
CODE(1048625)
virtual void execute()
{
  int thisGraspGear = 0;
  
  //   set drX
  drX = ggX[thisGraspGear];
  drY = ggY[thisGraspGear];
  
  //   rotate
  setGGRotation(thisGraspGear);
  
  //   set currentGraspGear;
  currentGraspGear = thisGraspGear;
  // numlock + 1
}
END_WORD

WORD(Pause)
CODE('C')
virtual void execute()
{
  if (auto_pilot || (holding_pattern != 0)) {
    pilot_call_stack.push_back('C');
  } else {
    holding_pattern = 0;
    auto_pilot = 0;
    go_on_lock = 0;
  }
}
END_WORD

}

using namespace  ein_words;

std::map<int, Word *> create_character_code_to_word(std::vector<Word *> words) {
  std::map<int, Word *> character_code_to_word;
  for (unsigned int i = 0; i < words.size(); i++) {
    character_code_to_word[words[i]->character_code()] = words[i];
  }
  return character_code_to_word;
}

std::map<string, Word *> create_name_to_word(std::vector<Word *> words) {
  std::map<string, Word *> name_to_word;
  for (unsigned int i = 0; i < words.size(); i++) {
    name_to_word[words[i]->name()] = words[i];
  }
  return name_to_word;
}




std::vector<Word *> create_words() {
  std::vector<Word *> words;
  words.push_back(new XUp());
  words.push_back(new XDown());
  words.push_back(new YUp());
  words.push_back(new YDown());
  words.push_back(new ZUp());
  words.push_back(new ZDown());
  words.push_back(new Pause());
  words.push_back(new GraspGear1());
  words.push_back(new SynchronicServoTakeClosest());
  words.push_back(new GradientServoTakeClosest());
  words.push_back(new IncrementTargetClass());
  words.push_back(new PrintState());
  words.push_back(new SetHeightMemoriesFromClassHeightMemories());
  words.push_back(new SetGraspMemoriesFromClassGraspMemories());
  words.push_back(new DrawMapRegisters());
  words.push_back(new ScanObject());
  words.push_back(new ExecuteStack());
  words.push_back(new PauseStackExecution());
  words.push_back(new PauseAndClearVariables());
  words.push_back(new ClearStack());
  words.push_back(new OpenGripper());
  words.push_back(new CloseGripper());
  words.push_back(new ChangeToCounterTable());
  words.push_back(new ChangeToPantryTable());
  words.push_back(new AssumeWholeFoodsPantry1());
  words.push_back(new AssumeWholeFoodsCounter1());
  words.push_back(new SetMovementSpeedNowThatsFast());
  words.push_back(new SetMovementSpeedMoveEvenFaster());
  words.push_back(new SetMovementSpeedMoveFaster());
  words.push_back(new SetMovementSpeedMoveFast());
  words.push_back(new SetMovementSpeedMoveMedium());
  words.push_back(new SetMovementSpeedMoveSlow());
  words.push_back(new SetMovementSpeedMoveVerySlow());
  words.push_back(new ChangeToHeight0());
  words.push_back(new ChangeToHeight1());
  words.push_back(new ChangeToHeight2());
  words.push_back(new ChangeToHeight3());
  words.push_back(new WaitUntilAtCurrentPosition());
  words.push_back(new Beep());
  words.push_back(new VisionCycle());
  words.push_back(new Density());
  words.push_back(new ResetTemporalMap());
  words.push_back(new GoFindBlueBoxes());
  words.push_back(new GoClassifyBlueBoxes());
  words.push_back(new SynchronicServo());
  words.push_back(new GradientServo());
  words.push_back(new TwoDPatrolStart());
  words.push_back(new TwoDPatrolContinue());
  words.push_back(new SynchronicServoDoNotTakeClosest());
  words.push_back(new SynchronicServoTakeClosest());
  words.push_back(new InitializeAndFocusOnNewClass());
  words.push_back(new ResetAerialGradientTemporalFrameAverage());
  words.push_back(new SaveAerialGradientMap());
  words.push_back(new NeutralScan());
  words.push_back(new InitDepthScan());
  words.push_back(new TurnOnRecordRangeMap());
  words.push_back(new PrepareForSearch());
  words.push_back(new FullRender());
  words.push_back(new SampleHeight());
  words.push_back(new DownsampleIrScan());
  words.push_back(new PaintReticles());
  words.push_back(new SelectBestAvailableGrasp());
  words.push_back(new SelectMaxTargetNotCumulative());
  words.push_back(new SelectMaxTargetCumulative());
  words.push_back(new ApplyGraspFilter());
  words.push_back(new Blur());
  words.push_back(new ShiftIntoGraspGear1());
  words.push_back(new ShiftIntoGraspGear2());
  words.push_back(new ShiftIntoGraspGear3());
  words.push_back(new ShiftIntoGraspGear4());
  words.push_back(new TurnOffScanning());
  words.push_back(new OXDown());
  words.push_back(new OXUp());
  words.push_back(new OYDown());
  words.push_back(new OYUp());
  words.push_back(new OZDown());
  words.push_back(new OZUp());
  
  words.push_back(new SaveRegister1());
  words.push_back(new SaveRegister2());
  words.push_back(new SaveRegister3());
  words.push_back(new SaveRegister4());

  words.push_back(new MoveToRegister1());
  words.push_back(new MoveToRegister2());
  words.push_back(new MoveToRegister3());
  words.push_back(new MoveToRegister4());
  words.push_back(new MoveToRegister5());
  words.push_back(new MoveToRegister6());
  
  words.push_back(new PrepareToApplyGraspFilterFor1());
  words.push_back(new PrepareToApplyGraspFilterFor2());
  words.push_back(new PrepareToApplyGraspFilterFor3());
  words.push_back(new PrepareToApplyGraspFilterFor4());
  words.push_back(new SetTargetReticleToTheMaxMappedPosition());
  words.push_back(new ShiftGraspGear()); 
  words.push_back(new SaveCurrentClassDepthAndGraspMaps());
  words.push_back(new UniformlySampleHeight());
  words.push_back(new PhotoSpin());
  words.push_back(new RgbScan());
  words.push_back(new IncrementGraspGear());
  words.push_back(new SetRandomOrientationForPhotospin());
  words.push_back(new RecordExampleAsFocusedClass());
  words.push_back(new VisionCycleNoClassify());

  words.push_back(new SetPickModeToStaticPrior());
  words.push_back(new SetPickModeToLearningSampling());
  words.push_back(new SetPickModeToLearningAlgorithmC());
  words.push_back(new SetPickModeToStaticMarginals());
  words.push_back(new SetBoundingBoxModeToStaticPrior());
  words.push_back(new SetBoundingBoxModeToLearningSampling());
  words.push_back(new SetBoundingBoxModeToLearningAlgorithmC());
  words.push_back(new SetBoundingBoxModeToStaticMarginals());
  words.push_back(new TurnHistogrammingDuringServoingOn());
  words.push_back(new TurnHistogrammingDuringServoingOff());
  words.push_back(new PrepareForAndExecuteGraspFromMemory());

  words.push_back(new CountGrasp());
  words.push_back(new CheckGrasp());
  words.push_back(new CheckAndCountGrasp());
  words.push_back(new CalibrateGripper());
  words.push_back(new SetGripperThresh());
  words.push_back(new LoadTargetClassRangeMapIntoRegister1());

  words.push_back(new AssumeWinningGgAndXyInLocalPose());
  words.push_back(new MoveToTargetZAndGrasp());
  words.push_back(new ShakeItUpAndDown());
  words.push_back(new TryToMoveToTheLastPickHeight());
  words.push_back(new AssertYesGrasp());
  words.push_back(new AssertNoGrasp());
  words.push_back(new ShakeItOff1());
  words.push_back(new FindBestOfFourGraspsUsingMemory());
  words.push_back(new LoadSampledGraspMemory());
  words.push_back(new LoadMarginalGraspMemory());
  words.push_back(new LoadPriorGraspMemoryAnalytic());
  words.push_back(new LoadPriorGraspMemoryUniform());
  words.push_back(new LoadSampledHeightMemory());
  words.push_back(new LoadMarginalHeightMemory());
  words.push_back(new LoadPriorHeightMemoryAnalytic());
  words.push_back(new LoadPriorHeightMemoryUniform());
  words.push_back(new PerturbPosition());
  words.push_back(new TrainModels());
  words.push_back(new SetTargetClassToLastLabelLearned());
  words.push_back(new SetLastLabelLearned());

  words.push_back(new BeginHeightLearning());
  words.push_back(new ContinueHeightLearning());
  words.push_back(new RecordHeightLearnTrial());
  words.push_back(new SetRandomPositionAndOrientationForHeightLearning());
  words.push_back(new SaveLearnedModels());
  words.push_back(new DecrementTargetClass());
  return words;
}

