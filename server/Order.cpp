#include "Order.h"

/*
    Parse the incoming orderInfo string from the front end to create the object.
*/
Order::Order(const string& str) {
  //the order input string format is as following:
  //addrx:addry:amount:itemID:itemPrice:UPSID
  //res[0]:res[1]:res[2]:res[3]:res[4]:res[5]
  vector<string> res;
  stringstream input(str);
  string temp;
  const char pattern = ':';
  while (getline(input, temp, pattern)) {
    res.push_back(temp);
  }
  //Because the UPSID is optional, we need to judge if the UPSID is exists
  //if not exists, we set UPS_ID as -1 to indicate that
  if (res[5] == "") {
    UPS_ID = -1;
  }
  else {
    UPS_ID = stoi(res[5]);
  }
  address_x = stoi(res[0]);
  address_y = stoi(res[1]);
  amount = stoi(res[2]);
  item_id = stoi(res[3]);
  item_price = stof(res[4]);
}
