/*
 ============================================================================
 Name        : Bits.c
 Author      : Jônatas da Silva
 Version     :
 Copyright   : Só me pedir!
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
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
#include <unistd.h>
#include <errno.h>
#include <time.h>
//Includes para o uso do Select();
#include <sys/select.h>
#include <sys/time.h>

#define servidor_ocupado "Servidor ocupado, por favor tente mais tarde!\n"
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
char *nome = NULL, *msg =  NULL;
/* for select() */
fd_set select_set ;
////////////////////////////////Strutura para controle dos clientes////////////////////////////////////
/*Estrutura para cadastro dos usuário conectados. Usado no controle deles*/
typedef struct No{
	char nome[nome_user];
	bool conexao;
	//int porta;
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
bool cria_lista_clientes(Cliente *clientes){
	(*clientes) = NULL;
	return true;
}

bool cria_lista_comandos(Comandos *comand){
	(*comand) = NULL;
	return true;
}

bool inicializa_comandos(Comandos *c){
	/*Preenchendo a lista com os comandos e suas descrições.*/
	while(i >= 0){
		if (i == 3){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			strcpy((*c)->comand, "SEND");
			strcpy((*c)->descricao, "Envia <CLIENTS_NAME>: <MENSSAGEM> para todos os clientes conectados (menos o cliente emissor).");
			((*c)->dir) = NULL;
			i--;
			inicializa_comandos(&(*c)->dir);
		}
		if (i == 2){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			strcpy((*c)->comand, "SENDTO");
			strcpy((*c)->descricao, "Idêntico com SEND, ou seja, Envia <CLIENTS_NAME>: <MENSSAGEM>, porém envia a mensagem apenas para o cliente especificado pelo <CLIENT_NAME>.");
			(*c)->dir = NULL;
			i--;
			inicializa_comandos(&(*c)->dir);
		}
		if (i == 1){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			strcpy((*c)->comand, "WHO");
			strcpy((*c)->descricao, "Retorna a lista dos clientes conectados ao servidor.");
			(*c)->dir = NULL;
			i--;
			inicializa_comandos(&(*c)->dir);
		}
		if (i == 0){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			strcpy((*c)->comand, "HELP");
			strcpy((*c)->descricao, "Retorna a lista de comandos suportados e seu uso.");
			(*c)->dir = NULL;
			i--;
		}
	}
	return true;
}
/*compara as Strings retornando 0 s forem iguais.*/
int comparar(char *s1, char *s2){
    return (strcmp(s1,s2));
}
/*O buffer vem no formato [Comando:Receptor:Mensagem], ou [Comando], ou [Comando:Mensagem], e é dividodo a depender do comando passado!*/
void retorna_nome_msg(char *c){
	char *comando =NULL;
	int i;

	for(i = 0; i < 7; i++){
		if (buffer[i] != ':'){
			comando[i] = buffer[i];
		}else if(buffer[i] == ':'){
			i++;
			break;
		}
	}
	strcpy(c, comando);
	if(!(comparar("SENDTO", comando))){
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
						return;
					}
				}
			}
		}
	}else if(!(comparar("SEND", comando))){
		memset(nome, 0x0, max_text);
		for(; i < (strlen(buffer) - strlen(comando)); i++){
			if (buffer[i] != '\0'){
				msg[i] = buffer[i];
			}else{
				msg[i] = '\0';
				return;
			}
		}
	}else{
		memset(nome, 0x0, max_text);
		memset(msg, 0x0, max_text);
	}
}
/*Função que pega a primeira msg recebida e coleta o nome do usuário.*/
char recebe_nome(int socket){
	int t;
	//limpando o buffer para capturar a nova msg!
	memset(buffer, 0x0, max_text);
	//Recebendo a msg!
	t = recv(socket, buffer, max_text, 0);
	//Verificando se o socket é valido
	if (t == -1)
		return '-';
	strcpy(nome, buffer);
	return '+';
}
/*Lista de Clientes Conectados, quando o cliente sai ele tem o campo conexao setado como false, se
conectado com true*/
bool lista_clientes(int socket, Cliente *clientes){
	if (!(*clientes)){
        if (!((*clientes)=(Cliente)malloc(sizeof(No)))) //Alocação
            return false;
		strcpy((*clientes)->nome, nome);//Usando o retorno da função de cadastro para captura do nome.
		(*clientes)->conexao = true; // para determinar se o usuário estar conectado.
		(*clientes)->socket = socket; //Cada cliente tem um valor diferente de socket
        (*clientes)->dir = NULL;
    }else{
		/*Aqui temos um retorno de controle caso o usuário já exista e esteja conectado.*/
        if (!(comparar((*clientes)->nome, nome))){
			if((*clientes)->conexao)
            	return false;
        }else{
			//chama recursivamente a função até que ache o espaço vazio para inserir o novo usuário.
			lista_clientes(socket, &(*clientes)->dir);
		}
    }
    return true;
}
/*Função que passa para a lista de cleintes o nome do novo cliente!*/
bool cadastra_cliente(int socket, Cliente *c){
	char retorno;
	memset(nome, 0x0, max_text);
	retorno = recebe_nome(socket); //O nome foi encontrado na função anterior.
	if(retorno == '-')
		return false;
	if (!(lista_clientes(socket, &(*c))))
		return false;
	return true;
}
/*Procura o cliente e devolve a estrutura que detem seus dados*/
Cliente retorna_cliente(Cliente c, char *nome, int socket){
	Cliente aux;

	if(nome){
		for(aux = c; (aux) && (comparar(aux->nome, nome)!=0); aux = aux->dir);
	}else if(socket){
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
	char retorno[7], *saida = NULL;

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
	strcpy(saida, retorno);
	return (*saida);
}
/////////////////////////////////////////Funções para sockets//////////////////////////////////////////
bool comando_send(Cliente clientes, Cliente emisor){
	char send_msg[max_text]; //Conteudo total a ser enviado!
	Cliente aux; //Auxiliar para envio à todos os clientes!
	char *hora = NULL;
	int envio = 0;

	(*hora) = imprime_hora();//capturando hora;
	/*Criando a msg com o nome e a msg a ser enviada!*/
	snprintf(send_msg, max_text, "%s diz: %s", emisor->nome, msg);
	/*Loop que envia a msg para todos os clientes.*/
	for(aux = clientes; (aux) ; aux = aux->dir){
		//System("clear");
		if((aux->socket != emisor->socket)){
			send(aux->socket, send_msg, strlen(send_msg), 0);
			envio++;
		}
	}
	if((envio - 1) == max_users){
		printf("%s\t%s\tSEND\tExecutado:Sim", hora, emisor->nome);
		memset(nome, 0x0, max_text);
		memset(msg, 0x0, max_text);
		return true;
	}
	memset(nome, 0x0, max_text);
	memset(msg, 0x0, max_text);
	printf("%s\t%s\tSENDTO\tExecutado:Nao", hora, emisor->nome);

	return false;
}

bool comando_send_to(Cliente receptor, Cliente emisor){
	char send_to_msg [max_text];
	char *hora =  NULL;

	/*Criando a msg com o nome e a msg a ser enviada!*/
	snprintf(send_to_msg, max_text, "%s diz: %s", emisor->nome, msg);
	(*hora) = imprime_hora();//capturando hora;
	/*Enviando a msg para o usuário especificado!*/
	if ((receptor)&&(receptor->conexao)){
		//System("clear");
		send(receptor->socket, send_to_msg, strlen(send_to_msg), 0);
		printf("%s\t%s\tSENDTO\tExecutado:Sim", hora, emisor->nome);
		memset(nome, 0x0, max_text);
		memset(msg, 0x0, max_text);
		return true;
	}
	printf("%s\t%s\tSENDTO\tExecutado:Nao", hora, emisor->nome);
	memset(nome, 0x0, max_text);
	memset(msg, 0x0, max_text);
	return false;
}

bool comando_who(Cliente emisor, Cliente clientes){
	Cliente aux;
	char msg[max_text];

	snprintf(msg, sizeof(max_text), "\n---------------------------Bits Mensseger WHO-------------------------------\n");
	snprintf(msg, sizeof(max_text), "Usuarios conectados: ");
	for (aux = clientes; (aux); aux = aux->dir){
		if(comparar(aux->nome, emisor->nome) != 0)
			snprintf(msg, sizeof(max_text), "%s, ", aux->nome);
	}
	snprintf(msg, sizeof(max_text), "\n----------------------------------------------------------------------------\n");
	send(emisor->socket, msg, sizeof(msg), 0);
	memset(msg, 0x0, max_text);
	return true;
}

bool comando_help(Cliente emisor, Comandos comand){
	Comandos aux;
	char msg_help[max_text];

	snprintf(msg_help, max_text, "---------------------------Bits Mensseger HELP-------------------------------\n");
	for (aux = comand; (aux); aux = aux->dir){
		snprintf(msg_help, max_text, "| %s -- %s |\n", aux->comand, aux->descricao);
	}
	snprintf(msg_help, max_text, "-----------------------------------------------------------------------------\n");
	recv(emisor->socket, msg, sizeof(msg), 0);
	memset(msg, 0x0, max_text);
	return true;
}
void comando_erro(Cliente emisor, char *comando){
	char erro[max_text];

	snprintf(erro, max_text, "%s, comando não encontrado!\nPro favor use o comando <HELP> para verer a lista de comandos!\n", comando);
	recv(emisor->socket, erro, sizeof(erro), 0);
}
/*função para Saldação do servidor!*/
void bem_vindo(int socket){
	char *bem_vindo = "-----------------------------Bem Vindo ao Bits Mensseger---------------------------------";
	send(socket, bem_vindo, strlen(bem_vindo), 0);
	//printf("%s", bem_vindo);
}
/*Função que fará a leitura da mensagem, e processará os próximos passos!*/
char recebe_mensagem(int socket){
	int t;
	char *comando = NULL;

	memset(buffer, 0x0, max_text);
	memset(nome, 0x0, max_text);
	memset(msg, 0x0, max_text);
	t = recv(socket, buffer, max_text, 0);

	if (t == -1)
		return '-';
	retorna_nome_msg(comando);
	return (*comando);
}

int main(int argc, char **argv) {
	Cliente clientes, c;
		Cliente emisor;
		Comandos comandos;
		char *comando = NULL;
		char *bem_vindo = NULL;
		strcpy(bem_vindo, "-----------------------------Bem Vindo ao Bits Mensseger---------------------------------");
		int t;

		/*Variáveis para o socket*/
		int porta;
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

	    if (argc == 1) {
	        return -1;
	    }
		/*Capturando a porta passada na inicialização, e número máximo de usuáarios*/
	    if (argc > 2) {
	        users = atoi(argv[2]) ;
	    }

	    porta = atoi(argv[1]) ;
		/*definindo socket de escuta*/
	    socket_listen = socket(AF_INET, SOCK_STREAM, 0) ;

	    if (socket_listen < 0){
	        perror("socket");
	        return -1;
	    }
		/*Preenchendo a estrutura do socket*/
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

	  	/*usaremos Cntrl+C para sair, mas o certo será "quit"*/
	    while(1){
	        /*pegar todos os usuarios e colocar na estrutura*/
	        FD_ZERO(&select_set);
	        FD_SET(socket_listen, &select_set);
			/*vai setar o descritor para cada nova conexão.*/
	        for (t = 0, c = clientes; (c) && t < max_users; t++, c = c->dir){
	            if (c->socket != -1){
	                FD_SET(c->socket, &select_set);
	            }
	        }
			//atualiza a cada 5 segundos
	        select_time.tv_sec = 3;
	        select_time.tv_usec = 0;
			/*Definindo o select*/
	        if((t=select(FD_SETSIZE, &select_set, NULL, NULL, &select_time)) < 0){
	            perror("select");
	            return -1;
	        }

	        if(t > 0){
	           /*Aceitar a conexão de entrada e adicionar  novo socket na lista.*/
	            if (FD_ISSET(socket_listen, &select_set)){
	                int n;

	                if ((n=accept(socket_listen, NULL, NULL)) < 0){
	                    perror("accept");
	                }else if(!(cadastra_cliente(n, &clientes))){ /* server is busy */
	                    send(n, servidor_ocupado, strlen(servidor_ocupado), 0);
	                    close(n);
	                }else{
	                	for(emisor = clientes; (emisor) && (emisor->socket != n); i++, emisor = emisor->dir);
	                		snprintf(msg, max_text, "%s estar online!\n", emisor->nome);
	                    	comando_send(clientes, emisor);
	                }
	            }else{
	                int i ;
	                /*processa os dados recebidos*/
	                for (i = 0, emisor = clientes; (emisor) && (i < users); i++, emisor = emisor->dir) {
	                    if (FD_ISSET(emisor->socket, &select_set)){
	                        if (((*comando) = recebe_mensagem(emisor->socket))){
	                           	if(comparar(comando, "HELP")){
	                           		comando_help(emisor, comandos);
	                           	}else if(comparar(comando, "WHO")){
	                           		comando_who(emisor, clientes);
	                           	}else if(comparar(comando, "SEND")){
	                           		comando_send(clientes, emisor);
	                            }else if(comparar(comando, "SENDTO")){
	                            	comando_send_to(retorna_cliente(clientes, nome, 0), emisor);
	                            }else{
	                            	comando_erro(emisor, comando);
	                            }
	                        }
	                    }
	                }
	            }
	        }
	    }
	return EXIT_SUCCESS;
}
