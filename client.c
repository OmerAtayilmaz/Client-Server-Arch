
// Gerekli Kütüphanelerin Eklenmesi:

#include <stdio.h> // standart giriş/çıkış kütüphanesini printf ve fprintf
#include <winsock2.h>  // Windows üzerinde soket programlaması için gerekli fonksiyonları  SOCKET, bind, listen, accept, vb.
#include <ws2tcpip.h>  //  gerekli olan TCP/IP fonksiyonlarını Kullanıldığı yer: inet_addr fonksiyonu
#include <windows.h>  //Windows API işlemleri ve işletim sistemi ile ilgili fonksiyonları içerir. Kullanıldığı yer (CreateThread, HANDLE).
#include <process.h>  // İşlem oluşturma, işlem kontrolü gibi işlemleri içeren kütüphanedir. Kullanıldığı yer: CreateThread fonksiyonu.
#include <time.h>  // Zaman ile ilgili işlemleri içeren kütüphanedir. Kullanıldığı yer:(time, localtime)

// Sabit Değerlerin Tanımlanması:

#define MAX 80
#define SERVER_IP "127.0.0.1"
#define bzero(b, len) (memset((b), '\0', (len)), (void)0)  // Bellek bloğunu temizle, b adlı bellek bloğu len uzunluğunca '/0' yani null olarak doldurulur(temizlenir)
#define PORT 20248  

int crc_table[256];

char log_file_name[100];

void sent(int server, char *sendbuf) {   // sunucuya veri göndermek için kullanılan fonksiyon (soket, mesaj)
    send(server, sendbuf, (int)strlen(sendbuf), 0);
    return;
}

void crc_init(int polynomial) {  // crc tablosunu başlatır
    int i = 0;
    int j = 0;
    for (i; i < 256; i++) {  // bir byte ın olası tüm değerleri
        int value = i;
        for (j; j < 8; j++) {
            if (value & 1) {   // value tek sayı ise 
                value = (value >> 1) ^ polynomial;  // value değeri 1 sağa kaydırılır ve ploynomial ile xor işlemi yapılır
            } else {  // çift ise sadece 1 sağa kaydırılır
                value >>= 1;
            }
        }
        crc_table[i] = value;  // hesaplanan value değeri crc tablosuna eklenir
    }
}

int crc(int *data, size_t len) {  // data kontrol yapılacak verinin başlangıcı, len ise uzunluğudur
    int i = 0;
    int crc = 0;
    for (i; i < len; i++) {
        crc = crc_table[crc ^ data[i]];  // crc değeri ile kontol edilecek verinin ilgili indexindeki değer xor edilir ve tablodan ilgili değer alınır, crc güncellenir
    }
    return crc;
}

int crc_stob(char *str) {  // bir CRC değeri hesaplar ve bu değeri geri döndürür
    int len = strlen(str);  // Verinin uzunluğunu hesapla
    int data[len];          // Her bir karakteri temsil eden bir dizi oluştur
    int crc_last;           // Hesaplanan CRC değerini tutan değişken
    int i;

    for (i; i < len; i++) {
        data[i] = str[i];   // Her karakteri ilgili dizi elemanına ata
    }

    crc_init(0x8C);         // CRC tablosunu başlat

    crc_last = crc(data, len);  // CRC kontrolünü gerçekleştir

    return crc_last;        // Hesaplanan CRC değerini döndür
}


int checksum(char *data) {  // *data, parametre ile gelen karakter dizisi
    int sum = 0;            // Toplamı saklamak için bir değişken
    int len = strlen(data);  // Verinin uzunluğunu hesapla

    for (int i = 0; i < len; i++) {
        sum += data[i];      // Her karakterin ASCII değerini topla
    }

    return sum;              // Hesaplanan toplamı döndür
}


