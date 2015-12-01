#include "ein_words.h"
#include "ein.h"

#include "qtgui/einwindow.h"

#include "ein_aibo.h"

/*

Using MS_URBI pink memory stick image downloaded from 

https://github.com/Diamg/AiboRosPackages/tree/master/MS_URBI

cloned from

https://github.com/Diamg/AiboRosPackages.git

Looked briefly for the source since we can cross compile for Aibo.
The image is an URBI server for the dog. 

Wrapped some URBI in Back. 

XXX be careful, some of these actions can cause Aibo to hit the ground
harder than I would allow...

///////////// Start Original forward to motion.u ///////////////////////

Contributors: * Laboratory of Robotics in Versailles (original walk)
                  (if you have other walks, contact us)
              * Matthieu Nottale (Fourier extraction)
              * Assif Mirza (incremental turn)
              * Frederic Lettelier (adaptation of the inc. turn for walk)

Functions:
  robot.walk(duration) : walk for duration milliseconds. 
                         => Walk backward if duration is negative
  robot.stopwalk()     : interrupt the walk
  robot.turn(duration) : turn couterclockwise for duration milliseconds.
                         => Turn clockwise if duration is negative
  robot.stopturn()     : interrupt the turn

  robot.initial()   : initial position sitting down (strech and lay)
  robot.stretch()   : stretching like in the morning...
  robot.lay()       : laying (sitting down)
  robot.sit()       : sit on the back
  robot.beg()       : stand up with knees bent
  robot.stand()     : stand up

Variables:
  robot.walkspeed      : speed of walk, the smaller the faster: 
                         period of a step. Defaults to 1s
  robot.turnspeed      : speed of turn, the smaller the faster:
                         period of a step. Defaults to 1s

///////////// End   Original forward to motion.u ///////////////////////

///////////// Start Original forward to swtrans.u ///////////////////////
##############################################################
#                  AiboUrbi Postures                         #
#                                                            #
#                                                            #
#  Postures of the robot are set with                        #
#                                        robot.StandUp()     #
#                                        robot.SitDown()     #
#                                        robot.LayDown()     #
#                                                            #
##############################################################
///////////// End   Original forward to swtrans.u ///////////////////////

We open a socket for Ein to talk to the URBI server over WiFi.  WiFi must be WirelessB. 

You can send URBI directly or use the Back wrappers. 

////////////////////
///// To use WiFi you need to set up a MS_URBI_TEST/OPEN-R/SYSTEM/CONF/WLANCONF.TXT
///////
#
# WLAN
#
HOSTNAME=peregrin
ESSID=tinesWorld
WEPENABLE=1
WEPKEY=0xAD4278E4A4
# 0=ad-hoc ; 1=infrastructure ; 2=inf || ad
APMODE=1
CHANNEL=8

#
# IP network
#
USE_DHCP=1
#ETHER_IP=192.168.0.2
#ETHER_NETMASK=255.255.255.0
#IP_GATEWAY=192.168.0.1
#DNS_SERVER_1=192.168.0.1
#DNS_DEFDNAME=example.net

#
# SSDP - for UPnP
#
SSDP_ENABLE=1
///////

*/

#define DOG_READ_VAR(x) \
  nextCoreMessage = findStringInDogBuffer(ms, this_dog, string("] "), nextCoreMessage); \
  r = stod(&(ms->pack[this_dog].aibo_sock_buf[nextCoreMessage]), &idx);  \
  x = r; \
  cout << "dgbi got: " << r << " for " << #x << endl; 

void dogReconnect(std::shared_ptr<MachineState> ms) {
  ms->pack[ms->focusedMember].aibo_socket_did_connect = 0;
  ms->pack[ms->focusedMember].dog_needs_reinit = 1;
  close(ms->pack[ms->focusedMember].aibo_socket_desc);
  ms->evaluateProgram("dogFocusedIP 54000 socketOpen ( endStackCollapseNoop dogSocketDidConnect not ) ( dogFocusedIP 54000 socketOpen ) while 15.0 waitForSeconds dogMotorsOn 1.0 waitForSeconds");
}

void sendOnDogSocket(std::shared_ptr<MachineState> ms, int member, string message) {
  int p_send_poll_timeout = 5000;
  struct pollfd fd = { ms->pack[member].aibo_socket_desc, POLLOUT, 0 };
  if (poll(&fd, 1, p_send_poll_timeout) != 1) {
    ROS_ERROR_STREAM("poll says unavailable for sending after " << p_send_poll_timeout << " ms" << endl);
    dogReconnect(ms);
    return;
  } else {
    if( send(ms->pack[member].aibo_socket_desc, message.c_str(), message.size(), 0) != message.size() ) {
      ROS_ERROR_STREAM("send failed..." << endl);
      dogReconnect(ms);
      return;
    } else {
      cout << "sent: " << endl << message << endl;
    }
  }
}

void flushDogBuffer(std::shared_ptr<MachineState> ms, int member) {
  int p_flush_wait_milliseconds = 250;

  int read_size = 1;
  while (read_size > 0) {
    // return if not ready to read.
    struct pollfd fd;
    int ret;

    fd.fd = ms->pack[member].aibo_socket_desc;  
    fd.events = POLLIN;
    ret = poll(&fd, 1, p_flush_wait_milliseconds);  
    switch (ret) {
      case -1:
	// Error
	cout << "file descriptor error during poll in flush..." << endl;
	read_size = -1;
	break;
      case 0:
	// Timeout 
	cout << "timeout during poll in flush..." << endl;
	read_size = 0;
	break;
      default:
	// read from dog
	read_size = read(ms->pack[member].aibo_socket_desc, ms->pack[member].aibo_sock_buf, ms->pack[member].aibo_sock_buf_size);
	if (read_size <= 0) {
	  cout << "oops, read failed, closing socket..." << endl;
	  dogReconnect(ms);
	  return;
	} else {
	  cout << "read " << read_size << " bytes during poll in flush..." << endl;
	  break;
	}
    }
  }
}

int getBytesFromDog(std::shared_ptr<MachineState> ms, int member, int bytesToGet, int timeout) {
  int read_size = 1;
  ms->pack[member].aibo_sock_buf_valid_bytes = 0;
  int sbStart = ms->pack[member].aibo_sock_buf_valid_bytes;

  while ( (read_size > 0) && (sbStart < bytesToGet) ){
    // return if not ready to read.
    struct pollfd fd;
    int ret;

    fd.fd = ms->pack[member].aibo_socket_desc;  
    fd.events = POLLIN;
    ret = poll(&fd, 1, timeout);  
    switch (ret) {
      case -1:
	// Error
	cout << "file descriptor error during poll in gbfd..." << endl;
	read_size = -1;
	break;
      case 0:
	// Timeout 
	cout << "timeout during poll in gbfd..." << endl;
	read_size = 0;
	break;
      default:
	// read from dog
	sbStart = ms->pack[member].aibo_sock_buf_valid_bytes;
	read_size = read(ms->pack[member].aibo_socket_desc, &(ms->pack[member].aibo_sock_buf[sbStart]), ms->pack[member].aibo_sock_buf_size-sbStart);
	if (read_size <= 0) {
	  cout << "oops, read failed, closing socket..." << endl;
	  dogReconnect(ms);
	  return ms->pack[member].aibo_sock_buf_valid_bytes;
	} else {
	  ms->pack[member].aibo_sock_buf_valid_bytes += read_size;
	}
	sbStart = ms->pack[member].aibo_sock_buf_valid_bytes;
	cout << "read " << read_size << " bytes during poll in gbfd..." << endl;
	break;
    }
  }
  return ms->pack[member].aibo_sock_buf_valid_bytes;
}

