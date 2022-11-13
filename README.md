# od-task-scheduler

## od-task-scheduler is a header only task scheduler for C++17 & 17+


User need to provider a function to convert data(int, float, custom struct, etc.) to a runable task, and a method to dispatch the task to target thread. Then, the user just append the data into the scheduler, and the scheduler would select a proper executor to run the task.


support to set cocurrent task number.

```C++
task_scheduler(
    size_t max_executor, // Maximum cocurrent task number
    std::function<std::function<void()>(T)> to_task, // A function that covert a T to a std::function<void()>
    std::function<void(std::function<void()>)> dispatch // A function to make a std::function<void()> run on the target thread
)

```

usage

```C++
std::shared_ptr<odts::task_scheduler<int>> scheduler = nullptr;
void init() {
    // a scheduler that maintain a int queue and run the task with 3 executor
    auto scheduler = std::make_shared<odts::task_scheduler<int>>(3, [](int num){
        return [num](){
            std::cout << num << std::endl; // just output it
        };
    }, [](std::function<void()> func){ // run the executor on other thread
        std::thread t(func);
        t.detach();
    });
}
...

void test() {
    scheduler->append(1);
}
```
