WORD(VisionPatrol)
CODE(196727) // capslock + W
virtual void execute() {
  cout << "vision patrol" << endl;
  pushWord("visionPatrol");
  pushWord("publishRecognizedObjectArrayFromBlueBoxMemory");
  pushWord("setRandomPositionAndOrientationForHeightLearning");
  pushWord("recordBlueBoxes");
  pushWord("visionCycle");
  pushCopies("noop", 100);
}
END_WORD

WORD(PublishRecognizedObjectArrayFromBlueBoxMemory)
virtual void execute() {
  object_recognition_msgs::RecognizedObjectArray roa;
  visualization_msgs::MarkerArray ma; 
 
  roa.objects.resize(0);

  roa.header.stamp = ros::Time::now();
  roa.header.frame_id = "/base";


  for (int class_i = 0; class_i < classLabels.size(); class_i++) {
    string class_label = classLabels[class_i];
    if (class_label != "background") {
      eePose centroid;
      centroid.px = 0;
      centroid.py = 0;
      centroid.pz = 0;
      int class_count = 0;
      for (int j = 0; j < blueBoxMemories.size(); j++) {
        if (blueBoxMemories[j].labeledClassIndex == class_i) {
          centroid.px += blueBoxMemories[j].eeCentroid.px;
          centroid.py += blueBoxMemories[j].eeCentroid.py;
          centroid.pz += blueBoxMemories[j].eeCentroid.pz;
          class_count += 1;
        }
      }
      if (class_count == 0) {
        continue;
      }
      centroid.px = centroid.px / class_count;
      centroid.py = centroid.py / class_count;
      centroid.pz = centroid.pz / class_count;
      int closest_idx = -1;
      double min_square_dist = VERYBIGNUMBER;

      for (int j = 0; j < blueBoxMemories.size(); j++) {
        if (blueBoxMemories[j].labeledClassIndex == class_i) {
          double square_dist = 
            squareDistanceEEPose(centroid, blueBoxMemories[j].eeCentroid);
          if (square_dist < min_square_dist) {
            min_square_dist = square_dist;
            closest_idx = j;
          }
        }
      }


      if (closest_idx != -1) {
        geometry_msgs::Pose pose;
        int aI = roa.objects.size();
        roa.objects.resize(roa.objects.size() + 1);
        ma.markers.resize(ma.markers.size() + 1);

        pose.position.x = blueBoxMemories[closest_idx].eeCentroid.px;
        pose.position.y = blueBoxMemories[closest_idx].eeCentroid.py;
        pose.position.z = blueBoxMemories[closest_idx].eeCentroid.pz;

        cout << "blueBoxMemories: " << blueBoxMemories[closest_idx].eeCentroid.px << endl;
        cout << "pose: " << pose.position.x << endl;

        roa.objects[aI].pose.pose.pose.position = pose.position;

        cout << "roa objects x: " << roa.objects[aI].pose.pose.pose.position.x << endl;
        roa.objects[aI].type.key = class_label;

        ma.markers[aI].pose = roa.objects[aI].pose.pose.pose;
        cout << "marker pose x: " << ma.markers[aI].pose.position.x << endl;
        roa.objects[aI].header = roa.header;
        ma.markers[aI].header = roa.header;

        ma.markers[aI].type =  visualization_msgs::Marker::SPHERE;
        ma.markers[aI].scale.x = 0.15;
        ma.markers[aI].scale.y = 0.15;
        ma.markers[aI].scale.z = 0.15;
        ma.markers[aI].color.a = 0.5;
        ma.markers[aI].color.r = 0.9;
        ma.markers[aI].color.g = 0.9;
        ma.markers[aI].color.b = 0.0;
        ma.markers[aI].id = aI;
        ma.markers[aI].lifetime = ros::Duration(1.0);



      }
    }
  }

  rec_objs_blue_memory.publish(roa);
  markers_blue_memory.publish(ma);

}
END_WORD


WORD(RecordBlueBoxes)
virtual void execute() {
  cout << "Recording blue boxes: " << bTops.size() << endl;
  for (int c = 0; c < bTops.size(); c++) {
    BoxMemory box;
    box.bTop = bTops[c];
    box.bBot = bBots[c];
    box.cameraPose = currentEEPose;
    box.eeTop = pixelToGlobalEEPose(box.bTop.x, box.bTop.y, trueEEPose.position.z + currentTableZ);
    box.eeBot = pixelToGlobalEEPose(box.bBot.x, box.bBot.y, trueEEPose.position.z + currentTableZ);
    box.eeCentroid.px = (box.eeTop.px + box.eeBot.px) * 0.5;
    box.eeCentroid.py = (box.eeTop.py + box.eeBot.py) * 0.5;
    box.eeCentroid.pz = (box.eeTop.pz + box.eeBot.pz) * 0.5;
    box.cameraTime = ros::Time::now();
    box.labeledClassIndex = bLabels[c];
    blueBoxMemories.push_back(box);
  }

}
END_WORD



WORD(ClearBlueBoxMemories)
CODE(196709) // capslock + E
virtual void execute() {
  cout << "Clearing blue box memory: " << blueBoxMemories.size() << endl;
  blueBoxMemories.resize(0);
}
END_WORD

WORD(VisionCycle)
CODE(131153)  // capslock + q
virtual void execute() {
  pushWord("goClassifyBlueBoxes"); 
  pushWord("goFindBlueBoxes"); 
  pushCopies("density", 4); 
  pushCopies("resetTemporalMap", 1); 
  pushCopies("density", 1); 
}
END_WORD

WORD(Density)
CODE(131121)     // capslock + 1
virtual void execute() {
  goCalculateDensity();
  goCalculateObjectness();
}
END_WORD


WORD(ResetTemporalMap)
CODE(1179737) // capslock + numlock + y
virtual void execute() {
  if (temporalDensity != NULL && preDensity != NULL) {
    //cout << "preDensity<<<<***" << endl;
    Size sz = objectViewerImage.size();
    int imW = sz.width;
    int imH = sz.height;
    for (int x = 0; x < imW; x++) {
      for (int y = 0; y < imH; y++) {
        temporalDensity[y*imW+x] = preDensity[y*imW+x];
      }
    }
  }
}
END_WORD

WORD(GoFindBlueBoxes)
CODE(131122) // capslock + 2
virtual void execute() {
  goFindBlueBoxes();
}
END_WORD


WORD(GoClassifyBlueBoxes)
CODE(131123) // capslock + 3
virtual void execute() {
  lastVisionCycle = ros::Time::now();
  oscilStart = ros::Time::now();
  goClassifyBlueBoxes();
}
END_WORD