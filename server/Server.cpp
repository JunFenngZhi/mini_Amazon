#include "Server.h"

#include "OrderProcess.h"
#include "protobufCommunication.hpp"
#include "socket.h"
#include "sql_function.h"


/* ------------------------ "server singleton pattern" ------------------------ */
Server::Ptr Server::m_instance_ptr = nullptr;
mutex Server::m_mutex;

Server::Ptr Server::get_instance() {
  if (m_instance_ptr == nullptr) {
    std::lock_guard<std::mutex> lk(m_mutex);
    if (m_instance_ptr == nullptr) {
      m_instance_ptr = std::shared_ptr<Server>(new Server);
    }
  }
  return m_instance_ptr;
}

/* ------------------------ "server initialize functions" ------------------------ */
Server::Server() {
  cout << "initialize server configuration...." << endl;
  curSeqNum = 0;
  for (size_t i = 0; i < MAX_SEQNUM; i++) {
    seqNumTable.push_back(false);
  }
  n_warehouse = 9;  // should be an odd number for symetric
  wh_distance = 20;
  webPortNum = "9999";
  worldHostName = "vcm-25941.vm.duke.edu";
  worldPortNum = "12345";
  upsHostName = "0.0.0.0";
  upsPortNum = "8888";

  //used in development
  worldID = 3;
}

/* ------------------------ "server runtime functions" ------------------------ */
/*
  Initialize the connection to server and ups and get essential infomation. Then it 
  will keep listening the front-end web to receive order request.
*/
void Server::run() {
  try {
    //getWorldIDFromUPS();
    initializeWorld();
    thread tI(&Server::keepReceivingMsg, this);
    thread tO_world(&Server::keepSendingMsgToWorld, this);
    thread tO_ups(&Server::keepSendingMsgToUps, this);
    acceptOrderRequest();
  }
  catch (const std::exception & e) {
    std::cerr << e.what() << '\n';
    close(ups_fd);
    close(world_fd);
    return;
  }
}

/*
  Connect to UPS to get worldID. 
*/
void Server::getWorldIDFromUPS() {
  ups_fd = clientRequestConnection(upsHostName, upsPortNum);
  string msg = recvMsg(ups_fd);

  worldID = stoi(msg);
  cout << "get worldID = " << worldID << " from UPS.\n";
}

/*
  Connect to a new world, and then initialize warehouses for amazon.
  If connect unsuccessfully, it will throw exception. If worldID = -1, 
  server will create a new world and set worldID to the new worldID.
*/
void Server::initializeWorld() {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  //connect to world
  world_fd = clientRequestConnection(worldHostName, worldPortNum);

  //set AConnect command
  AConnect ac;
  for (int i = 0; i <= n_warehouse / 2; i++) {
    if (i == 0) {
      AInitWarehouse * w = ac.add_initwh();
      w->set_id(i);
      w->set_x(0);
      w->set_y(0);
      whList.push_back(Warehouse(0, 0, i));
    }
    else {
      AInitWarehouse * w1 = ac.add_initwh();
      w1->set_id(i);
      w1->set_x(0.707 * wh_distance * i);
      w1->set_y(0.707 * wh_distance * i);
      whList.push_back(Warehouse(0.707 * wh_distance * i, 0.707 * wh_distance * i, i));

      AInitWarehouse * w2 = ac.add_initwh();
      w2->set_id(i + n_warehouse / 2);
      w2->set_x(-1 * 0.707 * wh_distance * i);
      w2->set_y(-1 * 0.707 * wh_distance * i);
      whList.push_back(
          Warehouse(-1 * 0.707 * wh_distance * i, -1 * 0.707 * wh_distance * i, i));
    }
  }
  if (worldID != -1) {
    ac.set_worldid(worldID);
  }
  ac.set_isamazon(true);

  //show warehouses
  for (auto & w : whList) {
    w.show();
  }

  //send AConnect command
  unique_ptr<socket_out> out(new socket_out(world_fd));
  if (sendMesgTo<AConnect>(ac, out.get()) == false) {
    throw MyException("fail to send AConnect to world.");
  }

  //receive AConnected command
  AConnected aced;
  unique_ptr<socket_in> in(new socket_in(world_fd));
  if (recvMesgFrom<AConnected>(aced, in.get()) == false) {
    throw MyException("fail to recv AConnected from world.");
  }

  //check AConnected command
  if (aced.result() != "connected!") {
    throw MyException("fail to initialize the world.");
  }
  worldID = aced.worldid();
  cout << "succefully connect to world and initialize warehouses.\n";
}