void addlogs(char *message) {
    FILE *log_file = fopen(log_file_name, "a+");  // Log dosyasını "a+" modunda aç  ("a+" - "append and read/write")
    if (log_file == NULL) {
        perror("Log dosyası açılırken hata oluştu");  // Dosya açma hatası kontrolü
    }

    fprintf(log_file, "%s\n", message);  // Mesajı log dosyasına yaz

    fclose(log_file);  // Dosyayı kapat
}


DWORD WINAPI threadvoid(void *lpParam) {
    char logbuf[MAX];       // Loglama için kullanılacak karakter dizisi
    char recvbuf[MAX];      // Alınan veriyi saklamak için karakter dizisi
    int status;             // recv fonksiyonunun dönüş değeri
    int conn = *((int *)lpParam);  // Soket bağlantısını temsil eden değişken
    char *userdiv, *userdiv1, *userdiv2;  // Veriyi parçalamak için kullanılacak karakter dizileri
    char *divide = "|";     // Veri parçalama işlemi için kullanılacak ayırıcı karakter
    while (1) {
        while ((status = recv(conn, recvbuf, sizeof(recvbuf), 0) > 0)) {  // eğer veri alınıyorsa
            userdiv = strtok(recvbuf, divide);    // Alınan veriyi parçala
            userdiv1 = strtok(NULL, divide);  // daha önce başladığı parçalama işlemine devam eder
            userdiv2 = strtok(NULL, divide);

            if (userdiv1 == NULL && userdiv2 == NULL) {
                printf("%s \n", userdiv);  // Eğer sadece bir veri varsa ekrana yaz
                bzero(userdiv, MAX);  // userdiv karakter dizisini temizle
            } else if (userdiv2 == NULL) {
                printf("%s:%s \n", userdiv1, userdiv);  // İki veri varsa ekrana yaz
                bzero(userdiv, MAX);  // userdiv karakter dizisini temizle
            } else {
                if (checksum(userdiv) == atoi(userdiv1)) {  // Veri bütünlüğünü kontrol et, atoi(userdiv1): userdiv1 karakter dizisini tamsayıya dönüştürür
                    strcat(logbuf, userdiv2);  // Log verisini oluştur
                    strcat(logbuf, ":");
                    strcat(logbuf, userdiv);
                    addlogs(logbuf);  // Log verisini dosyaya ekle
                    bzero(logbuf, MAX);  // logbuf karakter dizisini temizle
                    printf("%s: %s   --doğru veri \n", userdiv2, userdiv);
                } else {
                    sent(conn, "merr");  // Hatalı veri durumunda istemciye hata mesajı gönder
                }
                bzero(userdiv, MAX);  // userdiv karakter dizisini temizle
            }
        }
    }
    bzero(userdiv1, MAX);  // userdiv1 karakter dizisini temizle
    bzero(userdiv, MAX);   // userdiv karakter dizisini temizle
    bzero(recvbuf, MAX);   // recvbuf karakter dizisini temizle
    closesocket(conn);      // Soketi kapat
    return 0;               // İş parçacığını sonlandır
}
 

