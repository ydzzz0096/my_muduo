#include "base/Thread.h"
#include "base/Logger.h"
#include <iostream>
#include <unistd.h> // for sleep

void thread_func()
{
    LOG_INFO << "Thread running...";
    sleep(1);
}

int main()
{
    LOG_INFO << "Main thread started. Creating a new thread...";

    Thread myThread(thread_func, "MyWorkerThread");
    myThread.start();

    LOG_INFO << "Thread name: " << myThread.name();
    LOG_INFO << "Thread started, waiting for it to join...";

    myThread.join();

    LOG_INFO << "Thread joined. Main thread exiting.";

    return 0;
}