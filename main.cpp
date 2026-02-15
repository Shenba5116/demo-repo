#include <iostream>
#include <queue>
#include <stack>
#include <string>
#include <vector>
#include <map>
#include <deque>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
using namespace std;

// ======= Structs and Classes =======
struct OrderItem {
    string itemName;
    int quantity;
};

struct Order {
    int id;
    string restaurantName;
    vector<OrderItem> items;
};

class MenuItem {
public:
    string name;
    double price;
    MenuItem* next;
    MenuItem(string n, double p) : name(n), price(p), next(nullptr) {}
};

class Menu {
private:
    MenuItem* head;
public:
    Menu() : head(nullptr) {}
    void addItem(string n, double p) {
        MenuItem* newItem = new MenuItem(n, p);
        if (!head) head = newItem;
        else {
            MenuItem* temp = head;
            while (temp->next) temp = temp->next;
            temp->next = newItem;
        }
    }
    void display();
    MenuItem* getItem(int idx);
    double getAveragePrice();
};

void Menu::display() {
    MenuItem* temp = head;
    int idx = 1;
    bool discount = false; // simply for menu display, discount shown only in order placement
    while (temp) {
        cout << idx++ << ". " << temp->name << " - Rs." << temp->price;
        cout << endl;
        temp = temp->next;
    }
}
MenuItem* Menu::getItem(int idx) {
    MenuItem* temp = head;
    int count = 1;
    while (temp && count < idx) {
        temp = temp->next;
        count++;
    }
    return temp;
}
double Menu::getAveragePrice() {
    MenuItem* temp = head;
    double total = 0;
    int count = 0;
    while (temp) {
        total += temp->price;
        count++;
        temp = temp->next;
    }
    return count ? (total / count) : 0;
}

class Restaurant {
public:
    string name;
    double rating;
    map<string, Menu> categoryMenus;
    Restaurant(string n, double r) : name(n), rating(r) {}
    void addMenuItemToCategory(string category, string itemName, double price) {
        categoryMenus[category].addItem(itemName, price);
    }
    void displayCategories();
    void displayCategoryMenu(string category);
    double getAveragePrice();
};

void Restaurant::displayCategories() {
    int idx = 1;
    for (auto& entry : categoryMenus) {
        cout << idx++ << ". " << entry.first << endl;
    }
}
void Restaurant::displayCategoryMenu(string category) {
    if (categoryMenus.find(category) != categoryMenus.end()) {
        cout << category << " Menu:\n";
        categoryMenus[category].display();
    } else {
        cout << "No such category found.\n";
    }
}
double Restaurant::getAveragePrice() {
    double total = 0;
    int count = 0;
    for (auto& entry : categoryMenus) {
        Menu& m = entry.second;
        MenuItem* temp = m.getItem(1);
        MenuItem* iter = temp;
        while (iter) {
            total += iter->price;
            count++;
            iter = iter->next;
        }
    }
    return count ? (total / count) : 0;
}

// ======= Global Data Structures =======
Restaurant r1("Mount Bilal", 4.5);
Restaurant r2("Jaya Mess", 4.0);
Restaurant r3("Ulavan Restaurant", 4.2);
Restaurant* restaurants[3] = {&r1, &r2, &r3};

queue<Order> previousOrders;
stack<Order> cancelledOrders;

// ======= File Management Functions =======
// Serialize order to file (append)
void saveOrderToFile(const Order& order, const string& filename) {
    ofstream fout(filename, ios::app);
    fout << order.id << "|" << order.restaurantName << "|";
    for (const auto& item : order.items) {
        fout << item.itemName << ":" << item.quantity << ",";
    }
    fout << endl;
    fout.close();
}

