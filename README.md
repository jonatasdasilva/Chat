
/** TRABALHO FINAL DA DISCIPLINA DE SISTEMAS OPERACIONAIS
PROFESSOR: Paul Regnier
ALUNOS: Fagner Bezerra Batista, Jonatas da Silva Santos, Wendel de Araujo Dourado
Emails: bfagner@hotmail.com; jonatas.silva.roots@gmail.com; wendelad@live.com; **/

/**Instruções para execução do Servidor de Mensagens BITS MESSENGER**/

/**

Para executar este programa faça o seguinte:

1) Abra o terminal e navegue até a pasta onde se encontram os arquivos necessários para a execução: "server_chat.c, client_chat.c e Makefile"

2) Quando estiver no diŕetório digite o comando: make 

3) Agora basta iniciar o servidor para isto faça: ./server_chat [PORTA]

4) Em seguida basta iniciar o cliente, para isto faça: ./client_server [CLIENTE_NAME] [IP_SERVIDOR] [PORTA_SERVIDOR]

5) Existem alguns comandos predefinidos no servidor de mensagens, são eles:

	5.1) SEND - Envia [CLIENTS_NAME]:[MENSSAGEM] para todos os clientes conectados (menos o cliente emissor). 
	Uso: SEND:[MENSSAGEM].
	OBS: Use os ':' entre as intruções.

	5.2) SENDTO - Idêntico com SEND, ou seja, Envia [CLIENTS_NAME]:[MENSSAGEM], porém envia a mensagem apenas para o cliente especificado pelo [CLIENT_NAME].
	Uso: SENDTO:[CLIENT_NAME_DE_DESTINO]:[MENSSAGEM].
	OBS: Use os ':' entre as intruções.

	5.3) WHO - Retorna a lista dos clientes conectados ao servidor.

	5.4) HELP - Retorna a lista de comandos suportados e seu uso.

	OBS.: Os comandos devem ser usados com letras maiúsculas.

6) Para finalizar basta pressionar a combinação de teclas CNTRL+C.

**/
