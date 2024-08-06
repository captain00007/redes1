#include "RawSocket.h"
#include <sys/syscall.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>  

int counter_seq = 0;
int last_seq = 15;
int last_cmd;

void server_controller(int client);
int choose_command(unsigned char *buf, int client);
void list_videos(unsigned char *buf, int client, int *counter_seq, int *last_seq);
void baixar_videos(int socket,unsigned char *buf);
void descritor(int socket,unsigned char *arq,unsigned char *file_name,unsigned char *buf, struct stat fileStat);
void reader(int socket, char *file, int *counter_seq, int *last_seq);

void server_controller(int client)
{

    unsigned char *buf = calloc(MAX_DATA_BYTES, sizeof(unsigned char));

    while (1)
    {
        if (recvfrom(client, buf, MAX_DATA_BYTES, 0, NULL, 0) < 0)
        {
            perror("Error while receiving data. Aborting\n");
            exit(-2);
        }

        if (buf[0] == INIT_MARKER)
        {

            if (unpack_msg(buf, client, &counter_seq, &last_seq, 100))
            {
                choose_command(buf, client);
            }
        }

        memset(buf, 0, MAX_DATA_BYTES);
    }

    free(buf);
}


int choose_command(unsigned char *buf, int client)
{
    msgHeader *header = (msgHeader *)(buf);

    switch (header->type)
    {
        case LISTA:
            fprintf(stderr,"Enviando lista de videos.\n");
            list_videos(buf, client, &counter_seq, &last_seq);
            break;
        case BAIXAR:
            fprintf(stderr,"Baixando video.\n");
            baixar_videos(client,buf);
            break;
        case OK:
            return OK;
            ;
        case ACK:
            return ACK;
        case NACK:
            return NACK;
        default:
            break;
    }

    return 0;
}


void list_videos(unsigned char *buf, int socket, int *counter_seq, int *last_seq)
{
    unsigned char *data = calloc(MAX_DATA_BYTES, sizeof(unsigned char));
    msgHeader *header = (msgHeader *)(buf);

    struct dirent *entry;
    DIR *dp = opendir(VIDEOS_DIR);

    if (dp == NULL)
    {
        perror("Erro ao abrir diretorios de videos\n");
        exit(1);
    }

    while (entry = readdir(dp))
    {
        memset(data, 0, DATA_BYTES);
        if ((strstr(entry->d_name, ".mp4")) || (strstr(entry->d_name, ".avi")))
        {
            strncat((char *)data, entry->d_name, DATA_BYTES - 1);
            send_msg(socket, data, MOSTRA, counter_seq, strlen(data));
            int last_cmd = MOSTRA;

            while (1)
            {
                if (recvfrom(socket, buf, MAX_DATA_BYTES, 0, NULL, 0) < 0)
                {
                    *counter_seq-=1;
                    send_msg(socket, data, last_cmd, counter_seq, strlen(data));
                    continue;
                }

                if (buf[0] == INIT_MARKER)
                {

                    if (unpack_msg(buf, socket, counter_seq, last_seq, 100))
                    {
                        int received = header->type;
                        if (received == ACK)

                            break;

                        else if (received == NACK)
                        {

                            if ((*counter_seq) == 0)
                            {
                                (*counter_seq) = 15;
                            }
                            else
                            {
                                (*counter_seq) -= 1;
                            }

                            if ((*last_seq) == 0)
                            {
                                (*last_seq) = 15;
                            }
                            else
                            {
                                (*last_seq) -= 1;
                            }

                            send_msg(socket, data, last_cmd, counter_seq, strlen(data));
                        }
                    }
                }
            }
        }
    }
    
    send_msg(socket, 0, END, counter_seq,0);
    last_cmd = END;

    while (1)
    {
        if (recvfrom(socket, buf, MAX_DATA_BYTES, 0, NULL, 0) < 0)
        {
            *counter_seq-=1;
            send_msg(socket, 0, last_cmd, counter_seq, 0);
            continue;
        }

        if (buf[0] == INIT_MARKER)
        {

            if (unpack_msg(buf, socket, counter_seq, last_seq, 100))
            {
                if (choose_command(buf, socket) == OK )
                    break;
            }
        }

        memset(buf, 0, MAX_DATA_BYTES);
    }

    closedir(dp);
}

