/*
 ============================================================================
 Name        : server_chat.c
 Author      : Jônatas da Silva
 Version     :
 Copyright   : Só me pedir!
 Description : Servidor do chat Bits Messenger in C, Ansi-style
 ============================================================================
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h> //Includes para o uso do Select();
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h> //pra Threads
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

#define max_users 50
#define max_text 2002
#define nome_user 30
#define command 7

Cliente lista_clientes;
struct tm *tempo_atual; /*Armazena data e hora*/
time_t segundos; /*Valor em segundos armazenados pra conversão em horas ou minutos*/
int comand = 3; /*Para ser usado na função que seta os comandos*/
bool cadastro = false;
fd_set select_acao, select_aux;

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
	char comando[7];
	char descricao[max_text];
	struct comandos *dir;
}comandos, *Comandos;
//////////////////////////////////////////////Funções//////////////////////////////////////////////////////
/*Cria a lista de clientes que conectarão com o servidor*/
bool cria_lista_clientes(Cliente *clientes){
	(*clientes) = NULL;
	return true;
}
/*Cria a lista de comandos que será usada para */
bool cria_lista_comandos(Comandos *comand){
	(*comand) = NULL;
	return true;
}
/*Define os comandos, eles serão passados quando o help for acionado!*/
bool inicializa_comandos(Comandos *c){
	/*Preenchendo a lista com os comandos e suas descrições.*/
	while(comand >= 0){
		if (comand == 3){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			strcpy((*c)->comando, "SEND");
			strcpy((*c)->descricao, "Envia [CLIENTS_NAME]:[MENSSAGEM] para todos os clientes conectados (menos o cliente emissor).");
			((*c)->dir) = NULL;
			comand--;
			inicializa_comandos(&(*c)->dir);
		}
		if (comand == 2){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			strcpy((*c)->comando, "SENDTO");
			strcpy((*c)->descricao, "Idêntico com SEND, ou seja, Envia [CLIENTS_NAME]:[MENSSAGEM], porém envia a mensagem apenas para o cliente especificado pelo [CLIENT_NAME].");
			(*c)->dir = NULL;
			comand--;
			inicializa_comandos(&(*c)->dir);
		}
		if (comand == 1){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			strcpy((*c)->comando, "WHO");
			strcpy((*c)->descricao, "Retorna a lista dos clientes conectados ao servidor.");
			(*c)->dir = NULL;
			comand--;
			inicializa_comandos(&(*c)->dir);
		}
		if (comand == 0){
			if (!((*c)=(Comandos)malloc(sizeof(comandos))))
            	return false;
			strcpy((*c)->comando, "HELP");
			strcpy((*c)->descricao, "Retorna a lista de comandos suportados e seu uso.");
			(*c)->dir = NULL;
			comand--;
		}
	}
	return true;
}
/*O buffer vem no formato [Comando], ou [Comando:Mensagem], ou [Comando:Receptor:Mensagem],
e é dividodo a depender do comando passado!*/
bool separa_buffer(char *c, char *buffer, char *nome, char *msg){
	char *comando = malloc(sizeof(command));
	int i;

	for(i = 0; i < 7; i++){
		if (buffer[i] != ':'){
			comando[i] = buffer[i];
		}else if(buffer[i] == ':'){
			comando[i] = '\0';
			i++;
			break;
		}
	}
	memset(nome, 0x0, nome_user);
	memset(msg, 0x0, max_text);
	strcpy(c, comando);
	free(comando);
	if(!(strcmp("SENDTO", c))){
		for(; i < (strlen(buffer) - strlen(c)); i++){
			if(buffer[i] != ':'){
				nome[i] = buffer[i];
			}else if(buffer[i] == ':'){
				nome[i] = '\0';
				i++;
				for(; i < ((strlen(buffer) - (strlen(c)+strlen(nome)))); i++){
					if(buffer[i] != '\0'){
						msg[i] = buffer[i];
					}else{
						msg[i] = '\0';
						return true;
					}
				}
			}
		}
	}else if(!(strcmp("SEND", c))){
		for(; i < (strlen(buffer) - strlen(c)); i++){
			if(buffer[i] != '\0'){
				msg[i] = buffer[i];
			}else{
				msg[i] = '\0';
				return true;
			}
		}
	}
}
/*Função que pega a primeira msg recebida e coleta o nome do usuário.*/
bool recebe_nome(int socket, char *nome){
	int t;
	
	memset(buffer, 0x0, max_text); /*limpando o buffer para capturar a nova msg!*/
	t = recv(socket, nome, nome_user, 0);/*Recebendo o nome*/
	if (t == -1) /*Verificando se o socket é valido*/
		return false;

	return true;
}
/*Procura o cliente e devolve a estrutura que detem seus dados*/
Cliente retorna_cliente(char *nome, int socket){
	Cliente aux;

	if((nome)||(socket)){ /*Procurando por nome ou socket*/
		for(aux = clientes; (aux) && ((strcmp(aux->nome, nome)!=0) || (aux->socket == socket)); aux = aux->dir);
		return (aux);
	}
	return NULL;
}
/*Função que Obtem a hora da biblioteca time*/
int Obtem_hora(void){
    time(&segundos); //obtém a hora em segundos.
    tempo_atual = localtime(&segundos); //converte horas em segundos.
    return(tempo_atual->tm_hour); //retorna as horas de 0 a 24.
}
/*Função que Obtem os minutos da biblioteca time*/
int Obtem_minuto(void){
    time(&segundos); //obtém a hora em segundos.
    tempo_atual = localtime(&segundos); //converte horas em segundos.
    return(tempo_atual->tm_min); //retorna os minutos de 0 a 59.
}
/*Função para Impressão da Hora no formato fornecido!*/
bool imprime_hora(char *tempo){
	int hora, minuto;
	char retorno[7];

	hora = Obtem_hora();
	minuto = Obtem_minuto();

	if (hora <10)
		snprintf(retorno, 7, "0%d:", hora);
	else
		snprintf(retorno, 7, "%d:", hora);
	if (minuto < 10)
		snprintf(retorno, 7, "0%d", minuto);
	else
		snprintf(retorno, 7, "%d", minuto);
	strcpy(tempo, retorno);
	return true;
}
/*Lista de Clientes Conectados, quando o cliente sai ele tem o campo conexao setado como false, se
conectado com true*/
bool cadastra_clientes(int socket, char *nome, Cliente *clientes){
	if(!(*clientes)){
        if(!((*clientes)=(Cliente)malloc(sizeof(No)))) //Alocação
            return false;
		strcpy((*clientes)->nome, nome);//Usando o retorno da função de cadastro para captura do nome.
		(*clientes)->conexao = true; // para determinar se o usuário estar conectado.
		(*clientes)->socket = socket; //Cada cliente tem um valor diferente de socket
        (*clientes)->dir = NULL;
    }else{
		/*Aqui temos um retorno de controle caso o usuário já exista e esteja conectado.*/
        if(retorna_cliente(nome, socket)){
            return false;
        }else{
			//chama recursivamente a função até que ache o espaço vazio para inserir o novo usuário.
			cadastra_clientes(socket, nome, &(*clientes)->dir);
		}
    }
    return true;
}
//////////////////////////////////////////COMANDOS DO CHAT/////////////////////////////////////////////

