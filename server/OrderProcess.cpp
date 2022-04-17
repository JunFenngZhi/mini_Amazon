#include "OrderProcess.h"

/*
    create new thread, let it to process PurchaseMore
    (Go to the database to increase the corresponding product inventory in the corresponding warehouse)
*/
void processPurchaseMore(APurchaseMore r) {
  // Connect the database
  connection * C = Server::connectDB("mini_amazon", "postgres", "passw0rd");

  // get warehouse id
  int whID = r.whnum();

  // get all products
  vector<AProduct> products;
  for (int i = 0; i < r.things_size(); i++) {
    products.push_back(std::move(r.things(i)));
  }

  // process each product purchasemore
  for (int i = 0; i < products.size(); i++) {
    int count = products[i].count();
    int productId = products[i].id();
    addInventory(C, whID, count, productId);
  }
}

/*
    create new thread, let it to process Packed
    (Go to the database and change the order status to 'packed')
*/
void processPacked(APacked r) {
  //Connect the database
  connection * C = Server::connectDB("mini_amazon", "postgres", "passw0rd");

  //get shipid
  int packageId = r.shipid();

  //process this order status to be 'packed'
}

/*
    create new thread, let it to process Loaded
    (Send AStartDeliver to UPS)
*/
void processLoaded(ALoaded r) {
}
