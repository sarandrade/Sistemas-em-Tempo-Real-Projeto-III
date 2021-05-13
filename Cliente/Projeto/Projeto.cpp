// AtividadeInOutThreads.cpp : Este arquivo contém a função 'main'. A execução do programa começa e termina ali.
/*  # Disciplina: Sistemas em Tempo Real
    # Professor: Danilo Freire de Souza Santos
    # Aluna: Sara Andrade Dias - 116110719
    # Projeto: Aquário
*/

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <thread>
#include <chrono>
#include <ctime>
#include <mutex>
#include <string>
#include <iomanip>
#include <sstream>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

// Defines
#define COMMAND_BUFF_SIZE 50
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"


// Global Variables *******************************************************
HANDLE hStdin;
DWORD fdwSaveOldMode;
int flagExecuteCommand = 0;

std::mutex mutexHandler;

int setpointTemperatura = 27;
double setpointOxigenio = 8.3;
double setpointPhMax = 7.8;
double setpointPhMin = 7.2;

int sensorTemperatura = 0;
double sensorOxigenio = 0;
double sensorPh = 0;
int sensorNivel = 0;

int AtuadorAquecedor = 0;
int AtuadorResfriador = 0;
int AtuadorFiltro = 0;
int AtuadorValvulaEncher = 0;
int AtuadorValvulaEsvaziar = 0;
int AtuadorAumentaPh = 0;
int AtuadorReduzPh = 0;

// Functions Prototypes ***************************************************

void ErrorExit(LPSTR);
void appSetup(void);
void appExit(void);

// Funções Auxiliares *****************************************************

std::string formataDouble(double variavel_double) {
    // Função que transforma uma variável double em uma variável string com apenas uma casa decimal
    std::stringstream stream;
    stream << std::fixed << std::setprecision(1) << variavel_double;
    std::string variavel_string = stream.str();
    
    // Retorna a string com o valor da variável double
    return variavel_string;
}

std::string formataDados() {
    // Transforma variáveis em string
    // Para as variáveis int utilizou-se a função to_string
    // Para as variáveis double utilizou-se a função formataDouble
    std::string setTemp = std::to_string(setpointTemperatura);
    std::string setOxi = formataDouble(setpointOxigenio); // Double
    std::string setPhMax = formataDouble(setpointPhMax); // Double
    std::string setPhMin = formataDouble(setpointPhMin); // Double
    std::string senTemp = std::to_string(sensorTemperatura);
    std::string senOxi = formataDouble(sensorOxigenio); // Double
    std::string senPh = formataDouble(sensorPh); // Double
    std::string senNivel = std::to_string(sensorNivel);

    // Concatena os valores de todas as variáveis em uma só string (separados por espaço)
    std::string dados_envio = setTemp + " " + setOxi + " " + setPhMax + " " + setPhMin + " " + senTemp + " " + senOxi + " " + senPh + " " + senNivel;

    // Retorna a string com os valores das variáveis concatenados
    return dados_envio;
}

