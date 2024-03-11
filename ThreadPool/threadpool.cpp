#include<thread>
#include<iostream>
#include<memory>
#include<vector>
#include<mutex>
#include<algorithm>
#include<condition_variable>
#include<queue>
#include<functional>
#include<chrono>
using namespace std;


class Threadpool
{
public:
    Threadpool(int threadnum):stop(false)
    {
        for(int i=0;i<threadnum;i++)
        {
            threads.emplace_back(threadwork,this);
        }
    }
    ~Threadpool()
    {
        {
          unique_lock<mutex> lock(mtx);
          stop = true;
        }
        cv.notify_all();
        for(auto &threadnum : threads)
        {
            threadnum.join();
        }
    }
    static void threadwork(Threadpool *tp)
    {
        function<void()> task;
        while(1)
        {
            unique_lock<mutex> lock(tp->mtx);
            tp->cv.wait(lock,[tp]
            {
                return tp->stop || !tp->tasks.empty();
            });
            if(tp->stop&&tp->tasks.empty()) return;
            task = move(tp->tasks.front());
            tp->tasks.pop();
            task();
        }
    }
    template<class F,class ...Args>
    auto enque(F&& f,Args&& ...args)
    {
      {
        unique_lock<mutex> lock(mtx);
        tasks.emplace(bind(forward<F>(f),forward<Args>(args)...));
      }
      cv.notify_one();
    }

    Threadpool(const Threadpool& x) = delete;
	  const Threadpool& operator = (const Threadpool& x) = delete;

private:
    mutex mtx;
    queue<function<void()>> tasks;
    vector<std::thread> threads;
    condition_variable cv;
    bool stop;
};

int main()
{
    Threadpool pool(4);
    for(int i = 0;i<=5;i++)
    {
        cout << "启动" << endl;
        pool.enque([i]
        {
            cout << "任务" << i <<"正在执行" << endl;
            std::this_thread::sleep_for(chrono::seconds(1));
            cout << "任务" << i <<"成功" << endl;
        });
    }
    return 0;
}