int main() {
    SOCKET server;
    WSADATA wsaData;


    // Zaman bilgisinin alınması
    time_t current_time = time(NULL);
    struct tm *tm = localtime(&current_time);

    char sendbufname[MAX];  // Kullanıcı adını ve log dosyası adını tutan karakter dizisi
    char sendbuf[MAX];      // Kullanıcı tarafından girilen mesajları tutan karakter dizisi
    char sendbufa[MAX];     // İstemci tarafından sunucuya gönderilen mesajları tutan karakter dizisi
    char sendbufb[MAX];     // İstemci tarafından girilen mesajın işlenmiş hali
    char recvbuf[512];      // Sunucudan alınan mesajları tutan karakter dizisi

    struct sockaddr_in local;
    int wsaret = WSAStartup(0x101, &wsaData);
    
    // WSAStartup fonksiyonu başarısız olursa program sonlandırılır
    if (wsaret != 0) {  // WSA, "Windows Sockets API
        perror("WSA başlatma hatası");
        return 0;
    }
local.sin_family = AF_INET;                      // Ağ ailesi IPv4 olarak belirlenir.
local.sin_addr.s_addr = inet_addr(SERVER_IP);    // Sunucunun IP adresi atanır (inet_addr fonksiyonu ile string IP adresi, struct sockaddr_in'in s_addr üyesine dönüştürülür).
local.sin_port = htons((u_short)PORT);           // Port numarası atanır ve htons fonksiyonu ile uygun ağ byte sırasına dönüştürülür.


    // Soket oluşturma
    server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Bağlantı yapılacak sunucuya bağlanma işlemi
    int *conn1;
    conn1 = &server;

    int *conn;

    conn = malloc(sizeof(int));
    *conn = connect(server, (struct sockaddr *)&local, sizeof(local));

    // Sunucu ile iletişim için yeni bir iş parçacığı oluşturma 
    DWORD myThreadID;
    HANDLE thread;
    thread = CreateThread(
        NULL,
        0,
        threadvoid,
        conn1,
        0,
        &myThreadID);

    char *mesgdiv, *mesgdiv1, *mesg;       // Mesaj bölme için kullanılan değişkenler
    char *mesgdiver = "->";                // Mesaj bölme karakteri
    char *userdiv, *userdiv1;              // Kullanıcı adı bölme için kullanılan değişkenler
    char *divide = "|";                    // Genel bölme karakteri
    char c_sum[MAX];                       // Checksum değerini tutan karakter dizisi
    boolean regist = FALSE;                // Kullanıcının kayıtlı olup olmadığını tutan değişken
    
    printf("Sunucuya bağlanmak için kullanıcı adını girin \n");

    while (1) {
        if (regist == FALSE) {
            // Yeni bir kullanıcı adı alındığında gerekli işlemleri gerçekleştir
            bzero(sendbufa, MAX);
            fgets(sendbuf, sizeof sendbuf, stdin);
            strcpy(sendbufname, sendbuf);
            sendbufname[strcspn(sendbufname, "\n")] = 0;
            
            // Log dosyasının adını oluştur
            sprintf(log_file_name, "logs/%d-%d-%d_%d-%d-%d_%s.txt",
                    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                    tm->tm_hour, tm->tm_min, tm->tm_sec, sendbufname);

            // Sunucuya bağlanma mesajını oluştur
            strcat(sendbufa, "conn|");
            strcat(sendbufa, "0");
            strcat(sendbufa, "|");
            strcat(sendbufa, sendbuf);

            // Kullanıcı bilgilerini ekrana yazdır
            printf("Online kullanıcılar:\n", sendbufa);
            
            // Kullanıcının kayıtlı olduğunu işaretle ve sunucuya bağlanma mesajını gönder
            regist = TRUE;
            sent(server, sendbufa);
        } else {
            // Kullanıcı kayıtlıysa, mesaj gönderme işlemini gerçekleştir
            fgets(sendbuf, sizeof sendbuf, stdin);
            
            // Kullanıcı "quit" komutunu girdiyse bağlantıyı sonlandır
            if (!strcmp(sendbuf, "quit\n")) {
                sent(server, "gone");
            } else {
                bzero(sendbufa, MAX);
                bzero(sendbufb, MAX);
                bzero(c_sum, MAX);

                // Mesajın içeriğini kontrol et ve checksum değerini oluştur
                strcpy(sendbufb, sendbuf);
                mesgdiv = strtok(sendbufb, mesgdiver);
                mesgdiv1 = strtok(NULL, mesgdiver);
                int len2 = strlen(mesgdiv1);
                
                // Eğer mesajın sonunda newline karakteri varsa, onu kaldır
                if (mesgdiv1[len2 - 1] == '\n') {
                    mesgdiv1[len2 - 1] = '\0';
                }
                
                // Checksum değerini hesapla
                sprintf(c_sum, "%d", checksum(mesgdiv1));
                
                // Sunucuya gönderilecek mesajı oluştur
                strcat(sendbufa, "mesg|");
                strcat(sendbufa, c_sum);
                strcat(sendbufa, "|");
                strcat(sendbufa, sendbuf);
                
                // Log dosyasına gönderilen mesajı ekle
                strcpy(sendbufname, sendbuf);
                sendbufname[strcspn(sendbuf, "\n")] = 0;
                printf("gönderilen\n");
                addlogs(sendbufname);
                
                // Sunucuya mesajı gönder
                sent(server, sendbufa);
            }
        }
    }

    // Soketi kapat ve Winsock kütüphanesini temizle    NOTT: HOCANIN SON LAB DA ANLATTIĞI PROJE DETALARINI DA GÖZDEN GEÇİR
    closesocket(server);
    WSACleanup();
    return 0;
}

