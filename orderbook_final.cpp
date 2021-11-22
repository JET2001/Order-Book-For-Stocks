// ATTEMPT #3: orderbook_final.cpp
// ============ PART 2 =============
#include <iostream>
#include <string>
#include <unordered_map>
#include <limits>
#include <vector>
#include <queue>
#include <algorithm>
// Create an enum for buying orders and selling orders.
enum class OrderSide {BUY, SELL};

// Declaration of interface
class Order
{
private:
    std::string orderClass; // this variable cannot be changed.
protected:
    OrderSide side;
    std::string orderID;
    int price;
    int quantity;
public:
    // Constructor
    Order(char side_, std::string orderID_, int price_, int quantity_, std::string orderClass_): orderID(orderID_), price(price_), quantity(quantity_), orderClass(orderClass_){
        if (side_ == 'B') side = OrderSide::BUY;
        else side = OrderSide::SELL;
        //std::cout << "Order(...)" << std::endl;
    }
    std::string getOrderClass(void) const {return orderClass;}
    // Order related get methods
    OrderSide getOrderSide(void) const {return side;}
    std::string getOrderID(void) const {return orderID;}
    int getPrice(void) const {return price;}
    int getQuantity(void) const {return quantity;}
    // Order related set methods
    void setOrderSide(OrderSide side) { this->side = side;}
    void setOrderID(std::string orderID) {this->orderID = orderID;}
    void setPrice(int price){this->price = price;}
    void setQuantity(int quantity){this->quantity = quantity;}

    // The return type of this method is a success code:
    // A value 0 means that the order is being processed.
    // A value 1 means that the order has been fully executed.
    // A value -1 means that the order cannot be fully executed.
    // O(1) time | O(1) space
    int processOrder(int& amt, Order* otherOrder)
    {
        // Each order is either a buy order or a sell order, and these orders would be evaluated differently.
        if (side == OrderSide::BUY){ // if current order is a buy order
            Order* bestSellOrder = otherOrder;
            if (bestSellOrder == nullptr && quantity > 0) return -1; // No more Sell orders left but the buyOrder is not fully executed.
            if (bestSellOrder->getPrice() > price) return -1; // The best sell order is higher than this buy order. So No trade can happen.
            if (bestSellOrder->getQuantity() < quantity)
            {
                // If the best Sell Order's quantity is lower than that of the stock.
                quantity -= bestSellOrder->getQuantity();
                amt += bestSellOrder->getQuantity() * bestSellOrder->getPrice();
                return 0;
            }
            // If code reaches here it also means that that the order is fully executed.
            bestSellOrder->setQuantity(bestSellOrder->getQuantity() - quantity);
            amt += bestSellOrder->getPrice() * quantity;
            return 1;
        } else { // if current order is a Sell Order
            Order* bestBuyOrder = otherOrder;
            if (bestBuyOrder == nullptr && quantity > 0) return -1; // No more Buy orders left but the buyOrder is not fully executed.
            if (bestBuyOrder->getPrice() < price) return -1; // The best sell order is higher than this buy order. So No trade can happen.
            if (bestBuyOrder->getQuantity() < quantity)
            {
                // If the best Buy Order's quantity is lower than that of the stock.
                quantity -= bestBuyOrder->getQuantity();
                amt += bestBuyOrder->getQuantity() * bestBuyOrder->getPrice();
                return 0;
            }
            // If code reaches here it also means that that the order is fully executed.
            bestBuyOrder->setQuantity(bestBuyOrder->getQuantity() - quantity);
            amt += bestBuyOrder->getPrice() * quantity;
            return 1;
        }
    }
    // Destructor
    ~Order(){
        //std::cout << "~Order()" << std::endl;
    }
};

// This namespace contains utility structures for the OrderBook implementation.
namespace OrderBookUtils
{
    int HeapNodeID {0}; // global to the namespace.

    // Definition of the MaxHeap Interface.
    struct HeapNode{ // this interface is abstract.
        int NodeID; // this is used to track which node is inserted before another node.
        Order* order;
        // Constructor
        HeapNode(Order* order_): order(order_){
            NodeID = ++HeapNodeID;
            //std::cout << "HeapNode(...)" << std::endl;
        }
        // Overide this method to adapt the functionality of the priority queue.
        virtual bool hasHigherPriority(HeapNode* otherNode) = 0;