// returns the byte after the searched string
int readBytesFromDogUntilString(std::shared_ptr<MachineState> ms, int member, int timeout, string toFind) {
  int read_size = 1;
  ms->pack[member].aibo_sock_buf_valid_bytes = 0;
  int sbStart = ms->pack[member].aibo_sock_buf_valid_bytes;

  int searchedBytes = 0;

  while ( (read_size > 0) ){

    for (; searchedBytes < (ms->pack[member].aibo_sock_buf_valid_bytes - int(toFind.size()) + 1); searchedBytes++) {
      //cout << searchedBytes << " " << ms->pack[member].aibo_sock_buf[searchedBytes] << " " << ms->pack[member].aibo_sock_buf_valid_bytes << " " << toFind.size() << " " << (ms->pack[member].aibo_sock_buf_valid_bytes - int(toFind.size()) + 1) << endl;
      if (toFind.compare(0, toFind.size(), &(ms->pack[member].aibo_sock_buf[searchedBytes]), toFind.size()) == 0) {
	searchedBytes += toFind.size();
	cout << "readBytesFromDogUntilString found " << toFind << " at " << searchedBytes << endl;
	return searchedBytes;
      } else {
      }
    }

    // return if not ready to read.
    struct pollfd fd;
    int ret;

    fd.fd = ms->pack[member].aibo_socket_desc;  
    fd.events = POLLIN;
    ret = poll(&fd, 1, timeout);  
    switch (ret) {
      case -1:
	// Error
	cout << "file descriptor error during poll in rbfdus..." << endl;
	read_size = -1;
	break;
      case 0:
	// Timeout 
	cout << "timeout during poll in rbfdus..." << endl;
	read_size = 0;
	break;
      default:
	// read from dog
	sbStart = ms->pack[member].aibo_sock_buf_valid_bytes;
	read_size = read(ms->pack[member].aibo_socket_desc, &(ms->pack[member].aibo_sock_buf[sbStart]), ms->pack[member].aibo_sock_buf_size-sbStart);
	if (read_size <= 0) {
	  cout << "oops, read failed, closing socket..." << endl;
	  dogReconnect(ms);
	  return -1;
	} else {
	  ms->pack[member].aibo_sock_buf_valid_bytes += read_size;
	}
	cout << "read " << read_size << " bytes during poll in rbfdus..." << endl;
	break;
    }
  }
  return -1;
}

// returns the byte after the searched string
int findStringInDogBuffer(std::shared_ptr<MachineState> ms, int member, string toFind, int start) {
  int searchedBytes = 0;
  for (searchedBytes = start; searchedBytes < (ms->pack[member].aibo_sock_buf_valid_bytes - int(toFind.size()) + 1); searchedBytes++) {
    if (toFind.compare(0, toFind.size(), &(ms->pack[member].aibo_sock_buf[searchedBytes]), toFind.size()) == 0) {
      searchedBytes += toFind.size();
      cout << "findStringInDogBuffer found " << toFind << " at " << searchedBytes << endl;
      return searchedBytes;
    } else {
    }
  }

  cout << "findStringInDogBuffer failed to find " << toFind << " at " << searchedBytes << endl;
  string searchedText(ms->pack[member].aibo_sock_buf, searchedBytes);
  cout << "  searched: " << endl << searchedText << endl;

  return -1;
}



// XXX REQUIRE_FOCUSED_DOG macro

namespace ein_words {

WORD(DogSendPack)
virtual void execute(std::shared_ptr<MachineState> ms) {
  string message;
  GET_STRING_ARG(ms, message);

  for (int m = 0; m < ms->pack.size(); m++) {
    sendOnDogSocket(ms, m, message);
  }
}
END_WORD
REGISTER_WORD(DogSendPack)

WORD(DogDoPack)
virtual void execute(std::shared_ptr<MachineState> ms) {
  shared_ptr<Word> w1;
  GET_WORD_ARG(ms, Word, w1);

  for (int m = 0; m < ms->pack.size(); m++) {
    ms->pushWord(w1);
    ms->pushWord("dogIncrementFocusedMember");
  }
}
END_WORD
REGISTER_WORD(DogDoPack)

WORD(DogStop)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  string message("stopall;");
  sendOnDogSocket(ms, this_dog, message);
}
END_WORD
REGISTER_WORD(DogStop)

WORD(DogIncrementFocusedMember)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->focusedMember = (ms->focusedMember + 1) % ms->pack.size();
}
END_WORD
REGISTER_WORD(DogIncrementFocusedMember)

WORD(DogDecrementPackMember)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->focusedMember = (ms->focusedMember - 1) % ms->pack.size();
}
END_WORD
REGISTER_WORD(DogDecrementPackMember)


WORD(DogSetPackSize)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int toSet = 0;
  GET_INT_ARG(ms, toSet);

  if (toSet >= 0) {
    cout << "dogSetPackSize setting pack size: " << toSet << endl;
    ms->pack.resize(toSet);
  } else {
    cout << "dogSetPackSize: invalid size..." << endl;
  }
}
END_WORD
REGISTER_WORD(DogSetPackSize)

WORD(DogDispersePack)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->pack.resize(2);
  for (int m = 0; m < ms->pack.size(); m++) {
    stringstream ss;
    ss << m << " dogSetFocusedMember socketClose";
    ms->evaluateProgram(ss.str());
  }
}
END_WORD
REGISTER_WORD(DogDispersePack)

WORD(DogSetFocusedMember)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int toSet = 0;
  GET_INT_ARG(ms, toSet);
  if ((toSet >= 0) && (toSet < ms->pack.size())) {
    ms->focusedMember = toSet;
    cout << "dogSetFocusedMember focusedMember: " << ms->focusedMember << endl;
  } else {
    cout << "dogSetFocusedMember invalid selection: " << toSet << endl;
  }
}
END_WORD
REGISTER_WORD(DogSetFocusedMember)

WORD(DogFocusedMember)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->pushWord(make_shared<IntegerWord>(ms->focusedMember));
}
END_WORD
REGISTER_WORD(DogFocusedMember)

WORD(DogBark)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("socketSend");
  ms->pushWord(make_shared<StringWord>("speaker.play(\"bark.wav\");"));
}
END_WORD
REGISTER_WORD(DogBark)

WORD(DogFocusedIP)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->pushWord(make_shared<StringWord>(ms->pack[ms->focusedMember].ip_string));
}
END_WORD
REGISTER_WORD(DogFocusedIP)

WORD(DogNeedsReinit)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->pushWord(make_shared<IntegerWord>(ms->pack[ms->focusedMember].dog_needs_reinit));
}
END_WORD
REGISTER_WORD(DogNeedsReinit)

WORD(DogSignalReinitDone)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->pack[ms->focusedMember].dog_needs_reinit = 0;
}
END_WORD
REGISTER_WORD(DogSignalReinitDone)

WORD(SocketOpen)
virtual void execute(std::shared_ptr<MachineState> ms) {
  signal(SIGCHLD,SIG_IGN);
  string t_ip;
  int t_port = 0;

  GET_INT_ARG(ms, t_port);
  GET_STRING_ARG(ms, t_ip);

  cout << "opening socket to IP, port: " << t_ip << " " << t_port << endl;

  // destroy old socket
  close(ms->pack[ms->focusedMember].aibo_socket_desc);
  // create socket
  ms->pack[ms->focusedMember].aibo_socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (ms->pack[ms->focusedMember].aibo_socket_desc == -1) {
    cout <<("could not create socket");
    return;
  }

  cout << "created socket..." << endl;
       
  ms->pack[ms->focusedMember].ip_string = t_ip;
  ms->pack[ms->focusedMember].aibo_server.sin_addr.s_addr = inet_addr(t_ip.c_str());
  ms->pack[ms->focusedMember].aibo_server.sin_family = AF_INET;
  ms->pack[ms->focusedMember].aibo_server.sin_port = htons(t_port);

  //Connect to remote aibo_server
  if (connect(ms->pack[ms->focusedMember].aibo_socket_desc , (struct sockaddr *)&ms->pack[ms->focusedMember].aibo_server , sizeof(ms->pack[ms->focusedMember].aibo_server)) < 0) {
    cout << "connect error" << endl;
    ms->pack[ms->focusedMember].aibo_socket_did_connect = 0;
    ms->pack[ms->focusedMember].dog_needs_reinit = 1;
    close(ms->pack[ms->focusedMember].aibo_socket_desc);
    return;
  } else {
    ms->pack[ms->focusedMember].aibo_socket_did_connect = 1;
  }
   
  cout << "connected!" << endl;
}
END_WORD
REGISTER_WORD(SocketOpen)


WORD(DogSocketDidConnect)
virtual void execute(std::shared_ptr<MachineState> ms) {
  shared_ptr<IntegerWord> word = std::make_shared<IntegerWord>(ms->pack[ms->focusedMember].aibo_socket_did_connect);
  ms->pushWord(word);
}
END_WORD
REGISTER_WORD(DogSocketDidConnect)

WORD(SocketClose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  // destroy old socket
  close(ms->pack[ms->focusedMember].aibo_socket_desc);
  // XXX whick?
  //close(ms->pack[ms->focusedMember].aibo_socket_desc);
}
END_WORD
REGISTER_WORD(SocketClose)

WORD(SocketSend)
virtual void execute(std::shared_ptr<MachineState> ms) {
  // send to dog
  string message;
  GET_STRING_ARG(ms, message);

  sendOnDogSocket(ms, ms->focusedMember, message);
}
END_WORD
REGISTER_WORD(SocketSend)