void baixar_videos(int socket,unsigned char *buf)
{
    unsigned char *file_name = calloc(DATA_BYTES, sizeof(unsigned char));
    struct dirent *entry;
    struct stat fileStat;
    unsigned char arq[DATA_BYTES];

    strncpy(file_name, (char *)buf + 3, DATA_BYTES);

    if(!existe_arquivo(file_name, VIDEOS_DIR))
        send_msg(socket, 0, NOT_FOUND, &counter_seq,0);
    else
    {
        descritor(socket,arq,file_name,buf,fileStat);
        reader(socket, arq, &counter_seq, &last_seq);
    }
}

void descritor(int socket,unsigned char *arq,unsigned char *file_name,unsigned char *buf, struct stat fileStat)
{

        strcpy(arq,VIDEOS_DIR);
        strcat(arq,file_name);
        if (stat(arq, &fileStat) == -1)
        {
            perror("Erro ao obter as informações do arquivo com stat");
            exit(1);
        }
        char fileSizeStr[DATA_BYTES];
        
        snprintf(fileSizeStr, sizeof(fileSizeStr), "%lld", (long long)fileStat.st_size);
        send_msg(socket,fileSizeStr, DESCRIPTOR, &counter_seq, strlen(fileSizeStr));
        last_cmd = DESCRIPTOR;
        
        while(1)
        {
            if (recvfrom(socket, buf, MAX_DATA_BYTES, 0, NULL, 0) < 0)
            {
                send_msg(socket,fileSizeStr, DESCRIPTOR, &counter_seq, strlen(fileSizeStr));
                continue;
            }

            if (buf[0] == INIT_MARKER)
            {

                if (unpack_msg(buf, socket, &counter_seq, &last_seq, 100))
                {
                    if(choose_command(buf, socket) == ACK);
                        break;
                }
            }

            memset(buf, 0, MAX_DATA_BYTES);
        }
}

void reader(int socket, char *file, int *counter_seq, int *last_seq)
{
    unsigned char *buf = calloc(MAX_DATA_BYTES, sizeof(unsigned char));
    unsigned char *data = calloc(DATA_BYTES, sizeof(unsigned char));
    msgHeader *header = (msgHeader *)(buf);

    FILE *file_op = fopen(file, "rb");
    
    if (file_op == NULL)
    {
        perror("Erro ao abrir diretorios de videos\n");
        exit(1);
    }

    size_t bytesRead;
    while ((bytesRead = fread(data, sizeof(unsigned char), DATA_BYTES, file_op)) > 0)
    {
        size_t size = strlen(data);
        send_msg(socket, data, SENDING, counter_seq,bytesRead);
        last_cmd = SENDING;
        memset(data, 0, DATA_BYTES);

        while (1)
        {
            if (recvfrom(socket, buf, MAX_DATA_BYTES, 0, NULL, 0) < 0)
            {
                counter_seq -=1;
                send_msg(socket, data, last_cmd, counter_seq,bytesRead);
                continue;
            }

            if (buf[0] == INIT_MARKER)
            {

                if (unpack_msg(buf, socket, counter_seq, last_seq, 100))
                {
                    int received = header->type;
                    if (received == ACK){
                        break;
                    }

                    else if (received == NACK)
                    {
                        if ((*counter_seq) == 0)
                        {
                            (*counter_seq) = 15;
                        }
                        else
                        {
                            (*counter_seq) -= 1;
                        }

                        if ((*last_seq) == 0)
                        {
                            (*last_seq) = 15;
                        }
                        else
                        {
                            (*last_seq) -= 1;
                        }

                        send_msg(socket, data, last_cmd, counter_seq,bytesRead);
                    }
                }
            }
        }
    }

    send_msg(socket, 0, FTX, counter_seq,0);
    last_cmd = FTX;

    while (1)
    {
        if (recvfrom(socket, buf, MAX_DATA_BYTES, 0, NULL, 0) < 0)
        {
            counter_seq-=1;
            send_msg(socket, 0, last_cmd, counter_seq,0);
            continue;
        }

        if (buf[0] == INIT_MARKER)
        {

            if (unpack_msg(buf, socket, counter_seq, last_seq, 100))
            {
                if (choose_command(buf, socket) == OK )
                    break;
            }
        }

        memset(buf, 0, MAX_DATA_BYTES);
    }

    fclose(file_op);
    free(data);
    free(buf);
}

int main()
{
    int client = ConexaoRawSocket("enp2s0");

    system("clear");
    fprintf(stderr, "Server up.\n");
    server_controller(client);

    return 1;
}