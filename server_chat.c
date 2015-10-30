//Includes básicos para programas
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
//Includes Para funcionamento básico de Sockets
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
//Includes adicionais para sockets
#include <sys/signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
//#include <mem.h>
//Includes para o uso do Select();
#include <sys/select.h>
#include <sys/time.h>

#define SERVER_OCUPADO "Servidor ocupado\n"
#define max_users 50
#define max_text 2024
#define command 3
#define nome_user 30
///////////////////////////////Variáveis globais, para capturar horas//////////////////////////////////
struct tm *DataAtual; //estrutura para armazenar data e hora.
time_t Segundos; //Valor dos segundos a serem gardados para a conversão para impressão do <time>
/////////////////////////////////////////Variáveis globais/////////////////////////////////////////////
int i = command; //Número de comandos que existirão na lista.
int  users = max_users; //número máximo de usuários, essa variável pode ser alterada.
int socket_listen; // Listen para espera na escuta.
int on; //Garda quantos clientes ainda estão conectados.
char buffer[max_text]; //Buffer para a troca de Mensagens.
char *nome, *msg;
////////////////////////////////Strutura para controle dos clientes////////////////////////////////////
/*Estrutura para cadastro dos usuário conectados. Usado no controle deles*/
typedef struct No{
	char nome[nome_user];
	bool conexao;
	int porta;
	int socket;
	struct No *dir;
}No, *Cliente;
/*Estrutura para impressão dos comandos muito util, na hora do camando help*/
typedef struct comandos{
	char comand[7];
	char descricao[max_text];
	struct comandos *dir;
}comandos, *Comandos;
/////////////////////////////////////////////Funções///////////////////////////////////////////////////
/*Inicializando lista de clientes*/
void cria_lista_clientes(Cliente *clientes){
	(*clientes) = NULL;
}

void cria_lista_comandos(Comandos *comand){
	(*comand) = NULL;
}

