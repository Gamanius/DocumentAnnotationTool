#include "DocantoLib.h"
#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <chrono> // For std::chrono::duration_cast
using namespace Docanto;

// Existing test functions (copied for completeness, though not strictly necessary to rewrite them)
void test_read(ReadWriteThreadSafeMutex<int>& data, int thread_num) {
    auto lock = data.get_read();
    Logger::log("[R", thread_num, "] got value: ", *lock);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

void test_write(ReadWriteThreadSafeMutex<int>& data, int thread_num) {
    auto lock = data.get_write();
    (*lock)++;
    Logger::log("[W", thread_num, "] incremented value to: ", *lock);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
}

void test_reentrant_read(ReadWriteThreadSafeMutex<int>& data) {
    auto lock1 = data.get_read();
    Logger::log("[Reentrant Read] First: ", *lock1);
    auto lock2 = data.get_read();
    Logger::log("[Reentrant Read] Second: ", *lock2);
}

void test_reentrant_write(ReadWriteThreadSafeMutex<int>& data) {
    auto lock1 = data.get_write();
    Logger::log("[Reentrant Write] First: ", *lock1);
    auto lock2 = data.get_write();
    (*lock2) += 10;
    Logger::log("[Reentrant Write] Second: ", *lock2);
}

void test_read_write_conflict(ReadWriteThreadSafeMutex<int>& data) {
    std::promise<void> reader_started;
    std::shared_future<void> reader_ready = reader_started.get_future();
    std::thread reader([&]() {
        auto lock = data.get_read();
        Logger::log("[ReadBeforeWrite] Reader got value: ", *lock);
        reader_started.set_value();
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // hold it
        });

    std::thread writer([&]() {
        reader_ready.wait(); // wait until reader has the lock
        auto start_wait = std::chrono::high_resolution_clock::now();
        auto lock = data.get_write(); // should block
        auto end_wait = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_wait - start_wait).count();
        if (duration >= 250) { // Check if it blocked for a significant amount of time
            Logger::log("[ReadBeforeWrite] Writer successfully blocked for ~", duration, "ms as expected.");
        }
        else {
            Logger::log("[ReadBeforeWrite] WARNING: Writer might not have blocked as expected. Duration: ", duration, "ms.");
        }
        (*lock) += 1;
        Logger::log("[ReadBeforeWrite] Writer incremented to: ", *lock);
        });

    reader.join();
    writer.join();
}

// New test for Write-Read conflict (Writer holds, then Reader tries)
void test_write_read_conflict(ReadWriteThreadSafeMutex<int>& data) {
    std::promise<void> writer_started;
    std::shared_future<void> writer_ready = writer_started.get_future();

    std::thread writer([&]() {
        auto lock = data.get_write();
        Logger::log("[WriteBeforeRead] Writer got value: ", *lock);
        (*lock)++; // Modify the value
        writer_started.set_value();
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Hold the write lock
        Logger::log("[WriteBeforeRead] Writer released lock.");
        });

    std::thread reader([&]() {
        writer_ready.wait(); // Wait until writer has the lock
        auto start_wait = std::chrono::high_resolution_clock::now();
        auto lock = data.get_read(); // Should block until writer releases
        auto end_wait = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_wait - start_wait).count();
        if (duration >= 250) { // Check if it blocked for a significant amount of time
            Logger::log("[WriteBeforeRead] Reader successfully blocked for ~", duration, "ms as expected.");
        }
        else {
            Logger::log("[WriteBeforeRead] WARNING: Reader might not have blocked as expected. Duration: ", duration, "ms.");
        }
        Logger::log("[WriteBeforeRead] Reader got value: ", *lock);
        });

    writer.join();
    reader.join();
}

// Test for Write-Write conflict (Writer holds, then another Writer tries)
void test_write_write_conflict(ReadWriteThreadSafeMutex<int>& data) {
    std::promise<void> writer1_started_promise;
    std::shared_future<void> writer1_started_future = writer1_started_promise.get_future();

    std::thread writer1([&]() {
        auto lock = data.get_write();
        Logger::log("[WriteBeforeWrite] Writer 1 got lock, value: ", *lock);
        (*lock) += 100; // Modify the value
        writer1_started_promise.set_value();
        std::this_thread::sleep_for(std::chrono::milliseconds(300)); // Hold the write lock
        Logger::log("[WriteBeforeWrite] Writer 1 released lock.");
        });

    std::thread writer2([&]() {
        writer1_started_future.wait(); // Wait until writer1 has the lock
        auto start_wait = std::chrono::high_resolution_clock::now();
        auto lock = data.get_write(); // Should block until writer1 releases
        auto end_wait = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_wait - start_wait).count();
        if (duration >= 250) { // Check if it blocked for a significant amount of time
            Logger::log("[WriteBeforeWrite] Writer 2 successfully blocked for ~", duration, "ms as expected.");
        }
        else {
            Logger::log("[WriteBeforeWrite] WARNING: Writer 2 might not have blocked as expected. Duration: ", duration, "ms.");
        }
        (*lock) += 10;
        Logger::log("[WriteBeforeWrite] Writer 2 incremented to: ", *lock);
        });

    writer1.join();
    writer2.join();
}

int main() {
    Logger::init(&std::wcout);
    ReadWriteThreadSafeMutex<int> shared_data(0);

    Logger::log("=== Concurrent Readers ===");
    {
        std::vector<std::thread> threads;
        for (int i = 0; i < 3; ++i)
            threads.emplace_back(test_read, std::ref(shared_data), i);
        for (auto& t : threads) t.join();
    }

    Logger::log("=== Concurrent Writers ===");
    {
        std::vector<std::thread> threads;
        for (int i = 0; i < 2; ++i)
            threads.emplace_back(test_write, std::ref(shared_data), i);
        for (auto& t : threads) t.join();
    }

    // Reset data for consistent testing of conflict scenarios
    {
        auto lock = shared_data.get_write();
        *lock = 0;
    }

    Logger::log("=== Reentrant Read ===");
    {
        std::thread t(test_reentrant_read, std::ref(shared_data));
        t.join();
    }

    // Reset data
    {
        auto lock = shared_data.get_write();
        *lock = 0;
    }

    Logger::log("=== Reentrant Write ===");
    {
        std::thread t(test_reentrant_write, std::ref(shared_data));
        t.join();
    }

    // Reset data
    {
        auto lock = shared_data.get_write();
        *lock = 0;
    }

    Logger::log("=== Read-Write Conflict Test: Reader holds, Writer tries ===");
    test_read_write_conflict(shared_data);

    // Reset data for next conflict test
    {
        auto lock = shared_data.get_write();
        *lock = 0;
    }

    Logger::log("=== Write-Read Conflict Test: Writer holds, Reader tries ===");
    test_write_read_conflict(shared_data);

    // Reset data for next conflict test
    {
        auto lock = shared_data.get_write();
        *lock = 0;
    }

    Logger::log("=== Write-Write Conflict Test: Writer holds, Another Writer tries ===");
    test_write_write_conflict(shared_data);

    Logger::log("=== Final Check ===");
    auto final = shared_data.get_read();
    Logger::log("[Final Value] ", *final);

    return 0;
}