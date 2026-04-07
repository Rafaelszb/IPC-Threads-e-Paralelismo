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

    struct PGM img = lerPGM("entrada.pgm");

    struct Header header;
    header.w = img.w;
    header.h = img.h;
    header.maxv = img.maxv;

    //Envia header
    write(fd, &header, sizeof(header));

    //Envia pixels
    write(fd, img.data, img.w * img.h);

    close(fd);
    free(img.data);

    return 0;
}
