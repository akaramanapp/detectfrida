#include <jni.h>
#include <string>
#include <stdio.h>
#include <jni.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <android/log.h>
#include <unistd.h>
#include <errno.h>
#define APPNAME "FridaDetectionTest"
#define MAX_LINE 512
#include <jni.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <pthread.h>



#define DEBUG_TAG "RootInspector"

static int child_pid;


extern "C" JNIEXPORT jstring JNICALL
Java_com_example_nativecr_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_nativecr_MainActivity_detectFrida(
        JNIEnv* env,
        jobject /* this */) {
    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    inet_aton("127.0.0.1", &(sa.sin_addr));

    int sock;

    int fd;
    char map[MAX_LINE];
    char res[7];
    int num_found;
    int ret;
    int i;

    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME,  "START FRIDA DETECTION");

    for(i = 0 ; i <= 65535 ; i++) {
        sock = socket(AF_INET , SOCK_STREAM , 0);
        sa.sin_port = htons(i);

        if (connect(sock , (struct sockaddr*)&sa , sizeof sa) != -1) {
            memset(res, 0 , 7);
            send(sock, "\x00", 1, NULL);
            send(sock, "AUTH\r\n", 6, NULL);
            usleep(100); // Give it some time to answer

            if ((ret = recv(sock, res, 6, MSG_DONTWAIT)) != -1) {
                if (strcmp(res, "REJECT") == 0) {
                    __android_log_print(ANDROID_LOG_VERBOSE, APPNAME,  "FRIDA DETECTED [1] - frida server running on port %d!", i);
                }
            }
        }

        close(sock);
    }

    std::string username = "Abdulkerim";
    return env->NewStringUTF(username.c_str());
}

void *monitor_pid() {

    int status;

    waitpid(child_pid, &status, 0);

    /* Child status should never change. */

    _exit(0); // Commit seppuku

}

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_nativecr_MainActivity_antiDebug(
        JNIEnv* env,
        jobject /* this */) {

    child_pid = fork();

    if (child_pid == 0)
    {
        int ppid = getppid();
        int status;

        if (ptrace(PTRACE_ATTACH, ppid, NULL, NULL) == 0)
        {
            waitpid(ppid, &status, 0);

            ptrace(PTRACE_CONT, ppid, NULL, NULL);

            while (waitpid(ppid, &status, 0)) {

                if (WIFSTOPPED(status)) {
                    ptrace(PTRACE_CONT, ppid, NULL, NULL);
                } else {
                    // Process has exited
                    _exit(0);
                }
            }
        }

    } else {
        pthread_t t;
        pthread_create(&t, NULL, reinterpret_cast<void *(*)(void *)>(monitor_pid), (void *)NULL);
    }

    return env->NewStringUTF("Anti Debugging");
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_nativecr_MainActivity_runpmlist(JNIEnv * env, jobject obj, jstring packageName, jboolean exactMatch) {
    jboolean isCopy;
    const char * package = env->GetStringUTFChars(packageName, &isCopy);
    char searchString[100];
    if (exactMatch) {
        sprintf(searchString, "%s%s%s", "\npackage:", package, "\n");
    } else {
        sprintf(searchString, "%s", package);
    }


    int link[2];
    pid_t pid;
    char buf[4096];
    ssize_t bytesRead;

    if (pipe(link)==-1) {
        __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: Error with pipe");
        exit(-1);
    }

    if ((pid = fork()) == -1) {
        __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: Error with fork");
        exit(-1);
    }

    if(pid == 0) {
        //__android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: Child reporting in!");
        dup2 (link[1], STDOUT_FILENO);
        close(link[0]);
        execlp("pm", "pm", "list", "packages", (char *)0);
        __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: After execl :( error: [%s]", strerror(errno));
        exit(-1);

    } else {
        close(link[1]);
        //__android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: Parent ready.");
        wait(NULL);
        //__android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: Parent done waiting.");
        while(1) {
            bytesRead = read(link[0], buf, sizeof(buf)-1);
            if (bytesRead <= 0) {
                __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: bytesread: [%zd]", bytesRead);
                break;
            }
            buf[bytesRead] = 0; // append null terminator
            //__android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: Output: [%s]", buf);
            if (strstr(buf, searchString) != NULL) {
                __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: Found [%s]!", package);
                close(link[0]);
                return 1;
            }
        }
        //read(link[0], buf, sizeof(buf));


        close(link[0]);
        //__android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "NATIVE: Parent done.");
    }
    return 0;
}