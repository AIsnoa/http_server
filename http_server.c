#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<pthread.h>

#define SIZE 1024

typedef struct Http_Header
{
     char  Fist_line[SIZE];
     char* method;
     char* url;
     char* parameter;
     char* body;
     
}Http_Header;


int http_strat(char* arg1, char* arg2)
{

      int sock= socket(AF_INET, SOCK_STREAM,0);
      if(sock<0)
      {
          perror("sock");
          return -1;
      }

      struct sockaddr_in addr;
      addr.sin_family=AF_INET;
      addr.sin_addr.s_addr=inet_addr(arg1);
      addr.sin_port=htons(atoi(arg2));

      int ret=bind(sock, (struct sockaddr*)&addr, sizeof(addr));
      if(ret<0)
      {
          perror("bind");
          return -1;
      }
       
      ret=listen(sock,5);
      if(ret<0)
      {
          perror("bind");
          return -1;
      }


      return sock;
}


void get_line(int64_t socket,char* buf)
{
        int i=0;

        while(i<SIZE)
        {
            char c;
            recv(socket,&c,1,0);

            if(c=='\r')
            {
                recv(socket,&c,1,MSG_PEEK);
                if(c=='\n')
                {
                    recv(socket,&c,1,0);
                }
                else
                {
                    c='\n';
                }
            }
 
            buf[i]=c;
            
            if(c=='\n')
            {
                break;
            }

            i++;
        }
}

void parse_header(Http_Header * header)
{
    
    int i=0;
    while(i<SIZE)
    {
        if(header->Fist_line[i]==' ')
        {
             header->Fist_line[i]='\0';
        }
        if(header->Fist_line[i]=='\n')
        {
            break;
        }
        i++;
    }
     
    header->method=header->Fist_line;
    i=0;
    while(i<SIZE)
    {
       if(header->Fist_line[i]=='\0')
       {
           header->url=&header->Fist_line[i+1];
           break;
       }
    }
    i=0;
    while(header->url[i]!='\0')
    {

        if(header->url[i]=='?')
        {
            header->url[i]='\0';
            header->parameter=&header->url[++i];
            break;
        }
        i++;
    }

    if(header->url[i]=='\0')
    {
        header->parameter=NULL;
    }
    
}

void read_all_in_socket(int64_t new_sock)
{
        int length;
       while(1)
       {

           char buff[1024]={0};
           get_line(new_sock,buff);

           if(strncasecmp(buff,"Content-Length: ",strlen("Content-Length: "))==0)
           {
              length= atoi(&buff[strlen("Context-Length: ")]);
           }
           if(buff[0]=='\n')
           {
                read(new_sock,buff,length);
                return;
           }
       }

}

int  CGI_Handle()
{
 return 404;
}




void ConnectRequest(int64_t new_sock)
{
       printf("in request\n");
       int errcode=0;
       Http_Header header;
       memset(&header,0,sizeof(header));
       get_line(new_sock,header.Fist_line);
       parse_header(&header);
              
       if(strcasecmp(header.method,"GET")==0)
       {
           printf("in GET");
           if(header.parameter==NULL)
           {
                //静态页面
              int fd= open("staitc.html",O_RDONLY);
              if(fd<0)
              {
                  errcode=404;
                  goto END;
              }
               
              read_all_in_socket(new_sock); 
              
              write(new_sock,"HTTP/1.1 200 OK\n",strlen("HTTP/1.1 200 OK\n"));
              
              write(new_sock,"\n",strlen("\n"));

              while(1)
              {
                char buff[4096]={0};
                ssize_t s=read(fd,buff,sizeof(buff));
                if(s<0)
                {
                    perror("read");
                    errcode=404;
                    goto END;
                }
                if(s==0)
                {
                    break;
                }
                write(new_sock,buff,s);
              }
           }
           else
           {
                //动态页面
               int ret=CGI_Handle();
               if(ret >=400)
               {
                   goto END;
               }
           }
                     
       }
       else if(strcasecmp(header.method,"POSE")==0)
       {
              //动态页面    
             int ret=CGI_Handle();
               if(ret >=400)
               {
                   goto END;
               }
       }
       else
       {
             errcode=404;
       }

END:
       if(errcode==404)
       {
           char buff[1024]={"<h1>你的页面被喵星人吃掉了！</h1>\0"};
           
           write(new_sock,buff,strlen(buff));
       }
       
}


void* thread(void* arg)
{
   int64_t new_sock=(int64_t)arg;
    printf("pthread create\n");
   ConnectRequest(new_sock);
   return NULL;
}


int main(int argc, char* argv[])
{
    if(argc!=3)
    {
        printf("./http [ip] [port]\n");
        return 1;
    }


    int fd=http_strat(argv[1], argv[2]);
    if(fd<0)
    {
        perror("http_strat");
        return 1;
    }

    while(1)
    {
        struct sockaddr_in client;
        socklen_t len= sizeof(client);
        printf("in accept\n");       
        int64_t new_sock=accept(fd, (struct sockaddr*)&client, &len);
        if(new_sock<0)
        {
            perror("accept");
            continue;
        }
        
        printf("before create pthread\n");
        pthread_t tid;
        pthread_create(&tid,NULL,thread,(void*)new_sock);
        
        pthread_detach(tid);
        

    }



    return 0;
}