bool comando_send(Cliente emisor, char *msg){
	char send_msg[max_text]; //Conteudo total a ser enviado!
	Cliente aux; //Auxiliar para envio à todos os clientes!

	/*Criando a msg com o nome e a msg a ser enviada!*/
	snprintf(send_msg, max_text, "%s diz: %s", emisor->nome, msg);
	/*Loop que envia a msg para todos os clientes.*/
	for(aux = clientes; (aux) ; aux = aux->dir){
		if((aux->socket != emisor->socket)){
			send(aux->socket, send_msg, strlen(send_msg), 0);
			return true;
		}
	}
	return false;
}

bool comando_send_to(Cliente receptor, Cliente emisor, char *msg){
	char send_to_msg [max_text];

	/*Criando a msg com o nome e a msg a ser enviada!*/
	snprintf(send_to_msg, max_text, "%s diz: %s", emisor->nome, msg);
	/*Enviando a msg para o usuário especificado!*/
	if ((receptor)&&(receptor->conexao)){
		send(receptor->socket, send_to_msg, strlen(send_to_msg), 0);
		return true;
	}
	return false;
}
/*Comando que envia a lista de usuários conectados*/
bool comando_who(Cliente emisor){
	Cliente aux;
	char msg[max_text];

	snprintf(msg, sizeof(max_text), "\n---------------------------Bits Mensseger WHO-------------------------------\n");
	snprintf(msg, sizeof(max_text), "Usuarios conectados: ");
	for (aux = clientes; (aux); aux = aux->dir){
		if(strcmp(aux->nome, emisor->nome) != 0)
			snprintf(msg, sizeof(max_text), "%s, ", aux->nome);
	}
	snprintf(msg, sizeof(max_text), "\n\n----------------------------------------------------------------------------\n");
	send(emisor->socket, msg, sizeof(msg), 0);
	memset(msg, 0x0, max_text);
	return true;
}
/*Comando que envia, a lista de comando para o usuário*/
bool comando_help(Cliente emisor, Comandos comand){
	Comandos aux;
	char msg_help[max_text];

	snprintf(msg_help, max_text, "---------------------------Bits Mensseger HELP-------------------------------\n");
	for (aux = comand; (aux); aux = aux->dir){
		snprintf(msg_help, max_text, "| %s -- %s |\n", aux->comando, aux->descricao);
	}
	snprintf(msg_help, max_text, "-----------------------------------------------------------------------------\n");
	send(emisor->socket, msg_help, strlen(msg_help), 0);
	return true;
}
/*Envia mensagem de desconhecimento do comando*/
void comando_erro(Cliente emisor, char *comando){
	char erro[max_text];

	snprintf(erro, max_text, "%s, comando não encontrado!\nPro favor use o comando [HELP] para verer a lista de comandos!\n", comando);
	send(emisor->socket, erro, strlen(erro), 0);
}
//////////////////////////////////////////////////Para Threads//////////////////////////////////////////////////
/*função para threads, neste caso para o cadastro do cliente*/
void *nova_conexao(void *novo_cliente){
	int socket = *(int*)novo_cliente, tamanho;
	char *mensagem, nome[nome_user], buffer[max_text];

	recebe_nome(novo_cliente, nome);
	if(cadastra_clientes(socket, nome, &lista_clientes)){
		cadastro = true;
		imprime_hora(horas);//capturando hora;
		memset(buffer, 0x0, max_text);
		strcpy(buffer, "===============================Bem Vindo ao BITS MESSENGER============================\n");
		send(socket, buffer, max_text, 0);
		printf("%s\t%s\tConectado\n", horas, nome);
	}else{
		cadastro = false;
	}
}

