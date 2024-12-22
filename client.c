#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

void display_menu() {
    printf("\n=== Oyun Indirme Platformu ===\n");
    printf("1. Oyun listesini goruntule\n");
    printf("2. Oyun indir\n");
    printf("3. Indirmeyi durdur\n");
    printf("4. Indirmeyi devam ettir\n");
    printf("5. Indirmeyi iptal et\n");
    printf("6. Cikis\n");
    printf("Seciminizi yapin: ");
}

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE] = {0};
    char command[BUFFER_SIZE];
    int choice, game_id;

    printf("Winsock baslatiliyor...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Winsock baslatilamadi. Hata kodu: %d\n", WSAGetLastError());
        return 1;
    }

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket olusturulamadi. Hata kodu: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        printf("Sunucuya baglanilamadi. Hata kodu: %d\n", WSAGetLastError());
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Sunucuya basariyla baglanildi.\n");

    while (1) {
        display_menu();
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                strcpy(command, "list");
                send(sock, command, strlen(command), 0);
                recv(sock, buffer, BUFFER_SIZE, 0);
                printf("\nOyun Listesi:\n%s", buffer);
                break;

            case 2:
                printf("Indirmek istediginiz oyunun ID'sini girin (1-10): ");
                scanf("%d", &game_id);
                sprintf(command, "download %d", game_id);
                send(sock, command, strlen(command), 0);
                recv(sock, buffer, BUFFER_SIZE, 0);
                printf("\nSunucu: %s", buffer);
                break;

            case 3:
                printf("Durdurmak istediginiz oyunun ID'sini girin (1-10): ");
                scanf("%d", &game_id);
                sprintf(command, "pause %d", game_id);
                send(sock, command, strlen(command), 0);
                recv(sock, buffer, BUFFER_SIZE, 0);
                printf("\nSunucu: %s", buffer);
                break;

            case 4:
                printf("Devam ettirmek istediginiz oyunun ID'sini girin (1-10): ");
                scanf("%d", &game_id);
                sprintf(command, "download %d", game_id);
                send(sock, command, strlen(command), 0);
                recv(sock, buffer, BUFFER_SIZE, 0);
                printf("\nSunucu: %s", buffer);
                break;

            case 5:
                printf("Iptal etmek istediginiz oyunun ID'sini girin (1-10): ");
                scanf("%d", &game_id);
                sprintf(command, "cancel %d", game_id);
                send(sock, command, strlen(command), 0);
                recv(sock, buffer, BUFFER_SIZE, 0);
                printf("\nSunucu: %s", buffer);
                break;

            case 6:
                printf("Baglanti kapatiliyor...\n");
                closesocket(sock);
                WSACleanup();
                return 0;

            default:
                printf("Gecersiz secim! Tekrar deneyin.\n");
                break;
        }
        memset(buffer, 0, BUFFER_SIZE);
    }

    return 0;
}

