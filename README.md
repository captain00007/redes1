Georges Guy Gustinvil GRR20199989
Kokouvi Hola Kanyi Kodjovi GRR20170300

Universidade Federal do Paraná
Curitiba – PR – Brasil

1. Introdução

O trabalho consiste na implementação de um streaming de vídeo baseado no protocolo definido em aula. O sistema desenvolvido consiste em dois elementos, um cliente e um servidor. A ideia é permitir que o cliente consiga baixar e listar os vídeos .avi ou .mp4 que estão no servidor 3, e depois chamando um player para reproduzir esses vídeos ao seu lado.

2. Mensagem
O cabeçalho da mensagem funciona da seguinte forma:

INIT MARKER: 8 bits,
size: 6 bits,
seq: 5 bits,
type: 5 bits,
Dados	: 63 bytes,
CRC-8: 8 bits

3. Implementação

Foi implementado os seguintes comandos2:

lista : Para listar os vídeos com a extensão .avi e/ou .mp4 que estão disponíveis no servidor. A lista aparece no lado do cliente.
Baixar 4 nome_video : Para baixar um vídeo.

Esses comandos abaixo não fazem requisição no servidor mas servem para simular o player de vídeo no lado do cliente.

joe nome_video1 : Para chamar o player e reproduzir um vídeo.
poz nome_video1 : Para pausar o vídeo que está sendo reproduzido
enc nome_video1 : Para reproduzir o vídeo suspensa.


OBS 1: nome_video: Nome do vídeo que deseja.
OBS 2: Todos os comandos devem ser executados no cliente, pois o servidor não responde aos comandos via terminal, o servidor só responde as requisições vindo do cliente.
OBS 3: Os vídeos disponíveis no servidor ficam na pasta videos-redes do projeto.
OBS 4: Os vídeos baixados no cliente ficam disponíveis na pasta downloads do projeto.
