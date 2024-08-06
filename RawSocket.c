#include "RawSocket.h"
uint8_t crc8(const unsigned char *data, size_t len);

int ConexaoRawSocket(char *device)
{

    int soquete;
    struct ifreq ir;
    struct sockaddr_ll endereco;
    struct packet_mreq mr;

    soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); /*cria socket*/
    if (soquete == -1)
    {
        printf("Erro no Socket\n");
        exit(-1);
    }

    memset(&ir, 0, sizeof(struct ifreq)); /*dispositivo eth0*/
    memcpy(ir.ifr_name, device, sizeof(device));
    if (ioctl(soquete, SIOCGIFINDEX, &ir) == -1)
    {
        printf("Erro no ioctl\n");
        exit(-1);
    }

    memset(&endereco, 0, sizeof(endereco)); /*IP do dispositivo*/
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ir.ifr_ifindex;
    if (bind(soquete, (struct sockaddr *)&endereco, sizeof(endereco)) == -1)
    {
        printf("Erro no bind\n");
        exit(-1);
    }

    memset(&mr, 0, sizeof(mr)); /*Modo Promiscuo*/
    mr.mr_ifindex = ir.ifr_ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1)
    {
        printf("Erro ao fazer setsockopt\n");
        exit(-1);
    }

    return soquete;
}

void create_msgHeader(msgHeader *header, int seq, int size, int type)
{

    header->init_mark = INIT_MARKER;
    header->seq = seq;
    header->size = size;
    header->type = type;
}

void send_msg(int socket, unsigned char *data, int type, int *seq, size_t bytesRead)
{
    unsigned char *buf = calloc(MAX_DATA_BYTES, sizeof(unsigned char));
    msgHeader *header = (msgHeader *)(buf);

    if (data)
    {
        create_msgHeader(header, *seq, bytesRead, type);
    }
    else
    {
        create_msgHeader(header, *seq, 0, type);
    }

    int msg_size = sizeof(msgHeader) + header->size;

    for (int i = sizeof(msgHeader); i < msg_size; i++)
    {
        buf[i] = data[i - sizeof(msgHeader)];
    }

    if (data)
    {

        unsigned char te[] = {0x01, 0x02, 0x03, 0x04, 0x05};
        size_t len = sizeof(data) / sizeof(data[0]);

        uint8_t checksum = crc8(data, len);
        buf[MAX_DATA_BYTES - 1] = checksum;
    }

    sendto(socket, buf, MAX_DATA_BYTES, 0, NULL, 0);

    inc_seq(seq);
    free(buf);
}

void inc_seq(int *counter)
{
    *counter = ((*counter + 1) % 16);
}

int unpack_msg(unsigned char *buf, int socket, int *seq, int *last_seq, int type)
{
   
    msgHeader *header = (msgHeader *)(buf);
    unsigned char *data = (sizeof(msgHeader) + buf);

    if (*last_seq == header->seq || type == header->type)
    {
        return 0;
    }

    
    if (data)
    {
        size_t len = sizeof(data) / sizeof(data[0]);

        uint8_t checksum = crc8(data, len);

        if (checksum != buf[MAX_DATA_BYTES - 1])
        {
            send_msg(socket, NULL, NACK, seq,0);
            return 0;
        }
    }

    int shouldBe_seq = *last_seq;

    inc_seq(&shouldBe_seq);

    if (header->seq != shouldBe_seq)
    {
        send_msg(socket, NULL, NACK, seq,0);
        return 0;
    }

    *last_seq = header->seq;
    return 1;
}

// Função para calcular o CRC-8 de um vetor de bytes
uint8_t crc8(const unsigned char *data, size_t len)
{
    uint8_t crc = 0;

    for (size_t i = 0; i < len; ++i)
    {
        crc ^= data[i];

        for (int j = 0; j < 8; ++j)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ CRC_POLY;
            }
            else
            {
                crc <<= 1;
            }
        }
    }

    return crc;
}

int existe_arquivo(unsigned char *file_name, char *folder)
{
    unsigned char *data = calloc(MAX_DATA_BYTES, sizeof(unsigned char));
    
    struct dirent *entry;
    DIR *dp = opendir(folder);

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
            if (!strcmp(entry->d_name,file_name))
                return 1;          
        }
    }

    return 0;
}
