#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h> //Includes para o uso do Select();
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdbool.h>
#include <pthread.h> //pra Threads
#include <signal.h> //Para os Sinais, Ex.: Ctrl+C
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <time.h>

#define max_users 100
#define max_text 2002
#define nome_user 30
#define command 7

/*Estrutura para cadastro dos usuário conectados. Usado no controle deles*/
typedef struct No{
	char nome[nome_user];
	bool conexao;
	int socket;
	struct No *dir;
}No, *Cliente;
/*Estrutura para impressão dos comandos muito util, na hora do camando help*/
typedef struct comandos{
	char comando[7];
	char descricao[max_text];
	struct comandos *dir;
}comandos, *Comandos;


struct tm *tempo_atual; /*Armazena data e hora*/
time_t segundos; /*Valor em segundos armazenados pra conversão em horas ou minutos*/
int comand = 4; /*Para ser usado na função que seta os comandos*/
bool cadastro = false;
int conectados = 0;
fd_set select_acao, select_aux;
static int espera_sinal = 1;
int clientes, novo_cliente, c, tamanho;
//struct No *pontero_cliente;
char buffer[max_text];

//////////////////////////////////////////////Funções//////////////////////////////////////////////////////
/*Cria a lista de clientes que conectarão com o servidor*/
bool cria_lista_clientes(Cliente *clientes){
	(*clientes) = NULL;
	return true;
}
/*Cria a lista de comandos que será usada para*/
bool cria_lista_comandos(Comandos *comand){
	(*comand) = NULL;
	return true;
}
/*Define os comandos, eles serão passados quando o help for acionado!*/
bool inicializa_comandos(Comandos *c){
	/*Preenchendo a lista com os comandos e suas descrições.*/
	do{
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
	}while(comand);
	return true;
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

	if(hora <10){
		if(minuto < 10){
			sprintf(retorno, "0%d:0%d", hora, minuto);
		}else
			sprintf(retorno, "0%d:%d", hora, minuto);	
	}else{
		if(minuto < 10){
			sprintf(retorno, "%d:0%d", minuto);
		}else
			sprintf(retorno, "%d:%d", minuto);
	}
	strcpy(tempo, retorno);
	return true;
}
/*O buffer vem no formato [Comando], ou [Comando:Mensagem], ou [Comando:Receptor:Mensagem],
e é dividodo a depender do comando passado!*/
bool separa_buffer(char *comando, char *buffer, char *nome, char *msg){
	//char *comando = malloc(sizeof(command));
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
	//strcpy(c, comando);
	//free(comando);
	if(!(strcmp("SENDTO", comando))){
		for(; i < (strlen(buffer) - strlen(comando)); i++){
			if(buffer[i] != ':'){
				nome[i] = buffer[i];
			}else if(buffer[i] == ':'){
				nome[i] = '\0';
				i++;
				for(; i < ((strlen(buffer) - (strlen(comando)+strlen(nome)))); i++){
					if(buffer[i] != '\0'){
						msg[i] = buffer[i];
					}else{
						msg[i] = '\0';
						return true;
					}
				}
			}
		}
	}else if(!(strcmp("SEND", comando))){
		for(; i < (strlen(buffer) - strlen(comando)); i++){
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
	
	memset(nome, 0x0, nome_user); /*limpando o buffer para capturar a nova msg!*/
	t = recv(socket, nome, nome_user, 0);/*Recebendo o nome*/
	if (t == -1) /*Verificando se o socket é valido, se o retorno for 0 é porque o cliente saiu*/
		return false;
	return true;
}
/*Procura o cliente e devolve a estrutura que detem seus dados*/
Cliente retorna_cliente(Cliente lista_clientes, char *nome, int socket){
	Cliente aux = lista_clientes;

	if((nome)||(socket)){ /*Procurando por nome ou socket*/
		for(; (aux) && ((strcmp(aux->nome, nome)!=0) || (aux->socket == socket)); aux = aux->dir);
		return (aux);
	}
	return NULL;
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
        if(retorna_cliente((*clientes), nome, socket)){
            return false;
        }else{
			//chama recursivamente a função até que ache o espaço vazio para inserir o novo usuário.
			cadastra_clientes(socket, nome, &(*clientes)->dir);
		}
    }
    return true;
}
/*função para threads, neste caso para o cadastro do cliente*/
void *nova_conexao(void *novo_cliente){
	int *s = (int *)novo_cliente;
	int socket = (*s);
	char nome[nome_user], buffer[max_text], horas[8];
	Cliente clientes_conexao;

	if(!(clientes_conexao)){
		if (!(cria_lista_clientes(&clientes_conexao))){
			perror("Lista clientes erro!");
			exit(5);
		}
	}

	if(recebe_nome(socket, nome)){
		if(conectados < (max_users + 1)){
			if(cadastra_clientes(socket, nome, &clientes_conexao)){
				cadastro = true;
				conectados++;
				imprime_hora(horas);//capturando hora;
				memset(buffer, 0x0, max_text);
				strcpy(buffer, "===========================Bem Vindo ao BITS MESSENGER==========================\n");
				send(socket, buffer, max_text, 0);
				printf("%s\t%s\tConectado\n", horas, nome);
			}else{
				cadastro = false;
				//break;
			}
		}
	}
	
}
/////////////////////////////////////////////////////////////Principal////////////////////////////////////////////////////////////
int main (int argc, char *argv[]){
	struct sockaddr_in servidor, client;
	bool controle = true;
	int porta;
	
	if(argc != 2){
		system("clear");
		printf("Modo de uso: ./servidor [porta]\n\n");
		exit(1);
	}
	porta = atoi(argv[1]);
	bzero((void *) &servidor, sizeof(servidor));
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
	int sim = 1;
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
	
	/*Inicializando e criando listas a serem usadas.*/
 	/*
	if(!(cria_lista_comandos(&comandos))){
		perror("Lista Comandos erro!");
	}
	if(!(inicializa_comandos(&comandos))){
		perror("Lista Comandos erro!");
	}
	*/
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
				//close(clientes);  aqui tem q fechar todos os clientes que estiverem abertos e depois sair.
				exit(6);
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
			exit(7);
		}
		/*
		while(conectados){
			printf("Clientes existentes!\n");

			pthread_t thread_comando;

			/*aqui chama a função que decide o qual comando será feito e para qual usuário
			if(pthread_create(&thread_comando, NULL, (void*)&decide_acao, NULL) < 0){
				perror("Impossivel criar Thread!");
				//aqui tem q fechar todos os clientes que estiverem abertos e depois sair.
				exit(6);
			}

		}*/

	}
	close(clientes);
	return 0;
}
