#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <ctype.h>

#define MAX 80
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)

// Thread ve soket yönetimi için global değişkenler
DWORD myThreadID;
HANDLE thread;

// Bir istemciyi temsil eden yapı: kullanıcı adı, bağlantı soketi ve bir sonraki işaretçi
struct sockaddr_in from;
struct client {
    char *username;
    int conn;
    struct client *next;
};
struct client *header = NULL;

// Kullanıcıyı bağlı listeye eklemek için fonksiyon
void add_user(struct client *user) {
    if (header == NULL) {
        header = user;
        user->next = NULL;
    } else {
        user->next = header;
        header = user;
    }
}

// Kullanıcıyı bağlı listeden silmek için fonksiyon
void delete_user(struct client *user) {
    struct client *temp = header;
    struct client *prev = NULL;

    while (temp != NULL) {
        if (temp == user) {
            if (prev == NULL) {
                header = temp->next;
            } else {
                prev->next = temp->next;
            }

            free(temp);
            break;
        }

        prev = temp;
        temp = temp->next;
    }
}

// Bir istemciye veri göndermek için fonksiyon
void sent(int server, char *sendbuf) {
    send(server, sendbuf, (int)strlen(sendbuf), 0);
}

// İstemci ile iletişimi yöneten thread fonksiyonu
DWORD WINAPI threadvoid(void *lpParam) {
    // Thread fonksiyonu için yerel değişkenler
    char userinfo[MAX];
    char static temp[MAX];
    char mesgdivfull[MAX];
    char recvbuf[MAX];

    struct client *user;
    struct client *current = user;
    struct client *current1 = user;
    char nconn[10];
    int conn = *((int *)lpParam);
    char str[6] = {'\0'};

    char *userdiv, *userdiv1, *userdiv2;
    char *divide = "|";
    
    while (1) {
        bzero(recvbuf, MAX);
        // İstemciden veri al
        if (recv(conn, recvbuf, sizeof(recvbuf), 0) > 0) {
            // Alınan veriyi "|" ayraçına göre parçala
            userdiv = strtok(recvbuf, divide);
            userdiv2 = strtok(NULL, divide);
            userdiv1 = strtok(NULL, divide);

            // "conn" mesaj türünü işle (yeni kullanıcı bağlantısı)
            if (!strcmp(userdiv, "conn")) {
                int len = strlen(userdiv1);
                if (userdiv1[len - 1] == '\n') {
                    userdiv1[len - 1] = '\0';
                }

                // Yeni bir kullanıcı yapısı oluştur ve bağlı listeye ekle
                user = malloc(sizeof(struct client));
                user->username = malloc(sizeof(userdiv1));
                memcpy(user->username, userdiv1, strlen(userdiv1) + 1);
                user->conn = conn;
                add_user(user);

                // Yeni bağlantı hakkında bilgiyi ekrana yazdır
                printf("Kullanıcı adı:%s IP:%s Bağlantı:%d bağlandı\n", userdiv1, inet_ntoa(from.sin_addr), conn);
                
                // Çevrimiçi kullanıcıların listesini hazırla ve yeni kullanıcıya gönder
                current = header;
                bzero(userinfo, MAX);
                while (current != NULL) {
                    strcat(userinfo, current->username);
                    strcat(userinfo, "\r\n");
                    current = current->next;
                }
                sent(conn, userinfo);
                
                // Diğer çevrimiçi kullanıcılara yeni kullanıcı hakkında bilgi ver
                current = header;
                strcat(userdiv1, "|");
                strcat(userdiv1, "Katıl");
                while (current != NULL) {
                    sent(current->conn, userdiv1);
                    current = current->next;
                }
            } 
            // "mesg" mesaj türünü işle (gelen mesajlar)
            else if (!strcmp(userdiv, "mesg")) {
                // Mesajları işlemek için yerel değişkenler
                char *mesgdiv, *mesgdiv1;
                char *mesgdiver = "->";
                mesgdiv = strtok(userdiv1, mesgdiver);
                mesgdiv1 = strtok(NULL, mesgdiver);
                int len = strlen(mesgdiv1);
                if (mesgdiv1[len - 1] == '\n') {
                    mesgdiv1[len - 1] = '\0';
                }
                int flag = 0, conn_n;
                current = header;
                current1 = header;
                int a = rand() % 6; // Mesajları kontrol etmek için rastgele sayı
                while (current != NULL) {
                    if (!strcmp(mesgdiv, current->username)) {
                        while (current1 != NULL) {
                            if (conn == current1->conn) {
                                if (a % 2 == 0) {
                                    strcat(mesgdivfull, mesgdiv1);
                                } else {
                                    sprintf(nconn, "%d", current->conn);
                                    strcat(temp, nconn);
                                    strcat(temp, "|");
                                    strcat(temp, mesgdiv1);
                                    strcat(temp, "|");
                                    strcat(temp, userdiv2);
                                    strcat(temp, "|");
                                    strcat(temp, current1->username);
                                    strcat(mesgdivfull, "x");
                                    strcat(mesgdivfull, mesgdiv1);
                                }
                                strcat(mesgdivfull, "|");
                                strcat(mesgdivfull, userdiv2);
                                strcat(mesgdivfull, "|");
                                strcat(mesgdivfull, current1->username);
                                sent(current->conn, mesgdivfull);
                                flag = 1;
                                break;
                            } else {
                                current1 = current1->next;
                            }
                        }
                        if (flag == 1) {
                            bzero(mesgdivfull, MAX);
                            break;
                        }
                    }
                    current = current->next;
                }
            } 
            // "merr" mesaj türünü işle (hata mesajları)
            else if (!strcmp(userdiv, "merr")) {
                // Hata mesajlarını işlemek için yerel değişkenler
                char *tempdiv, *tempdiv1, tempdiv2, tempdiv3;
                char *tempdiver = "|";
                tempdiv = strtok(temp, tempdiver);
                tempdiv1 = strtok(NULL, "");
                sent(atoi(tempdiv), tempdiv1);
                bzero(temp, MAX);
            } 
            // "gone" mesaj türünü işle (kullanıcı bağlantısı kesildi)
            else if (!strcmp(userdiv, "gone")) {
                // Kullanıcı bağlantısı kesilirse işlem yap
                char disconnect[20];
                bzero(disconnect, 20);
                current1 = header;
                while (current1 != NULL) {
                    if (current1->conn == conn) {
                        strcat(disconnect, current1->username);
                        printf("Kullanıcı adı:%s IP:%s Bağlantı:%d bağlandı ", current1->username, inet_ntoa(from.sin_addr), conn);
                        delete_user(current1);
                        break;
                    }
                    current1 = current1->next;
                }
                struct client *temp = header;
                strcat(disconnect, "|");
                strcat(disconnect, "Bağlantı Kesildi");

                while (temp != NULL) {
                    sent(temp->conn, disconnect);
                    temp = temp->next;
                }

                DWORD exitCode;
                if (GetExitCodeThread(thread, &exitCode) != 0) {
                    printf("Thread kapatıldı\n");
                    TerminateThread(thread, exitCode);
                }
            } 
            // Bilinmeyen veri türü için hata mesajı gönder
            else {
                sent(conn, "yanlış veri");
            }
        }
    }
}