int main (int argc, char *argv[]){
	Cliente c, emisor, aux;
	Comandos comandos;
	int clientes, novo_cliente, tamanho, t;
	char buffer [max_text];
	char nome[nome_user], msg [max_text], comando[7], horas[8];
	struct sockaddr_in servidor, client;
	struct timeval select_time;
	bool controle = true;
	int porta;
	ssize_t nbytes;
	//pid_t pid;
	
	if(argc != 2){
		system("clear");
		printf("Modo de uso: ./servidor [porta]\n\n");
		exit(1);
	}
	porta = atoi(argv[1]);
	//Tipo de conexão
	servidor.sin_family = AF_INET;
	//Usar IP local
	servidor.sin_addr.s_addr = htons(INADDR_ANY);
	//Usa PORTA passada
	servidor.sin_port = htons(porta);
	/*Criando o socket do servidor*/
	if ((clientes = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Socket");
		exit(2);
	}
	int sim=1;
	/*impede falha que retorna o erro: "Endereço já em uso", está parte do código permite
	reutilizar uma porta que ta bloqueada por um processo que a estar monopolizando.*/
	if (setsockopt(clientes,SOL_SOCKET,SO_REUSEADDR,&sim,sizeof(int)) == -1) {
    	perror("setsockopt");
    	close(clientes);
    	exit(4);
	}
	/**/
	if(bind(clientes, (struct sockaddr *)&servidor, sizeof(servidor)) < 0){
		perror("bind");
		close(clientes);
		exit(3);
	}
	system("clear");
	listen(clientes, max_users);
	tamanho = sizeof(servidor);
	/*atualiza a cada 3 segundos no uso do select*/
    select_time.tv_sec = 3;
    select_time.tv_usec = 0;
	/*Inicializando e criando listas a serem usadas.*/
    if (!(cria_lista_clientes(&lista_clientes))){
		perror("Lista clientes erro!");
	}
	if(!(cria_lista_comandos(&comandos))){
		perror("Lista Comandos erro!");
	}
	if(!(inicializa_comandos(&comandos))){
		perror("Lista Comandos erro!");
	}

	printf("========================Servidor BITS MESSENGER iniciado!=======================\n");
	printf("Esperando conexao!\n");

	while(1){
		/*Loop que ira cuidar do cadastro do Usuários, ele aceita a conexão, e cadastra o usuário na lista*/
		while((novo_cliente = accept(clientes, (struct sockaddr *)&client, (socklen_t*)&c))){
			printf("Conexao aceita!\n");

			pthread_t thread_cadastro;
			/*aqui chama a função que cadastra o cliente*/
			if(pthread_create(&thread_cadastro, NULL, nova_conexao, (void*)&novo_cliente) < 0){
				perror("Impossivel criar o Thread!");
				close(clientes);
				exit(5);
			}
			if(cadastro){
				break;
			}else{
				memset(buffer, 0x0, max_text);
				strcpy(buffer, "Nao foi possivel estabelecer a conexao!\n");
				send(novo_cliente, buffer, max_text, 0);
			}
			break;
		}
		/*Verifica se o accept() falhou, se sim ele termina o servidor*/
		if(novo_cliente < 0){
			perror("accept falhou!");
			close(clientes);
			exit(6);
		}
		/**/
		while(clientes){
			printf("Clientes existentes!\n");

			pthread_t thread_comando;


		}

	}
	

	return 0;
}