bool inicializa_comandos(Comandos *c){
	char *comando, *descricao;

	/*Preenchendo a lista com os comandos e suas descrições.*/
	while(i >= 0){
		if (i == 3){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			comando = "SEND";
			descricao = "Envia <CLIENTS_NAME>: <MENSSAGEM> para todos os clientes conectados (menos ocliente emissor).";
			strcpy((*c->comand), comando);
			strcpy((*c->descricao), descricao);
			(*c->dir) = NULL;
			i--;
			inicializa_comandos(&(*c)->dir);
		}
		if (i == 2){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			comando = "SENDTO";
			descricao = "Idêntico com SEND, ou seja, Envia <CLIENTS_NAME>: <MENSSAGEM>, porém envia a mensagem apenas para o cliente especificado pelo <CLIENT_NAME>.";
			strcpy((*c->comand), comando);
			strcpy((*c->descricao), descricao);
			(*c->dir) = NULL;
			i--;
			inicializa_comandos(&(*c)->dir);
		}
		if (i == 1){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			comando = "WHO";
			descricao = "Retorna a lista dos clientes conectados ao servidor.";
			strcpy((*c->comand), comando);
			strcpy((*c->descricao), descricao);
			(*c->dir) = NULL;
			i--;
			inicializa_comandos(&(*c)->dir);
		}
		if (i == 0){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			comando = "HELP";
			descricao = "Retorna a lista de comandos suportados e seu uso.";
			strcpy((*c->comand), comando);
			strcpy((*c->descricao), descricao);
			(*c->dir) = NULL;
			i--;
		}
	}
	return true;
}
/*compara as Strings retornando 0 s forem iguais.*/
int comparar(char *s1, char *s2){
    return (strcmp(s1,s2));
}
/*Lista de Clientes Conectados, quando o cliente sai ele tem o campo conexao setado como
false, se conectado com true*/
int lista_clientes(char nome, int socket, int porta, Cliente *clientes){
	 if (!(*clientes)){
        if (!((*clientes)=(Cliente)malloc(sizeof(No))))
            return 0;
		strcpy((*clientes)->nome, nome);
		(*clientes)->conexao = true;
		(*clientes)->porta = porta;
		(*clientes)->socket = socket;
        (*clientes)->dir = NULL;
    }else{
		/*Aqui temos um retorno de controle caso o usuário já exista e esteja conectado.*/
        if (!(comparar((*clientes)->nome, nome))){
			if((*clientes)->conexao)
            	return 2;
        }else if (!(comparar((*clientes)->nome, nome))){
			if(!((*clientes)->conexao)){
				strcpy((*clientes)->nome, nome);
				(*clientes)->conexao = true;
				(*clientes)->porta = porta;
				(*clientes)->socket = socket;
				return 1;
			}
        }else{
			lista_clientes(nome, &(*clientes)->dir);
		}
    }
    return 1;
}
/*Procura o cliente e devolve a estrutura que detem seus dados*/
Cliente retorna_cliente(Cliente c, char *nome, int socket){
	Cliente aux;

	if(nome){
		for(aux = c; (aux) && (comparar(aux->nome, nome)!=0); aux = aux->dir);
	}else if (socket){
		for(aux = c; (aux) && (aux->socket == socket); aux = aux->dir);
	}
	return (aux);
}
/*Função que Obtem a hora da biblioteca time*/
int Obtem_hora(void){
    time(&Segundos); //obtém a hora em segundos.
    DataAtual = localtime(&Segundos); //converte horas em segundos.
    return(DataAtual->tm_hour); //retorna as horas de 0 a 24.
}
/*Função que Obtem os minutos da biblioteca time*/
int Obtem_minuto(void){
    time(&Segundos); //obtém a hora em segundos.
    DataAtual = localtime(&Segundos); //converte horas em segundos.
    return(DataAtual->tm_min); //retorna os minutos de 0 a 59.
}
/*Função para Impressão da Hora no formato fornecido!*/
char imprime_hora(void){
	int hora, minuto;
	char retorno[7];

	hora = Obtem_hora();
	minuto = Obtem_minuto();

	//printf("<%d:%d\>t<Cliente>\t", hora, minuto);
	if (hora <10)
		snprintf(retorno, 7, "0%d:", hora);
	else
		snprintf(retorno, 7, "%d:", hora);
	if (minuto < 10)
		snprintf(retorno, 7, "0%d\t", minuto);
	else
		snprintf(retorno, 7, "%d\t", minuto);
	return retorno;
}
/////////////////////////////////////////Funções para sockets//////////////////////////////////////////
bool comando_send(Cliente clientes, Cliente emisor, char *msg){
	char send_msg[max_text]; //Conteudo total a ser enviado!
	Cliente aux; //Auxiliar para envio à todos os clientes!
	char hora;
	
	hora = imprime_hora();//capturando hora;
	/*Criando a msg com o nome e a msg a ser enviada!*/
	snprintf(send_msg, max_text, "%s: %s", emisor->nome, msg);
	/*Loop que envia a msg para todos os clientes.*/
	for(aux = clientes; (aux); aux = aux->dir){
		System("clear");
		send(aux->socket, send_msg, strlen(send_msg), 0);
		printf("%s\t%s\tSEND\tExecutado:Sim", hora, emisor->nome);
		return true;
	}
	printf("%s\t%s\tSENDTO\tExecutado:Nao", hora, emisor->nome);
	return false;
}

bool comando_send_to(Cliente receptor, Cliente emisor, char *msg){
	char send_to_msg [max_text];
	char hora;
	
	/*Criando a msg com o nome e a msg a ser enviada!*/
	snprintf(send_to_msg, max_text, "%s: %s", emisor->nome, msg);
	hora = imprime_hora();//capturando hora;
	/*Enviando a msg para o usuário especificado!*/
	if ((receptor)&&(receptor->conexao)){
		System("clear");
		send(receptor->socket, send_to_msg, strlen(send_to_msg), 0);
		printf("%s\t%s\tSENDTO\tExecutado:Sim", hora, emisor->nome);
		return true;
	}
	printf("%s\t%s\tSENDTO\tExecutado:Nao", hora, emisor->nome);
	return false;
}