/*
  IO function. Server keep receiving order requests from front-end web.
  It will throw exception when server_socket create unsuccessfully. For each 
  order request, send it to task queue for processing.
*/
void Server::acceptOrderRequest() {
  // create server socket, listen to port.
  int server_fd = createServerSocket(webPortNum);

  // keep receving orders request from front-end web.
  while (1) {
    // wait to accept connection.
    int client_fd;
    string clientIP;
    try {
      client_fd = serverAcceptConnection(server_fd, clientIP);
    }
    catch (const std::exception & e) {
      std::cerr << e.what() << '\n';
      continue;
    }

    // receive request.
    string msg;
    try {
      msg = recvMsg(client_fd);
      sendMsg(client_fd, "ACK", 4);
      close(client_fd);
    }
    catch (const std::exception & e) {
      std::cerr << e.what() << '\n';
      continue;
    }

    // TODO: put request into task queue, using thread pool
    thread t(parseOrder, msg);
    t.detach();
  }
}

/* ------------------------ "DB related functions" ------------------------ */
/*
  connect to specific database using the given userName and passWord, reutnr 
  the connection* C. It will throw an exception if fails. 
*/
connection * Server::connectDB(string dbName, string userName, string password) {
  //mtx_server.lock();
  //connection * C = new connection("dbname=" + dbName + " user=" + userName + " password=" + password);   // use in real sys
  connection * C =
      new connection("host=db port=5432 dbname=" + dbName + " user=" + userName +
                     " password=" + password);  // use in docker
  if (C->is_open()) {
    cout << "Opened database successfully: " << C->dbname() << endl;
  }
  else {
    throw MyException("Can't open database.");
  }
  //mtx_server.unlock();
  return C;
}

/*
  initialize database when server run at the first time. It will drop all the existing tables,
  and then create new ones.
*/
void Server::initializeDB(connection * C) {
  dropAllTable(C);
  createTable(C, "./sql/table.sql");
}

/*
  close the connection to database.
*/
void Server::disConnectDB(connection * C) {
  C->disconnect();
}

/* ------------------------ "IO Functions" ------------------------ */
/*
  using select() function to keep receiving message from world and ups.
*/
void Server::keepReceivingMsg() {
  unique_ptr<socket_in> world_in(new socket_in(world_fd));
  unique_ptr<socket_in> ups_in(new socket_in(ups_fd));

  vector<int> allSockets = {ups_fd, world_fd};
  int maxFd = *max_element(allSockets.begin(), allSockets.end());

  //set fd_set
  fd_set allFds;
  fd_set read_fds;  // only need to monitor reading events
  FD_ZERO(&allFds);
  FD_ZERO(&read_fds);
  for (auto fd : allSockets) {
    FD_SET(fd, &allFds);
  }

  //keep listening
  while (1) {
    read_fds = allFds;  // reset read_fds

    int ret = select(maxFd + 1, &read_fds, NULL, NULL, NULL);
    if (ret < 0) {
      throw MyException("Fail to select!\n");
    }

    if (FD_ISSET(world_fd, &read_fds)) {
      AResponses r;
      if (recvMesgFrom<AResponses>(r, world_in.get()) == false) {
        throw MyException("fail to recv AResponses from world.");
      }
      AResponseHandler h(r);
      h.handle();
    }

    if (FD_ISSET(ups_fd, &read_fds)) {
      AUResponse r;
      if (recvMesgFrom<AUResponse>(r, ups_in.get()) == false) {
        throw MyException("fail to recv AUResponse from ups.");
      }
      // AUResponseHandler h(r);
    }
  }
}

/*
  keep sending message from worldQueue to world. this function will block when
  the queue is empty. 
*/
void Server::keepSendingMsgToWorld() {
  unique_ptr<socket_out> out(new socket_out(world_fd));
  while (1) {
    ACommands msg;
    worldQueue.wait_and_pop(msg);
    if (sendMesgTo(msg, out.get()) == false) {
      throw MyException("fail to send message in world.");
    }
  }
}

/*
  keep sending message from upsQueue to UPS. this function will block when
  the queue is empty. 
*/
void Server::keepSendingMsgToUps() {
  unique_ptr<socket_out> out(new socket_out(ups_fd));
  while (1) {
    AUCommand msg;
    upsQueue.wait_and_pop(msg);
    if (sendMesgTo(msg, out.get()) == false) {
      throw MyException("fail to send message in ups.");
    }
  }
}
