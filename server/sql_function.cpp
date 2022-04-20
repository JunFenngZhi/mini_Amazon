#include "sql_function.h"
#include "exception.h"



/*
    read sql command from the file and then create tabel using connection *C.
    If fails, it will throw exception.
*/
void createTable(connection* C, string fileName) {
    cout<<"create new Tables..."<<endl;
    string sql;
    ifstream ifs(fileName.c_str(), ifstream::in);
    if (ifs.is_open() == true) {
        string line;
        while (getline(ifs, line))
            sql.append(line);
    } else {
        throw MyException("fail to open file.");
    }

    work W(*C);
    W.exec(sql);
    W.commit();
}

/*
    Drop all the table in the DataBase. Using for test.
*/
void dropAllTable(connection* C) {
    work W(*C);
    string sql =
        "DROP TABLE IF EXISTS symbol;DROP TABLE IF EXISTS account;DROP TABLE "
        "IF EXISTS orders;";

    W.exec(sql);
    W.commit();
    cout << "Drop all the existed table..." << endl;
}


void setTableDefaultValue(connection* C){
    work W(*C);
    string sql =
        "ALTER TABLE orders ALTER COLUMN version SET DEFAULT 1;ALTER TABLE inventory ALTER COLUMN version SET DEFAULT 1;"
        "ALTER TABLE item ALTER COLUMN version SET DEFAULT 1;ALTER TABLE orders ALTER COLUMN time SET DEFAULT CURRENT_TIME (0);"
        "ALTER TABLE orders ALTER COLUMN status set DEFAULT 'packing';";
    W.exec(sql);
    W.commit();
    cout << "set orders, inventory, item tables version column default value 1, and orders time column as now" << endl;
}

/*
    Check the order item amount in the inventory table, return boolean
    True means: enough inventory, False means: not enough inventory
*/
bool checkInventory(connection * C, int itemId, int itemAmount, int whID, int & version){
    //create nontransaction object for SELECT operation
    nontransaction N(*C);

    // create sql statement, we need to select item amount from inventory table
    stringstream sql;  
    sql << "SELECT ITEM_AMOUNT, VERSION FROM INVENTORY WHERE "
            "ITEM_ID= " << itemId << "AND WH_ID= " << whID <<";";

    // execute sql statement and get the result set    
    result InventoryRes( N.exec(sql.str()));

    // first we need to check if the result set is empty
    if(InventoryRes.size() == 0) {
        return false;
    }

    // Then we need to get inventory item amount from result R
    int inventoryAmt = InventoryRes[0][0].as<int>();
    // get the version from the table and change it
    version = InventoryRes[0][1].as<int>();

    // we compare inventory amt and item amount 
    if(inventoryAmt >= itemAmount){
        return true;
    } else{
        return false;
    }
    
}

void saveItemInDB(connection* C, const Order & order) {
    //first we need to check if there exists this item in table
    work W(*C);
    int itemid = order.getItemId();
    float item_price = order.getPrice();
    string item_description = order.getDescription();

    // create sql statement, we need to select item amount from inventory table
    stringstream sql;  
    sql << "INSERT INTO ITEM (ITEM_ID, DESCRIPTION, PRICE) "
    "VALUES(" << itemid << ", " << W.quote(item_description) << ", " << item_price << ")"
    <<"ON CONFLICT (ITEM_ID) DO NOTHING;";
    W.exec(sql.str());
    W.commit();
    }


    


/*
   saveorder into the database,and set package into order member through reference.
*/
void saveOrderInDB(connection* C, Order & order) {
    //we need to save item in item table if it not exist
    saveItemInDB(C, order);
  
    //finally we need to save order in the order table
    work W(*C);
    stringstream sql;
    int addrx = order.getAddressX();
    int addry = order.getAddressY();
    int amount = order.getAmount();
    int upsid = order.getUPSId();
    int itemid = order.getItemId();
    float item_price = order.getPrice();
    float total_price = item_price * amount;
    sql << "INSERT INTO ORDERS (ADDR_X, ADDR_Y, AMOUNT, UPS_ID, ITEM_ID, PRICE) "
            "VALUES(" << addrx << ", " << addry << ", " << amount << ", " << upsid << ", " << itemid << ", " << total_price << ");";

    W.exec(sql.str());

    //get pack_id for the current order, and set the pack_id field for this order
    sql.clear();
    sql << "SELECT PACK_ID FROM ORDERS ORDER BY PACK_ID DESC LIMIT 1;";
    result orderRes(W.exec(sql.str()));
    int packageId = orderRes[0][0].as<int>();
    order.setPackId(packageId);
    W.commit();
    
}

/*
    get description from the item 
*/
string getDescription(connection * C, int itemId) {
    //create nontransaction object for SELECT operation
    nontransaction N(*C);

    // create sql statement, we need to select item amount from inventory table
    stringstream sql;  
    sql << "SELECT DESCRIPTION FROM ITEM WHERE "
            "ITEM_ID= " << itemId << ";";

    // execute sql statement and get the result set    
    result ItemRes( N.exec(sql.str()));
    string description = ItemRes[0][0].as<string>();
    return description;
}

int getPackId(connection * C) {


}

/*
    add inventory of item in the warehouse and update its version id.
*/
void addInventory(connection * C, int whID, int count, int productId) {

    //check if inventory exist this item, if not exist, we need to insert, else we need upadte
    work W(*C);
    stringstream sql;  
    sql << "INSERT INTO INVENTORY (ITEM_ID, ITEM_AMOUNT, WH_ID) "
             "VALUES(" << productId << ", " << count << ", " << whID << ") ON CONFLICT (ITEM_ID) DO UPDATE "
             "set ITEM_AMOUNT = INVENTORY.ITEM_AMOUNT+" << count << ", VERSION = INVENTORY.VERSION+1" << " WHERE ITEM_ID= " << productId << "AND WH_ID= " << whID <<";";
        W.exec(sql.str());
        W.commit();  
    }      


/*
    update specific order status to be 'packed'
*/
void updatePacked(connection * C, int packageId) {
     work W(*C);
     stringstream sql;
     sql << "UPDATE ORDERS set STATUS= " << W.quote("packed") << "WHERE PACK_ID= " << packageId << ";";

     W.exec(sql.str());
     W.commit();    
}

/*
    decrease inventory of the product in the warehouse and check the version id of the inventory.
    If version is not matched, throw exception.
*/
void decreaseInventory(connection * C, int whID, int count, int productId, int version) {
    work W(*C);
    stringstream sql;

    sql << "UPDATE INVENTORY set ITEM_AMOUNT = INVENTORY.ITEM_AMOUNT-" << count << 
    ", VERSION = INVENTORY.VERSION+1" << " WHERE ITEM_ID= " << productId << " AND WH_ID= " << whID << " AND VERSION= "<< version << ";";

    result Updates(W.exec(sql.str()));
    result::size_type rows = Updates.affected_rows();
    if (rows == 0) {
        throw VersionErrorException(
            "Invalid update: version of this order does not match.\n");
    }
    W.commit();
}