#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <time.h>
#define MAX 80
#define SERVER_IP "127.0.0.1"
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)  
#define PORT 20248


int crc_table[256];

char log_file_name[100];

void sent(int server,char *sendbuf)//send server 
{
    send( server, sendbuf, (int)strlen(sendbuf), 0 );
    return;
}


void crc_init(int polynomial) //initialize a lookup table for a cyclic redundancy check (CRC)
{
  int i = 0;
  int j = 0;
  for (i; i < 256; i++) {
    int value = i;
    for (j; j < 8; j++) {
      if (value & 1) {
        value = (value >> 1) ^ polynomial;
      } 
      else {
        value >>= 1;
      }
    }
  crc_table[i] = value;
  }
}
int crc(int *data, size_t len)
{
	int i = 0;
  int crc = 0;
  for (i; i < len; i++) {
    crc = crc_table[crc ^ data[i]];
  }
  return crc;
}
int crc_stob(char *str){  //crc Convert the string to a byte array and return crc value
	 int len = strlen(str);
  int data[len];
  int crc_last;
	int i;
  for (i; i < len; i++) {
    data[i] = str[i];
  }
  crc_init(0x8C);
  crc_last = crc(data, len);
	return crc_last;
}

int checksum(char *data) {
  int sum = 0;
  int len = strlen(data);
  for (int i = 0; i < len; i++) {
    sum += data[i];
  }
  return sum;
}
void addlogs(char *message)  //write logs/date_username.txt
{
  FILE *log_file = fopen(log_file_name, "a+");
  if (log_file == NULL)
    {
      perror("Error opening log file");
    }
    fprintf(log_file,"%s\n",message);
    fclose(log_file);
}
DWORD WINAPI threadvoid(void *lpParam)
    {   
      char logbuf[MAX]; 
      char recvbuf[MAX]; 
      int status;
      int conn = *((int *)lpParam);
      char *userdiv,*userdiv1,*userdiv2;
      char *divide = "|";
      while(1){
        while((status=recv(conn, recvbuf, sizeof(recvbuf), 0)>0)){
          userdiv = strtok(recvbuf, divide); //what recv
          userdiv1 = strtok(NULL, divide);  //checksumcode if someone sent
          userdiv2 = strtok(NULL, divide);  //who sent message if someone sent
          if(userdiv1==NULL && userdiv2==NULL){
            printf ("%s \n",userdiv);
            bzero(userdiv, MAX);
          }
          else if(userdiv2==NULL){//new join 
            printf ("%s:%s \n",userdiv1,userdiv);
            bzero(userdiv, MAX);
          }
          else{
            //if(crc_stob(userdiv)==atoi(userdiv1)) {   **if we want crc error detection we can choose this**
            if(checksum(userdiv)==atoi(userdiv1)){//true data recv
              strcat(logbuf,userdiv2);
              strcat(logbuf,":");
              strcat(logbuf,userdiv);
              addlogs(logbuf);
              bzero(logbuf,MAX);
              printf ("%s: %s   --true data \n",userdiv2,userdiv);
              }
            else{//false data recv
            sent(conn,"merr");//send merr commands ands take without error 
            //  printf ("%s: %s csum:%s test:%d\n",userdiv2,userdiv,userdiv1,checksum(userdiv));
            //  printf ("error data send merr code");
            }
          
          bzero(userdiv, MAX);
          }
         }
        }
    bzero(userdiv1, MAX);
    bzero(userdiv, MAX);
    bzero(recvbuf, MAX);
    closesocket(conn);
  }

int main()
{
    SOCKET server;
    WSADATA wsaData;
 
  
    time_t current_time = time(NULL);
    struct tm *tm = localtime(&current_time);
    char sendbufname[MAX];
    char sendbuf[MAX];
    char sendbufa[MAX];
    char sendbufb[MAX];
    char recvbuf[512];

    struct sockaddr_in local;
    int wsaret=WSAStartup(0x101,&wsaData);
    if(wsaret!=0)
    {
        return 0;
    }

    local.sin_family=AF_INET; //Address family
    local.sin_addr.s_addr= inet_addr("127.0.0.1"); //Wild card IP  address
    local.sin_port=htons((u_short)20248); //port to use

    server=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

    int *conn1;
    conn1=&server;

    int *conn;
   
    conn=malloc(sizeof(int));
    *conn= connect(server, (struct sockaddr*)&local,sizeof(local));
         
    DWORD myThreadID;
            HANDLE thread ;
            CreateThread( 
                NULL,                   
                0,                     
                threadvoid,       // thread function name
               conn1,          // argument to thread function 
                0,                     
                &myThreadID);   // returns the thread identifier 

    char *mesgdiv,*mesgdiv1,*mesg;
    char *mesgdiver = "->";
    char *userdiv,*userdiv1;
    char *divide = "|";     
    char c_sum[MAX];
    boolean regist =FALSE;
    printf("Enter username for connect server \n");
    while (1)
    {

      if(regist==FALSE) // username and connection
       {
          bzero(sendbufa, MAX);      
          fgets(sendbuf, sizeof sendbuf, stdin);
          strcpy(sendbufname,sendbuf);
          sendbufname[strcspn(sendbufname, "\n")] = 0;   
          sprintf(log_file_name, "logs/%d-%d-%d_%d-%d-%d_%s.txt", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec,sendbufname );

              strcat(sendbufa,"conn|");
              strcat(sendbufa,"0");
              strcat(sendbufa,"|");
              strcat(sendbufa,sendbuf);
              printf("Online users:\n",sendbufa);
              regist=TRUE;
              sent(server,sendbufa);
          }
          else
          {  
              fgets(sendbuf, sizeof sendbuf, stdin);
              if(!strcmp(sendbuf,"quit\n"))
                { 
                sent(server,"gone");   // gone server
                 }  
              else{
      
                bzero(sendbufa, MAX);  
                bzero(sendbufb, MAX);  
                bzero(c_sum, MAX);   
            
                strcpy(sendbufb,sendbuf);
                mesgdiv = strtok(sendbufb, mesgdiver);// div name and message ... -> ....
                mesgdiv1 = strtok(NULL, mesgdiver);
                int len2 = strlen(mesgdiv1);
                if (mesgdiv1[len2-1] == '\n') {
                    mesgdiv1[len2-1] = '\0';
                }
                sprintf(c_sum, "%d", checksum(mesgdiv1));  //compute checksum
              // sprintf(c_sum, "%d", crc_stob(mesgdiv1)); //compute crc
                strcat(sendbufa,"mesg|");
                strcat(sendbufa,c_sum);
                strcat(sendbufa,"|");
                strcat(sendbufa,sendbuf);
                strcpy(sendbufname,sendbuf);
                sendbufname[strcspn(sendbuf, "\n")] = 0;   
                printf("sent\n");
                addlogs(sendbufname);// addlogs
                sent(server,sendbufa);//sent server 
          }
          
      }
    }
  closesocket(server);
  WSACleanup();
  return 0;
}