WORD(SocketRead)
virtual void execute(std::shared_ptr<MachineState> ms) {
  // return if not ready to read.
  // 1 millisecond timeout
  struct pollfd fd;
  int ret;

  fd.fd = ms->pack[ms->focusedMember].aibo_socket_desc;  
  fd.events = POLLIN;
  ret = poll(&fd, 1, 1);  
  switch (ret) {
    case -1:
      // Error
      cout << "file descriptor error during poll..." << endl;
      return;
    case 0:
      // Timeout 
      cout << "timeout during poll..." << endl;
      return;
    default:
      break;
  }

  // read from dog
  int read_size = read(ms->pack[ms->focusedMember].aibo_socket_desc, ms->pack[ms->focusedMember].aibo_sock_buf, ms->pack[ms->focusedMember].aibo_sock_buf_size);
  if (read_size <= 0) {
    cout << "oops, read failed, closing socket..." << endl;
    dogReconnect(ms);
    return;
  }

  ms->pack[ms->focusedMember].aibo_sock_buf[read_size] = '\0';

  string message(ms->pack[ms->focusedMember].aibo_sock_buf);

  cout << "socketRead read " << read_size << " bytes and contained: " << endl << message << endl;
}
END_WORD
REGISTER_WORD(SocketRead)


WORD(DogWhistle)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"192.168.1.127\" 54000 socketOpen");
}
END_WORD
REGISTER_WORD(DogWhistle)

WORD(DogMotorsOn)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"motors on;\" socketSend");
}
END_WORD
REGISTER_WORD(DogMotorsOn)

WORD(DogMotorsOff)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"motors off;\" socketSend");
}
END_WORD
REGISTER_WORD(DogMotorsOff)

//// start swtrans.u wrappers

WORD(DogLayDown)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.LayDown();\" socketSend");
}
END_WORD
REGISTER_WORD(DogLayDown)

WORD(DogStandUp)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.StandUp();\" socketSend");
}
END_WORD
REGISTER_WORD(DogStandUp)

WORD(DogSitDown)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.SitDown();\" socketSend");
}
END_WORD
REGISTER_WORD(DogSitDown)

//// end swtrans.u wrappers


/*
  robot.walk(duration) : walk for duration milliseconds. 
                         => Walk backward if duration is negative
  robot.stopwalk()     : interrupt the walk
  robot.turn(duration) : turn couterclockwise for duration milliseconds.
                         => Turn clockwise if duration is negative
  robot.stopturn()     : interrupt the turn

  robot.initial()   : initial position sitting down (strech and lay)
  robot.stretch()   : stretching like in the morning...
  robot.lay()       : laying (sitting down)
  robot.sit()       : sit on the back
  robot.beg()       : stand up with knees bent
  robot.stand()     : stand up

Variables:
  robot.walkspeed      : speed of walk, the smaller the faster: 
                         period of a step. Defaults to 1s
  robot.turnspeed      : speed of turn, the smaller the faster:
                         period of a step. Defaults to 1s

*/

//// start motion.u wrappers

WORD(DogWalkSeconds)
virtual void execute(std::shared_ptr<MachineState> ms) {
  // specify in seconds
  double duration = 0.0;
  GET_NUMERIC_ARG(ms, duration);
  
  stringstream ss;
  ss << "\"robot.walk(" << duration*1000 << ");\" socketSend";
  ms->evaluateProgram(ss.str());
}
END_WORD
REGISTER_WORD(DogWalkSeconds)

WORD(DogTurnSeconds)
virtual void execute(std::shared_ptr<MachineState> ms) {
  // specify in seconds
  double duration = 0.0;
  GET_NUMERIC_ARG(ms, duration);
  
  stringstream ss;
  ss << "\"robot.turn(" << duration*1000 << ");\" socketSend";
  ms->evaluateProgram(ss.str());
}
END_WORD
REGISTER_WORD(DogTurnSeconds)

WORD(DogWalkMeters)
virtual void execute(std::shared_ptr<MachineState> ms) {
  // specify in meters measured with robot.walkspeed = 1s
  // this is not meant to be very accurate
  double meters = 0.0;
  GET_NUMERIC_ARG(ms, meters);
  
  stringstream ss;
  ss << "\"robot.walk(" << meters*12*1000 << ");\" socketSend";
  ms->evaluateProgram(ss.str());
}
END_WORD
REGISTER_WORD(DogWalkMeters)

WORD(DogTurnRadians)
virtual void execute(std::shared_ptr<MachineState> ms) {
  // specify in meters measured with robot.turnspeed = 1s
  double radians = 0.0;
  GET_NUMERIC_ARG(ms, radians);
  
  stringstream ss;
  // ten seconds turns it all the way around
  ss << "\"robot.turn(" << radians / (2 * 3.1415926) * 10 * 1000 << ");\" socketSend";
  ms->evaluateProgram(ss.str());
}
END_WORD
REGISTER_WORD(DogTurnRadians)

// this might not work...
WORD(DogStopWalkTurn)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.stopturn(), robot.stopwalk();\" socketSend");
}
END_WORD
REGISTER_WORD(DogStopWalkTurn)

WORD(DogInitial)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.initial();\" socketSend");
}
END_WORD
REGISTER_WORD(DogInitial)

WORD(DogStretch)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.stretch();\" socketSend");
}
END_WORD
REGISTER_WORD(DogStretch)

WORD(DogLay)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.lay();\" socketSend");
}
END_WORD
REGISTER_WORD(DogLay)

WORD(DogSit)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.sit();\" socketSend");
}
END_WORD
REGISTER_WORD(DogSit)

WORD(DogBeg)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.beg();\" socketSend");
}
END_WORD
REGISTER_WORD(DogBeg)

WORD(DogStand)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.stand();\" socketSend");
}
END_WORD
REGISTER_WORD(DogStand)

WORD(DogSetTurnSpeed)
virtual void execute(std::shared_ptr<MachineState> ms) {
  // specify the period in seconds
  double seconds = 0.0;
  GET_NUMERIC_ARG(ms, seconds);
  
  stringstream ss;
  ss << "\"robot.turnspeed = " << seconds  << ";\" socketSend";
  ms->evaluateProgram(ss.str());
}
END_WORD
REGISTER_WORD(DogSetTurnSpeed)

WORD(DogSetWalkSpeed)
virtual void execute(std::shared_ptr<MachineState> ms) {
  // specify the period in seconds
  double seconds = 0.0;
  GET_NUMERIC_ARG(ms, seconds);
  
  stringstream ss;
  ss << "\"robot.walkspeed = " << seconds  << ";\" socketSend";
  ms->evaluateProgram(ss.str());
}
END_WORD
REGISTER_WORD(DogSetWalkSpeed)

//// end motion.u wrappers

WORD(DogFormatImageDefault)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;
  ros::NodeHandle n("~");
  stringstream ss;
  ss << "dog_" << ms->focusedMember << "_snout"; 
  ms->pack[this_dog].aibo_snout_pub = n.advertise<sensor_msgs::Image>(ss.str(),10);

  ms->evaluateProgram("\"camera.format = 0; camera.reconstruct = 0; camera.resolution = 0;\" socketSend 0.5 waitForSeconds");
}
END_WORD
REGISTER_WORD(DogFormatImageDefault)

// XXX this should be redone to detect and handle format using new dog read methods
WORD(DogGetImage)
virtual void execute(std::shared_ptr<MachineState> ms) {

  int this_dog = ms->focusedMember;

  int p_get_image_timeout = 1000;

  // read until there is nothing
  flushDogBuffer(ms, this_dog); 

  // request the image
  string image_request("camera.val;");
  sendOnDogSocket(ms, this_dog, image_request);

  //usleep(400*1000);

  // get specs, verify format, ready image
  int rows = 160;
  int cols = 208;
  ms->pack[this_dog].snoutImage = Mat(rows, cols, CV_8UC3);

  int yCbCr208x160MessageLength = 99875;
  int chosenFormatLength = yCbCr208x160MessageLength;

  int dogGottenBytes = getBytesFromDog(ms, this_dog, chosenFormatLength, p_get_image_timeout);
  if (dogGottenBytes == chosenFormatLength) {
    cout << "dogGetImage: got " << dogGottenBytes << endl;
    // start immediately before the good stuff
    int dataIdx = chosenFormatLength - (rows * cols * 3);
    for (int y = 0; y < rows; y++) {
      for (int x = 0; x < cols; x++) {
	// dog is in YCbCr and opencv uses YCrCb
	ms->pack[this_dog].snoutImage.at<Vec3b>(y,x)[0] = ms->pack[this_dog].aibo_sock_buf[dataIdx];
	//cout << int( ms->pack[this_dog].snoutImage.at<Vec3b>(y,x)[0] ) << " ";
	dataIdx++;
	ms->pack[this_dog].snoutImage.at<Vec3b>(y,x)[2] = ms->pack[this_dog].aibo_sock_buf[dataIdx];
	//cout << int( ms->pack[this_dog].snoutImage.at<Vec3b>(y,x)[1] ) << " ";
	dataIdx++;
	ms->pack[this_dog].snoutImage.at<Vec3b>(y,x)[1] = ms->pack[this_dog].aibo_sock_buf[dataIdx];
	//cout << int( ms->pack[this_dog].snoutImage.at<Vec3b>(y,x)[2] ) << " ";
	dataIdx++;
      }
    }

    // convert YCrCb explicity to BGR
    cvtColor(ms->pack[this_dog].snoutImage, ms->pack[this_dog].snoutImage, CV_YCrCb2BGR);

    // for now this is done for display purposes only
    Size p_toBecome(640, 400);
    cv::resize(ms->pack[this_dog].snoutImage, ms->pack[this_dog].snoutImage, p_toBecome);

    ms->config.dogSnoutViewWindow->updateImage(ms->pack[this_dog].snoutImage);
  } else {
    ROS_ERROR_STREAM("dogGetImage: Failed to get dog image... got " << dogGottenBytes << " bytes." << endl);
  }
}
END_WORD
REGISTER_WORD(DogGetImage)