// Ana fonksiyon
int main() {
    struct client *user = header;
    SOCKET server;
    WSADATA wsaData;
    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons((u_short)20248);
    int wsaret = WSAStartup(0x101, &wsaData);

    if (wsaret != 0) {
        return 0;
    }

    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) {
        return 0;
    }

    if (bind(server, (struct sockaddr *)&local, sizeof(local)) != 0) {
        return 0;
    }

    if (listen(server, 100) != 0) {
        return 0;
    }

    int fromlen = sizeof(from);
    int *conn;

    while (1) {
        conn = malloc(sizeof(int));
        *conn = accept(server, (struct sockaddr *)&from, &fromlen);

        // Yeni bir bağlantıyı yönetmek için yeni bir thread oluştur
        thread = CreateThread(
            NULL,
            0,
            threadvoid,
            conn,
            0,
            &myThreadID);
    }

    closesocket(server);
    WSACleanup();
    return 0;
}

/*
Çalışma Mantığı:
Sunucu başlatılır ve kullanıcı bağlantılarını kabul etmeye başlar.
Her bağlantı için yeni bir iş parçacığı oluşturulur ve bağlantıyı dinlemeye alır.
İstemci, sunucuya bağlanır, kullanıcı adını girdikten sonra sunucuya bağlantı mesajını gönderir.
İstemci ve sunucu arasında iletişim threadvoid fonksiyonları üzerinden gerçekleşir.
Mesajlar belirli bir protokole göre işlenir ve kontrol edilir.
Çevrimiçi kullanıcılar listesi sunucu tarafından güncellenir ve diğer kullanıcılara bildirilir.
İstemci, kullanıcıdan alınan mesajları kontrol eder ve sunucuya gönderir.
Hata kontrolü, CRC algoritması ve basit bir toplam kontrolü uygulanır.
Kullanıcıların bağlantıları sonlandığında sunucu ve diğer kullanıcılara durumu bildirir.
*/
