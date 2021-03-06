#ifndef _ARESPONSEHANDLER_H
#define _ARESPONSEHANDLER_H

#include <vector>

#include "OrderProcess.h"
#include "Server.h"
#include "protobufCommunication.hpp"

using namespace std;

class AResponseHandler {
 private:
  vector<APurchaseMore> apurchasemores;
  vector<APacked> apackeds;
  vector<ALoaded> aloadeds;
  vector<int> seqNums;

 public:
  AResponseHandler(const AResponses & r);
  ~AResponseHandler() {}
  void handle();

 private:
  bool checkExecutedAndRecordIt(int seqNum);
};

#endif