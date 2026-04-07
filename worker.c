// gcc worker.c -o worker -lpthread

// ./worker canal saida.pgm negativo
// ./worker canal saida.pgm slice 100 200

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>

#define NEGATIVO 0
#define SLICE 1
#define QMAX 128

// 🔹 Estruturas
struct PGM {
    int w, h, maxv;
    unsigned char* data;
};

struct Header {
    int w, h, maxv;
    int mode;
    int t1, t2;
};

struct Task {
    int row_start;
    int row_end;
};

// 🔹 Globais
struct PGM img;
struct Header header;

struct Task queue_buf[QMAX];
int q_head = 0, q_tail = 0;

pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;
sem_t sem_items;
sem_t sem_space;

int done = 0;

// 🔹 Adiciona tarefa na fila
void enqueue(struct Task t) {
    sem_wait(&sem_space);

    pthread_mutex_lock(&q_lock);
    queue_buf[q_tail] = t;
    q_tail = (q_tail + 1) % QMAX;
    pthread_mutex_unlock(&q_lock);

    sem_post(&sem_items);
}

// 🔹 Remove tarefa da fila
struct Task dequeue() {
    struct Task t;

    sem_wait(&sem_items);

    pthread_mutex_lock(&q_lock);
    t = queue_buf[q_head];
    q_head = (q_head + 1) % QMAX;
    pthread_mutex_unlock(&q_lock);

    sem_post(&sem_space);

    return t;
}

// 🔹 Worker thread
void* worker_thread(void* arg) {
    while (1) {

        struct Task t = dequeue();

        // condição de parada
        if (t.row_start == -1)
            break;

        for (int i = t.row_start; i < t.row_end; i++) {
            for (int j = 0; j < img.w; j++) {

                int idx = i * img.w + j;

                if (header.mode == NEGATIVO) {
                    img.data[idx] = header.maxv - img.data[idx];
                }
                else {
                    if (img.data[idx] >= header.t1 && img.data[idx] <= header.t2)
                        img.data[idx] = header.maxv;
                    else
                        img.data[idx] = 0;
                }
            }
        }
    }
    return NULL;
}

// 🔹 Salvar imagem
void salvarPGM(const char* nome) {
    FILE* f = fopen(nome, "wb");

    fprintf(f, "P5\n%d %d\n%d\n", img.w, img.h, img.maxv);
    fwrite(img.data, 1, img.w * img.h, f);

    fclose(f);
}

int main(int argc, char** argv) {

    if (argc < 4) {
        printf("Uso: %s <fifo> <saida.pgm> <modo> [parametros]\n", argv[0]);
        exit(1);
    }

    const char* fifo = argv[1];
    const char* outpath = argv[2];

    // 🔹 abrir FIFO
    int fd = open(fifo, O_RDONLY);
    if (fd < 0) {
        perror("FIFO");
        exit(1);
    }

    // 🔹 receber header
    read(fd, &header, sizeof(header));

    // 🔹 configurar modo
    if (strcmp(argv[3], "negativo") == 0) {
        header.mode = NEGATIVO;
    }
    else {
        header.mode = SLICE;
        header.t1 = atoi(argv[4]);
        header.t2 = atoi(argv[5]);
    }

    // 🔹 receber imagem
    img.w = header.w;
    img.h = header.h;
    img.maxv = header.maxv;

    int size = img.w * img.h;
    img.data = malloc(size);

    read(fd, img.data, size);
    close(fd);

    // 🔹 inicializar semáforos
    sem_init(&sem_items, 0, 0);
    sem_init(&sem_space, 0, QMAX);

    int n_threads = 4;
    pthread_t threads[n_threads];

    // 🔹 criar threads
    for (int i = 0; i < n_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, NULL);
    }

    // 🔹 dividir tarefas
    int bloco = 50;

    for (int i = 0; i < img.h; i += bloco) {
        struct Task t;
        t.row_start = i;
        t.row_end = (i + bloco > img.h) ? img.h : i + bloco;
        enqueue(t);
    }

    // 🔹 enviar tarefas de parada
    for (int i = 0; i < n_threads; i++) {
        struct Task t;
        t.row_start = -1;
        t.row_end = -1;
        enqueue(t);
    }

    // 🔹 esperar threads
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    salvarPGM(outpath);

    free(img.data);

    printf("Processamento concluído com thread pool!\n");

    return 0;
}