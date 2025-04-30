#include <semaphore.h>

class Semaphore
{
    sem_t  sem;
public:
    Semaphore(void);
    ~Semaphore(void);
    void post(void);
    void wait(void);
};

