#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define ELEMENT_SIZE    1
#define ELEMENT_NUMB    30

int main(int argc, char *argv[])
{
    char buffer[ELEMENT_NUMB + 2];
    FILE * fp;
    size_t read;


    fp = fopen("/dev/MPU9250_0", "r");
    if (fp == 0)
    {
	    printf("fopen() File read error\n");
	    perror("Error");
	    return -1;
    }
    printf("open() success\n");


    fseek(fp, 0, SEEK_SET);
    printf("fseek() success\n");
    
    
    read = fread(buffer, ELEMENT_SIZE, ELEMENT_NUMB, fp);
    printf("read = %d\n", read);
    printf("%s\n", buffer);
    
    
    fclose(fp);
    printf("File closed\n");
    return 0;
}
