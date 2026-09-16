#include <iostream>
#include <fstream>
#include <taskflow/taskflow.hpp>
#include <taskflow/algorithm/for_each.hpp>

void print_item(int item) {
    std::cout << item;}

void print_start() {
    std::cout << "\nS-Start\n";}

void print_end() {
    std::cout << "\nT-End\n";}

int main(){
    tf::Taskflow taskflow;
    std::vector<int> items{1,2,3,4,5,6,7,8};

    auto task = taskflow.for_each(items.begin(), items.end(), print_item);
    taskflow.emplace(print_start).name("S").precede(task);
    taskflow.emplace(print_end).name("T").succeed(task);
    std::ofstream os("taskflow.dot");
    taskflow.dump(os);
    tf::Executor executor;
    executor.run(taskflow).wait();
    return 0;
}   
