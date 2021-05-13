// AtividadeWinsock.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.

#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <vector>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

int __cdecl main(void)
{
    WSADATA wsaData;
    int iResult;
    int AtuadorAquecedor = 0;
    int AtuadorResfriador = 0;
    int AtuadorFiltro = 0;
    int AtuadorValvulaEncher = 0;
    int AtuadorValvulaEsvaziar = 0;
    int AtuadorAumentaPh = 0;
    int AtuadorReduzPh = 0;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL;
    struct addrinfo hints;

    int iSendResult;
    char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf("listen failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed with error: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);

    // Receive until the peer shuts down the connection
    do {

        memset(recvbuf, 0x0, recvbuflen);

        // Recebe os valores de setpoint e dos sensores
        iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            recvbuf[iResult] = '\0';
            printf("Bytes received: %d\n", iResult);
            printf("Mensagem: %s \n", recvbuf);

            // Separa a string recebida criando um array de strings
            std::istringstream iss(recvbuf);
            std::vector<std::string> recvbuf_array;
            for (std::string s;iss >> s;) {
                recvbuf_array.push_back(s);
            }

            for (int i = 0; i < recvbuf_array.size(); i++) {
                std::cout << recvbuf_array[i] << std::endl;
            }

            // Variáveis de setpoint
            int setpointTemperatura;
            double setpointOxigenio;
            double setpointPhMax;
            double setpointPhMin;

            // Variáveis medidas pelos sensores
            int sensorTemperatura;
            double sensorOxigenio;
            double sensorPh;
            int sensorNivel;

            // Tranformação das strings de setpoint recebidas para valores int e double
            std::istringstream(recvbuf_array[0]) >> setpointTemperatura;
            std::istringstream(recvbuf_array[1]) >> setpointOxigenio;
            std::istringstream(recvbuf_array[2]) >> setpointPhMax;
            std::istringstream(recvbuf_array[3]) >> setpointPhMin;

            // Tranformação das strings de setpoint recebidas para valores int e double
            std::istringstream(recvbuf_array[4]) >> sensorTemperatura;
            std::istringstream(recvbuf_array[5]) >> sensorOxigenio;
            std::istringstream(recvbuf_array[6]) >> sensorPh;
            std::istringstream(recvbuf_array[7]) >> sensorNivel;

            // Calculando quais atuadores devem ser acionados
            // -> Temperatura *********************************************************
            if (sensorTemperatura == setpointTemperatura) {
                // Se a temperatura medida for igual ao setpoint...
                std::cout << "-> Temperatura perfeita!!" << std::endl;

                // Desliga o aquecedor ou resfriador
                AtuadorAquecedor = 0;
                AtuadorResfriador = 0;
            }
            else if (sensorTemperatura > setpointTemperatura) {
                // Se a temperatura medida for maior que o setpoint...
                std::cout << "-> Diminuindo a temperatura..." << std::endl;
                std::cout << "\t(" << setpointTemperatura << " <- " << sensorTemperatura << ")" << std::endl;

                // Aciona o resfriador
                AtuadorResfriador = 1;
            }
            else if (sensorTemperatura < setpointTemperatura) {
                // Se a temperatura medida for menor que o setpoint...
                std::cout << "-> Aumentando a temperatura..." << std::endl;
                std::cout << "\t(" << sensorTemperatura << " -> " << setpointTemperatura << ")" << std::endl;

                // Aciona o aquecedor
                AtuadorAquecedor = 1;
            }

            // -> Oxigênio ************************************************************
            if (sensorOxigenio >= setpointOxigenio) {
                // Se o nível de oxigênio medido for maior ou igual ao setpoint...
                std::cout << "-> Nivel de Oxigenio perfeito!!" << std::endl;

                // Desliga o filtro de água
                AtuadorFiltro = 0;
            }
            else if (sensorOxigenio < setpointOxigenio) {
                // Se o nível de oxigênio medido for menor ao setpoint...
                std::cout << "-> Aumentando nivel de oxigenio da agua..." << std::endl;
                // Exibe o valor obtido pelo sensor e o setpoint
                std::cout << "\t(" << sensorOxigenio << " -> " << setpointOxigenio << ")" << std::endl;

                // Aciona o filtro de água
                AtuadorFiltro = 1;
            }

            // -> Ph ******************************************************************
            if ((setpointPhMin < sensorPh) && (sensorPh < setpointPhMax)) {
                // Se o ph estiver dentro dos limites...
                std::cout << "-> Nivel de Ph perfeito!!" << std::endl;

                // Não adiciona nenhum reagente
                AtuadorAumentaPh = 0;
                AtuadorReduzPh = 0;
            }
            else if (sensorPh < setpointPhMin) {
                // Se o ph estiver abaixo do setpoint mínimo...
                std::cout << "-> Adicionando reagente que aumenta ph..." << std::endl;

                // Adiciona reagente que aumenta o ph da água
                AtuadorAumentaPh = 1;
            }
            else if (sensorPh > setpointPhMax) {
                // Se o ph estiver acima do setpoint máximo...
                std::cout << "-> Adicionando reagente que reduz ph..." << std::endl;

                // Adiciona reagente que reduz o ph da água
                AtuadorReduzPh = 1;
            }

            // -> Nível de água *******************************************************
            if (sensorNivel == 1) {
                // Se sensorNivel = 0, nível dentro dos limites
                std::cout << "-> Nivel da agua perfeito!!" << std::endl;

                // Desativa as válvulas de encher e esvaziar o aquário 
                AtuadorValvulaEncher = 0;
                AtuadorValvulaEsvaziar = 0;
            }
            else if (sensorNivel == 2) {
                // Se sensorNivel = 1, nível abaixo do limite mínimo
                std::cout << "-> Enchendo aquario..." << std::endl;

                // Ativa a válvula de encher o aquário 
                AtuadorValvulaEncher = 1;
            }
            else if (sensorNivel == 3) {
                // Se sensorNivel = 2, nível acima do limite máximo
                std::cout << "-> Esvaziando aquario..." << std::endl;

                // Ativa a válvula de esvaziar o aquário 
                AtuadorValvulaEsvaziar = 1;
            }

            // Formatando mensagem para enviar ao cliente
            // Transforma as variáveis int em string
            std::string AtAquec = std::to_string(AtuadorAquecedor);
            std::string AtResf = std::to_string(AtuadorResfriador);
            std::string AtFiltro = std::to_string(AtuadorFiltro);
            std::string AtEncher = std::to_string(AtuadorValvulaEncher);
            std::string AtEsvaziar = std::to_string(AtuadorValvulaEsvaziar);
            std::string AtAumPh = std::to_string(AtuadorAumentaPh);
            std::string AtRedPh = std::to_string(AtuadorReduzPh);

            // Concatena os valores de todas as variáveis acima em uma só string (separados por espaço)
            std::string dados_envio = AtAquec + " " + AtResf + " " + AtFiltro + " " + AtEncher + " " + AtEsvaziar + " " + AtAumPh + " " + AtRedPh;
            const char* dados_char = &dados_envio[0];

            // Echo the buffer back to the sender
            iSendResult = send(ClientSocket, dados_char, (int)strlen(dados_char), 0);
            if (iSendResult == SOCKET_ERROR) {
                printf("send failed with error: %d\n", WSAGetLastError());
                closesocket(ClientSocket);
                WSACleanup();
                return 1;
            }
            printf("Bytes sent: %d\n", iSendResult);
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(ClientSocket);
            WSACleanup();
            return 1;
        }
    } while (iResult > 0);

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();

    return 0;
}

// Executar programa: Ctrl + F5 ou Menu Depurar > Iniciar Sem Depuração
// Depurar programa: F5 ou menu Depurar > Iniciar Depuração

// Dicas para Começar: 
//   1. Use a janela do Gerenciador de Soluções para adicionar/gerenciar arquivos
//   2. Use a janela do Team Explorer para conectar-se ao controle do código-fonte
//   3. Use a janela de Saída para ver mensagens de saída do build e outras mensagens
//   4. Use a janela Lista de Erros para exibir erros
//   5. Ir Para o Projeto > Adicionar Novo Item para criar novos arquivos de código, ou Projeto > Adicionar Item Existente para adicionar arquivos de código existentes ao projeto
//   6. No futuro, para abrir este projeto novamente, vá para Arquivo > Abrir > Projeto e selecione o arquivo. sln