int enviaDados(std::string dados_para_enviar) {
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct addrinfo* result = NULL,
        * ptr = NULL,
        hints;

    const char* dados_char = &dados_para_enviar[0];

    // const char* sendbuf = "this is a test";
    char recvbuf[DEFAULT_BUFLEN];
    int iResult;
    int recvbuflen = DEFAULT_BUFLEN;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (ConnectSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    // Send an initial buffer
    iResult = send(ConnectSocket, dados_char, (int)strlen(dados_char), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %ld\n", iResult);

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    do {

        iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
            recvbuf[iResult] = '\0';
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

            // Tranformação das strings dos atuadores recebidas para valores int
            std::istringstream(recvbuf_array[0]) >> AtuadorAquecedor;
            std::istringstream(recvbuf_array[1]) >> AtuadorResfriador;
            std::istringstream(recvbuf_array[2]) >> AtuadorFiltro;
            std::istringstream(recvbuf_array[3]) >> AtuadorValvulaEncher;
            std::istringstream(recvbuf_array[3]) >> AtuadorValvulaEsvaziar;
            std::istringstream(recvbuf_array[3]) >> AtuadorAumentaPh;
            std::istringstream(recvbuf_array[3]) >> AtuadorReduzPh;
        }
        else if (iResult == 0)
            printf("Connection closed\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (iResult > 0);

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

// Threads ****************************************************************
void readCommand(char* command)
{
    DWORD cNumRead, fdwMode, i;
    INPUT_RECORD irInBuf[128];
    KEY_EVENT_RECORD ker;
    char keyRead;
    std::string concatKeys;

    while (true)
    {
        // Read Keys ====================================

        // Wait for the events.
        if (!ReadConsoleInput(
            hStdin,      // input buffer handle
            irInBuf,     // buffer to read into
            128,         // size of read buffer
            &cNumRead)) // number of records read
            ErrorExit((LPSTR)"ReadConsoleInput");

        // Dispatch the events to the appropriate handler.
        for (i = 0; i < cNumRead; i++)
        {
            switch (irInBuf[i].EventType)
            {
            case KEY_EVENT: // keyboard input
                // KeyEventProc(irInBuf[i].Event.KeyEvent);
                ker = irInBuf[i].Event.KeyEvent;
                keyRead = ker.uChar.AsciiChar;

                if (ker.bKeyDown)
                {
                    std::cout << keyRead;
                    // std::cout << " - VK Code: " << std::hex<< ker.wVirtualKeyCode; // << std::endl;
                    // std::cout << " - dwControlKeyState: " << std::hex << irInBuf[i].Event.KeyEvent.dwControlKeyState << std::endl;

                    // Interpret Command 
                    switch (ker.wVirtualKeyCode)
                    {
                    case VK_RETURN:
                        std::cout << std::endl;

                        // Return the Command 
                        memcpy(command, concatKeys.c_str(), concatKeys.size());

                        //std::cout << "concatKeys: " << concatKeys << std::endl;
                        //std::cout << "Command: " << command << std::endl;

                        concatKeys = "";
                        flagExecuteCommand = 1;

                        break;

                    case VK_BACK:
                        std::cout << " \b";
                        concatKeys.pop_back();
                        break;

                    case VK_SHIFT:
                    case VK_CAPITAL:
                        break;

                    default:
                        concatKeys += keyRead;
                        break;
                    }
                }
                break;

            default:
                //ErrorExit( (LPSTR) "Unknown event type");
                break;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    std::cout << "Exit readCommand!" << std::endl;
}

void executeCommand(char* command, int commandSize)
{
    while (true)
    {
        if (flagExecuteCommand)
        {
            char* ptr = strstr(command, "set");
            char* ptrVar;
            char varName[32] = "";
            int varValue = 0;

            if (ptr != NULL)
            {
                ptr += 4;
                ptrVar = strstr(ptr, "=");

                memcpy(varName, ptr, (ptrVar - ptr));

                if (ptrVar != NULL)
                {
                    ptrVar += 1;
                    varValue = std::atoi(ptrVar);
                }
                if (!strcmp(varName, "setpointTemperatura"))
                {
                    std::cout << varName << "=" << varValue << std::endl;
                    setpointTemperatura = varValue;
                }
                else if (!strcmp(varName, "setpointOxigenio"))
                {
                    std::cout << varName << "=" << varValue << std::endl;
                    setpointOxigenio = varValue;
                }
                else if (!strcmp(varName, "setpointPhMax"))
                {
                    std::cout << varName << "=" << varValue << std::endl;
                    setpointPhMax = varValue;
                }
                else if (!strcmp(varName, "setpointPhMin"))
                {
                    std::cout << varName << "=" << varValue << std::endl;
                    setpointPhMin = varValue;
                }
            }
            else if (!strcmp(command, "exit"))
            {
                ExitProcess(0);
            }
            else if (!strcmp(command, "status"))
            {
                std::cout << "Setpoints ------------------------------ " << std::endl;
                std::cout << "\t- setpointTemperatura: " << setpointTemperatura << std::endl;
                std::cout << "\t- setpointOxigenio: " << setpointOxigenio << std::endl;
                std::cout << "\t- setpointPhMin: " << setpointPhMin << std::endl;
                std::cout << "\t- setpointPhMax: " << setpointPhMax << std::endl;
                std::cout << std::endl;

                std::cout << "Sensors -------------------------------- " << std::endl;
                std::cout << "\t- sensorTemperatura: " << sensorTemperatura << std::endl;
                std::cout << "\t- sensorOxigenio: " << sensorOxigenio << std::endl;
                std::cout << "\t- sensorPh: " << sensorPh << std::endl;
                std::cout << "\t- sensorNivel: " << sensorNivel << std::endl;
                std::cout << "\t   -> 1 - Dentro dos limites " << std::endl;
                std::cout << "\t   -> 2 - Abaixo do limite minimo" << std::endl;
                std::cout << "\t   -> 3 - Acima do limite maximo" << std::endl;
                std::cout << std::endl;

                std::cout << "Atuadores ------------------------------ " << std::endl;
                std::cout << "\t   -> 0 - Atuador desativado " << std::endl;
                std::cout << "\t   -> 1 - Atuador ativado" << std::endl;
                std::cout << "\t   ----------------------------- " << std::endl;
                std::cout << "\t- AtuadorAquecedor : " << AtuadorAquecedor << std::endl;
                std::cout << "\t- AtuadorResfriador : " << AtuadorResfriador << std::endl;
                std::cout << "\t- AtuadorFiltro : " << AtuadorFiltro << std::endl;
                std::cout << "\t- AtuadorValvulaEncher : " << AtuadorValvulaEncher << std::endl;
                std::cout << "\t- AtuadorValvulaEsvaziar : " << AtuadorValvulaEsvaziar << std::endl;
                std::cout << "\t- AtuadorAumentaPh : " << AtuadorAumentaPh << std::endl;
                std::cout << "\t- AtuadorReduzPh  : " << AtuadorReduzPh << std::endl;
                std::cout << std::endl;
            }
            else if (!strcmp(command, "clear"))
            {
                system("cls");
            }
            else if (!strcmp(command, "help"))
            {
                std::cout << "Defined commands:" << std::endl;
                std::cout << "\texit - exit the program." << std::endl;
                std::cout << "\tstatus - show selected variables. You need to change this function to show the desired variables." << std::endl;
                std::cout << "\tclear - clear the screen." << std::endl;
                std::cout << "\tset - set a new value for a defined variable. Syntax: set <VARIABLE_NAME>=<VALUE>" << std::endl;
            }
            else
            {
                std::cout << "Command not found!" << std::endl;
            }


            std::cout << ">>>";

            memset(command, 0, commandSize);
            flagExecuteCommand = 0;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::cout << "Exit executeCommand!" << std::endl;
}

// Sensores ***************************************************************
/*  Função que simula o funcionamento dos sensores de:
    1 - Temperatura
    2 - Oxigênio
    3 - Ph
    4 - Nível de água                                   */                    

void Sensores() {
    while (true) {
        mutexHandler.lock();
        // -> Temperatura *********************************************************
        // A variável recebe um número gerado aleatoriamente entre 15 e 35 (°C)
        sensorTemperatura = rand() % 21 + 15;

        // -> Oxigênio ************************************************************
        // A variável recebe um número gerado aleatoriamente entre 5 e 10 (mg/l)
        sensorOxigenio = rand() % 6 + 5;

        // -> Ph ******************************************************************
        // A variável recebe um número gerado aleatoriamente entre 2 e 8
        sensorPh = rand() % 7 + 2;

        // -> Nível de água *******************************************************
        // A variável recebe 0 ou 1, número gerado aleatoriamente
        // Se nivel_alto = 1, indica que o nível máximo de água foi atingido
        int nivel_alto = rand() % 2;

        if (nivel_alto == 0) {
            // A variável recebe 0 ou 1, número gerado aleatoriamente
            // Se nivel_baixo = 1, indica que o nível mínimo de água foi atingido
            int nivel_baixo = rand() % 2;

            if (nivel_baixo == 1) {
                // nivel_alto = 0 e nivel_baixo = 1
                // A água está dentro do nível estabelecido
                sensorNivel = 1;
            }
            else {
                // nivel_alto = 0 e nivel_baixo = 0
                // A água está abaixo do nível estabelecido
                sensorNivel = 2;
            }
        }
        else {
            // nivel_alto = 1
            // A água está acima do nível estabelecido
            sensorNivel = 3;
        }
        mutexHandler.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(20));
    }
    std::cout << "Exit Sensores!" << std::endl;
}

// Processo ***************************************************************

void Processo() {
    while (true) {
        mutexHandler.lock();
        int tempo_regulagem_temperatura = 0;
        int tempo_regulagem_oxigenio = 0;
        // -> Temperatura *********************************************************
        if (AtuadorAquecedor) {
            // Diferença entre temperatura medida e setpoint
            int diferenca_temp = setpointTemperatura - sensorTemperatura;
            //Contador do tempo
            int cont_tempo = 0;
            //Contador do temperatura
            int cont_temperatura = diferenca_temp;

            do
            {
                // Considerando que em 5 min (simulando com 5s) o aquecedor pode aumentar a temperatura numa faixa de 2 - 6 °C
                int variacao = rand() % 5 + 2;

                cont_temperatura = cont_temperatura - variacao;

                cont_tempo++;
                
            } while (cont_temperatura > 0);
            
            // Tempo para atingir temperatura de setpoint
            tempo_regulagem_temperatura = cont_tempo * 5;

            // Pausa a execução da Thread durante o tempo de regulagem
            std::this_thread::sleep_for(std::chrono::seconds(tempo_regulagem_temperatura));
         
            // Valor da temperatura após tempo de regulagem
            sensorTemperatura = sensorTemperatura + diferenca_temp;

            // Nova temperatura exibida
            std::cout << "-> Temperatura: " << sensorTemperatura << std::endl;
        }
        if (AtuadorResfriador) {
            // Diferença entre temperatura medida e setpoint
            int diferenca_temp = sensorTemperatura - setpointTemperatura;
            //Contador do tempo
            int cont_tempo = 0;
            //Contador do temperatura
            int cont_temperatura = diferenca_temp;

            do
            {
                //Considerando que em 5 min (simulando com 5s) o resfriador pode diminuir a temperatura numa faixa de 2 - 4 °C
                int variacao = rand() % 3 + 2;

                cont_temperatura = cont_temperatura - variacao;

                cont_tempo++;

            } while (cont_temperatura > 0);

            // Tempo para atingir temperatura de setpoint
            int tempo_regulagem = cont_tempo * 5;

            // Pausa a execução da Thread durante o tempo de regulagem
            std::this_thread::sleep_for(std::chrono::seconds(tempo_regulagem));

            // Valor da temperatura após tempo de regulagem
            sensorTemperatura = sensorTemperatura - diferenca_temp;

            // Nova temperatura exibida
            std::cout << "-> Temperatura: " << sensorTemperatura << std::endl;
        }

        // -> Oxigênio ************************************************************
        if (AtuadorFiltro) {
            // Diferença entre níveis de oxigênio medido e setpoint
            int diferenca_oxigenio = setpointOxigenio - sensorOxigenio;
            //Contador do tempo
            int cont_tempo = 0;
            //Contador do nível de oxigênio
            int cont_oxigenio = diferenca_oxigenio;

            do
            {
                // Considerando que em 5 min (simulando com 5s) o filtro pode aumentar o nível de oxigênio na água numa faixa de 1 - 2 (mg/l)
                double variacao = rand() % 2 + 1;

                cont_oxigenio = cont_oxigenio - variacao;

                cont_tempo++;

            } while (cont_oxigenio > 0);

            // Tempo para atingir o nível de oxigênio do setpoint
            tempo_regulagem_oxigenio = cont_tempo * 5 - tempo_regulagem_temperatura;

            // Se o tempo de regulagem da temperatura não for suficiente para regular o oxigênio, tempo_regulagem_oxigenio será maior que zero
            if (tempo_regulagem_oxigenio > 0) {
                // Pausa a execução da Thread durante o tempo de regulagem
                std::this_thread::sleep_for(std::chrono::seconds(tempo_regulagem_oxigenio));
            }

            // Valor do nível de oxigênio após tempo de regulagem
            sensorOxigenio = sensorOxigenio + diferenca_oxigenio;

            // Novo nível de oxigênio exibido
            std::cout << "-> Oxigenio: " << sensorOxigenio << std::endl;
        }

        // -> Ph ******************************************************************
        if (AtuadorAumentaPh) {
            // Determinando o déficit de ph no aquário
            double deficit_ph = setpointPhMin - sensorPh;

            // Se a cada 2 ml de reagente o ph aumenta em 0.5
            double reagente = (2 * deficit_ph) / 0.5;

            // Exibe quanto do reagente deve ser adicionado para regular o Ph
            std::cout << "-> Adicionando Reagente: " << reagente << "ml" << std::endl;
        }
        if (AtuadorReduzPh) {
            // Determiando o superávit de ph no aquário
            double superavit_ph = sensorPh - setpointPhMax;

            // Se a cada 3 ml de reagente o ph reduz em 0.75
            double reagente = (3 * superavit_ph) / 0.75;

            // Exibe quanto do reagente deve ser adicionado para regular o Ph
            std::cout << "-> Adicionando Reagente: " << reagente << "ml" << std::endl;
        }

        // -> Nível de água *******************************************************
        if (AtuadorValvulaEsvaziar) {
            std::cout << "-> Ezvaziando aquario até atingir o sensor alto " << std::endl;
        }
        if (AtuadorValvulaEncher) {
            std::cout << "-> Enchendo aquario até atingir o sensor baixo " << std::endl;
        }
        mutexHandler.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(25));
    }
    std::cout << "Exit Processo!" << std::endl;
}

// Controladores **********************************************************

void ControladorCliente() {
    while (true) {
        std::cout << "Executando envio de dados" << std::endl;

        mutexHandler.lock();

        std::string dados_envio = formataDados();

        std::cout << "mensagem:" << dados_envio << std::endl;
        std::cout << "mensagem:" << dados_envio.size() << std::endl;

        // Envia os valores de setpoint e dos sensores para ControladorServer 
        enviaDados(dados_envio);

        mutexHandler.unlock();
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    std::cout << "Exit ControladorCliente!" << std::endl;
}

// Main ********************************************************************
int main()
{
    char command[COMMAND_BUFF_SIZE] = "";

    appSetup();

    // Criação das Threads do sistema
    std::thread readCommandThread(readCommand, command);
    std::thread execCommandThread(executeCommand, command, COMMAND_BUFF_SIZE);
    std::thread SensoresThread(Sensores);
    std::thread ProcessoThread(Processo);
    std::thread ControladorClienteThread(ControladorCliente);
    
    // Execução das Threads
    readCommandThread.join();
    execCommandThread.join();
    SensoresThread.join();
    ProcessoThread.join();
    ControladorClienteThread.join();

    appExit();
}

// Functions ***************************************************************

void ErrorExit(LPSTR lpszMessage)
{
    fprintf(stderr, "%s\n", lpszMessage);

    // Restore input mode on exit.
    SetConsoleMode(hStdin, fdwSaveOldMode);

    ExitProcess(0);
}

void appSetup(void)
{
    // Get the standard input handle.
    hStdin = GetStdHandle(STD_INPUT_HANDLE);
    if (hStdin == INVALID_HANDLE_VALUE)
    {
        ErrorExit((LPSTR)"GetStdHandle");
    }

    // Save the current input mode, to be restored on exit.
    if (!GetConsoleMode(hStdin, &fdwSaveOldMode))
    {
        ErrorExit((LPSTR)"GetConsoleMode");
    }

    std::cout << ">>> ";
}

void appExit(void)
{
    // Restore input mode on exit.
    SetConsoleMode(hStdin, fdwSaveOldMode);

    ExitProcess(0);
}