// micro2.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <random>
#include <thread>
#include <ctime>


std::mutex              lock_cout;
std::mutex              lock_queue;
std::condition_variable queuecheck;
std::queue<int>         queue_clients;
bool                    done;
bool                    notify;

void clientFunc(int id, std::mt19937& generator)
{
    // симуляция работы
    std::this_thread::sleep_for(std::chrono::seconds(1 + generator() % 20));
    // симуляция ошибки
    int errorcode = id;
    {
        std::unique_lock<std::mutex> locker(lock_cout);
        std::cout << "Time: " << clock() / 1000.0 << "\tclient " << id << ":\tsigned up for a haircut" << std::endl;
    }
    // сообщаем об ошибке
    {
        std::unique_lock<std::mutex> locker(lock_queue);
        queue_clients.push(errorcode);
        notify = true;
        queuecheck.notify_one();
    }
}

void barberFunc()
{
    // стартовое сообщение
    {
        std::unique_lock<std::mutex> locker(lock_cout);
        std::cout << "Time: " << clock() / 1000.0 << "\tbarber:\tsleep..." << std::endl;
    }
    // до тех пор, пока не будет получен сигнал
    while (!done)
    {
        std::unique_lock<std::mutex> locker(lock_queue);
        while (!notify) // от ложных пробуждений
            queuecheck.wait(locker);

        // если есть очередь
        while (!queue_clients.empty())
        {
            std::unique_lock<std::mutex> locker(lock_cout);
            std::cout << "Time: " << clock() / 1000.0 << "\tbarber:\t\tfinished haircut for client " << queue_clients.front() << std::endl;
            queue_clients.pop();
        }
        notify = false;
    }
}

int main()
{
    //вводимые пользователем значения количества клиентов
    int threadsNum;

    std::cout << "input the number of clients: ";
    std::cin >> threadsNum;

    //обработка данных
    if (threadsNum <= 0) {
        std::cout << "Wrong! Threads number or matrix size can not be less than 0." << std::endl;
    }
    else {
        // инициализация генератора случайных чисел
        std::mt19937 generator((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
        // запуск парикмахера
        std::thread loggerThread(barberFunc);
        // запуск клиентов
        std::vector<std::thread> threads;
        for (int i = 0; i < threadsNum; ++i)
        {
            threads.push_back(std::thread(clientFunc, i + 1, std::ref(generator)));
        }
        for (auto& t : threads)
            t.join();
        // сообщаем парикмахеру о завершении и ожидаем его
        done = true;
        loggerThread.join();
        std::cout << "The working day is over!" << std::endl;
    }
    return 0;
}