        // Destructor
        virtual ~HeapNode(){
            //std::cout << "~HeapNode()" << std::endl;
        }
    };
    // Create 2 kinds of HeapNodes - a heapNode for BuyOrders and a heapNode for sell orders.
    struct BuyHeapNode: HeapNode{
        BuyHeapNode(Order* order): HeapNode(order){
            //std::cout << "BuyHeapNode(Order*)" << std::endl;
        }
        bool hasHigherPriority(HeapNode* otherNode) override{
            // This node has higher priority if the order has a higher price.
            if (this->order->getPrice() > otherNode->order->getPrice()) return true;
            else if (this->order->getPrice() == otherNode->order->getPrice() && this->NodeID < otherNode->NodeID) return true;
            else return false;
        }
        ~BuyHeapNode(){
            //std::cout << "~BuyHeapNode()" << std::endl;
        }
    };
    struct SellHeapNode: HeapNode{
        SellHeapNode(Order* order): HeapNode(order){
            //std::cout << "SellHeapNode(Order*)" << std::endl;
        }
        bool hasHigherPriority(HeapNode* otherNode) override{
            // This node has a higher priority if the order has a lower price.
            if (this->order->getPrice() < otherNode->order->getPrice()) return true;
            else if (this->order->getPrice() == otherNode->order->getPrice() && this->NodeID < otherNode->NodeID) return true;
            else return false;
        }
        ~SellHeapNode(){
            //std::cout << "~SellHeapNode()" << std::endl;
        }
    };
    // Max heap data structure
    class MaxHeap
    {
        // Interface methods for the maxheap
    public:
        // O(log n) time to insert
        void insert(HeapNode* newNode){
            heap.push_back(newNode);
            NodeIdxMap[newNode] = size; // Add heapnode-idx kv pair.
            siftUp(size);
            size++;
        }
        // O(log n) time to remove the minimum element.
        void removeMax(){
            std::swap(heap[0], heap[size-1]); // swap the minimum element with the last element.
            swapIdxMap(0, size-1); // swap the kv-pairs in the index map.
            NodeIdxMap.erase(heap[size-1]); // remove entry from map
            delete heap[size-1]; heap.pop_back();
            size--;
            siftDown(0); // sift down from the root index.
        }
        // O(log n) time to remove the minimum element AND store it in a buffer.
        // A crucial difference between these 2 overloads is that this one doesn't delete the heapNode element yet.
        void removeMax(std::queue<HeapNode*>& buffer){
            if (size == 0) return; // guard against access memory violations
            std::swap(heap[0], heap[size-1]);
            swapIdxMap(0, size-1);
            NodeIdxMap.erase(heap[size-1]);
            buffer.push(heap[size-1]);// store in the buffer.
            heap.pop_back();
            size--;
            siftDown(0); // sift down from the root index.
        }
        // Inserting Nodes from a buffer back into the binary heap.
        // O(k log n) time, where k nodes are in the buffer.
        void insert(std::queue<HeapNode*> &buffer){
            while (!buffer.empty()){
                insert(buffer.front());
                buffer.pop();
            }
        }
        // O(log n) time for removal in the worst case.
        void removeNode(HeapNode *node){
            // After finding this node,
            int nodeIdx = NodeIdxMap[node];
            std::swap(heap[nodeIdx], heap[size-1]); // swap this element with the minimum element in the array.
            swapIdxMap(nodeIdx, size-1);
            NodeIdxMap.erase(heap[size-1]); // remove entry from map
            delete heap[size-1]; heap.pop_back();
            size--;
            siftDown(nodeIdx);
        }
        // O(1) time
        HeapNode* peek() const {
            return (size != 0)? heap[0] : nullptr;
        }
        // O(log n) time for modification in the worst case, O(1) in the best case.
        void modifyNode(HeapNode* node, int newQuantity, int newPrice){
            // Corner case - if the node's order Price is constant, and quantity decreases, just update the node in place.
            if (node->order->getPrice() == newPrice && node->order->getQuantity() >= newQuantity){
                node->order->setQuantity(newQuantity);
                return;
            }
            // In other cases then reinsert this Node into the heap, we can do this by sifting up and sifting down this node after modification.
            else modifyNodeTemp(NodeIdxMap[node], newPrice, newQuantity);
        }
        void printAndDestroyHeap(){
            while (!heap.empty()){
                auto curr = peek()->order;
                std::cout << curr->getQuantity() << "@" << curr->getPrice()  << "#" << curr->getOrderID() << " ";
                delete peek()->order;
                removeMax();
            }
        }
    private:
        // Properties:
        int size {}; // initialised to 0.
        std::vector<HeapNode*> heap;
        // A hash table which maps a key (HeapNode*) to the value (index in the array). By storing an additional n space, I am able to optimise the find() operations, removal operations and modification operations to be log n time.
        std::unordered_map<HeapNode*, int> NodeIdxMap;

