#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <unistd.h>
#include <random>
#include "../include/task_scheduler.hpp"

void output(int num) {
    std::ofstream fout(std::to_string(std::hash<std::thread::id>()(std::this_thread::get_id())) + ".out", std::ios::out|std::ios::app);
    fout << num << std::endl;
    fout.close();
}

void test() {
    std::atomic<int> count = 0;
    auto scheduler = std::make_shared<odts::task_scheduler<int>>(3, [&count](int num){
        return [&count, num](){
            usleep(num * 1000);
            output(num);
            count += 1;
        };
    }, [](std::function<void()> func){
        std::thread t(func);
        t.detach();
    });
    std::thread t[100];
    for (size_t i = 0; i < 100; i++)
    {
        t[i] = std::thread([scheduler](){
            scheduler->append(random() % 100 + 1);
        });
    }
    
    for (size_t i = 0; i < 100; i++)
    {
        t[i].join();
    }
    while(count.load() < 100) {
        usleep(10);
    }
    usleep(200 * 1000);
}

int main() {
    test();
    test();
    return 0;
}