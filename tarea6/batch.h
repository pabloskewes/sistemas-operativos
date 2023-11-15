
typedef struct job Job;
typedef void *(*JobFun)(void *ptr);

void initBatch(void);
void cleanBatch(void);

Job *submitJob(JobFun fun, void *ptr);
void *waitJob(Job *job);
void batchServerFun(void);
