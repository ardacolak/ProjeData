#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Winsock kütüphanesi

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_GAMES 10
#define MAX_CLIENTS 5  // Maksimum kabul edilecek istemci sayýsý

struct Game {
    char name[50];
    float size;           // GB
    int download_time;    // Saniye
    int remaining_time;   // Kalan süre
    int is_downloading;   // 1: indiriliyor, 0: durduruldu
};

struct Client {
    SOCKET client_socket;
    struct Game games[MAX_GAMES]; // Her client için ayrý oyun verisi
};

struct DownloadArgs {
    int client_id;
    int game_id;
};

// Globaldeki client veri yapýsý
struct Client clients[MAX_CLIENTS];

void initialize_games() {
    char *game_names[MAX_GAMES] = {"Valorant", "Cs2", "ETS2", "Payday2", "Age of Empires", "Rust", "Sims4", "Cities: Skylines", "Rocket League", "LOL"};
    float sizes[MAX_GAMES] = {10.5, 20.0, 15.3, 25.0, 5.8, 18.2, 12.5, 22.0, 7.8, 30.0};
    int times[MAX_GAMES] = {105, 200, 153, 250, 58, 182, 125, 220, 78, 300};

    for (int i = 0; i < MAX_GAMES; i++) {
        for (int j = 0; j < MAX_CLIENTS; j++) {
            strcpy(clients[j].games[i].name, game_names[i]);
            clients[j].games[i].size = sizes[i];
            clients[j].games[i].download_time = times[i];
            clients[j].games[i].remaining_time = times[i];
            clients[j].games[i].is_downloading = 0;
        }
    }
}

DWORD WINAPI start_download(LPVOID arg) {
    struct DownloadArgs *args = (struct DownloadArgs *)arg;
    int client_id = args->client_id;
    int game_id = args->game_id;

    free(arg); // Dinamik belleði serbest býrak

    while (clients[client_id].games[game_id].remaining_time > 0 && clients[client_id].games[game_id].is_downloading) {
        Sleep(1000); // 1 saniye bekle
        clients[client_id].games[game_id].remaining_time--;
    }
    return 0;
}

void handle_client(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    int game_id;
    int client_id = -1;

    // Client'in hangi id'ye sahip olduðunu bul
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].client_socket == client_socket) {
            client_id = i;
            break;
        }
    }

    if (client_id == -1) {
        printf("Client bulunamadi!\n");
        return;
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        recv(client_socket, buffer, BUFFER_SIZE, 0);

        if (strncmp(buffer, "list", 4) == 0) {
            char game_list[BUFFER_SIZE] = "";
            for (int i = 0; i < MAX_GAMES; i++) {
                char game_info[100];
                sprintf(game_info, "%d. %s (%.1f GB) - Kalan Sure: %d saniye\n",
                        i + 1, clients[client_id].games[i].name, clients[client_id].games[i].size, clients[client_id].games[i].remaining_time);
                strcat(game_list, game_info);
            }
            send(client_socket, game_list, strlen(game_list), 0);
        } else if (sscanf(buffer, "download %d", &game_id) == 1) {
            game_id--;
            if (game_id >= 0 && game_id < MAX_GAMES) {
                clients[client_id].games[game_id].is_downloading = 1;

                // Ýndirmenin argümanlarýný hazýrla ve iþ parçacýðýný baþlat
                struct DownloadArgs *args = (struct DownloadArgs *)malloc(sizeof(struct DownloadArgs));
                args->client_id = client_id;
                args->game_id = game_id;

                DWORD thread_id;
                HANDLE download_thread = CreateThread(NULL, 0, start_download, args, 0, &thread_id);
                send(client_socket, "Indirme basladi!\n", 17, 0);
            }
        } else if (sscanf(buffer, "pause %d", &game_id) == 1) {
            game_id--;
            if (game_id >= 0 && game_id < MAX_GAMES) {
                clients[client_id].games[game_id].is_downloading = 0;
                send(client_socket, "Indirme durduruldu!\n", 21, 0);
            }
        } else if (sscanf(buffer, "cancel %d", &game_id) == 1) {
            game_id--;
            if (game_id >= 0 && game_id < MAX_GAMES) {
                clients[client_id].games[game_id].is_downloading = 0;
                clients[client_id].games[game_id].remaining_time = clients[client_id].games[game_id].download_time;
                send(client_socket, "Indirme iptal edildi!\n", 22, 0);
            }
        } else {
            send(client_socket, "Gecersiz komut!\n", 17, 0);
        }
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    int addrlen = sizeof(client_address);
    int client_count = 0;

    printf("Winsock baslatiliyor...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock baslatilamadi. Hata kodu: %d\n", WSAGetLastError());
        return 1;
    }

    initialize_games();

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Soket olusturulamadi. Hata kodu: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == SOCKET_ERROR) {
        printf("Soket baglanamadi. Hata kodu: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    if (listen(server_socket, 3) == SOCKET_ERROR) {
        printf("Dinleme basarisiz. Hata kodu: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }
    printf("Sunucu %d portunda calisiyor...\n", PORT);

    while (1) {
        if ((client_socket = accept(server_socket, (struct sockaddr *)&client_address, &addrlen)) == INVALID_SOCKET) {
            int error_code = WSAGetLastError();
            printf("Istemci kabul edilemedi. Hata kodu: %d\n", error_code);
            continue;
        }

        // Maksimum istemci sayýsýný kontrol et
        if (client_count >= MAX_CLIENTS) {
            printf("Maksimum istemci sayisina ulasildi. Baglanti reddediliyor.\n");
            send(client_socket, "Sunucu dolu. Lutfen daha sonra deneyin.\n", 40, 0);
            closesocket(client_socket);
            continue;
        }

        // Baðlantý baþarýlý ise
        printf("Istemci baglandi.\n");
        clients[client_count].client_socket = client_socket;
        client_count++;

        // Client ile baðlantýyý iþlemek için bir thread baþlat
        DWORD thread_id;
        HANDLE client_thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)handle_client, (LPVOID)client_socket, 0, &thread_id);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}

