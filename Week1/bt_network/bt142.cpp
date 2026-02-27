#include <iostream>
#include <pthread.h>
using namespace std;

long long count = 0;

void* dem(void* var)
{
    for (int i = 0; i < 1000000; i++)
    {
        count++;
        // cout << count << endl;
    }
    return NULL;
}

int main()
{
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, dem, NULL);
    pthread_create(&thread2, NULL, dem, NULL);
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    cout << count << endl;

    return 0;
}