WORD(DogPublishSnout)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;
  {
    sensor_msgs::Image msg;
    Mat topub = ms->pack[this_dog].snoutImage;

    msg.header.stamp = ros::Time::now();
    msg.width = topub.cols;
    msg.height = topub.rows;
    msg.step = topub.cols * topub.elemSize();
    msg.is_bigendian = false;
    msg.encoding = sensor_msgs::image_encodings::RGB8;
    msg.data.assign(topub.data, topub.data + size_t(topub.rows * msg.step));

    ms->pack[this_dog].aibo_snout_pub.publish(msg);
  }
}
END_WORD
REGISTER_WORD(DogPublishSnout)

WORD(DogGetSensoryMotorStates)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  // read until there is nothing
  flushDogBuffer(ms, this_dog); 

  // request the info 
  string request("dgsms +begin: legLF1.val; legLF2.val; legLF3.val; legLH1.val; legLH2.val; legLH3.val; legRH1.val; legRH2.val; legRH3.val; legRF1.val; legRF2.val; legRF3.val; neck.val; headPan.val; headTilt.val; tailPan.val; tailTilt.val; mouth.val; legLF1.PGain; legLF2.PGain; legLF3.PGain; legLH1.PGain; legLH2.PGain; legLH3.PGain; legRH1.PGain; legRH2.PGain; legRH3.PGain; legRF1.PGain; legRF2.PGain; legRF3.PGain; neck.PGain; headPan.PGain; headTilt.PGain; tailPan.PGain; tailTilt.PGain; mouth.PGain; legLF1.IGain; legLF2.IGain; legLF3.IGain; legLH1.IGain; legLH2.IGain; legLH3.IGain; legRH1.IGain; legRH2.IGain; legRH3.IGain; legRF1.IGain; legRF2.IGain; legRF3.IGain; neck.IGain; headPan.IGain; headTilt.IGain; tailPan.IGain; tailTilt.IGain; mouth.IGain; legLF1.DGain; legLF2.DGain; legLF3.DGain; legLH1.DGain; legLH2.DGain; legLH3.DGain; legRH1.DGain; legRH2.DGain; legRH3.DGain; legRF1.DGain; legRF2.DGain; legRF3.DGain; neck.DGain; headPan.DGain; headTilt.DGain; tailPan.DGain; tailTilt.DGain; mouth.DGain; distanceNear.val; distance.val; distanceChest.val; accelX.val; accelY.val; accelZ.val; pawLF.val; pawLH.val; pawRF.val; pawRH.val; chinSensor.val; headSensor.val; backSensorR.val; backSensorM.val; dgsms +end: backSensorF.val;");

  sendOnDogSocket(ms, this_dog, request);

  // read until all info is in
  readBytesFromDogUntilString(ms, this_dog, 5000, string(":dgsms] *** end\n"));

  int startCoreMessage = findStringInDogBuffer(ms, this_dog, string(":dgsms] *** begin\n"), 0);
  cout << ms->pack[this_dog].aibo_sock_buf[startCoreMessage] << endl;

  size_t idx;
  int nextCoreMessage = startCoreMessage; 
  double r = 0.0;

  DOG_READ_VAR(ms->pack[this_dog].truePose.legLF1);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legLF2);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legLF3);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legLH1);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legLH2);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legLH3);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legRH1);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legRH2);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legRH3);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legRF1);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legRF2);
  DOG_READ_VAR(ms->pack[this_dog].truePose.legRF3);
  DOG_READ_VAR(ms->pack[this_dog].truePose.neck);
  DOG_READ_VAR(ms->pack[this_dog].truePose.headPan);
  DOG_READ_VAR(ms->pack[this_dog].truePose.headTilt);
  DOG_READ_VAR(ms->pack[this_dog].truePose.tailPan);
  DOG_READ_VAR(ms->pack[this_dog].truePose.tailTilt);
  DOG_READ_VAR(ms->pack[this_dog].truePose.mouth);

  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legLF1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legLF2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legLF3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legLH1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legLH2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legLH3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legRH1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legRH2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legRH3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legRF1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legRF2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].legRF3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].neck);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].headPan);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].headTilt);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].tailPan);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].tailTilt);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[0].mouth);

  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legLF1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legLF2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legLF3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legLH1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legLH2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legLH3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legRH1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legRH2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legRH3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legRF1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legRF2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].legRF3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].neck);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].headPan);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].headTilt);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].tailPan);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].tailTilt);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[1].mouth);

  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legLF1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legLF2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legLF3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legLH1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legLH2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legLH3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legRH1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legRH2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legRH3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legRF1);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legRF2);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].legRF3);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].neck);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].headPan);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].headTilt);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].tailPan);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].tailTilt);
  DOG_READ_VAR(ms->pack[this_dog].trueGain[2].mouth);

  DOG_READ_VAR(ms->pack[this_dog].trueSensors.distanceNearSnout);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.distanceFarSnout);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.distanceChest);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.accelerometer[0]);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.accelerometer[1]);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.accelerometer[2]);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.pawLF);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.pawLH);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.pawRF);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.pawRH);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.chinSensor);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.headTouch);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.backTouchR);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.backTouchM);
  DOG_READ_VAR(ms->pack[this_dog].trueSensors.backTouchF);

  cout << "dogGetSensoryMotorStates: finished" << endl;
}
END_WORD
REGISTER_WORD(DogGetSensoryMotorStates)

WORD(DogGetIndicators)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  // read until there is nothing
  flushDogBuffer(ms, this_dog); 

  // request the info 
  string request("dgbi +begin:  ledBFC.val; ledBFW.val; ledBMC.val; ledBMW.val; ledBRC.val; ledBRW.val; ledF1.val; ledF2.val; ledF3.val; ledF4.val; ledF5.val; ledF6.val; ledF7.val; ledF8.val; ledF9.val; ledF10.val; ledF11.val; ledF12.val; ledF13.val; ledF14.val; ledHC.val; modeR.val; modeG.val; modeB.val; earL.val; dgbi +end: earR.val;");
  sendOnDogSocket(ms, this_dog, request);

  // read until all info is in
  readBytesFromDogUntilString(ms, this_dog, 250, string(":dgbi] *** end\n"));

  int startCoreMessage = findStringInDogBuffer(ms, this_dog, string(":dgbi] *** begin\n"), 0);
  cout << ms->pack[this_dog].aibo_sock_buf[startCoreMessage] << endl;

  size_t idx;
  int nextCoreMessage = startCoreMessage; 
  double r = 0.0;

  //cout << ms->pack[this_dog].aibo_sock_buf[nextCoreMessage] << endl;
  //cout << "dgbi got: " << r << endl;

  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledBFC);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledBFW);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledBMC);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledBMW);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledBRC);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledBRW);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF1);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF2);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF3);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF4);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF5);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF6);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF7);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF8);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF9);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF10);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF11);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF12);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF13);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledF14);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.ledHC);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.modeR);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.modeG);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.modeB);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.earL);
  DOG_READ_VAR(ms->pack[this_dog].trueIndicators.earR);

  cout << "dogGetIndicators: finished" << endl;
  // parse the info
}
END_WORD
REGISTER_WORD(DogGetIndicators)

#define DOG_SEND_JOINT_VAR(x) << " " << #x << ".val = " << ms->pack[this_dog].intendedPose.x << ","
#define DOG_SEND_JOINT_PGAIN_VAR(x) << " " << #x << ".PGain = " << ms->pack[this_dog].intendedGain[0].x << ","
#define DOG_SEND_JOINT_IGAIN_VAR(x) << " " << #x << ".IGain = " << ms->pack[this_dog].intendedGain[1].x << ","
#define DOG_SEND_JOINT_DGAIN_VAR(x) << " " << #x << ".DGain = " << ms->pack[this_dog].intendedGain[2].x << ","
#define DOG_SEND_INDICATOR_VAR(x) << " " << #x << ".val = " << ms->pack[this_dog].intendedIndicators.x << ","

