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
int cm = 3; /*Para ser usado na função que seta os comandos*/
bool cadastro = false;
int conectados = 0;
fd_set select_acao, select_aux;
int clientes, novo_cliente, c, tamanho, nbytes; //para Sockets
char buffer[max_text];
pthread_t thread_cadastro, thread_comando; //Threads
int fd[2]; //para o pipe
Cliente passagem = NULL;
pthread_mutex_t mutex; //mutex
pthread_cond_t cond; //Variável condicional para o Mutex
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

	if (cm == 3){
		if (!((*c)=(Comandos)malloc(sizeof(comandos))))
           	return false;
		strcpy((*c)->comando, "SEND");
		strcpy((*c)->descricao, "Envia [CLIENTS_NAME]:[MENSSAGEM] para todos os clientes conectados (menos o cliente emissor). Uso: SEND:[MENSSAGEM]. OBS: Use os ':' Entre as intruções.");
		((*c)->dir) = NULL;
		cm = 2;
		inicializa_comandos(&(*c)->dir);
	}
	if (cm == 2){
		if (!((*c)=(Comandos)malloc(sizeof(comandos))))
           	return false;
		strcpy((*c)->comando, "SENDTO");
		strcpy((*c)->descricao, "Idêntico com SEND, ou seja, Envia [CLIENTS_NAME]:[MENSSAGEM], porém envia a mensagem apenas para o cliente especificado pelo [CLIENT_NAME]. Uso: SENDTO:[CLIENT_NAME_DE_DESTINO]:[MENSSAGEM]. OBS: Use os ':' Entre as intruções.");
		(*c)->dir = NULL;
		cm = 1;
		inicializa_comandos(&(*c)->dir);
	}
	if (cm == 1){
		if (!((*c)=(Comandos)malloc(sizeof(comandos))))
           	return false;
		strcpy((*c)->comando, "WHO");
		strcpy((*c)->descricao, "Retorna a lista dos clientes conectados ao servidor.");
		(*c)->dir = NULL;
		cm = 0;
		inicializa_comandos(&(*c)->dir);
	}
	if (cm == 0){
		if (!((*c)=(Comandos)malloc(sizeof(comandos))))
           	return false;
		strcpy((*c)->comando, "HELP");
		strcpy((*c)->descricao, "Retorna a lista de comandos suportados e seu uso.");
		(*c)->dir = NULL;
		cm = -1;
	}
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
void imprime_hora(char *tempo){
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
			sprintf(retorno, "%d:0%d", hora, minuto);
		}else
			sprintf(retorno, "%d:%d", hora, minuto);
	}
	strcpy(tempo, retorno);
}
/*O buffer vem no formato [Comando], ou [Comando:Mensagem], ou [Comando:Receptor:Mensagem],
e é dividodo a depender do comando passado!*/
bool separa_buffer(char comando[], char buffer[], char nome[], char msg[]){
	int i = 0, sd = 0, sdt = 0;
	char who[] = "WHO", help[] = "HELP", send[] = "SEND", sendto[] = "SENDTO";

	for (i = 0; (i < strlen(buffer)) && (i < 7); i++){
		if((buffer[i] != ':')&&(buffer[i] != '\0')&&(buffer[i] != '\n')){  // responsavel por tratar o erro de o usuario digitar valor nao especificado
			comando[i] = buffer[i];
		}
	}
	comando[i] = '\0';
	i++;
	int j;
	if(!(strcmp(comando, who)))
		return true;
	if(!(strcmp(comando, help)))
		return true;
	if(!(strcmp(comando, sendto))){
		for(j = 0; i < strlen(buffer); i++, j++){
			if(buffer[i] != ':'){
				nome[j] = buffer[i];
			}else if(buffer[i] == ':'){
				nome[j]='\0';
				i++;
				break;
			}
		}
		for(j = 0; i < (strlen(buffer)); j++, i++){
			if(buffer[i] != '\0'){
				msg[j] = buffer[i];
			}else{
				msg[j] = '\0';
				return true;
			}
		}	
	}else if(!(strcmp(comando, send))){
		for(j =0 ; i < (strlen(buffer)); j++, i++){
			if(buffer[i] != '\0'){
				msg[j] = buffer[i];
			}else{
				msg[j] = '\0';
				return true;
			}
		}
	}
	return false;
}

