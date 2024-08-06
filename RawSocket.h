#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

// Polinômio gerador para CRC-8 (x^8 + x^2 + x^1 + 1)
#define CRC_POLY 0x07
#define VIDEOS_DIR "videos-redes/" // server
#define DOWNLOADS "downloads/"     // cliente

enum fields
{
    INIT_MARKER = 126,
    ACK = 0,
    NACK = 1,
    LISTA = 10,
    BAIXAR = 11,
    MOSTRA = 16,
    DESCRIPTOR = 17,
    DADOS = 18,
    FTX = 30,
    DATA_BYTES = 63,
    ERROR = 31,
    DENIED_ACCESS = 4,
    NOT_FOUND = 5,
    DISK_FULL = 6,
    MAX_DATA_BYTES = 67,

    // Extra
    GET = 7,
    SENDING = 8,
    OK = 9,
    END = 12,
    JOE = 13,
    POZ = 14,
    ENC = 15,
};

typedef struct __attribute__((packed)) msg_s
{
    unsigned int init_mark : 8;
    unsigned int size : 6;
    unsigned int seq : 5;
    unsigned int type : 5;
} msgHeader;

int ConexaoRawSocket(char *device);
void create_msgHeader(msgHeader *header, int seq, int size, int type);
void send_msg(int socket, unsigned char *data, int type, int *seq, size_t bytesRead);
int unpack_msg(unsigned char *buf, int socket, int *seq, int *last_seq, int type);
void inc_seq(int *counter);
uint8_t crc8(const unsigned char *data, size_t len);
int existe_arquivo(unsigned char *file_name, char *folder);