WORD(DogSendMotorState)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  stringstream ss;
/*
  ss <<
  "legLF1.val = " << ms->pack[this_dog].intendedPose.legLF1 << "; " <<
  "legLF2.val = " << ms->pack[this_dog].intendedPose.legLF2 << "; " <<
  "legLF3.val = " << ms->pack[this_dog].intendedPose.legLF3 << "; " <<
  "legLH1.val = " << ms->pack[this_dog].intendedPose.legLH1 << "; " <<
  "legLH2.val = " << ms->pack[this_dog].intendedPose.legLH2 << "; " <<
  "legLH3.val = " << ms->pack[this_dog].intendedPose.legLH3 << "; " <<
  "legRH1.val = " << ms->pack[this_dog].intendedPose.legRH1 << "; " <<
  "legRH2.val = " << ms->pack[this_dog].intendedPose.legRH2 << "; " <<
  "legRH3.val = " << ms->pack[this_dog].intendedPose.legRH3 << "; " <<
  "legRF1.val = " << ms->pack[this_dog].intendedPose.legRF1 << "; " <<
  "legRF2.val = " << ms->pack[this_dog].intendedPose.legRF2  << "; " <<
  "legRF3.val = " << ms->pack[this_dog].intendedPose.legRF3  << "; " <<
  "neck.val = " << ms->pack[this_dog].intendedPose.neck << "; " <<
  "headPan.val = " << ms->pack[this_dog].intendedPose.headPan << "; " <<
  "headTilt.val = " << ms->pack[this_dog].intendedPose.headTilt << "; " <<
  "tailPan.val = " << ms->pack[this_dog].intendedPose.tailPan << "; " <<
  "tailTilt.val = " << ms->pack[this_dog].intendedPose.tailTilt << "; " <<
  "mouth.val = " << ms->pack[this_dog].intendedPose.mouth << "; "
*/
  ss  
    DOG_SEND_JOINT_VAR(legLF1)
    DOG_SEND_JOINT_VAR(legLF2)
    DOG_SEND_JOINT_VAR(legLF3)
    DOG_SEND_JOINT_VAR(legLH1)
    DOG_SEND_JOINT_VAR(legLH2)
    DOG_SEND_JOINT_VAR(legLH3)
    DOG_SEND_JOINT_VAR(legRH1)
    DOG_SEND_JOINT_VAR(legRH2)
    DOG_SEND_JOINT_VAR(legRH3)
    DOG_SEND_JOINT_VAR(legRF1)
    DOG_SEND_JOINT_VAR(legRF2)
    DOG_SEND_JOINT_VAR(legRF3)
    DOG_SEND_JOINT_VAR(neck)
    DOG_SEND_JOINT_VAR(headPan)
    DOG_SEND_JOINT_VAR(headTilt)
    DOG_SEND_JOINT_VAR(tailPan)
    DOG_SEND_JOINT_VAR(tailTilt)
    DOG_SEND_JOINT_VAR(mouth) 
    DOG_SEND_JOINT_PGAIN_VAR(legLF1)
    DOG_SEND_JOINT_PGAIN_VAR(legLF2)
    DOG_SEND_JOINT_PGAIN_VAR(legLF3)
    DOG_SEND_JOINT_PGAIN_VAR(legLH1)
    DOG_SEND_JOINT_PGAIN_VAR(legLH2)
    DOG_SEND_JOINT_PGAIN_VAR(legLH3)
    DOG_SEND_JOINT_PGAIN_VAR(legRH1)
    DOG_SEND_JOINT_PGAIN_VAR(legRH2)
    DOG_SEND_JOINT_PGAIN_VAR(legRH3)
    DOG_SEND_JOINT_PGAIN_VAR(legRF1)
    DOG_SEND_JOINT_PGAIN_VAR(legRF2)
    DOG_SEND_JOINT_PGAIN_VAR(legRF3)
    DOG_SEND_JOINT_PGAIN_VAR(neck)
    DOG_SEND_JOINT_PGAIN_VAR(headPan)
    DOG_SEND_JOINT_PGAIN_VAR(headTilt)
    DOG_SEND_JOINT_PGAIN_VAR(tailPan)
    DOG_SEND_JOINT_PGAIN_VAR(tailTilt)
    DOG_SEND_JOINT_PGAIN_VAR(mouth) 
    DOG_SEND_JOINT_IGAIN_VAR(legLF1)
    DOG_SEND_JOINT_IGAIN_VAR(legLF2)
    DOG_SEND_JOINT_IGAIN_VAR(legLF3)
    DOG_SEND_JOINT_IGAIN_VAR(legLH1)
    DOG_SEND_JOINT_IGAIN_VAR(legLH2)
    DOG_SEND_JOINT_IGAIN_VAR(legLH3)
    DOG_SEND_JOINT_IGAIN_VAR(legRH1)
    DOG_SEND_JOINT_IGAIN_VAR(legRH2)
    DOG_SEND_JOINT_IGAIN_VAR(legRH3)
    DOG_SEND_JOINT_IGAIN_VAR(legRF1)
    DOG_SEND_JOINT_IGAIN_VAR(legRF2)
    DOG_SEND_JOINT_IGAIN_VAR(legRF3)
    DOG_SEND_JOINT_IGAIN_VAR(neck)
    DOG_SEND_JOINT_IGAIN_VAR(headPan)
    DOG_SEND_JOINT_IGAIN_VAR(headTilt)
    DOG_SEND_JOINT_IGAIN_VAR(tailPan)
    DOG_SEND_JOINT_IGAIN_VAR(tailTilt)
    DOG_SEND_JOINT_IGAIN_VAR(mouth) 
    DOG_SEND_JOINT_DGAIN_VAR(legLF1)
    DOG_SEND_JOINT_DGAIN_VAR(legLF2)
    DOG_SEND_JOINT_DGAIN_VAR(legLF3)
    DOG_SEND_JOINT_DGAIN_VAR(legLH1)
    DOG_SEND_JOINT_DGAIN_VAR(legLH2)
    DOG_SEND_JOINT_DGAIN_VAR(legLH3)
    DOG_SEND_JOINT_DGAIN_VAR(legRH1)
    DOG_SEND_JOINT_DGAIN_VAR(legRH2)
    DOG_SEND_JOINT_DGAIN_VAR(legRH3)
    DOG_SEND_JOINT_DGAIN_VAR(legRF1)
    DOG_SEND_JOINT_DGAIN_VAR(legRF2)
    DOG_SEND_JOINT_DGAIN_VAR(legRF3)
    DOG_SEND_JOINT_DGAIN_VAR(neck)
    DOG_SEND_JOINT_DGAIN_VAR(headPan)
    DOG_SEND_JOINT_DGAIN_VAR(headTilt)
    DOG_SEND_JOINT_DGAIN_VAR(tailPan)
    DOG_SEND_JOINT_DGAIN_VAR(tailTilt)
    DOG_SEND_JOINT_DGAIN_VAR(mouth) 
  ;

  string message = ss.str();
  sendOnDogSocket(ms, this_dog, message);
}
END_WORD
REGISTER_WORD(DogSendMotorState)

WORD(DogSendIndicators)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  stringstream ss;
  ss 
    DOG_SEND_INDICATOR_VAR(ledBFC)
    DOG_SEND_INDICATOR_VAR(ledBFW)
    DOG_SEND_INDICATOR_VAR(ledBMC)
    DOG_SEND_INDICATOR_VAR(ledBMW)
    DOG_SEND_INDICATOR_VAR(ledBRC)
    DOG_SEND_INDICATOR_VAR(ledBRW)
    DOG_SEND_INDICATOR_VAR(ledF1)
    DOG_SEND_INDICATOR_VAR(ledF2)
    DOG_SEND_INDICATOR_VAR(ledF3)
    DOG_SEND_INDICATOR_VAR(ledF4)
    DOG_SEND_INDICATOR_VAR(ledF5)
    DOG_SEND_INDICATOR_VAR(ledF6)
    DOG_SEND_INDICATOR_VAR(ledF7)
    DOG_SEND_INDICATOR_VAR(ledF8)
    DOG_SEND_INDICATOR_VAR(ledF9)
    DOG_SEND_INDICATOR_VAR(ledF10)
    DOG_SEND_INDICATOR_VAR(ledF11)
    DOG_SEND_INDICATOR_VAR(ledF12)
    DOG_SEND_INDICATOR_VAR(ledF13)
    DOG_SEND_INDICATOR_VAR(ledF14)
    DOG_SEND_INDICATOR_VAR(ledHC)
    DOG_SEND_INDICATOR_VAR(modeR)
    DOG_SEND_INDICATOR_VAR(modeG)
    DOG_SEND_INDICATOR_VAR(modeB)
    DOG_SEND_INDICATOR_VAR(earL)
    DOG_SEND_INDICATOR_VAR(earR)
  ;

  string message = ss.str();
  sendOnDogSocket(ms, this_dog, message);
}
END_WORD
REGISTER_WORD(DogSendIndicators)