/*Função que pega a primeira msg recebida e coleta o nome do usuário.*/
bool recebe_nome(int socket, char *nome){
	int t;
	
	memset(nome, 0x0, nome_user); /*limpando o buffer para capturar a nova msg!*/
	t = recv(socket, nome, nome_user, 0);/*Recebendo o nome*/
	if (t <= 0) /*Verificando se o socket é valido, se o retorno for 0 é porque o cliente saiu*/
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
bool cadastra_clientes(int socket, char nome[], Cliente *clientes){
	if(!(*clientes)){
        if(!((*clientes)=(Cliente)malloc(sizeof(No)))) //Alocação (negacao )
            return false;
		strcpy((*clientes)->nome, nome);//Usando o retorno da função de cadastro para captura do nome.
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
/*remove o cliente da lista s ele estiverdeconectado*/
void remove_cliente(Cliente desconectado, Cliente *clientes){
	Cliente lista, aux;

	for(lista = (*clientes), aux = NULL; (lista) && (strcmp(lista->nome, desconectado->nome) != 0) ; aux = lista, lista = lista->dir);
	if(lista){
		if(aux){
			aux->dir = lista->dir;
			close((lista)->socket);
			free(lista);
		}else{
			if(lista->dir){
				aux = lista->dir;
				close((lista)->socket);
				free(lista);
			}
		}
		if((aux == NULL)&&(lista->dir == NULL)){
			close((lista)->socket);
			free(lista);
			(*clientes) = NULL;
		}
		conectados--;	
	}
}
//////////////////////////////////////////COMANDOS DO CHAT/////////////////////////////////////////////
/*Função que recebe a mensagem e envia para todos os clientes cadastrados, e que não sairam ainda.*/
bool comando_send(Cliente emisor, Cliente lista_clientes, char msg[]){
	char send_msg[max_text]; //Conteudo total a ser enviado!
	Cliente aux; //Auxiliar para envio à todos os clientes!
	bool erro = true;

	/*Criando a msg com o nome e a msg a ser enviada!*/
	snprintf(send_msg, max_text, "%s diz: %s", emisor->nome, msg);
	/*Loop que envia a msg para todos os clientes.*/
	for(aux = lista_clientes; (aux) ; aux = aux->dir){
		if((aux->socket != emisor->socket)){
			send(aux->socket, send_msg, strlen(send_msg), 0);
			erro = false;
		}
	}
	if(erro){
		return false;
	}
	return true;
}
/*Função que recebe a mensagem e o nome de um cliente e envia a mensagem para o cliente pedido.*/
bool comando_send_to(Cliente receptor, Cliente emisor, char msg[]){
	char send_to_msg[max_text];

	/*Criando a msg com o nome e a msg a ser enviada!*/
	snprintf(send_to_msg, max_text, "%s diz: %s", emisor->nome, msg);
	/*Enviando a msg para o usuário especificado!*/
	if (receptor){
		if(strcmp(receptor->nome, emisor->nome) != 0){
			send(receptor->socket, send_to_msg, strlen(send_to_msg), 0);
			return true;
		}
	}
	return false;
}
/*Função que envia a lista de usuários conectados*/
bool comando_who(Cliente emisor, Cliente lista_clientes){
	Cliente aux;
	char msg_who[max_text];

	snprintf(msg_who, max_text, "\n-----------------------------Bits Mensseger WHO---------------------------------\n");
	send(emisor->socket, msg_who, strlen(msg_who), 0);
	memset(msg_who, 0x0, max_text);
	snprintf(msg_who, max_text, "Usuários conectados: ");
	send(emisor->socket, msg_who, strlen(msg_who), 0);
	for (aux = lista_clientes; (aux); aux = aux->dir){
		memset(msg_who, 0x0, max_text);
		snprintf(msg_who, max_text, " %s", aux->nome);
		send(emisor->socket, msg_who, strlen(msg_who), 0);
	}
	memset(msg_who, 0x0, max_text);
	snprintf(msg_who, max_text, "\n--------------------------------------------------------------------------------\n");
	send(emisor->socket, msg_who, strlen(msg_who), 0);
	return true;
}
/*Função que envia, a lista de comando para o usuário*/
bool comando_help(Cliente emisor, Comandos comand){
	Comandos aux;
	char msg_help[max_text];

	snprintf(msg_help, max_text, "------------------------------Bits Mensseger HELP-------------------------------\n\n\n");
	send(emisor->socket, msg_help, strlen(msg_help), 0);
	for (aux = comand; (aux); aux = aux->dir){
		memset(msg_help, 0x0, max_text);
		snprintf(msg_help, max_text, "%s - %s\n\n", aux->comando, aux->descricao);
		send(emisor->socket, msg_help, strlen(msg_help), 0);
	}
	memset(msg_help, 0x0, max_text);
	snprintf(msg_help, max_text, "--------------------------------------------------------------------------------\n");
	send(emisor->socket, msg_help, strlen(msg_help), 0);
	return true;
}
/*Envia mensagem de desconhecimento do comando*/
void comando_erro(Cliente emisor, char comando[]){
	char erro[max_text];

	snprintf(erro, max_text, "%s, comando não encontrado - Pro favor use o comando [HELP] para ver a lista de comandos existentes!\n", comando);
	send(emisor->socket, erro, strlen(erro), 0);
}
/*Envia a MSG informando que o usuário desconctou*/
void usuario_desconectado(Cliente emissor, Cliente clientes){
	Cliente aux;
	char msg_desconect[max_text], horas[8];

	snprintf(msg_desconect, max_text, "%s desconectou!\n", (emissor)->nome);
	for(aux = clientes; (aux) ; aux = aux->dir){
		if((aux->socket != emissor->socket)){
			send(aux->socket, msg_desconect, strlen(msg_desconect), 0);
		}
	}
	imprime_hora(horas);
	printf("%s\t%s", horas, (emissor)->nome);
    printf("\tDesconectado\n");
}
///////////////////////////////////////////////////////////////Threads///////////////////////////////////////////////////////////////
/*função para threads, neste caso para o cadastro do cliente*/
void *nova_conexao(void){
	//int socket;
	char nome[nome_user], horas[8], msg[max_text], pipe[nome_user];
	Cliente clientes_conexao = passagem;
	struct sockaddr_in client;

	/*Loop que ira cuidar do cadastro do Usuários, ele aceita a conexão, e cadastra o usuário na lista*/
	while(true){
		novo_cliente = accept(clientes, (struct sockaddr *)&client, (socklen_t*)&c);
		if(recebe_nome(novo_cliente, nome)){
			//printf("Cadastro - Nome recebido\n");
			if(conectados < (max_users + 1)){
				if(cadastra_clientes(novo_cliente, nome, &clientes_conexao)){
					cadastro = true;
					conectados++;
					memset(msg, 0x0, max_text);
					strcpy(msg, "===========================Bem Vindo ao BITS MESSENGER==========================\n");
					send(novo_cliente, msg, strlen(msg), 0);
				}else{
					cadastro = false;
				}
			}else{
				memset(msg, 0x0, max_text);
				strcpy(msg, "Nao foi possivel estabelecer a conexao, servidor lotado!\n");
				send(novo_cliente, msg, strlen(msg), 0);
				close(novo_cliente);
			}
		}else{
			cadastro = false;
		}
		if(cadastro){
			imprime_hora(horas);//capturando hora;
			printf("%s\t%s\tConectado\n", horas, nome);
			//limpa o buffer
			pthread_mutex_lock(&mutex);
			memset(pipe, 0x0, max_text);
			//junta em buffer o que será passado
			sprintf(pipe, "%d", novo_cliente);
			/* Escrevendo a string no pipe */
    	    write(fd[1], &pipe, strlen(pipe));
			write(fd[1], &pipe, strlen(pipe));
			passagem = clientes_conexao;
			pthread_mutex_unlock(&mutex);
		}else{
			memset(msg, 0x0, max_text);
			strcpy(msg, "Nao foi possivel estabelecer a conexao, Usuario existente ou impossivel Cadastrar!\n");
			send(novo_cliente, msg, strlen(msg), 0);

			close(novo_cliente);
		}
		/*Verifica se o accept() falhou, se sim ele termina o servidor*/
		if(novo_cliente < 0){
			perror("accept falhou!");
			close(clientes);
			exit(8);
		}
	}
}
/*Função que atenderá os pedidos dos comando e decidirá qual será a função chamada*/
void *decide_acao(void){
	char mensagem[max_text], nome[nome_user], horas[8], comando[6], pipe[nome_user];
	char who[] = "WHO", help[] = "HELP", send[] = "SEND", sendto[] = "SENDTO";
	Cliente clientes_acao, aux, aux2, emissor, receptor, a;
	Comandos comandos;
	ssize_t nbytes, npipe;
	int i, j;

    if(!(cria_lista_comandos(&comandos))){
		perror("Lista Comandos erro!");
	}

	if(!(inicializa_comandos(&comandos))){
		perror("Lista Comandos erro!");
	}
    //Aqui tem que percorrer os usuários usando o select e atender aqueles que precisarem ser atendidos
    FD_ZERO(&select_acao);
    FD_ZERO(&select_aux);
    FD_SET(fd[0], &select_acao);
    clientes_acao = passagem;
    while(1){
    	FD_ZERO(&select_aux);
		select_aux = select_acao;
    	for(aux = clientes_acao; (aux); aux = aux->dir){
    	    if(aux->socket){
    	        FD_SET(aux->socket, &select_aux);
        	}
        }
    	if((select(FD_SETSIZE, &select_aux, NULL, NULL, NULL)) < 0){
           	perror("select");
           	exit(10);
       	}
    	if(FD_ISSET(fd[0], &select_aux)){
			//usar u mutex para para a thread e liberar a outra.
			memset(pipe, 0x0, max_text);
			npipe = read (fd [0], &pipe, sizeof (pipe));
			if((strlen(pipe)) > 1){
				pthread_mutex_lock(&mutex);
				clientes_acao = passagem;
				pthread_mutex_unlock(&mutex);
			}
    	} 
		for(emissor = clientes_acao, aux2 = NULL, i = 0; (emissor) && (i < conectados); aux2 = emissor, emissor = emissor->dir, i++){
			if(FD_ISSET(emissor->socket, &select_aux)){
    			memset(buffer, 0x0, max_text);
        		nbytes = recv(emissor->socket, buffer, sizeof(buffer), 0);
        		/*Verifica erro ou usuário desconectado*/
        		if (nbytes == 0){
        			//conexão finalizada, remoção do usuário necessária.
        			FD_CLR(emissor->socket, &select_aux);
        			usuario_desconectado(emissor, clientes_acao);
        			a = emissor;
        			remove_cliente(a, &clientes_acao);
        			if(aux2){
						emissor = aux2;
					}else{
						emissor = clientes_acao;
					}
					if(!(clientes_acao)){
						break;
					}
        			//Se ele não saiu tem que ver o que ele quer.
        		}else{
     				separa_buffer(comando, buffer, nome, mensagem);
        			if(strcmp(comando, help) == 0){
   						if(comando_help(emissor, comandos)){
    						imprime_hora(horas);
    						printf("%s\t%s\t", horas, emissor->nome);
    						printf("HELP\tExecutado:Sim\n");
    					}else{
    						imprime_hora(horas);
    						printf("%s\t%s", horas, emissor->nome);
    						printf("\tHELP\tExecutado:Nao\n");
    					}
   					}
   					if(strcmp(comando, who) == 0){
    					if(comando_who(emissor, clientes_acao)){
    						imprime_hora(horas);
    						printf("%s\t%s", horas, emissor->nome);
    						printf("\tWHO\tExecutado:Sim\n");
    					}else{
    						imprime_hora(horas);
    						printf("%s\t%s", horas, emissor->nome);
    						printf("\tWHO\tExecutado:Nao\n");
    					}
    				}
    				if(strcmp(comando, send) == 0){
    					if(comando_send(emissor, clientes_acao, mensagem)){
    						imprime_hora(horas);
    						printf("%s\t%s", horas, emissor->nome);
    						printf("\tSEND\tExecutado:Sim\n");
    					}else{
    						imprime_hora(horas);
    						printf("%s\t%s", horas, emissor->nome);
    						printf("\tSEND\tExecutado:Nao\n");
    					}
    				}
    				if(strcmp(comando, sendto) == 0){
    					receptor = retorna_cliente(clientes_acao, nome, 0);
    					if(comando_send_to(receptor, emissor, mensagem)){
    		 				imprime_hora(horas);
    						printf("%s\t%s", horas, emissor->nome);
    						printf("\tSENDTO\tExecutado:Sim\n");
    					}else{
    						imprime_hora(horas);
    						printf("%s\t%s", horas, emissor->nome);
    						printf("\tSENDTO\tExecutado:Nao\n");
    					}
    				}
    				if ((strcmp(comando, sendto) != 0)&&(strcmp(comando, send) != 0)&&(strcmp(comando, who) != 0)&&strcmp(comando, help) != 0){
    					comando_erro(emissor, comando);
    				}
        		}
        	}
       	}
    }
}
/*Função que desconecta todos os usuários antes de saír.*/
void desconect_all(void){
	Cliente aux = NULL;
	char msg_desc[] = "Servidor desconectado!\n";
	
	while(passagem){
		aux = passagem;
		send(aux->socket, msg_desc, strlen(msg_desc), 0);
		passagem = passagem->dir;
		close((aux)->socket);
		free(aux);
		aux = NULL;
	}
	close(clientes);
}
/*Função que destruirá as threads e o mutex antes de finalizar.*/ 
void destroi_mutex(void){
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);
}
/*Função para o Sinal Ctrl+C que deverar desconectar todos os usuários e desconectar o Servidor.*/
void termina_servidor(int sig){
	desconect_all();
	destroi_mutex();
	printf("\n-----------------------SERVIDOR BITS MESSENGER FINALIZADO-----------------------\n");
	exit(sig);	
}
/////////////////////////////////////////////////////////////Principal////////////////////////////////////////////////////////////
int main (int argc, char *argv[]){
	struct sockaddr_in servidor;
	int porta;
	
	if(argc != 2){
		system("clear");
		printf("Modo de uso: ./server_chat [PORTA]\n\n");
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
    	exit(3);
	}
	/**/
	if(bind(clientes, (struct sockaddr *)&servidor, sizeof(servidor)) < 0){
		perror("bind");
		close(clientes);
		exit(4);
	}
	system("clear");
	listen(clientes, max_users);
	tamanho = clientes;
	
	/* Criando nosso Pipe */
    if(pipe(fd) < 0){
        perror("pipe");
        exit(5);
    }
    //Redefini o Sinal Ctrl-C
	signal(SIGINT, termina_servidor);
   	// Cria o mutex
    pthread_mutex_init(&mutex, NULL);
    // Cria a variavel de condicao
    pthread_cond_init(&cond, NULL);
	printf("========================Servidor BITS MESSENGER iniciado!=======================\n");

	/*aqui chama a função que cadastra o cliente*/
	if(pthread_create(&thread_cadastro, NULL, (void*)&nova_conexao, NULL) < 0){
		termina_servidor(SIGINT);
		perror("Impossivel criar o Thread!");
		//aqui tem q fechar todos os clientes que estiverem abertos e depois sair.
		exit(7);
	}
	if(pthread_create(&thread_comando, NULL, (void*)&decide_acao, NULL) < 0){
		termina_servidor(SIGINT);
		perror("Impossivel criar Thread!");
		//aqui tem q fechar todos os clientes que estiverem abertos e depois sair.
		exit(10);
	}
	/*Loop infivnito para o servidor ficar sempre em execução*/
	int s, estado;
	s = pthread_join(thread_cadastro, (void **) &estado);
	s = pthread_join(thread_comando, (void **) &estado);
	close(clientes);
	return 0;
}
