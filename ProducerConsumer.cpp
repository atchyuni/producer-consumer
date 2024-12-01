// compile using g++ -std=c++20 -o ProducerConsumer ProducerConsumer.cpp -pthread

#include <iostream>
#include <cassert>
#include <vector> // push_back to append
#include <cstdlib> // for rand()

// member functions: empty, front, push, pop
#include <queue>

// member functions: sleep_for, join
#include <thread>
#include <chrono>

// documentation: critical section
// between acquire() and release()
#include <semaphore.h>

using namespace std;

queue<int> buffer; // each location represents job process time

void producer(int unique_id, const int jobs, counting_semaphore<>& spc, counting_semaphore<>& itm, binary_semaphore& mtx);
void consumer(int unique_id, counting_semaphore<>& spc, counting_semaphore<>& itm, binary_semaphore& mtx);

int main(int argc, char* argv[]) {
    assert(argc == 5); // error handling: necessary args

    // 1. set-up and initialise the required data structures and variables
    int size_of_queue = atoi(argv[1]);
    int jobs_per_producer = atoi(argv[2]);
    int num_of_producers = atoi(argv[3]);
    int num_of_consumers = atoi(argv[4]);

    // error handling
    assert(size_of_queue > 0);
    assert(jobs_per_producer > 0);
    assert(num_of_producers > 0);
    assert(num_of_consumers > 0);

    // 2. set-up and initialise semaphores
    counting_semaphore<> space(size_of_queue); // to ensure buffer not full
    counting_semaphore<> item(0); // to ensure buffer not empty
    binary_semaphore mutex(1);

    // 3. create the required producers and consumers
    vector<thread> producers; // dynamic array of threads
    vector<thread> consumers;

    for (int i = 0; i < num_of_producers; i++) {
        producers.push_back(thread(producer, i + 1, jobs_per_producer, ref(space), ref(item), ref(mutex)));
    }

    for (int j = 0; j < num_of_consumers; j++) {
        consumers.push_back(thread(consumer, j + 1, ref(space), ref(item), ref(mutex)));
    }

    // join() waits for thread to finish execution
    for (int i = 0; i < num_of_producers; i++) {
        producers[i].join();
    }

    for (int j = 0; j < num_of_consumers; j++) {
        consumers[j].join();
    }

    // 4. quit
    return 0;
}

void producer(int unique_id, int jobs, counting_semaphore<>& spc, counting_semaphore<>& itm, binary_semaphore& mtx) {
    // add required number of jobs to queue
    for (int i = 0; i < jobs; i++) {
        // down (space)
        // block on full buffer
        if (!spc.try_acquire_for(chrono::seconds(10))) {
            cout << "Producer " << unique_id << " is quitting";
            cout << " after waiting 10 seconds\n";
            return; // quit after waiting 10 seconds with no new slot
        }

        mtx.acquire(); // down (mutex)

        // CRITICAL SECTION: deposit item aka produce job
        buffer.push(rand() % 10 + 1); // writing random int (1-10)
        cout << "Producer " << unique_id << " produced job ";
        cout << (i + 1) << '\n';

        mtx.release(); // up (mutex)
        itm.release(); // up (item) aka signal not empty
    }
}

void consumer(int unique_id, counting_semaphore<>& spc, counting_semaphore<>& itm, binary_semaphore& mtx) {
    while (true) {
        // down (item)
        // block on empty buffer
        if (!itm.try_acquire_for(chrono::seconds(10))) {
            cout << "Consumer " << unique_id << " is quitting";
            cout << " after waiting 10 seconds\n";
            return; // quit after waiting 10 seconds with no new jobs
        }

        mtx.acquire(); // down (mutex)

        // CRITICAL SECTION: fetch item aka take job
        int job_time = buffer.front();
        buffer.pop(); // delete from queue

        mtx.release(); // up (mutex)
        spc.release(); // up (space) aka signal not full

        // consume item
        cout << "Consumer " << unique_id << " consumed a job";
        cout << " in " << job_time << " second";
        if (job_time > 1) {
            cout << "s";
        }
        cout << '\n';
        this_thread::sleep_for(chrono::seconds(job_time));
    }
}
