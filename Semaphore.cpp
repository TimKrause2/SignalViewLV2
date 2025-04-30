#include "Semaphore.h"

Semaphore::Semaphore(void)
{
    sem_init(&sem, 0, 0);
}

Semaphore::~Semaphore(void)
{
    sem_destroy(&sem);
}

void Semaphore::post(void)
{
    sem_post(&sem);
}

void Semaphore::wait(void)
{
    sem_wait(&sem);
}
