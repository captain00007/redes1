#include "RawSocket.h"
#include <sys/syscall.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

int counter_seq = 0; // contador para acompanhar o número de sequência das mensagens enviadas
int last_seq = 15;   // representar o último número de sequência válido ou o valor máximo permitido para o número de sequência em um protocolo de comunicação

void client_controller(int socket);
void videos_available(int socket);
int choose_response(unsigned char *buf, int socket);
int get_type(unsigned char *command);
void baixando_videos(int socket, unsigned char *input);
void criar_file(int socket, unsigned char *buf);
void writer(int socket, unsigned char *file, unsigned char *buf, int *counter_seq, int *last_seq);
void play_video(unsigned char *input);
void pause_video(unsigned char *input);
void resume_video(unsigned char *input);

int last_cmd;

struct video
{
    int status;
    char *name;
};

struct video vid;

void client_controller(int socket)
{
    unsigned char *input = calloc(MAX_DATA_BYTES, sizeof(unsigned char));
    vid.status = -1;
    videos_available(socket);

    while (1)
    {

        unsigned char input[MAX_DATA_BYTES];

        fgets(input, MAX_DATA_BYTES, stdin);

        int type = get_type(input);
        switch (type)
        {
        case LISTA:
            videos_available(socket);
            break;
        case BAIXAR:
            baixando_videos(socket, input);
            break;
        case JOE:
            play_video(input);
            break;
        case POZ:
            pause_video(input);
            break;
        case ENC:
            resume_video(input);
            break;
        default:
            break;
        }

        memset(input, 0, MAX_DATA_BYTES);
    }
}

void videos_available(int socket)
{

    send_msg(socket, 0, LISTA, &counter_seq, 0);
    unsigned char *buf = calloc(MAX_DATA_BYTES, sizeof(unsigned char));

    last_cmd = LISTA;

    while (1)
    {
        if (recvfrom(socket, buf, MAX_DATA_BYTES, 0, NULL, 0) < 0)
        {
            counter_seq -= 1;
            send_msg(socket, 0, last_cmd, &counter_seq, 0);
            continue;
        }

        if (buf[0] == INIT_MARKER)
        {
            if (unpack_msg(buf, socket, &counter_seq, &last_seq, 100))
            {
                if (choose_response(buf, socket) == 2)
                    break;
            }
        }
        memset(buf, 0, MAX_DATA_BYTES);
    }

    free(buf);
}

int choose_response(unsigned char *buf, int socket)
{
    msgHeader *header = (msgHeader *)(buf);
    unsigned char *data = calloc(DATA_BYTES, sizeof(unsigned char));
    switch (header->type)
    {
    case MOSTRA:
        strncpy((char *)data, buf + 3, DATA_BYTES);
        fprintf(stderr, "%s\n", data);
        send_msg(socket, 0, ACK, &counter_seq, 0);
        last_cmd = ACK;
        memset(data, 0, DATA_BYTES);
        return 1;
    case END:
        send_msg(socket, 0, OK, &counter_seq, 0);
        return 2;
    case DESCRIPTOR:
        return DESCRIPTOR;
    case SENDING:
        return SENDING;
    case FTX:
        send_msg(socket, 0, OK, &counter_seq, 0);
        return 2;
    case NOT_FOUND:
        fprintf(stderr, "Video não encontrada\n");
        return 2;
    default:
        fprintf(stderr, "Tipo desconhecido.\n");
        break;
    }
    return 0;
}

/// Retorna o comando que foi digitado
int get_type(unsigned char *command)
{
    if (!strncmp(command, "lista", 5))
    {
        return LISTA;
    }

    if (!strncmp(command, "baixar", 6))
    {
        return BAIXAR;
    }

    if (!strncmp(command, "joe", 3))
    {
        return JOE;
    }

    if (!strncmp(command, "poz", 3))
    {
        return POZ;
    }

    if (!strncmp(command, "enc", 3))
    {
        return ENC;
    }

    return 0;
}

