#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/msg.h>

/*
 10. Взаємодія процесів. Паралелізм. Обмін повідомленнями.
 Обчислити f(x) * g(x), використовуючи 2 допоміжні процеси:
 один обчислює f(x), а інший – g(x). Основна програма виконує
 ввод-вивід та операцію *. Використати обмін повідомленнями між
 процесами (Messages). Реалізувати варіант блокуючих операцій
 обміну повідомленнями, тобто з очікуванням обробки повідомлення
 і відповіді на повідомлення (і “зависанням” процесу на цей час).
 Функції f(x) та g(x) “нічого не знають друг про друга” і не
 можуть комунікувати між собою.
 */

int f(int);
int g(int);

#define MSG_KEY 12345
#define MSG_TYPE 1

struct Message {
    long type;
    int value;
};

int f(int x) {

    if (x < 0) {
        return x * x;
    }
    return x * 2;
}

int g(int x) {

    if (x == 0) {
        while (true);
    }

    if (x < 0) {
        return x * x;
    }
    return x * 3;
}

void process(int (*func)(int), int msgqid) {
    Message msg{};
    int x;
    //The process receives a message
    msgrcv(msgqid, &msg, sizeof(msg.value), MSG_TYPE, 0);

    //The process computes
    x = func(msg.value);

    msg.type = MSG_TYPE;
    msg.value = x;
    //The process sends the message into the queue
    msgsnd(msgqid, &msg, sizeof(msg.value), IPC_NOWAIT);
}

int main() {

    pid_t f_pid, g_pid;

    //Create a message identifier
    int msgqid = msgget(MSG_KEY, IPC_CREAT | 0666);

    int x;

    std::cout << "Enter the value of x: ";
    std::cin >> x;


    if((f_pid = fork()) < 0){
        perror("Cannot create fork");
        exit(EXIT_FAILURE);
    }
    if(f_pid == 0) {
        process(f, msgqid);
        exit(EXIT_SUCCESS);
    }

    if((g_pid = fork()) < 0){
        perror("Cannot create fork");
        exit(EXIT_FAILURE);
    }
    if(g_pid == 0) {
        process(g, msgqid);
        exit(EXIT_SUCCESS);
    }

    Message msg_init{};

    msg_init.type = MSG_TYPE;
    msg_init.value = x;

    //Send initializing messages to f process and g process
    msgsnd(msgqid, &msg_init, sizeof(msg_init.value), IPC_NOWAIT);
    msgsnd(msgqid, &msg_init, sizeof(msg_init.value), IPC_NOWAIT);

    int time = 1;

    int status1, status2 = 0;

    bool firstReceived = false;
    bool secondReceived = false;

    while(true){
        //Check if the processes have ended
        pid_t firstCheck = waitpid(f_pid, &status1, WNOHANG);
        pid_t secondCheck = waitpid(g_pid, &status2, WNOHANG);

        if(firstCheck > 0) firstReceived = true;
        if(secondCheck > 0) secondReceived = true;

        if(firstReceived && secondReceived) break;

        if(time % 10 == 0){
            std::cout << "The process is stalling. Do you wish to continue? (Y): ";
            char answer;
            std::cin >> answer;
            if(!(answer == 'Y' || answer == 'y')){
                // Kill processes
                kill(f_pid, SIGKILL);
                kill(g_pid, SIGKILL);
                return 0;
            }
        }
        sleep(1);
        time++;
    }

    Message msg_rcv1{}, msg_rcv2{};

    //Receive final messages
    msgrcv(msgqid, &msg_rcv1, sizeof(msg_rcv1.value), MSG_TYPE, 0);
    msgrcv(msgqid, &msg_rcv2, sizeof(msg_rcv2.value), MSG_TYPE, 0);

    //Compute f * g
    int result = msg_rcv1.value * msg_rcv2.value;

    printf("f(x)=%d and g(x)=%d\n", msg_rcv1.value, msg_rcv2.value);
    printf("Result: %d * %d = %d\n", msg_rcv1.value, msg_rcv2.value, result);

    //Clear Message Queue
    msgctl(msgqid, IPC_RMID, nullptr);

    return 0;
}