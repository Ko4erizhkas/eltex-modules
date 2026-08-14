#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define BLOCK   4096
#define MAXNAME 256

enum { MSG_READY, MSG_HEADER, MSG_QUIT };

struct message
{
    int   type;
    off_t size;
    char  name[MAXNAME];
};

static ssize_t readN(int fd, void *buf, size_t n)
{
    size_t done = 0;

    while (done < n)
    {
        ssize_t r = read(fd, (char *)buf + done, n - done);
        if (r == -1)
            return -1;
        if (r == 0)
            break;
        done += (size_t)r;
    }
    return (ssize_t)done;
}

static int writeN(int fd, const void *buf, size_t n)
{
    size_t done = 0;

    while (done < n)
    {
        ssize_t w = write(fd, (const char *)buf + done, n - done);
        if (w == -1)
            return -1;
        done += (size_t)w;
    }
    return 0;
}

static int waitReady(int rfd)
{
    struct message msg;

    if (readN(rfd, &msg, sizeof(msg)) != (ssize_t)sizeof(msg) || msg.type != MSG_READY)
    {
        fprintf(stderr, "Потомок не готов принимать данные\n");
        return -1;
    }
    return 0;
}

static int makeFifo(const char *path)
{
    struct stat st;

    if (mkfifo(path, 0666) == 0)
        return 0;

    if (errno != EEXIST)
    {
        fprintf(stderr, "mkfifo %s: %s\n", path, strerror(errno));
        return -1;
    }
    if (stat(path, &st) == -1 || !S_ISFIFO(st.st_mode))
    {
        fprintf(stderr, "%s существует и не является каналом\n", path);
        return -1;
    }
    return 0;
}

static int parent(int rfd, int wfd, int count, char *files[])
{
    struct message msg;
    char           block[BLOCK];

    for (int i = 0; i < count; i++)
    {
        struct stat st;
        off_t       left;
        int         fd = open(files[i], O_RDONLY);

        if (fd == -1 || fstat(fd, &st) == -1 || !S_ISREG(st.st_mode))
        {
            fprintf(stderr, "Не удалось открыть %s: %s\n", files[i],
                    fd == -1 ? strerror(errno) : "не обычный файл");
            if (fd != -1)
                close(fd);
            continue;
        }

        if (waitReady(rfd) == -1)
        {
            close(fd);
            return -1;
        }

        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_HEADER;
        msg.size = st.st_size;
        snprintf(msg.name, MAXNAME, "%s", files[i]);
        if (writeN(wfd, &msg, sizeof(msg)) == -1)
        {
            close(fd);
            return -1;
        }

        for (left = st.st_size; left > 0; left -= BLOCK)
        {
            size_t  want = (left < BLOCK) ? (size_t)left : BLOCK;
            ssize_t n = readN(fd, block, want);

            if (n != (ssize_t)want || writeN(wfd, block, want) == -1)
            {
                fprintf(stderr, "Ошибка передачи %s: %s\n", files[i], strerror(errno));
                close(fd);
                return -1;
            }
        }
        close(fd);
        printf("Передан %s (%lld байт)\n", files[i], (long long)st.st_size);
    }

    if (waitReady(rfd) == -1)
        return -1;

    memset(&msg, 0, sizeof(msg));
    msg.type = MSG_QUIT;
    return writeN(wfd, &msg, sizeof(msg));
}

static int child(int rfd, int wfd)
{
    struct message msg;
    char           block[BLOCK];
    char           name[MAXNAME + 8];

    for (;;)
    {
        off_t left;
        int   fd;

        memset(&msg, 0, sizeof(msg));
        msg.type = MSG_READY;
        if (writeN(wfd, &msg, sizeof(msg)) == -1)
            return -1;

        if (readN(rfd, &msg, sizeof(msg)) != (ssize_t)sizeof(msg))
            return -1;
        if (msg.type == MSG_QUIT)
            return 0;
        msg.name[MAXNAME - 1] = '\0';

        snprintf(name, sizeof(name), "%s.copy", msg.name);
        fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1)
        {
            fprintf(stderr, "Не удалось создать %s: %s\n", name, strerror(errno));
            return -1;
        }

        for (left = msg.size; left > 0; left -= BLOCK)
        {
            size_t  want = (left < BLOCK) ? (size_t)left : BLOCK;
            ssize_t n = readN(rfd, block, want);

            if (n != (ssize_t)want || writeN(fd, block, want) == -1)
            {
                fprintf(stderr, "Ошибка приёма %s: %s\n", name, strerror(errno));
                close(fd);
                return -1;
            }
        }
        close(fd);
        printf("Создан %s\n", name);
    }
}

int main(int argc, char *argv[])
{
    const char *fifo = NULL;
    char        back[MAXNAME + 8];
    int         toChild[2], toParent[2];
    int         rfd, wfd, opt, rc, status;
    pid_t       pid;

    while ((opt = getopt(argc, argv, "p:")) != -1)
    {
        if (opt != 'p')
        {
            fprintf(stderr, "Использование: %s [-p имя_канала] файл1 [файл2 ...]\n", argv[0]);
            return EXIT_FAILURE;
        }
        fifo = optarg;
    }
    if (optind == argc)
    {
        fprintf(stderr, "Не указано ни одного файла\n");
        return EXIT_FAILURE;
    }

    if (fifo != NULL)
    {
        snprintf(back, sizeof(back), "%s.back", fifo);
        if (makeFifo(fifo) == -1 || makeFifo(back) == -1)
            return EXIT_FAILURE;
    }
    else if (pipe(toChild) == -1 || pipe(toParent) == -1)
    {
        perror("pipe");
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);

    if ((pid = fork()) == -1)
    {
        perror("fork");
        return EXIT_FAILURE;
    }

    if (pid == 0)
    {
        if (fifo != NULL)
        {
            if ((rfd = open(fifo, O_RDONLY)) == -1 || (wfd = open(back, O_WRONLY)) == -1)
            {
                perror("open fifo");
                _exit(EXIT_FAILURE);
            }
        }
        else
        {
            close(toChild[1]);
            close(toParent[0]);
            rfd = toChild[0];
            wfd = toParent[1];
        }

        rc = child(rfd, wfd);
        close(rfd);
        close(wfd);
        fflush(stdout);
        _exit(rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    if (fifo != NULL)
    {
        if ((wfd = open(fifo, O_WRONLY)) == -1 || (rfd = open(back, O_RDONLY)) == -1)
        {
            perror("open fifo");
            return EXIT_FAILURE;
        }
    }
    else
    {
        close(toChild[0]);
        close(toParent[1]);
        rfd = toParent[0];
        wfd = toChild[1];
    }

    rc = parent(rfd, wfd, argc - optind, argv + optind);
    close(rfd);
    close(wfd);

    wait(&status);
    if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS)
    {
        fprintf(stderr, "Дочерний процесс завершился с ошибкой\n");
        rc = -1;
    }

    if (fifo != NULL)
    {
        unlink(fifo);
        unlink(back);
    }
    return rc == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