bool comando_who(int socket, Cliente emisor, Cliente clientes){
	Cliente aux;
	char msg[max_text];

	snprintf(msg, sizeof(msg), "\n---------------------------Bits Mensseger WHO-------------------------------\n");
	snprintf(msg, sizeof(msg), "Usuarios conectados: ");
	for (aux = clientes; (aux); aux = aux->dir){
		if(comparar(aux->nome, emisor->nome) != 0)
			snprintf(msg, sizeof(msg), "%s, ", aux->nome);
	}
	snprintf(msg, sizeof(msg), "\n----------------------------------------------------------------------------\n");
	recv(emisor->socket, msg, sizeof(msg), 0);
	return true;
}

bool comando_help(Cliente emisor, Comandos comand){
	Comandos aux;
	char msg[max_text];

	snprintf(msg, sizeof(msg), "---------------------------Bits Mensseger HELP-------------------------------\n");
	for (aux = comand; (aux); aux = aux->dir){
		snprintf(msg, sizeof(msg), "| %s -- %s |\n", aux->comand, aux->descricao);
	}
	snprintf(msg, sizeof(msg), "-----------------------------------------------------------------------------\n");
	recv(emisor->socket, msg, sizeof(msg), 0);
	return true;
}
/*O buffer vem no formato [Comando:Receptor:Mensagem], ou [Comando], ou [Comando:Mensagem], e é dividodo a depender do comando passado!*/
char retorna_nome_msg(char *buffer, char *nome, char *msg){
	char *comando;
	int i;
	
	for(i = 0; i < 7; i++){
		if (buffer[i] != ':'){
			comando[i] = buffer[i];
		}else if(buffer[i] == ':'){
			i++;
			break;
		}
	}
	if (!(comparar("SENDTO", comando))){
		for(; i < (strlen(buffer) - strlen(comando)); i++){
			if (buffer[i] != ':'){
				nome[i] = buffer[i];
			}else if(buffer[i] == ':'){
				i++;
				for(; i < (strlen(buffer) - (strlen(comando)+(strlen(nome)))); i++){
					if (buffer[i] != '\0'){
						msg[i] = buffer[i];
					}else{
						msg[i] = '\0';
						return comando;
					}
				}
			}
		}
	}else if (!(comparar("SEND", comando))){
		for(; i < (strlen(buffer) - strlen(comando)); i++){
			if (buffer[i] != '\0'){
				msg[i] = buffer[i];
			}else{
				msg[i] = '\0';
				return comando;
			}
		}
	}
	return comando;
}
/*função para Saldação do servidor!*/
void bem_vindo(char *msg, int socket){
	char *bem_vindo = "-----------------------------Bem Vindo ao Bits Mensseger---------------------------------";
	send(socket, bem_vindo, strlen(bem_vindo), 0);
	printf("%s", bem_vindo);
}
/*Função que fará a leitura da mensagem, e processará os próximos passos!*/
char recebe_mensagem(int socket){
	int t, comando;

	memset(buffer, 0x0, max_text);
	memset(nome, 0x0, max_text);
	memset(msg, 0x0, max_text);
	t = recv(socket, buffer, max_text, 0);

	//if (t == 0){
	//	remove_socket_lista(socket) ;
	 //   return 1;
	//}
	comando = retorna_msg_nome(&buffer, &nome, &msg);
	return comando;
}
////////////////////////////////////////////////Principal//////////////////////////////////////////////
int main(int argc, char **argv){
	Cliente clientes, c;
	Cliente emisor;
	Cliente receptor;
	Comandos comandos;
	char *comando;
	char *bem_vindo = "-----------------------------Bem Vindo ao Bits Mensseger---------------------------------";
	int t_socket, t;
	int sizeof_listen;
	int server_socket; /*Server descritor Socket*/

	/*Variáveis para o socket*/
	int porta, j;
	struct sockaddr_in server;
	struct timeval select_time;

	/*Inicializando e criando listas a serem usadas.*/
    if (!(cria_lista_clientes(&clientes))){
		perror("Lista clientes erro!");
	}
	if(!(cria_lista_comandos(&comandos))){
		perror("Lista Comandos erro!");
	}
	if(!(inicializa_comandos(&comandos))){
		perror("Lista Comandos erro!");
	}

    if ( argc == 1 ) {
        bem_vindo(argv[0]);
        return -1 ;
    }
	//Capturando a porta passada na inicialização, e número máximo de usuáarios
    if ( argc > 2 ) {
        users = atoi(argv[2]) ;
    }

    porta = atoi(argv[1]) ;
	//definindo socket de escuta
    socket_listen = socket(AF_INET, SOCK_STREAM, 0) ;

    if (socket_listen < 0){
        perror("socket");
        return -1;
    }
	//Preenchendo a estrutura do socket
    server.sin_family = AF_INET;
    server.sin_port = htons(porta);
    server.sin_addr.s_addr = INADDR_ANY;

    t = sizeof(struct sockaddr_in);
    if (bind(socket_listen, (struct sockaddr *) &server, t) < 0){
        perror("bind");
        return -1;
    }

    if (listen(socket_listen, 5) < 0){
        perror("listen");
        return -1;
    }

  	//usaremos Cntrl+C para sair, mas o certo será "quit"
    while (1){
        //pegar todos os usuarios e colocar na estrutura
        FD_ZERO(&select_set);
        FD_SET(socket_listen, &select_set);
		//vai setar o descritor para cada nova conexão.
        for (t = 0, c = clientes; (c) && t < max_users; t++, c = c->dir){
            if (c->socket != -1){
                FD_SET(c->socket, &select_set);
            }
        }
		//aqui vai printar na tela do servidor dizendo que está aguradando usuarios
        //printf("[+] Servidor aguardando usuarios %d [%d/%d] ...\n", port, list_tam, users) ;
		//atualiza a cada 5 segundos
        select_time.tv_sec = 5 ;
        select_time.tv_usec = 0 ;
		//Definindo o select
        if((t=select(FD_SETSIZE, &select_set, NULL, NULL, &select_time)) < 0){
            perror("select");
            return -1;
        }

        if(t > 0){
           //Aceitar a conexão de entrada e adicionar  novo socket na lista.
            if (FD_ISSET(socket_listen, &select_set)){
                int n;

                if ((n=accept(socket_listen, NULL, NULL)) < 0){
                    perror("accept");
                }else if(inserir_socket(n) == 1){ /* server is busy */
                    send(n,SERVER_OCUPADO,strlen(SERVER_OCUPADO),0);
                    close(n);
                }
                continue ;
            } else {
                int i ;

               //processa os dados recebidos
                for (i =0, c = clientes; (c) && (i < users); i++, c = c->dir) {
                    if ( FD_ISSET(c->socket, &select_set) ) {
                        if (comando = receber_msg(lista_de_clientes[i].sockfd) == 0 ) {
                            int flag_system_msg = 0 ;

                            if(lista_de_clientes[i].nick_ok == 0) { //para setar o apelido
                                  lista_de_clientes[i].nick_ok = 1 ;
                                  strncpy(lista_de_clientes[i].nick, msg, nome_user) ;


                                  if(lista_de_clientes[i].nick[strlen(lista_de_clientes[i].nick)-1]=='\n') {
                                        lista_de_clientes[i].nick[strlen(lista_de_clientes[i].nick)-1]=0 ;
                                  }

                                  snprintf(msg, max_text, "%s : entrou na conversa!\n", lista_de_clientes[i].nick) ;

                                  flag_system_msg = 1 ;
                            }
                           envia_mensagem_all(lista_de_clientes[i].sockfd, flag_system_msg) ;
                        }
                    }
                }
            }
        }
    }

    return 0;
}