        // Helper methods:
        // For restoring heap property:
        // O(log n) time
        void siftUp(int idx){
            if (getParent(idx) != -1){
                // Compare this node and the parent node.
                auto thisNode = heap[idx], parent = heap[getParent(idx)];
                // if the child has a higher priority than the parent
                if (thisNode->hasHigherPriority(parent)){
                    std::swap(heap[idx], heap[getParent(idx)]);  // swap child and parent both in the heap and in the idx in hash table.
                    swapIdxMap(thisNode, parent);
                    // Recursively call siftUp on parent
                    return siftUp(getParent(idx));
                } else return;
            }
        }
        // O(log n) time
        void siftDown(int idx){
            // Check for the case where both left and right children exist.
            if (getLeftChild(idx) != -1 && getRightChild(idx) != -1)
            {
                // Find the maximum priority node among the parent, left and right child.
                auto maxPriorityChild = heap[getLeftChild(idx)];
                auto maxPriorityChildIdx = getLeftChild(idx);
                if (heap[getRightChild(idx)]-> hasHigherPriority(heap[getLeftChild(idx)]))
                {
                    maxPriorityChild  = heap[getRightChild(idx)];
                    maxPriorityChildIdx = getRightChild(idx);
                }
                if (maxPriorityChild->hasHigherPriority(heap[idx])){
                    std::swap(heap[idx], heap[maxPriorityChildIdx]);
                    swapIdxMap(heap[idx], maxPriorityChild);
                    // Recursively sift down on the index of the subtree.
                    return siftDown(maxPriorityChildIdx);
                }
                else return;
            }// If there is only 1 child (if there is no left child there won't be a right child because it's a heap.)
            else if (getLeftChild(idx) != -1){
                if (heap[getLeftChild(idx)]->hasHigherPriority(heap[idx])){
                    std::swap(heap[getLeftChild(idx)], heap[idx]);
                    swapIdxMap(heap[getLeftChild(idx)], heap[idx]);
                    // Recursively sift down the index of the left subtree.
                    return siftDown(getLeftChild(idx));
                }
                else return;
            }
            else return; // no children exist - node is already a leaf in the heap.
        }
        // For updating the hash table. - This method is also overloaded to take in 2 heapnode pointers.
        void swapIdxMap(int idx1, int idx2){
            auto temp = NodeIdxMap[heap[idx1]];
            NodeIdxMap[heap[idx1]] = NodeIdxMap[heap[idx2]];
            NodeIdxMap[heap[idx2]] = temp;
        }
        // For updating the hash table.
        void swapIdxMap(HeapNode* node1, HeapNode* node2){
            auto temp = NodeIdxMap[node1];
            NodeIdxMap[node1] = NodeIdxMap[node2];
            NodeIdxMap[node2] = temp;
        }
        // For updating a node - removal and insertion: O(log n) time.
        void modifyNodeTemp(int idx, int newPrice, int newQuantity){
            // Update the HeapNodeID
            heap[idx]->NodeID = ++HeapNodeID; // treat it as if it is a new node.
            // Modify the node's content.
            heap[idx]->order->setPrice(newPrice);
            heap[idx]->order->setQuantity(newQuantity);
            // Basically the remove method, without the deletion from the heap.
            std::swap(heap[idx], heap[size-1]); // swap this element with the last element in the array
            swapIdxMap(idx, size-1);
            // Siftdown from the original index.
            siftDown(idx);
            // Reinsert node by sifting it back up in the heap.
            siftUp(size-1);

        }
        inline int getLeftChild(int idx){return (2*idx+1 < size)? 2*idx+1 : -1;}
        inline int getRightChild(int idx){return (2*idx+2 < size)? 2*idx+2 : -1; }
        inline int getParent(int idx){return (idx >= 1)? (idx-1)/2 : -1; }
    }; // end of the Maxheap class
}// end of the OrderBookUtils namespace

