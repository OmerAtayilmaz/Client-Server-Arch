#include <stdio.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>
#include <ctype.h>
#define MAX 80
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)  

DWORD myThreadID;
HANDLE thread ;

struct sockaddr_in from;
    struct client{
        char *username;
        int conn;
        struct client *next;
    };
struct client *header=NULL;
    
void add_user(struct client *user){
    if(header == NULL){
        header=user;
        user->next=NULL;
    }
    else{
        user->next=header;
        header=user;
    }
}
  
void delete_user(struct client *user){
    struct client *temp = header;
    struct client *prev = NULL;
    while(temp != NULL){
    if(temp == user){
      if(prev == NULL){
        header = temp->next;
      }
      else{
        prev->next = temp->next;
      }
    free(temp);
    break;
    }
    prev = temp;
    temp = temp->next;
  }
}

void sent(int server,char *sendbuf){
    send( server, sendbuf , (int)strlen(sendbuf), 0 );//send data to who receive
}
  
DWORD WINAPI threadvoid(void *lpParam)
    {   
        char userinfo[MAX];
        char static temp[MAX];
        char mesgdivfull[MAX];
        char recvbuf[MAX];

        struct client *user;  
        struct client * current= user;
        struct client * current1= user;
        char nconn[10];
        int conn = *((int *)lpParam);
        char str[6] = {'\0'};
  
        char *userdiv,*userdiv1,*userdiv2;
        char *divide = "|";
        while (1) {
            bzero(recvbuf, MAX);
            if(recv(conn,recvbuf,sizeof(recvbuf),0)>0){
                userdiv = strtok(recvbuf, divide);//what send "merr-mesg-conn-gone"
                userdiv2 = strtok(NULL, divide);//checksum code  
                userdiv1 = strtok(NULL, divide);//body
                if(!strcmp(userdiv,"conn")){
                    int len = strlen(userdiv1);
                    if (userdiv1[len-1] == '\n') {
                        userdiv1[len-1] = '\0';
                    }
                user =malloc(sizeof(struct client)); //creating memory space for username
                user->username=malloc(sizeof(userdiv1));
                memcpy(user->username,userdiv1,strlen(userdiv1)+1);
                user->conn=conn;
                add_user(user);// add user client list
                printf("username:%s ip:%s conn:%d connect\n",userdiv1,inet_ntoa(from.sin_addr),conn);
                current=header;  //reset node to head node
                bzero(userinfo, MAX); 
                while(current!=NULL)//add array online user
                {  
                    strcat(userinfo, current->username);
                    strcat(userinfo, "\r\n");
                    current=current->next;
                }
                sent(conn,userinfo);//sent user list when new user connect server
                current=header;  
                strcat(userdiv1, "|");
                strcat(userdiv1, "Join");
                while(current!=NULL)
                {  
                    sent(current->conn,userdiv1);//sent notification to online users new join
                    current=current->next;
                }
            }
       
        else if(!strcmp(userdiv,"mesg")){//if client send mesg code do those
            char *mesgdiv,*mesgdiv1;
            char *mesgdiver = "->";
            mesgdiv = strtok(userdiv1, mesgdiver); //Who sent the message to
            mesgdiv1 = strtok(NULL, mesgdiver); //message
            int len = strlen(mesgdiv1);
            if (mesgdiv1[len-1] == '\n') {
                mesgdiv1[len-1] = '\0';
            }
            int flag=0,conn_n;
            current=header;
            current1=header;
            int a=rand()%6; // for checksum distrub data
            while(current!=NULL)//find user loop
             {    
                if(!strcmp(mesgdiv,current->username)){  // find which user message
                    while(current1!=NULL){
                        if(conn==current1->conn){   // find which user connection port
                            if(a%2==0){//checksum distrub false
                                strcat(mesgdivfull, mesgdiv1); //send true message
                            }
                            else{//checksum distrub true
                                sprintf(nconn, "%d", current->conn);
                                strcat(temp,nconn);//save data to temp before distrub
                                strcat(temp, "|");
                                strcat(temp,mesgdiv1);
                                strcat(temp, "|");
                                strcat(temp,userdiv2);//checksum
                                strcat(temp, "|");
                                strcat(temp, current1->username);
                                strcat(mesgdivfull, "x");
                                strcat(mesgdivfull, mesgdiv1); //mesage change
                            }
                            strcat(mesgdivfull, "|");
                            strcat(mesgdivfull, userdiv2);//checksum
                            strcat(mesgdivfull, "|");
                            strcat(mesgdivfull, current1->username); //who sent message
                            sent(current->conn,mesgdivfull);
                            flag=1;
                            break;    
                        }
                        else
                        {   
                            current1=current1->next;
                        }
                    }
                    if(flag==1){
                        bzero(mesgdivfull,MAX);
                        break;
                    }
                }
                current=current->next;        
             }    
        }

        else if(!strcmp(userdiv,"merr")){//if user send merr code send last message again
            char *tempdiv,*tempdiv1,tempdiv2,tempdiv3;
            char *tempdiver = "|";  
            tempdiv = strtok(temp, tempdiver); // conn number
            tempdiv1 = strtok(NULL, ""); //who sent|checksum|mesage
            sent(atoi(tempdiv),tempdiv1);
            bzero(temp,MAX);                                  
        }
        
        else if(!strcmp(userdiv,"gone")){
            char disconnect[20];
            bzero(disconnect,20);     
            current1=header;
            while(current1!=NULL){
                if(current1->conn==conn){
                    strcat(disconnect, current1->username);
                    printf("username:%s ip:%s conn:%d disconnect ",current1->username,inet_ntoa(from.sin_addr),conn);
                    delete_user(current1);
                    break;  
                           
                }
            current1=current1->next;
        }
        struct client *temp = header;
        strcat(disconnect, "|");
        strcat(disconnect, "Disconnect");
       
        while(temp != NULL){
            sent(temp->conn,disconnect);
            temp = temp->next;   
        }

        DWORD exitCode;  //for terminate thread 
        if(GetExitCodeThread(thread,&exitCode) != 0){ 
            printf("Thread closed\n");   
            TerminateThread(thread,exitCode);  //terminate thread 
            }  
        }

        else{
        sent(conn,"wrong data");
            }
        }
    }
}


int main() {
    struct client *user=header;
    SOCKET server;
    WSADATA wsaData;
    struct sockaddr_in local;
    local.sin_family=AF_INET; 
    local.sin_addr.s_addr=INADDR_ANY ;
    local.sin_port=htons((u_short)20248); 
    int wsaret=WSAStartup(0x101,&wsaData);
    if(wsaret!=0) {return 0; }

    server=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(server==INVALID_SOCKET) {  return 0; }

    if(bind(server,(struct sockaddr*)&local,sizeof(local))!=0) { return 0;}

    if(listen(server,100)!=0){return 0;}
    int fromlen=sizeof(from);
    int *conn;
    
    while(1){  //wait for connection
        conn=malloc(sizeof(int));
        *conn=accept(server,(struct sockaddr*)&from,&fromlen);

        thread=  CreateThread(  //if new connection avalieble create thread
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