WORD(DogStay)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;
  ms->pack[this_dog].intendedPose = ms->pack[this_dog].truePose;
  ms->pack[this_dog].intendedGain[0] = ms->pack[this_dog].trueGain[0];
  ms->pack[this_dog].intendedGain[1] = ms->pack[this_dog].trueGain[1];
  ms->pack[this_dog].intendedGain[2] = ms->pack[this_dog].trueGain[2];
}
END_WORD
REGISTER_WORD(DogStay)

WORD(DogPushIntendedPose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  shared_ptr<AiboPoseWord> word = std::make_shared<AiboPoseWord>(ms->pack[this_dog].intendedPose);
  ms->pushWord(word);
}
END_WORD
REGISTER_WORD(DogPushIntendedPose)

WORD(DogPushIntendedGain)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int t_gain= 0;
  GET_INT_ARG(ms, t_gain);

  int this_dog = ms->focusedMember;

  shared_ptr<AiboPoseWord> word = std::make_shared<AiboPoseWord>(ms->pack[this_dog].intendedGain[t_gain]);
  ms->pushWord(word);
}
END_WORD
REGISTER_WORD(DogPushIntendedGain)

WORD(DogPushTrueGain)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int t_gain= 0;
  GET_INT_ARG(ms, t_gain);

  int this_dog = ms->focusedMember;

  shared_ptr<AiboPoseWord> word = std::make_shared<AiboPoseWord>(ms->pack[this_dog].trueGain[t_gain]);
  ms->pushWord(word);
}
END_WORD
REGISTER_WORD(DogPushTrueGain)

WORD(DogCreatePose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  EinAiboJoints toPush;

  GET_NUMERIC_ARG(ms, toPush.mouth);
  GET_NUMERIC_ARG(ms, toPush.tailTilt);
  GET_NUMERIC_ARG(ms, toPush.tailPan);
  GET_NUMERIC_ARG(ms, toPush.headTilt);
  GET_NUMERIC_ARG(ms, toPush.headPan);
  GET_NUMERIC_ARG(ms, toPush.neck);
  GET_NUMERIC_ARG(ms, toPush.legRF3);
  GET_NUMERIC_ARG(ms, toPush.legRF2);
  GET_NUMERIC_ARG(ms, toPush.legRF1);
  GET_NUMERIC_ARG(ms, toPush.legRH3);
  GET_NUMERIC_ARG(ms, toPush.legRH2);
  GET_NUMERIC_ARG(ms, toPush.legRH1);
  GET_NUMERIC_ARG(ms, toPush.legLH3);
  GET_NUMERIC_ARG(ms, toPush.legLH2);
  GET_NUMERIC_ARG(ms, toPush.legLH1);
  GET_NUMERIC_ARG(ms, toPush.legLF3);
  GET_NUMERIC_ARG(ms, toPush.legLF2);
  GET_NUMERIC_ARG(ms, toPush.legLF1);

  shared_ptr<AiboPoseWord> word = std::make_shared<AiboPoseWord>(toPush);
  ms->pushWord(word);
}
END_WORD
REGISTER_WORD(DogCreatePose)

WORD(DogSetIntendedPose)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  shared_ptr<AiboPoseWord> word ;
  GET_WORD_ARG(ms, AiboPoseWord, word);

  ms->pack[this_dog].intendedPose = word->value();
}
END_WORD
REGISTER_WORD(DogSetIntendedPose)

WORD(DogSetIntendedGain)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int t_gain= 0;
  GET_INT_ARG(ms, t_gain);

  shared_ptr<AiboPoseWord> word ;
  GET_WORD_ARG(ms, AiboPoseWord, word);

  int this_dog = ms->focusedMember;

  ms->pack[this_dog].trueGain[t_gain] = word->value();
}
END_WORD
REGISTER_WORD(DogSetIntendedGain)