// Orderbook class
class OrderBook
{
public: // Interface methods and their corresponding time complexities.

    // O(k log k + log m) time in the worst case, where k is the number of elements present in the heap of the other orderSide, and m is the number of elements present in the heap of the this orderSide.
    void createLimitOrder(char orderSide, std::string orderID, int quantity, int price)
    {
        Order* limitOrder = new Order(orderSide, orderID, price, quantity, "LO");
        Process(limitOrder); // O(k log k + log m)
    }
    // O(k log k) time in the worst case, where k is the number of elements present in the heap of the other orderside (i.e. if this order is a buy order, the number of elements in the Sell heap is denoted as k.)
    void createMarketOrder(char orderSide, std::string orderID, int quantity)
    {
        Order* marketOrder = new Order(orderSide, orderID, INT_MAX, quantity, "MO");
        Process(marketOrder); // O(k log k)
    }
    // O(k log k) time in the worst case, where k is the number of elements present in the heap of the other orderside.
    void createIOCOrder(char orderSide, std::string orderID, int quantity, int price){
        Order* iocOrder = new Order(orderSide, orderID, price, quantity, "IOC");
        Process(iocOrder); // O(k log k)
    }
    // Average: O(m log k) time where k is the number of elements present in the heap of the other orderSide, and m is the number of elements required to be moved into a buffer before deletion. Worst case: O(k log k) time.
    // O(m) space.
    void createFOKOrder(char orderSide, std::string orderID, int quantity, int price)
    {
        Order* fokOrder = new Order(orderSide, orderID, price, quantity, "FOK");
        Process(fokOrder); // O(m log k) time, O(m) space.
    }
    // Total O(log n) time: O(1) time to search, and O(log n) time for removal.
    void createCXLRequest(std::string orderID)
    {
        if (BuyOrdersMap.count(orderID)){
            BuyOrders.removeNode(BuyOrdersMap[orderID]);
            BuyOrdersMap.erase(BuyOrdersMap.find(orderID)); // remove order from heap.
            return;
        }
        if (SellOrdersMap.count(orderID)){
            SellOrders.removeNode(SellOrdersMap[orderID]);
            SellOrdersMap.erase(SellOrdersMap.find(orderID)); // remove order from heap
            return;
        }
    }
    // O(1) time in the best case - corner case, O(log n) time on average and in the worst case, where n is the number of elements within the heap.
    // Total O(log n) time: O(1) time to search, and O(log n) time for removal.
    void createCRPRequest(std::string orderID, int newQuantity, int newPrice)
    {
        if (BuyOrdersMap.count(orderID)){
            BuyOrders.modifyNode(BuyOrdersMap[orderID], newQuantity, newPrice);
            return;
        }
        if (SellOrdersMap.count(orderID)){
            SellOrders.modifyNode(SellOrdersMap[orderID], newQuantity, newPrice);
            return;
        }
    }
    // View Output log - O(m) time - where m is the number of commands.
    void printOutputLog() const {
        for (auto &i: outputLogger) std::cout << i << std::endl;
    }

    // Output remaining entries to the console:
    // O(m log m + k log k) time - where m and n are the number of outstanding entries present within the Buyorders and Sell Orders in the orderbook.
    void printAndDestroyRemainingEntries(){
        std::cout << "B: "; BuyOrders.printAndDestroyHeap(); std::cout << std::endl;
        std::cout << "S: "; SellOrders.printAndDestroyHeap(); std::cout << std::endl;
    }
private: // Properties, Implementation, Helper Methods
    // Properties

    OrderBookUtils::MaxHeap BuyOrders;
    OrderBookUtils::MaxHeap SellOrders;
    std::vector<int> outputLogger; // to store output from the transactions