// Load all orders from file into a queue
void loadOrdersFromFile(const string& filename, queue<Order>& orderQueue) {
    orderQueue = queue<Order>(); // Clear before loading
    ifstream fin(filename);
    string line;
    while (getline(fin, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string idStr, restName, itemsStr;
        getline(ss, idStr, '|');
        getline(ss, restName, '|');
        getline(ss, itemsStr);
        vector<OrderItem> items;
        istringstream itemsStream(itemsStr);
        string itemPair;
        while (getline(itemsStream, itemPair, ',')) {
            size_t colonPos = itemPair.find(':');
            if (colonPos == string::npos) continue;
            string name = itemPair.substr(0, colonPos);
            int quantity = stoi(itemPair.substr(colonPos + 1));
            items.push_back({name, quantity});
        }
        orderQueue.push({stoi(idStr), restName, items});
    }
    fin.close();
}

// Save stack to file (overwrites, saves all cancelled orders)
void saveCancelledOrdersToFile(const stack<Order>& orderStack, const string& filename) {
    ofstream fout(filename);
    stack<Order> tempStack = orderStack;
    while (!tempStack.empty()) {
        const Order& order = tempStack.top();
        fout << order.id << "|" << order.restaurantName << "|";
        for (const auto& item : order.items) {
            fout << item.itemName << ":" << item.quantity << ",";
        }
        fout << endl;
        tempStack.pop();
    }
    fout.close();
}

// Load cancelled orders from file into a stack
void loadCancelledOrdersFromFile(const string& filename, stack<Order>& orderStack) {
    orderStack = stack<Order>(); // Clear before loading
    ifstream fin(filename);
    string line;
    vector<Order> orders; // Read in reverse for stack
    while (getline(fin, line)) {
        if (line.empty()) continue;
        istringstream ss(line);
        string idStr, restName, itemsStr;
        getline(ss, idStr, '|');
        getline(ss, restName, '|');
        getline(ss, itemsStr);
        vector<OrderItem> items;
        istringstream itemsStream(itemsStr);
        string itemPair;
        while (getline(itemsStream, itemPair, ',')) {
            size_t colonPos = itemPair.find(':');
            if (colonPos == string::npos) continue;
            string name = itemPair.substr(0, colonPos);
            int quantity = stoi(itemPair.substr(colonPos + 1));
            items.push_back({name, quantity});
        }
        orders.push_back({stoi(idStr), restName, items});
    }
    fin.close();
    for (auto it = orders.rbegin(); it != orders.rend(); ++it) {
        orderStack.push(*it);
    }
}

// Helper to rewrite the orders file after cancellation
void rewriteOrdersFile(const queue<Order>& orderQueue, const string& filename) {
    ofstream fout(filename);
    queue<Order> tmp = orderQueue;
    while (!tmp.empty()) {
        const Order& o = tmp.front();
        fout << o.id << "|" << o.restaurantName << "|";
        for (auto& i : o.items) fout << i.itemName << ":" << i.quantity << ",";
        fout << endl;
        tmp.pop();
    }
    fout.close();
}

// ======= System Feature Implementations for Menu Options =======
void viewAllRestaurants() {
    cout << "\nRestaurants:\n";
    for (int i = 0; i < 3; i++) {
        cout << i + 1 << ". " << restaurants[i]->name << " (Rating: " << restaurants[i]->rating << ")\n";
    }
}
void initializeMenus() {
    r1.addMenuItemToCategory("Breakfast", "Idli", 40);
    r1.addMenuItemToCategory("Breakfast", "Pongal", 60);
    r1.addMenuItemToCategory("Breakfast", "Vada", 25);
    r1.addMenuItemToCategory("Lunch", "Mutton Briyani", 350);
    r1.addMenuItemToCategory("Lunch", "Parotta", 50);
    r1.addMenuItemToCategory("Lunch", "Chicken Curry", 150);
    r1.addMenuItemToCategory("Specials", "Malabar Fish Curry", 22);
    r1.addMenuItemToCategory("Specials", "Hyderabadi Mutton", 400);
    r1.addMenuItemToCategory("Specials", "Egg Podimas", 90);
    r1.addMenuItemToCategory("Starters", "Soup", 70);
    r1.addMenuItemToCategory("Starters", "Chicken 65", 130);
    r1.addMenuItemToCategory("Starters", "Paneer Tikka", 100);

    r2.addMenuItemToCategory("Breakfast", "Poori", 55);
    r2.addMenuItemToCategory("Breakfast", "Upma", 30);
    r2.addMenuItemToCategory("Breakfast", "Kesari", 35);
    r2.addMenuItemToCategory("Lunch", "Fried Rice", 120);
    r2.addMenuItemToCategory("Lunch", "Curry Meals", 140);
    r2.addMenuItemToCategory("Lunch", "Paneer Butter Masala", 160);
    r2.addMenuItemToCategory("Specials", "Chettinad Veg Curry", 110);
    r2.addMenuItemToCategory("Specials", "Schezwan Paneer", 100);
    r2.addMenuItemToCategory("Specials", "Banana Leaf Meals", 180);
    r2.addMenuItemToCategory("Starters", "Veg Soup", 35);
    r2.addMenuItemToCategory("Starters", "Gobi 65", 90);
    r2.addMenuItemToCategory("Starters", "Veg Spring Roll", 80);

    r3.addMenuItemToCategory("Breakfast", "Plain Dosa", 60);
    r3.addMenuItemToCategory("Breakfast", "Masala Dosa", 80);
    r3.addMenuItemToCategory("Breakfast", "Set Dosa", 70);
    r3.addMenuItemToCategory("Lunch", "Veg Meals", 120);
    r3.addMenuItemToCategory("Lunch", "Sambar Rice", 95);
    r3.addMenuItemToCategory("Lunch", "Curd Rice", 70);
    r3.addMenuItemToCategory("Specials", "Tirunelveli Halwa", 60);
    r3.addMenuItemToCategory("Specials", "Avial", 75);
    r3.addMenuItemToCategory("Specials", "Kanchipuram Idli", 80);
    r3.addMenuItemToCategory("Starters", "Onion Pakora", 40);
    r3.addMenuItemToCategory("Starters", "Vegetable Cutlet", 60);
    r3.addMenuItemToCategory("Starters", "Corn Tikki", 55);
}
void displayPreviousOrders() {
    if (previousOrders.empty()) {
        cout << "No previous orders placed.\n";
    } else {
        cout << "Previous Orders:\n";
        queue<Order> temp = previousOrders;
        while (!temp.empty()) {
            Order o = temp.front();
            temp.pop();
            cout << "Order #" << o.id << " from " << o.restaurantName << ":\n";
            for (auto& i : o.items) {
                cout << "  " << i.itemName << " x" << i.quantity << endl;
            }
        }
    }
}
void cancelLatestOrder() {
    if (previousOrders.empty()) {
        cout << "No orders to cancel.\n";
        return;
    }
    queue<Order> tempQueue;
    Order latestOrder;
    while (previousOrders.size() > 1) {
        tempQueue.push(previousOrders.front());
        previousOrders.pop();
    }
    latestOrder = previousOrders.front();
    previousOrders.pop();
    cancelledOrders.push(latestOrder);
    while (!tempQueue.empty()) {
        previousOrders.push(tempQueue.front());
        tempQueue.pop();
    }
    cout << "Order #" << latestOrder.id << " cancelled.\n";
    // File management updates
    saveCancelledOrdersToFile(cancelledOrders, "cancelled_orders.txt");
    rewriteOrdersFile(previousOrders, "orders.txt");
}
void viewCancelledOrders() {
    if (cancelledOrders.empty()) {
        cout << "No cancelled orders.\n";
    } else {
        cout << "Cancelled Orders:\n";
        stack<Order> temp = cancelledOrders;
        while (!temp.empty()) {
            Order o = temp.top();
            temp.pop();
            cout << "Order #" << o.id << " from " << o.restaurantName << ":\n";
            for (auto& i : o.items) {
                cout << "  " << i.itemName << " x" << i.quantity << "\n";
            }
        }
    }
}

// ======= BST for Restaurant Sorting by Average Price =======
class BSTNode {
public:
    Restaurant* data;
    BSTNode* left;
    BSTNode* right;
    BSTNode(Restaurant* r) : data(r), left(nullptr), right(nullptr) {}
};

class RestaurantBST {
private:
    BSTNode* root;
    BSTNode* insert(BSTNode* root, Restaurant* r);
    void inorder(BSTNode* node);
public:
    RestaurantBST() : root(nullptr) {}
    void insert(Restaurant* r);
    void displaySorted();
};

BSTNode* RestaurantBST::insert(BSTNode* root, Restaurant* r) {
    if (!root) return new BSTNode(r);
    if (r->getAveragePrice() < root->data->getAveragePrice())
        root->left = insert(root->left, r);
    else
        root->right = insert(root->right, r);
    return root;
}
void RestaurantBST::inorder(BSTNode* node) {
    if (!node) return;
    inorder(node->left);
    cout << node->data->name << " (Avg Price: Rs." << node->data->getAveragePrice() << ")\n";
    inorder(node->right);
}
void RestaurantBST::insert(Restaurant* r) {
    root = insert(root, r);
}
void RestaurantBST::displaySorted() {
    cout << "Restaurants sorted by average price:\n";
    inorder(root);
}
// Heap sort for restaurants by rating
void maxHeapify(vector<Restaurant*>& arr, int n, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;
    if (left < n && arr[left]->rating > arr[largest]->rating)
        largest = left;
    if (right < n && arr[right]->rating > arr[largest]->rating)
        largest = right;
    if (largest != i) {
        swap(arr[i], arr[largest]);
        maxHeapify(arr, n, largest);
    }
}
void heapSort(vector<Restaurant*>& arr) {
    int n = arr.size();
    for (int i = n / 2 - 1; i >= 0; i--)
        maxHeapify(arr, n, i);
    for (int i = n - 1; i >= 0; i--) {
        swap(arr[0], arr[i]);
        maxHeapify(arr, i, 0);
    }
}

// ======= Health Profile =======
map<string, vector<string>> foodIngredients = {
    {"Idli", {"rice","urad dal"}},
    {"Pongal", {"rice","ghee","pepper","milk"}},
    {"Vada", {"urad dal","oil"}},
    {"Mutton Briyani", {"mutton","rice","ghee","spices"}},
    {"Parotta", {"maida","oil"}},
    {"Chicken Curry", {"chicken","spices","oil"}},
    {"Malabar Fish Curry", {"fish","coconut","spices"}},
    {"Hyderabadi Mutton", {"mutton","spices","ghee"}},
    {"Egg Podimas", {"egg","pepper","oil"}},
    {"Soup", {"vegetables","salt"}},
    {"Chicken 65", {"chicken","maida","spices"}},
    {"Paneer Tikka", {"paneer","spices","oil"}},
    {"Poori", {"maida","oil"}},
    {"Upma", {"rava","ghee"}},
    {"Kesari", {"semolina","sugar","ghee"}},
    {"Fried Rice", {"rice","egg","soy sauce"}},
    {"Curry Meals", {"rice","dal","various veggies"}},
    {"Paneer Butter Masala", {"paneer","butter","cream","milk","cashew"}},
    {"Chettinad Veg Curry", {"coconut","spices","vegetables"}},
    {"Schezwan Paneer", {"paneer","chilli sauce","soy sauce"}},
    {"Banana Leaf Meals", {"rice","various curries"}},
    {"Veg Soup", {"vegetables","salt"}},
    {"Gobi 65", {"cauliflower","maida"}},
    {"Veg Spring Roll", {"vegetables","maida"}},
    {"Plain Dosa", {"rice","urad dal"}},
    {"Masala Dosa", {"rice","urad dal","potato"}},
    {"Set Dosa", {"rice","urad dal"}},
    {"Veg Meals", {"rice","dal","sambar","vegetables"}},
    {"Sambar Rice", {"rice","dal","sambar"}},
    {"Curd Rice", {"curd","milk","rice"}},
    {"Tirunelveli Halwa", {"wheat","sugar","ghee"}},
    {"Avial", {"vegetables","coconut"}},
    {"Kanchipuram Idli", {"rice","urad dal"}},
    {"Onion Pakora", {"onion","gram flour","spices"}},
    {"Vegetable Cutlet", {"potato","bread crumbs"}},
    {"Corn Tikki", {"corn","potato","spices"}}
};
class HealthProfile {
public:
    string name;
    vector<string> allergies;
    vector<string> conditions;
    void inputProfile();
    static void parseList(string input, vector<string>& list);
    bool isUnsafe(const string& ingredient) const;
    void viewProfile() const;
};

void HealthProfile::inputProfile() {
    cout << "\nEnter your name: ";
    cin >> ws;
    getline(cin, name);
    cout << "Enter your allergies (comma-separated, e.g., milk,ghee,sugar,egg,coconut). If none, press enter: ";
    string allergyInput;
    getline(cin, allergyInput);
    parseList(allergyInput, allergies);
}
void HealthProfile::parseList(string input, vector<string>& list) {
    list.clear();
    string word = "";
    for (char c : input) {
        if (c == ',' ) {
            if (!word.empty()) {
                string trimmed;
                for (char ch : word) if (!isspace((unsigned char)ch)) trimmed += tolower(ch);
                if(!trimmed.empty()) list.push_back(trimmed);
            }
            word = "";
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        string trimmed;
        for (char ch : word) if (!isspace((unsigned char)ch)) trimmed += tolower(ch);
        if(!trimmed.empty()) list.push_back(trimmed);
    }
}
bool HealthProfile::isUnsafe(const string& ingredient) const {
    string lowIng = ingredient;
    transform(lowIng.begin(), lowIng.end(), lowIng.begin(), [](unsigned char c){ return tolower(c); });
    for (const string& a : allergies) {
        if (a.empty()) continue;
        if (lowIng.find(a) != string::npos) return true;
    }
    return false;
}
void HealthProfile::viewProfile() const {
    cout << "\n--- Your Health Profile ---\n";
    cout << "Name: " << name << "\nAllergies: ";
    if (allergies.empty()) cout << "None";
    else for (auto& a : allergies) cout << a << " ";
    cout << "\nConditions: ";
    if (conditions.empty()) cout << "None";
    else for (auto& c : conditions) cout << c << " ";
    cout << "\n";
}
HealthProfile userProfile;

int orderCounter = 1;
deque<pair<time_t, int>> windowedOffers;
const int OFFER_LIMIT = 50;
const int PROMO_WINDOW_SECS = 7200;
const int PROMO_START_HOUR = 21;
const int PROMO_END_HOUR = 23;

// ======= Discount, Offers, VIP Heap Helpers =======
int getCurrentHour() {
    time_t now = time(nullptr);
    now += 19800; // for IST; adjust as needed for your region
    struct tm* now_tm = localtime(&now);
    return now_tm->tm_hour;
}
bool isClosingDiscountTime() {
    int hour = getCurrentHour();
    return (hour >= 22 && hour < 24);
}
bool isTwentyPercentOfferTime() {
    int hour = getCurrentHour();
    return (hour >= PROMO_START_HOUR && hour < PROMO_END_HOUR);
}
bool isEligibleForOffer(int orderId) {
    time_t currTime = time(nullptr);
    while (!windowedOffers.empty() &&
            difftime(currTime, windowedOffers.front().first) > PROMO_WINDOW_SECS)
        windowedOffers.pop_front();
    int hour = getCurrentHour();
    if (hour < PROMO_START_HOUR || hour >= PROMO_END_HOUR) return false;
    if ((int)windowedOffers.size() < OFFER_LIMIT) {
        windowedOffers.push_back({currTime, orderId});
        return true;
    }
    return false;
}

// VIP heap maintenance
map<string, int> customerOrderCount;
struct VipNode { string name; int count; };
VipNode minHeap[5];
int heapSize = 0;
void heapBubbleUp(int idx) {
    while (idx > 0 && minHeap[idx].count < minHeap[(idx-1)/2].count) {
        swap(minHeap[idx], minHeap[(idx-1)/2]);
        idx = (idx-1)/2;
    }
}
void heapify(int idx) {
    while (true) {
        int left = 2*idx + 1, right = 2*idx + 2, smallest = idx;
        if (left < heapSize && minHeap[left].count < minHeap[smallest].count)
            smallest = left;
        if (right < heapSize && minHeap[right].count < minHeap[smallest].count)
            smallest = right;
        if (smallest != idx) {
            swap(minHeap[idx], minHeap[smallest]);
            idx = smallest;
        } else break;
    }
}
void insertOrUpdateHeap(string name, int count) {
    for (int i = 0; i < heapSize; i++) {
        if (minHeap[i].name == name) {
            minHeap[i].count = count;
            heapBubbleUp(i);
            heapify(i);
            return;
        }
    }
    if (heapSize < 5) {
        minHeap[heapSize++] = VipNode{name, count};
        heapBubbleUp(heapSize-1);
    }
    else if (count > minHeap[0].count) {
        minHeap[0] = VipNode{name, count};
        heapify(0);
    }
}
void rebuildHeap() {
    heapSize = 0;
    for (auto& p : customerOrderCount)
        insertOrUpdateHeap(p.first, p.second);
}
bool isVIP(const string& name) {
    for (int i=0; i<heapSize; ++i)
        if (minHeap[i].name == name) return true;
    return false;
}
void showVIPs() {
    vector<VipNode> sortedVIPs(minHeap, minHeap+heapSize);
    sort(sortedVIPs.begin(), sortedVIPs.end(), [](VipNode a, VipNode b) {
        return a.count > b.count;
    });
    cout << "\n--- VIP Leaderboard (Top 5 Orderers) ---\n";
    for (int i = 0; i < sortedVIPs.size(); ++i) {
        cout << (i+1) << ". " << sortedVIPs[i].name << " (" << sortedVIPs[i].count << " orders)\n";
    }
}

void viewMenuAndPlaceOrder() {
    cout << "Enter customer name: ";
    string customerName;
    cin >> ws; getline(cin, customerName);

    viewAllRestaurants();
    cout << "Select a restaurant (1-3): ";
    int rInd;
    cin >> rInd;
    if (rInd < 1 || rInd > 3) {
        cout << "Invalid restaurant selection.\n";
        return;
    }
    Restaurant* rest = restaurants[rInd - 1];

    vector<OrderItem> itemsOrdered;
    char more = 'y';
    double totalPrice = 0.0;
    bool discountActive = isClosingDiscountTime();
    while (more == 'y' || more == 'Y') {
        cout << "\nCategories:\n";
        rest->displayCategories();
        cout << "Select category name (type exactly as shown): ";
        string category;
        cin >> ws; getline(cin, category);

        if (rest->categoryMenus.find(category) == rest->categoryMenus.end()) {
            cout << "Invalid category.\n";
            continue;
        }

        rest->displayCategoryMenu(category);

        cout << "Select item number to order: ";
        int itemNum;
        cin >> itemNum;
        MenuItem* item = rest->categoryMenus[category].getItem(itemNum);
        if (!item) {
            cout << "Invalid item selection.\n";
            continue;
        }

        auto it = foodIngredients.find(item->name);
        bool riskDetected = false;
        if (it != foodIngredients.end()) {
            for (const string& ing : it->second) {
                if (userProfile.isUnsafe(ing)) {
                    cout << "\n⚠  Warning: " << item->name << " contains '" << ing
                         << "' which may be unsafe for you (" << userProfile.name << ").\n";
                    riskDetected = true;
                }
            }
        }
        if (riskDetected) {
            cout << "Do you still want to proceed with this item? (y/n): ";
            char proceedChoice;
            cin >> proceedChoice;
            if (proceedChoice == 'n' || proceedChoice == 'N') {
                cout << "Item skipped due to health concerns.\n";
                cout << "Add more items? (y/n): ";
                cin >> more;
                continue;
            }
        }
        cout << "Enter quantity: ";
        int qty; cin >> qty;
        itemsOrdered.push_back({item->name, qty});
        double priceToUse = item->price;
        if (discountActive) {
            priceToUse *= 0.5;
        } else if (isTwentyPercentOfferTime()) {
            priceToUse *= 0.8;
        }
        totalPrice += priceToUse * qty;
        cout << "Add more items? (y/n): ";
        cin >> more;
    }
    if (itemsOrdered.empty()) {
        cout << "\nNo items were ordered. Order was not placed.\n";
        return; // Exit without creating an order
    }
    int id = orderCounter++;
    Order newOrder{id, rest->name, itemsOrdered};
    previousOrders.push(newOrder);
    // File management integration
    saveOrderToFile(newOrder, "orders.txt");

    if (isEligibleForOffer(id)) {
        cout << "\nCongratulations! You are among the first 50 customers from 9 to 11 PM and received a special offer!\n";
    } else {
        cout << "\nOrder placed (standard pricing).\n";
    }

    if (discountActive) {
        cout << "\nClosing Time Offer! 50% discount applied on items (10 PM - 12 AM).\n";
    }
    if (isTwentyPercentOfferTime() && !discountActive) {
        cout << "\nSpecial Offer! 20% discount applied on your items (9 PM - 11 PM).\n";
    }

    cout << "Order details:\n";
    for (auto& i : itemsOrdered) {
        cout << i.itemName << " x" << i.quantity << endl;
    }
    cout << "Total price: Rs." << totalPrice << endl;
    cout << "Is this a group order? (y/n): ";
    char isGroup;
    cin >> isGroup;
    if (isGroup == 'y' || isGroup == 'Y') {
        cout << "Enter number of members: ";
        int n; cin >> n;
        cout << "\n--- Group Split Summary ---\n";
        cout << "Total Bill: Rs." << totalPrice << endl;
        if (n > 0) {
            double perPerson = totalPrice / n;
            cout << "Each member should pay: Rs." << perPerson << endl;
            cout << "All settled perfectly!\n";
        } else {
            cout << "No members specified.\n";
        }
    }
    customerOrderCount[customerName]++;
    rebuildHeap();
    if (isVIP(customerName)) {
        cout << "[VIP] You are a Top 5 Customer!\n";
        showVIPs();
    }
}

// ======= Main Menu Driver =======
int main() {
    initializeMenus();
    loadOrdersFromFile("orders.txt", previousOrders);
    loadCancelledOrdersFromFile("cancelled_orders.txt", cancelledOrders);

    // Determine orderCounter (restore correct value)
    if (!previousOrders.empty()) {
        queue<Order> temp = previousOrders;
        while (!temp.empty()) {
            if (temp.front().id >= orderCounter) orderCounter = temp.front().id + 1;
            temp.pop();
        }
    }
    if (!cancelledOrders.empty()) {
        stack<Order> temp = cancelledOrders;
        while (!temp.empty()) {
            if (temp.top().id >= orderCounter) orderCounter = temp.top().id + 1;
            temp.pop();
        }
    }

    cout << "Welcome to the Food Delivery System!\n";
    userProfile.inputProfile();
    int choice;
    do {
        cout << "\n==== Food Delivery System ====\n";
        cout << "1. View all restaurants\n";
        cout << "2. View menu and place order\n";
        cout << "3. Display previous orders\n";
        cout << "4. Cancel latest order\n";
        cout << "5. View cancelled orders\n";
        cout << "6. Show restaurants sorted by rating\n";
        cout << "7. Show restaurants sorted by average price\n";
        cout << "8. Show time-windowed offer winners (9–11 PM)\n";
        cout << "9. View Health Profile\n";
        cout << "0. Exit\n";
        cout << "Enter your choice: ";
        cin >> choice;
        switch(choice) {
            case 1: viewAllRestaurants(); break;
            case 2: viewMenuAndPlaceOrder(); break;
            case 3: displayPreviousOrders(); break;
            case 4: cancelLatestOrder(); break;
            case 5: viewCancelledOrders(); break;
            case 6: {
                vector<Restaurant*> sortedRestaurants = {&r1, &r2, &r3};
                heapSort(sortedRestaurants);
                cout << "\nRestaurants sorted by rating (descending):\n";
                for (int i = sortedRestaurants.size() - 1; i >= 0; i--) {
                    cout << sortedRestaurants[i]->name << " (Rating: " << sortedRestaurants[i]->rating << ")\n";
                }
                break;
            }
            case 7: {
                RestaurantBST bst;
                bst.insert(&r1);
                bst.insert(&r2);
                bst.insert(&r3);
                bst.displaySorted();
                break;
            }
            case 8: {
                cout << "Offer winners (current hour window):\n";
                for (auto& entry : windowedOffers) {
                    cout << "OrderID: " << entry.second << " Time: " << ctime(&entry.first);
                }
                break;
            }
            case 9: userProfile.viewProfile(); break;
            case 0: cout << "Exiting...\n"; break;
            default: cout << "Invalid choice.\n"; break;
        }
    } while (choice != 0);
    cout << "Thank you for using the Food Delivery System!\n";
    return 0;
}