#define AIBO_POSE_ACCESSOR(J, C, D) \
WORD(AiboPose ## J) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  shared_ptr<AiboPoseWord> word ; \
  GET_WORD_ARG(ms, AiboPoseWord, word); \
  shared_ptr<DoubleWord> vword = make_shared<DoubleWord>(C); \
  ms->pushData(vword); \
} \
END_WORD \
REGISTER_WORD(AiboPose ## J) \
\
WORD(SetAiboPose ## J) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  double value; \
  GET_NUMERIC_ARG(ms, value); \
   \
  shared_ptr<AiboPoseWord> word; \
  GET_WORD_ARG(ms, AiboPoseWord, word); \
   \
  EinAiboJoints newPose = word->value(); \
  D = value; \
 \
  shared_ptr<AiboPoseWord> newWord = make_shared<AiboPoseWord>(newPose); \
 \
  ms->pushData(newWord); \
} \
END_WORD \
REGISTER_WORD(SetAiboPose ## J) \
/*
*/


AIBO_POSE_ACCESSOR(LegLF1  , word->value().legLF1  , newPose.legLF1  )                  
AIBO_POSE_ACCESSOR(LegLF2  , word->value().legLF2  , newPose.legLF2  )                  
AIBO_POSE_ACCESSOR(LegLF3  , word->value().legLF3  , newPose.legLF3  )                  
AIBO_POSE_ACCESSOR(LegLH1  , word->value().legLH1  , newPose.legLH1  )                  
AIBO_POSE_ACCESSOR(LegLH2  , word->value().legLH2  , newPose.legLH2  )                  
AIBO_POSE_ACCESSOR(LegLH3  , word->value().legLH3  , newPose.legLH3  )                  
AIBO_POSE_ACCESSOR(LegRH1  , word->value().legRH1  , newPose.legRH1  )                  
AIBO_POSE_ACCESSOR(LegRH2  , word->value().legRH2  , newPose.legRH2  )                  
AIBO_POSE_ACCESSOR(LegRH3  , word->value().legRH3  , newPose.legRH3  )                  
AIBO_POSE_ACCESSOR(LegRF1  , word->value().legRF1  , newPose.legRF1  )                  
AIBO_POSE_ACCESSOR(LegRF2  , word->value().legRF2  , newPose.legRF2  )                  
AIBO_POSE_ACCESSOR(LegRF3  , word->value().legRF3  , newPose.legRF3  )                  
AIBO_POSE_ACCESSOR(Neck    , word->value().neck    , newPose.neck    )                  
AIBO_POSE_ACCESSOR(HeadPan , word->value().headPan , newPose.headPan )                  
AIBO_POSE_ACCESSOR(HeadTilt, word->value().headTilt, newPose.headTilt)                  
AIBO_POSE_ACCESSOR(TailPan , word->value().tailPan , newPose.tailPan )                  
AIBO_POSE_ACCESSOR(TailTilt, word->value().tailTilt, newPose.tailTilt)                  
AIBO_POSE_ACCESSOR(Mouth   , word->value().mouth   , newPose.mouth   )                  

#define AIBO_POSE_DELTAS(J, C, D) \
WORD(Dog ## J ## Up) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  int this_dog = ms->focusedMember; \
  C += D; \
} \
END_WORD \
REGISTER_WORD(Dog ## J ## Up) \
\
WORD(Dog ## J ## Down) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  int this_dog = ms->focusedMember; \
  C -= D; \
} \
END_WORD \
REGISTER_WORD(Dog ## J ## Down) \
\
WORD(Dog ## J ## By) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  int this_dog = ms->focusedMember; \
\
  double amount = 0.0; \
  GET_NUMERIC_ARG(ms, amount); \
  C += amount; \
} \
END_WORD \
REGISTER_WORD(Dog ## J ## By) \

#define AIBO_POSE_DELTAS_GAIN(J, C, D) \
WORD(Dog ## J ## Up) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  int this_dog = ms->focusedMember; \
  C += D; \
  C = min( max( 0.0, C ), 18.0 ); \
} \
END_WORD \
REGISTER_WORD(Dog ## J ## Up) \
\
WORD(Dog ## J ## Down) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  int this_dog = ms->focusedMember; \
  C -= D; \
  C = min( max( 0.0, C ), 18.0 ); \
} \
END_WORD \
REGISTER_WORD(Dog ## J ## Down) \
\
WORD(Dog ## J ## By) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  int this_dog = ms->focusedMember; \
\
  double amount = 0.0; \
  GET_NUMERIC_ARG(ms, amount); \
  C += amount; \
  C = min( max( 0.0, C ), 18.0 ); \
} \
END_WORD \
REGISTER_WORD(Dog ## J ## By) \


AIBO_POSE_DELTAS(LegLF1, ms->pack[this_dog].intendedPose.legLF1, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegLF2, ms->pack[this_dog].intendedPose.legLF2, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegLF3, ms->pack[this_dog].intendedPose.legLF3, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegLH1, ms->pack[this_dog].intendedPose.legLH1, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegLH2, ms->pack[this_dog].intendedPose.legLH2, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegLH3, ms->pack[this_dog].intendedPose.legLH3, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegRH1, ms->pack[this_dog].intendedPose.legRH1, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegRH2, ms->pack[this_dog].intendedPose.legRH2, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegRH3, ms->pack[this_dog].intendedPose.legRH3, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegRF1, ms->pack[this_dog].intendedPose.legRF1, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegRF2, ms->pack[this_dog].intendedPose.legRF2, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(LegRF3, ms->pack[this_dog].intendedPose.legRF3, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(Neck, ms->pack[this_dog].intendedPose. neck, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(HeadPan, ms->pack[this_dog].intendedPose.headPan, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(HeadTilt, ms->pack[this_dog].intendedPose.headTilt, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(TailPan, ms->pack[this_dog].intendedPose.tailPan, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(TailTilt, ms->pack[this_dog].intendedPose.tailTilt, ms->pack[this_dog].dogPoseGridSize)
AIBO_POSE_DELTAS(Mouth, ms->pack[this_dog].intendedPose.mouth, ms->pack[this_dog].dogPoseGridSize)

AIBO_POSE_DELTAS_GAIN(LegLF1PGain, ms->pack[this_dog].intendedGain[0].legLF1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLF2PGain, ms->pack[this_dog].intendedGain[0].legLF2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLF3PGain, ms->pack[this_dog].intendedGain[0].legLF3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLH1PGain, ms->pack[this_dog].intendedGain[0].legLH1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLH2PGain, ms->pack[this_dog].intendedGain[0].legLH2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLH3PGain, ms->pack[this_dog].intendedGain[0].legLH3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRH1PGain, ms->pack[this_dog].intendedGain[0].legRH1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRH2PGain, ms->pack[this_dog].intendedGain[0].legRH2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRH3PGain, ms->pack[this_dog].intendedGain[0].legRH3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRF1PGain, ms->pack[this_dog].intendedGain[0].legRF1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRF2PGain, ms->pack[this_dog].intendedGain[0].legRF2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRF3PGain, ms->pack[this_dog].intendedGain[0].legRF3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(NeckPGain, ms->pack[this_dog].intendedGain[0]. neck, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(HeadPanPGain, ms->pack[this_dog].intendedGain[0].headPan, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(HeadTiltPGain, ms->pack[this_dog].intendedGain[0].headTilt, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(TailPanPGain, ms->pack[this_dog].intendedGain[0].tailPan, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(TailTiltPGain, ms->pack[this_dog].intendedGain[0].tailTilt, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(MouthPGain, ms->pack[this_dog].intendedGain[0].mouth, ms->pack[this_dog].dogGainGridSize)

AIBO_POSE_DELTAS_GAIN(LegLF1IGain, ms->pack[this_dog].intendedGain[1].legLF1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLF2IGain, ms->pack[this_dog].intendedGain[1].legLF2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLF3IGain, ms->pack[this_dog].intendedGain[1].legLF3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLH1IGain, ms->pack[this_dog].intendedGain[1].legLH1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLH2IGain, ms->pack[this_dog].intendedGain[1].legLH2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLH3IGain, ms->pack[this_dog].intendedGain[1].legLH3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRH1IGain, ms->pack[this_dog].intendedGain[1].legRH1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRH2IGain, ms->pack[this_dog].intendedGain[1].legRH2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRH3IGain, ms->pack[this_dog].intendedGain[1].legRH3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRF1IGain, ms->pack[this_dog].intendedGain[1].legRF1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRF2IGain, ms->pack[this_dog].intendedGain[1].legRF2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRF3IGain, ms->pack[this_dog].intendedGain[1].legRF3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(NeckIGain, ms->pack[this_dog].intendedGain[1]. neck, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(HeadPanIGain, ms->pack[this_dog].intendedGain[1].headPan, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(HeadTiltIGain, ms->pack[this_dog].intendedGain[1].headTilt, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(TailPanIGain, ms->pack[this_dog].intendedGain[1].tailPan, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(TailTiltIGain, ms->pack[this_dog].intendedGain[1].tailTilt, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(MouthIGain, ms->pack[this_dog].intendedGain[1].mouth, ms->pack[this_dog].dogGainGridSize)

AIBO_POSE_DELTAS_GAIN(LegLF1DGain, ms->pack[this_dog].intendedGain[2].legLF1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLF2DGain, ms->pack[this_dog].intendedGain[2].legLF2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLF3DGain, ms->pack[this_dog].intendedGain[2].legLF3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLH1DGain, ms->pack[this_dog].intendedGain[2].legLH1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLH2DGain, ms->pack[this_dog].intendedGain[2].legLH2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegLH3DGain, ms->pack[this_dog].intendedGain[2].legLH3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRH1DGain, ms->pack[this_dog].intendedGain[2].legRH1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRH2DGain, ms->pack[this_dog].intendedGain[2].legRH2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRH3DGain, ms->pack[this_dog].intendedGain[2].legRH3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRF1DGain, ms->pack[this_dog].intendedGain[2].legRF1, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRF2DGain, ms->pack[this_dog].intendedGain[2].legRF2, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(LegRF3DGain, ms->pack[this_dog].intendedGain[2].legRF3, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(NeckDGain, ms->pack[this_dog].intendedGain[2]. neck, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(HeadPanDGain, ms->pack[this_dog].intendedGain[2].headPan, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(HeadTiltDGain, ms->pack[this_dog].intendedGain[2].headTilt, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(TailPanDGain, ms->pack[this_dog].intendedGain[2].tailPan, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(TailTiltDGain, ms->pack[this_dog].intendedGain[2].tailTilt, ms->pack[this_dog].dogGainGridSize)
AIBO_POSE_DELTAS_GAIN(MouthDGain, ms->pack[this_dog].intendedGain[2].mouth, ms->pack[this_dog].dogGainGridSize)

WORD(DogSetPoseGridSize)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  double newgrid = 0.0;
  GET_NUMERIC_ARG(ms, newgrid);
  ms->pack[this_dog].dogPoseGridSize = newgrid;
  cout << "dogSetPoseGridSize: " << ms->pack[this_dog].dogPoseGridSize << endl;
}
END_WORD
REGISTER_WORD(DogSetPoseGridSize)

WORD(DogSetGainGridSize)
virtual void execute(std::shared_ptr<MachineState> ms) {
  int this_dog = ms->focusedMember;

  double newgrid = 0.0;
  GET_NUMERIC_ARG(ms, newgrid);
  ms->pack[this_dog].dogGainGridSize = newgrid;
  cout << "dogSetGainGridSize: " << ms->pack[this_dog].dogGainGridSize << endl;
}
END_WORD
REGISTER_WORD(DogSetGainGridSize)

#define AIBO_INDICATOR_ACCESSORS(J, C) \
WORD(Dog ## J) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  int this_dog = ms->focusedMember; \
  shared_ptr<DoubleWord> vword = make_shared<DoubleWord>(C); \
  ms->pushData(vword); \
} \
END_WORD \
REGISTER_WORD(Dog ## J) \
WORD(DogSet ## J) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  int this_dog = ms->focusedMember; \
  double value; \
  GET_NUMERIC_ARG(ms, value); \
   \
  C = value; \
} \
END_WORD \
REGISTER_WORD(DogSet ## J) \

AIBO_INDICATOR_ACCESSORS(IntendedLedBFC, ms->pack[this_dog].intendedIndicators.ledBFC);
AIBO_INDICATOR_ACCESSORS(IntendedLedBFW, ms->pack[this_dog].intendedIndicators.ledBFW);
AIBO_INDICATOR_ACCESSORS(IntendedLedBMC, ms->pack[this_dog].intendedIndicators.ledBMC);
AIBO_INDICATOR_ACCESSORS(IntendedLedBMW, ms->pack[this_dog].intendedIndicators.ledBMW);
AIBO_INDICATOR_ACCESSORS(IntendedLedBRC, ms->pack[this_dog].intendedIndicators.ledBRC);
AIBO_INDICATOR_ACCESSORS(IntendedLedBRW, ms->pack[this_dog].intendedIndicators.ledBRW);
AIBO_INDICATOR_ACCESSORS(IntendedLedF1, ms->pack[this_dog].intendedIndicators.ledF1);
AIBO_INDICATOR_ACCESSORS(IntendedLedF2, ms->pack[this_dog].intendedIndicators.ledF2);
AIBO_INDICATOR_ACCESSORS(IntendedLedF3, ms->pack[this_dog].intendedIndicators.ledF3);
AIBO_INDICATOR_ACCESSORS(IntendedLedF4, ms->pack[this_dog].intendedIndicators.ledF4);
AIBO_INDICATOR_ACCESSORS(IntendedLedF5, ms->pack[this_dog].intendedIndicators.ledF5);
AIBO_INDICATOR_ACCESSORS(IntendedLedF6, ms->pack[this_dog].intendedIndicators.ledF6);
AIBO_INDICATOR_ACCESSORS(IntendedLedF7, ms->pack[this_dog].intendedIndicators.ledF7);
AIBO_INDICATOR_ACCESSORS(IntendedLedF8, ms->pack[this_dog].intendedIndicators.ledF8);
AIBO_INDICATOR_ACCESSORS(IntendedLedF9, ms->pack[this_dog].intendedIndicators.ledF9);
AIBO_INDICATOR_ACCESSORS(IntendedLedF10, ms->pack[this_dog].intendedIndicators.ledF10);
AIBO_INDICATOR_ACCESSORS(IntendedLedF11, ms->pack[this_dog].intendedIndicators.ledF11);
AIBO_INDICATOR_ACCESSORS(IntendedLedF12, ms->pack[this_dog].intendedIndicators.ledF12);
AIBO_INDICATOR_ACCESSORS(IntendedLedF13, ms->pack[this_dog].intendedIndicators.ledF13);
AIBO_INDICATOR_ACCESSORS(IntendedLedF14, ms->pack[this_dog].intendedIndicators.ledF14);
AIBO_INDICATOR_ACCESSORS(IntendedLedHC, ms->pack[this_dog].intendedIndicators.ledHC);
AIBO_INDICATOR_ACCESSORS(IntendedModeR, ms->pack[this_dog].intendedIndicators.modeR);
AIBO_INDICATOR_ACCESSORS(IntendedModeG, ms->pack[this_dog].intendedIndicators.modeG);
AIBO_INDICATOR_ACCESSORS(IntendedModeB, ms->pack[this_dog].intendedIndicators.modeB);
AIBO_INDICATOR_ACCESSORS(IntendedEarL, ms->pack[this_dog].intendedIndicators.earL);
AIBO_INDICATOR_ACCESSORS(IntendedEarR, ms->pack[this_dog].intendedIndicators.earR);

#define AIBO_SENSOR_ACCESSORS(J, C) \
WORD(Dog ## J) \
virtual void execute(std::shared_ptr<MachineState> ms) { \
  int this_dog = ms->focusedMember; \
  shared_ptr<DoubleWord> vword = make_shared<DoubleWord>(C); \
  ms->pushData(vword); \
} \
END_WORD \
REGISTER_WORD(Dog ## J) \

AIBO_SENSOR_ACCESSORS(TrueLedBFC, ms->pack[this_dog].trueIndicators.ledBFC);
AIBO_SENSOR_ACCESSORS(TrueLedBFW, ms->pack[this_dog].trueIndicators.ledBFW);
AIBO_SENSOR_ACCESSORS(TrueLedBMC, ms->pack[this_dog].trueIndicators.ledBMC);
AIBO_SENSOR_ACCESSORS(TrueLedBMW, ms->pack[this_dog].trueIndicators.ledBMW);
AIBO_SENSOR_ACCESSORS(TrueLedBRC, ms->pack[this_dog].trueIndicators.ledBRC);
AIBO_SENSOR_ACCESSORS(TrueLedBRW, ms->pack[this_dog].trueIndicators.ledBRW);
AIBO_SENSOR_ACCESSORS(TrueLedF1, ms->pack[this_dog].trueIndicators.ledF1);
AIBO_SENSOR_ACCESSORS(TrueLedF2, ms->pack[this_dog].trueIndicators.ledF2);
AIBO_SENSOR_ACCESSORS(TrueLedF3, ms->pack[this_dog].trueIndicators.ledF3);
AIBO_SENSOR_ACCESSORS(TrueLedF4, ms->pack[this_dog].trueIndicators.ledF4);
AIBO_SENSOR_ACCESSORS(TrueLedF5, ms->pack[this_dog].trueIndicators.ledF5);
AIBO_SENSOR_ACCESSORS(TrueLedF6, ms->pack[this_dog].trueIndicators.ledF6);
AIBO_SENSOR_ACCESSORS(TrueLedF7, ms->pack[this_dog].trueIndicators.ledF7);
AIBO_SENSOR_ACCESSORS(TrueLedF8, ms->pack[this_dog].trueIndicators.ledF8);
AIBO_SENSOR_ACCESSORS(TrueLedF9, ms->pack[this_dog].trueIndicators.ledF9);
AIBO_SENSOR_ACCESSORS(TrueLedF10, ms->pack[this_dog].trueIndicators.ledF10);
AIBO_SENSOR_ACCESSORS(TrueLedF11, ms->pack[this_dog].trueIndicators.ledF11);
AIBO_SENSOR_ACCESSORS(TrueLedF12, ms->pack[this_dog].trueIndicators.ledF12);
AIBO_SENSOR_ACCESSORS(TrueLedF13, ms->pack[this_dog].trueIndicators.ledF13);
AIBO_SENSOR_ACCESSORS(TrueLedF14, ms->pack[this_dog].trueIndicators.ledF14);
AIBO_SENSOR_ACCESSORS(TrueLedHC, ms->pack[this_dog].trueIndicators.ledHC);
AIBO_SENSOR_ACCESSORS(TrueModeR, ms->pack[this_dog].trueIndicators.modeR);
AIBO_SENSOR_ACCESSORS(TrueModeG, ms->pack[this_dog].trueIndicators.modeG);
AIBO_SENSOR_ACCESSORS(TrueModeB, ms->pack[this_dog].trueIndicators.modeB);
AIBO_SENSOR_ACCESSORS(TrueEarL, ms->pack[this_dog].trueIndicators.earL);
AIBO_SENSOR_ACCESSORS(TrueEarR, ms->pack[this_dog].trueIndicators.earR);

AIBO_SENSOR_ACCESSORS(DistanceNearSnout, ms->pack[this_dog].trueSensors.distanceNearSnout);
AIBO_SENSOR_ACCESSORS(DistanceFarSnout, ms->pack[this_dog].trueSensors.distanceFarSnout);
AIBO_SENSOR_ACCESSORS(DistanceChest, ms->pack[this_dog].trueSensors.distanceChest);
AIBO_SENSOR_ACCESSORS(PawLF, ms->pack[this_dog].trueSensors.pawLF);
AIBO_SENSOR_ACCESSORS(PawLH, ms->pack[this_dog].trueSensors.pawLH);
AIBO_SENSOR_ACCESSORS(PawRF, ms->pack[this_dog].trueSensors.pawRF);
AIBO_SENSOR_ACCESSORS(PawRH, ms->pack[this_dog].trueSensors.pawRH);
AIBO_SENSOR_ACCESSORS(ChinSensor, ms->pack[this_dog].trueSensors.chinSensor);
AIBO_SENSOR_ACCESSORS(HeadTouch, ms->pack[this_dog].trueSensors.headTouch ); 
AIBO_SENSOR_ACCESSORS(BackTouchR, ms->pack[this_dog].trueSensors.backTouchR);
AIBO_SENSOR_ACCESSORS(BackTouchM, ms->pack[this_dog].trueSensors.backTouchM);
AIBO_SENSOR_ACCESSORS(BackTouchF, ms->pack[this_dog].trueSensors.backTouchF);

AIBO_SENSOR_ACCESSORS(AccelerometerX, ms->pack[this_dog].trueSensors.accelerometer[0]);
AIBO_SENSOR_ACCESSORS(AccelerometerY, ms->pack[this_dog].trueSensors.accelerometer[1]);
AIBO_SENSOR_ACCESSORS(AccelerometerZ, ms->pack[this_dog].trueSensors.accelerometer[2]);

/*

WORD(Dog)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\"robot.();\" socketSend");
}
END_WORD
REGISTER_WORD(Dog)


WORD(Dog)
virtual void execute(std::shared_ptr<MachineState> ms) {
  ms->evaluateProgram("\";\" socketSend");
}
END_WORD
REGISTER_WORD(Dog)

*/
 

  // convert dog image 
  // loop over relevant pixels
  // fill out gaussian map

  // convert dog image
  // loop over relevant pixels
  // lay down density at parts probably not background





}
