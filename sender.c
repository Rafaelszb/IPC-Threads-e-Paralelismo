// gcc sender.c -o sender
// ./sender canal entrada.pgm

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

struct Header {
    int w, h, maxv;
    int mode;
    int t1, t2;
};

struct PGM {
    int w, h, maxv;
    unsigned char* data;
};

struct PGM lerPGM(const char* nome) {
    FILE* f = fopen(nome, "rb");
    if (!f) {
    perror("Erro ao abrir imagem");
    exit(1);
    }

    struct PGM img;
    char tipo[3];

    fscanf(f, "%s", tipo);
    fscanf(f, "%d %d", &img.w, &img.h);
    fscanf(f, "%d", &img.maxv);
    fgetc(f);

    img.data = malloc(img.w * img.h);
    fread(img.data, 1, img.w * img.h, f);

    fclose(f);
    return img;
}

int main(int argc, char** argv) {
    
    char * myfifo = "/tmp/myfifo";
    
    const char* fifo = argv[1];
    const char* inpath = argv[2];
    
    mkfifo(fifo, 0666);
    int fd = open(fifo, O_WRONLY);

    struct PGM img = lerPGM(inpath);

    struct Header header;
    header.w = img.w;
    header.h = img.h;
    header.maxv = img.maxv;
    header.mode = 0;
    header.t1 = 0;
    header.t2 = 0;

    //Envia header
    write(fd, &header, sizeof(header));

    //Envia pixels
    write(fd, img.data, img.w * img.h);

    close(fd);
    free(img.data);

    return 0;
}