    // The folloing are hash table that maps orderIDs to HeapNode* when they are created - this hash table only contains those orders which have been inserted into the Orderbook.
    std::unordered_map<std::string, OrderBookUtils::HeapNode*> BuyOrdersMap;
    std::unordered_map<std::string, OrderBookUtils::HeapNode*> SellOrdersMap;

    // This process() method would have different time complexities depending on the order type.
    void Process(Order *order){
        int amt{}, success_code{};
        std::queue<OrderBookUtils::HeapNode*> buffer; // empty queue to act as a buffer.
        // This buffer will be used for FOK Orders.
        Order* bestOBOrder;
        do
        {
            bestOBOrder = (order->getOrderSide() == OrderSide::BUY)? getBestSellOrder() : getBestBuyOrder();
            if (bestOBOrder == nullptr) success_code = -1; // The relevant list is empty, order cannot be processed.
            else success_code = order->processOrder(amt, bestOBOrder);
            // Do the following with respect to the success code:
            finishOrderProcess(success_code, order, bestOBOrder, amt, buffer);
        }
        while(!success_code);
    }

    // Completes the orderProcess with respect to the success code.
    // This method is called at the end of each iteration of the Process.
    void finishOrderProcess(int success_code, Order* currOrder, Order* otherOrder, int amt, std::queue<OrderBookUtils::HeapNode*> &buffer){
        switch (success_code){
            case -1: // Order cannot be fully processed
            {
                // If order is a limit order
                if (currOrder->getOrderClass() == "LO") {
                    if (currOrder->getOrderSide() == OrderSide::BUY){
                        auto newNode = new OrderBookUtils::BuyHeapNode(currOrder);
                        // Update orderID heapNode hash table.
                        BuyOrdersMap[currOrder->getOrderID()] = newNode;
                        BuyOrders.insert(newNode);// insert it into the heap
                    } else {
                        auto newNode = new OrderBookUtils::SellHeapNode(currOrder);
                        // Update orderID-heapNode hashtable
                        SellOrdersMap[currOrder->getOrderID()] = newNode;
                        SellOrders.insert(newNode);
                    }
                    outputLogger.push_back(amt);
                }
                // If the order is a FOK order, insert all the buffer entries back into the respective heap. This is because the order cannot be fully processed.
                else if (currOrder->getOrderClass() == "FOK"){
                    // If this order was a buyOrder, then everything in the buffer would be sell orders.
                    if (currOrder->getOrderSide() == OrderSide::BUY){
                        SellOrders.insert(buffer);
                    } else { // Sell Order - means everything in the buffer would be buy orders.
                        BuyOrders.insert(buffer);
                    }
                    // After this is done, then proceed to delete the order.
                    delete currOrder; currOrder = nullptr;
                    outputLogger.push_back(0); // Order was not executed.
                }
                // If order is any other order type:
                else{
                    // Otherwise, destroy the order by removing it from heap memory
                    delete currOrder; currOrder = nullptr;
                    outputLogger.push_back(amt); //
                }
                break;
            }
            case 0: // In this case the order has not finished processing.
            {
                // In this case, we can remove the otherOrder by deleting it from the heap.
                if (currOrder->getOrderSide() == OrderSide::BUY){
                    // If this was an FOKOrder, then we can add the item into the buffer (temporarily removing from the heap).
                    if (currOrder->getOrderClass() == "FOK") SellOrders.removeMax(buffer);
                    else { // for any other orders:
                        SellOrdersMap.erase(otherOrder->getOrderID()); // remove from orderbook hash table.
                        delete otherOrder; otherOrder = nullptr;
                        // Remove order from the binary heap.
                        SellOrders.removeMax();
                    }
                }
                else // if currOrder is a sell order, then otherorder must be a buy order.
                {
                    // If this was an FOKOrder, then we can add the item into the buffer (temporarily removing from the heap).
                    if (currOrder->getOrderClass() == "FOK") BuyOrders.removeMax(buffer);
                    else { // for any other order, remove this order from the hash table.
                        BuyOrdersMap.erase(otherOrder->getOrderID()); // remove from orderbook hashtable
                        delete otherOrder; otherOrder = nullptr;
                        BuyOrders.removeMax();
                    }
                }
                // Order has yet to be finished, do not log output.
                break;
            }
            case 1: // Order has finished processing
            {
                // Check for the corner case where the otherOrder in the heaps are also zero.
                if (otherOrder->getQuantity() == 0)
                {
                    if (otherOrder->getOrderSide() == OrderSide::BUY){
                        BuyOrders.removeMax();
                        BuyOrdersMap.erase(otherOrder->getOrderID());
                    } else{
                        SellOrders.removeMax();
                        SellOrdersMap.erase(otherOrder->getOrderID());
                    }
                    delete otherOrder; otherOrder = nullptr;
                }
                // Also check if the buffer is not empty, if it isn't empty, clear everything within the buffer.
                while (!buffer.empty()){
                    auto item = buffer.front()->order;
                    if (item->getOrderSide() == OrderSide::BUY){
                        // Remove entry from the hash table
                        BuyOrdersMap.erase(item->getOrderID());
                    } else {
                        // Remove entry from the hash table
                        SellOrdersMap.erase(item->getOrderID());
                    }
                    delete item; item = nullptr; // ~Order()
                    delete buffer.front(); // ~HeapNode()
                    buffer.pop();
                }
                // Since the order has been fully executed, we can destroy the currOrder as well.
                delete currOrder; currOrder = nullptr;
                // log output to the logger.
                outputLogger.push_back(amt);

                break;
            }
            default:
            std::cout << "Code should not reach here!" << std::endl;
        }
    }
    // Other Helper Methods
    // Processes an Order after obtaining another order from the OB.
    // Gets the best buy order from the heap.
    Order* getBestBuyOrder() const {
        return (BuyOrders.peek() != nullptr)? BuyOrders.peek()->order : nullptr;
    }
    // Gets the best sell order from the heap.
    Order* getBestSellOrder() const{
        return (SellOrders.peek() != nullptr)? SellOrders.peek()->order : nullptr;
    }
}; // end of the orderbook class.

