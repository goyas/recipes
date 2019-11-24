#include<assert.h>
#include<signal.h>
#include<stdio.h>
#include<sys/epoll.h>
#include<sys/time.h>
#include<sys/wait.h>
#include<unistd.h>

int  fd[2];
int* write_fd;
int* read_fd;
const char msg[] = {'m','e','s','s','a','g','e'};

void SigHandler(int){
    size_t bytes = write(*write_fd, msg, sizeof(msg));
    printf("children process msg have writed : %ld bytes\n", bytes);
}

void ChildrenProcess() {
    struct sigaction sa;
    sa.sa_flags     =   0;
    sa.sa_handler   =   SigHandler;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval tick   =   {0};
    tick.it_value.tv_sec    =   1;   // 1s后将启动定时器
    tick.it_interval.tv_sec =   1;   // 定时器启动后，每隔1s将执行相应的函数

    // setitimer将触发SIGALRM信号，此处使用的是ITIMER_REAL，所以对应的是SIGALRM信号
    assert(setitimer(ITIMER_REAL, &tick, NULL) == 0);
    while(true) {
        pause();
    }
}

void FatherProcess() {
    epoll_event ev;
    epoll_event events[1];
    char buf[1024]  =   {0};
    int epoll_fd    =   epoll_create(1);
    ev.data.fd      =   *read_fd;
    ev.events       =   EPOLLIN | EPOLLET;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, *read_fd, &ev);

    while (true) {
        int fds = epoll_wait(epoll_fd, events, sizeof(events), -1);
        if(events[0].data.fd == *read_fd) {
            size_t bytes = read(*read_fd, buf, sizeof(buf));
            printf("father process read %ld bytes = %s\n", bytes, buf);
        }
    }

    int status;
    wait(&status);
}

int main() {
    int ret = pipe(fd);
    if (ret != 0) {
        printf("pipe failed\n");
        return -1;
    }
    write_fd = &fd[1];
    read_fd  = &fd[0];

    pid_t pid = fork();
    if (pid == 0) {//child process
        ChildrenProcess();
    } else if (pid > 0) {//father process
        FatherProcess();
    }

    return 0;
}