void baixando_videos(int socket, unsigned char *input)
{
    unsigned char *file_name = calloc(DATA_BYTES, sizeof(unsigned char));
    strncpy((char *)file_name, input + 7, DATA_BYTES);
    size_t size = strlen(input + 7);
    file_name[size - 1] = '\0';

    printf("Baixando %s\n", file_name);
    send_msg(socket, file_name, BAIXAR, &counter_seq, strlen(file_name));
    last_cmd = BAIXAR;

    unsigned char *buf = calloc(MAX_DATA_BYTES, sizeof(unsigned char));

    while (1)
    {
        if (recvfrom(socket, buf, MAX_DATA_BYTES, 0, NULL, 0) < 0)
        {
            counter_seq -= 1;
            send_msg(socket, file_name, last_cmd, &counter_seq, strlen(file_name));
            continue;
        }

        if (buf[0] == INIT_MARKER)
        {
            if (unpack_msg(buf, socket, &counter_seq, &last_seq, 100))
            {
                int res = choose_response(buf, socket);

                if (res == 2)
                    break;

                if (res == DESCRIPTOR)
                    criar_file(socket, file_name);

                if (res == SENDING)
                    writer(socket, file_name, buf, &counter_seq, &last_seq);

                send_msg(socket, 0, ACK, &counter_seq, 0);
            }
        }
        memset(buf, 0, MAX_DATA_BYTES);
    }

    free(buf);
}

void criar_file(int socket, unsigned char *file)
{
    unsigned char arq[DATA_BYTES];

    strcpy(arq, DOWNLOADS);

    strcat(arq, file);
    FILE *writer = fopen(arq, "a+");

    if (writer == NULL)
    {
        perror("Erro ao criar arquivo de video\n");
        exit(1);
    }
    fclose(writer);
}

void writer(int socket, unsigned char *file, unsigned char *buf, int *counter_seq, int *last_seq)
{
    unsigned char *data = calloc(DATA_BYTES, sizeof(unsigned char));
    msgHeader *header = (msgHeader *)(buf);

    unsigned char *arq = calloc(DATA_BYTES, sizeof(unsigned char));
    strcpy(arq, DOWNLOADS);
    strncat(arq, file, DATA_BYTES);

    strncpy(data, buf + 3, DATA_BYTES);

    FILE *file_op = fopen(arq, "a+");
    if (file_op == NULL)
    {
        perror("Erro ao abrir arquivo de video para escrita\n");
        exit(1);
    }

    fwrite(data, sizeof(unsigned char), DATA_BYTES - 1, file_op);

    fclose(file_op);
    free(data);
}

void play_video(unsigned char *input)
{
    unsigned char *file_name = calloc(DATA_BYTES, sizeof(unsigned char));
    strncpy((char *)file_name, input + 4, DATA_BYTES);
    size_t size = strlen(input + 4);
    file_name[size - 1] = '\0';

    if (!existe_arquivo(file_name, DOWNLOADS))
    {
        printf("%s nao encontrada\n", file_name);
    }
    else
    {
        vid.name = malloc(DATA_BYTES);
        strcpy(vid.name, file_name);
        vid.status = JOE;
        printf("%s está sendo reproduzida\n", file_name);
    }
}

void pause_video(unsigned char *input)
{
    unsigned char *file_name = calloc(DATA_BYTES, sizeof(unsigned char));
    strncpy((char *)file_name, input + 4, DATA_BYTES);
    size_t size = strlen(input + 4);
    file_name[size - 1] = '\0';

    if (!existe_arquivo(file_name, DOWNLOADS))
    {
        printf("%s nao encontrada\n", file_name);
    }

    else if (vid.status == JOE && (strcmp(vid.name, file_name) == 0))
    {
        vid.status = POZ;
        printf("%s está suspensa\n", file_name);
    }
}

void resume_video(unsigned char *input)
{
    unsigned char *file_name = calloc(DATA_BYTES, sizeof(unsigned char));
    strncpy((char *)file_name, input + 4, DATA_BYTES);
    size_t size = strlen(input + 4);
    file_name[size - 1] = '\0';

    if (!existe_arquivo(file_name, DOWNLOADS))
    {
        printf("%s nao encontrada\n", file_name);
    }

    else if (vid.status == POZ && (strcmp(vid.name, file_name) == 0))
    {
        vid.status = JOE;
        printf("%s está sendo reproduzida\n", file_name);
    }
}

int main()
{

    int server = ConexaoRawSocket("enp0s25"); // Define a interface de rede do cliente
    // Define qual interface vai usar para enviar e receber mensagem do servidor

    if (server < 0)
    {
        perror("Error while creating socket, aborting.\n");
        exit(-1);
    }

    // Define o tempo para enviar e receber mensagem(timeout)
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof tv); // receber
    setsockopt(server, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof tv); // enviar

    system("clear");
    fprintf(stderr, "Cliente up\n");
    client_controller(server);

    return 1;
}