#include <iostream>
#include <pthread.h>
#include <mutex>
using namespace std;

long long count = 0;
// pthread_mutex_t abc;
mutex abcd;

void* dem(void* var)
{
    for (int i = 0; i < 1000000; i++)
    {
        // pthread_mutex_lock(&abc);
        abcd.lock();
        
        count++;
        // cout << count << endl;
        abcd.unlock();

        // pthread_mutex_unlock(&abc);
    }
    return NULL; // do hàm được khởi tạo bằng void* bắt buộc phải trả về 1 con trỏ, nếu không cần dữ liệu trả về thì return NULL.
}

int main()
{
    // pthread_mutex_init(&abc, NULL); // NULL ở đây là thuộc tính mặc định mà khi hàm mutex_init được định nghĩa

    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, dem, NULL); // NULL thứ nhất cũng là thuộc tính mặc định,  NULL thứ 2 là do hàm dem không cần dữ liệu truyền vào
    pthread_create(&thread2, NULL, dem, NULL);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // pthread_mutex_destroy(&abc);

    cout << "Final count: " << count << endl;

    return 0;
}