// OrderBook Input Output manager.
// For input and output support.
struct OrderBookIOManager{
    // Public method
    void getInput(OrderBook& ob)
    {
        using std::cin;
        using std::cout;
        using std::string;
        string command{};
        while (command != "END")
        {
            cin >> command;
            if (command == "SUB")
            {
                string orderClass, orderID;
                char side;
                int quantity;
                cin >> orderClass >> side >> orderID >> quantity;
                if (orderClass == "LO"){
                    int price; cin >> price;
                    //std::cout << command << " " << orderClass << " " << side << " " << orderID << " " << quantity << " " << price << std::endl;
                    ob.createLimitOrder(side, orderID, quantity, price);
                }
                else if (orderClass == "IOC"){
                    int price; cin >> price;
                    //std::cout << command << " " << orderClass << " " << side << " " << orderID << " " << quantity << " " << price << std::endl;
                    ob.createIOCOrder(side, orderID, quantity, price);
                }
                else if (orderClass == "FOK"){
                    int price; cin >> price;
                    //std::cout << command << " " << orderClass << " " << side << " " << orderID << " " << quantity << " " << price << std::endl;
                    ob.createFOKOrder(side, orderID, quantity, price);
                }
                else
                {
                    //std::cout << command << " " << orderClass << " " << side << " " << orderID << " " << quantity << std::endl;
                    ob.createMarketOrder(side, orderID, quantity);
                }
            }
            if (command == "CXL"){
                string orderID; cin >> orderID;
                //std::cout << command << " " << orderID << std::endl;
                ob.createCXLRequest(orderID);
            }
            if (command == "CRP"){
                string orderID; int newQuantity, newPrice;
                cin >> orderID >> newQuantity >> newPrice;
                //std::cout << command << " " << orderID << " " << newQuantity << " " << newPrice << std::endl;
                ob.createCRPRequest(orderID, newQuantity, newPrice);
            }
        }
    }
    // Output to console.
    void printOutput(OrderBook &ob){
        ob.printOutputLog();
        ob.printAndDestroyRemainingEntries();
    }
}; // end of the OrderBookIOManager class

// Driver Code
int main(){
    OrderBook ob;
    OrderBookIOManager orderbookio;
    orderbookio.getInput(ob);
    orderbookio.printOutput(ob);
    return 0;
}
