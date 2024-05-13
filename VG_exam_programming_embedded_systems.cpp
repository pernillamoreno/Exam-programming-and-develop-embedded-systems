/*g++ -std=c++11 -pthread -DCAPACITY=8 VG_exam_programming_embedded_systems.cpp -o 
VG_exam_programming_embedded_systems && ./VG_exam_programming_embedded_systems*/

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>

// If CAPACITY is not defined externally, set a default value of 10
#ifndef CAPACITY
#define CAPACITY 10  
#endif

// Mutex to ensure thread-safe console output
std::mutex cout_mutex;

// Abstract base class for vehicles
class Vehicle {
public:
    virtual void printProperties() = 0; // Pure virtual function to enforce implementation in derived classes
    virtual ~Vehicle() {}

protected:
    int ID;
    std::string model;
    std::string type;
};

// Derived class for Car
class Car : public Vehicle {
public:
    Car(int id, const std::string &model) {
        this->ID = id;
        this->model = model;
        this->type = "Car";
        this->maxPassengers = 4; // Set max passengers
    }

    void printProperties() override {
        std::lock_guard<std::mutex> lock(cout_mutex); // Lock cout to prevent jumbled output
        std::cout << "\nID: " << ID << "\nModel: " << model << "\nType: " << type
                  << "\nMax Passengers: " << maxPassengers << std::endl;
    }

private:
    int maxPassengers;
};

// Derived class for Truck
class Truck : public Vehicle {
public:
    Truck(int id, const std::string &model) {
        this->ID = id;
        this->model = model;
        this->type = "Truck";
        this->maxLoad = 4000; // Set max load capacity
    }

    void printProperties() override {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "\nID: " << ID << "\nModel: " << model << "\nType: " << type
                  << "\nMax Load: " << maxLoad << " kg\n" << std::endl;
    }

private:
    int maxLoad;
};

// Warehouse class to store and manage vehicles
class Warehouse {
public:
    Warehouse() : count(0), front(0), rear(0) {
        vehicles.resize(CAPACITY, nullptr); // Initialize the storage vector
    }

    ~Warehouse() {
        for (auto &vehicle : vehicles) {
            delete vehicle; // Ensure all vehicles are deleted to prevent memory leaks
        }
    }

    // Add a new vehicle to the warehouse
    void addVehicle(Vehicle *vehicle) {
        std::unique_lock<std::mutex> locker(mu);
        cond.wait(locker, [this]() { return count < CAPACITY; }); // Wait until there's space
        vehicles[rear] = vehicle;
        rear = (rear + 1) % CAPACITY; // Circular buffer mechanics
        count++;
        locker.unlock();
        cond.notify_all(); // Notify waiting threads
    }

    // Remove a vehicle from the warehouse
    Vehicle *removeVehicle() {
        std::unique_lock<std::mutex> locker(mu);
        cond.wait(locker, [this]() { return count > 0; }); // Wait until there's a vehicle to remove
        Vehicle *vehicle = vehicles[front];
        vehicles[front] = nullptr;
        front = (front + 1) % CAPACITY;
        count--;
        locker.unlock();
        cond.notify_all();
        return vehicle;
    }

private:
    std::vector<Vehicle *> vehicles;
    int count, front, rear;
    std::mutex mu;
    std::condition_variable cond;

    Warehouse(const Warehouse &) = delete;
    Warehouse &operator=(const Warehouse &) = delete;
};

// Producer function to generate vehicles
static void producer(Warehouse &warehouse, int startID) {
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distr(0, 1);

    int id = startID;
    while (true) {
        if (distr(eng) == 0) {
            warehouse.addVehicle(new Car(id++, "SAAB"));
        } else {
            warehouse.addVehicle(new Truck(id++, "VolvoTruck"));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(700)); // Sleep to simulate time taken to produce a vehicle
    }
}

// Consumer function to process and remove vehicles
void consumer(Warehouse &warehouse, int id) {
    while (true) {
        Vehicle *vehicle = warehouse.removeVehicle();
        std::cout << "\n====== Dealer " << id << ": "
                  << " ======\n";
        vehicle->printProperties(); // Display vehicle properties
        delete vehicle; // Free memory
        std::this_thread::sleep_for(std::chrono::milliseconds(700)); // Sleep to simulate processing time
    }
}

// Main function to setup and manage threads
int main() {
    Warehouse warehouse;

    // Create one producer thread
    std::vector<std::thread> threads;
    threads.push_back(std::thread(producer, std::ref(warehouse), 1001));

    // Create consumer threads
    int numConsumers = 4; // Define number of consumer threads
    for (int i = 1; i <= numConsumers; ++i) {
        threads.push_back(std::thread(consumer, std::ref(warehouse), i));
    }

    // Ensure all threads are completed before exiting
    for (auto &t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}
