#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <termios.h>
#include <signal.h>

#define ENDSERV_LEN 100
#define NICK_LEN 20
#define BUFFTAMANHO 2000
#define RED 1
#define REDBOLD 2
#define GREEN 3
#define GREENBOLD 4
#define YELLOW 5
#define NOCOLOR 6
#define RED_COD "\x1b\x5b\x33\x31\x6d"
#define REDBOLD_COD "\x1b\x5b\x31\x3b\x33\x31\x6d"
#define GREEN_COD "\x1b\x5b\x33\x32\x6d"
#define GREENBOLD_COD "\x1b\x5b\x31\x3b\x33\x32\x6d"
#define YELLOW_COD "\x1b\x5b\x30\x3b\x33\x33\x6d"
#define NOCOLOR_COD "\x1b\x5b\x30\x6d"

int fd ; /* socket */

void muda_cor(char *msg, int cor){   
    switch(cor) {
        case RED:
            printf(RED_COD"%s"NOCOLOR_COD, msg);
            break;

        case REDBOLD:
            printf(REDBOLD_COD"%s"NOCOLOR_COD, msg);
            break;

        case GREEN:
            printf(GREEN_COD"%s"NOCOLOR_COD, msg);
            break;

        case GREENBOLD:
            printf(GREENBOLD_COD"%s"NOCOLOR_COD, msg);
            break;

        case YELLOW:
            printf(YELLOW_COD"%s"NOCOLOR_COD, msg);
            break;
    }
    
}

/*Função para o Sinal Ctrl+C que deverar desconectar todos os usuários e desconectar o Servidor.*/
void termina_cliente(int sig){
    close(fd);
    printf("\n------------------------CLIENTE BITS MESSENGER FINALIZADO-----------------------\n");
    exit(sig);  
}

int main(int argc, char *argv[]) {
    system("clear");

    char endServer[ENDSERV_LEN];
    char nome_cliente[NICK_LEN];
    char mensagem[BUFFTAMANHO];

    struct sockaddr_in addr ;
    struct hostent *hostPtr;

    struct timeval select_time ;
    fd_set select_set ;

    int t ;

    /* Usados para leitura de getchar(). */
    struct termios initial_settings, new_settings;

    if (argc != 4) {
        printf("cliente <nome de utilizador> <ip endereco> <porta> \n");
        return -1;
    }

    endServer[0]=0; nome_cliente[0]=0 ;

    /* Copia do endereço. strncat evita que os dados
     * ultrapassem o buffer. */
    strncat(endServer, argv[2], sizeof endServer);

    /* Copia do endereço. strncat evita que os dados
     * ultrapassem o buffer. */
    strncat(nome_cliente, argv[1], sizeof nome_cliente);

    if ((hostPtr = gethostbyname(endServer)) == 0) {
        herror("gethostbyname") ;
        return -1 ;
    }

    bzero((void *) &addr, sizeof(addr));
    addr.sin_family = AF_INET; /* Protocolo */
    addr.sin_addr.s_addr = ((struct in_addr *)(hostPtr->h_addr))->s_addr;
    addr.sin_port = htons((short) atoi(argv[3])); /* Porto */

    /* Abre um novo socket. */
    if((fd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("socket") ;
        return -1 ;
    }

    /* Conecta no servidor. */
    if((fd = socket(AF_INET,SOCK_STREAM,0)) == -1) {
        perror("socket") ;
        return -1 ;
    }
    if( connect(fd,(struct sockaddr *)&addr,sizeof (addr)) < 0) {
        perror("connect") ;
        return -1 ;
    }
    
    /* Obtem configuracoes antigas do terminal. */
    tcgetattr(0,&initial_settings);

    /* Seta novas configuracoes do terminal. */
    new_settings = initial_settings;
    new_settings.c_lflag &= ~ICANON;
    new_settings.c_lflag &= ~ECHO;
    new_settings.c_lflag &= ~ISIG;
    new_settings.c_cc[VMIN] = 0;
    new_settings.c_cc[VTIME] = 0;

    //Redefini o Sinal Ctrl-C
    signal(SIGINT, termina_cliente);

    /* Envia apelido. */
    send(fd, nome_cliente, strlen(nome_cliente),0);
    memset(mensagem, 0x0, BUFFTAMANHO);
    recv(fd, mensagem, sizeof mensagem,0);
    muda_cor(mensagem, GREENBOLD);
    printf("Conectado com sucesso\n");
    /* Loop principal. */
    while(1) {
        /* Seta o socket em select. */
        FD_ZERO(&select_set);
        FD_SET(fd, &select_set);
        FD_SET(fileno(stdin), &select_set);

        /* Sem tempo de block para select. */
        //select_time.tv_sec = 0 ;
        //select_time.tv_usec = 0 ;

        if((t=select(FD_SETSIZE, &select_set, NULL, NULL, NULL)) < 0){
            perror("select");
            return -1;
        }

        /* Aqui temos algo no socket. */
        if(t > 0){
            if(FD_ISSET(fd, &select_set)){
                int b ;
                memset(mensagem, 0x0, BUFFTAMANHO);
                b = recv(fd, mensagem, sizeof mensagem,0);
                if(b == 0){
                    termina_cliente(SIGINT);
                }
            
                /* Mostra mensagem na tela. */
                mensagem[b] = '\0';
                //puts(mensagem);
                muda_cor(mensagem, GREEN);
                fflush(stdout);
            }else if(FD_ISSET(fileno ( stdin ), &select_set)){
                /* Mostra o prompt de mensagem. */
                memset(mensagem, 0x0, BUFFTAMANHO);
                fgets(mensagem, sizeof mensagem, stdin) ;

                /* Quit encerra o loop principal. */
                if(strcmp(mensagem, "quit\n") == 0) {
                    break ;
                }

                /* Envia a mensagem. */
                if(mensagem[0] != '\n') { /* Nao envia um ENTER isolado. */
                    send(fd, mensagem, strlen(mensagem),0);
                }
            }
        }
    }
    termina_cliente(SIGINT);
    //close(fd);
    return 0 ;
}