/*
CRC ALGORİTMASI GENEL MANTIK

Temelde veri dizilerinin(binary sayılar) polinom şeklinde ifade edilip polinomsal bölme ve mod2 aritmetiği(burada sadece XOR) uygulanarak hata tespiti yapılmasıdır

G(x) üreteç fonksiyonu önceden alıcı ve verici arasında paylaşılmış olmalıdır

örneğin; veri dizisi 1 0 1 1 0 1 0 1 0 1 olsun,
bu veri dizisinin uzunluğu K = 10 olduğu görülmektedir o halde veri dizisinini temsil eden polinomun derecesi k-1 = 9 olacaktır 

bu veri dizisini temsil eden polinom  P(x) = 1.x9 +0.x8 +1.x7 +1.x6 +0.x5 +1.x4 +0.x3 +1.x2 +0.x1 +1.x0  =  x9 + x7 + x6 + x4 + x2 + 1 olacaktır

G(x) = x4 + x2 + x +1 olarak seçilsin; belli başlı G(x) fonksiyonları vardır    

Bilinen bazı G(x) polinomlar
○ G(x) = x16 + x15 + x2 + 1
○ G(x) = x16 + x12 + x5 + 1
○ G(x) = x12 + x11 + x3 + x2 + x + 1

P(x) polinomu önce x^(n-k) ile çarpılır buradaki n değeri P(x) ve G(x) polinomlarının dereceleri toplamıdır 

aslında olay şu, veri dizisine seçilen g(x) polinomu derecesi kadar hata biti(0) ekleniyor ve bu oluşan yeni, genişletirlmiş veri dizisi p(x).x(n-k)
ile ifade ediliyor örnekte seçilen g(x) polinomunun derecesi 4 yani 10 bitlik veriye 4 bitlik hata biti eklenir
1 0 1 1 0 1 0 1 0 1 -> 1 0 1 1 0 1 0 1 0 1 0 0 0 0 a dönüşmüş oldu

oluşan yeni veri dizisi G(x) polinomuna bölünür => X4.P(x) / G(x) polinomsal bölme işleminden sonra kalan R(X) = x3 + x2 (+ 0*x1 + 0*1 ! hata bitlerinin uzunluğu
üreteç polinomunun derecesi kadar olmalı burada binary olarak R(x) = 1100 a tekabül eder) polinomu orjinal veriye eklenir ve gönderilmeye hazır 
hale gelir orjinal veri + R(x) bizim mesajmız olacak : 1 0 1 1 0 1 0 1 0 1 1 1 0 0 

Alıcı, veri dizisini alır ve G(x) polinomuna böler eğer kalan 0 ise mesaj hatasızdır değil ise hatalıdır mevzu bu

! Xn-k.P(x) = Q(x). G(x)+ R(x) burada niyeyse mod2 aritmetği (toplamada XOR) kullanılmıştır ve denklem şuna döner           Xn-k.P(x) + R(x) = Q(x). G(x)
!!!! 'veriyi genişletip G(X) e böldük ve kalanı hata biti olarak veri dizisine ekledik ve sonuç G(x) polinomunun katı çıktı denkleme bak ^|^ , 
alıcı tekrar G(x) e böldüğünde kalan 0 olacaktır farklı bir değer elde ederse gönderilen verinin bozulduğu anlaşılır. anlamayan bana yazsın karışık biraz evet'!!!!



*/
