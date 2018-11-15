#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<pthread.h>
#include<sys/sendfile.h>

#define SIZE 1024
#define HOME_PAGE "index.html"
#define IP "0"
#define PORT 8080

typedef struct Http_Header
{
     char  Fist_line[SIZE];
     char* method;
     char* url;
     char* parameter;
     char* body;
     int   content_length;
    
     //记得free body! ! ! 
}Http_Header;


int http_strat()
{

      int sock= socket(AF_INET, SOCK_STREAM,0);
      if(sock<0)
      {
          perror("sock");
          return -1;
      }
      
        int opt;
        setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

       struct sockaddr_in addr;
       addr.sin_family=AF_INET;
       addr.sin_addr.s_addr=inet_addr(IP);
       addr.sin_port=htons(PORT);

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
        printf("in get line\n");
        printf("the socket is %d\n",(int)socket);
        int i=0;
        int ret=0;
        while(1)
        {   
            char c='\0';
            ret=recv(socket,&c,1,0);
            if(ret<=0)
            {
              printf("%d recv error\n",(int)socket);
              pthread_exit(NULL);
            }

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

void parse_first_line(int64_t new_sock,Http_Header * header)
{
    
    get_line(new_sock,header->Fist_line);
    printf("after get line\n");
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
       i++;

    }
    i=0;
    
    while(header->url[i]!='\0')
    {

        if(header->url[i]=='?')
        {
            header->url[i]='\0';
            header->parameter=&header->url[++i];
            
            printf("header_paremeter:%s \n",header->parameter);
            break;
        }
        i++;
    }

    if(header->url[i]=='\0')
    {
        header->parameter=NULL;
    }

}

void get_the_body(int64_t  new_sock,Http_Header* header)
{      printf("in get_the_body\n");
       while(1)
       {
         char buff[SIZE]={0};
         get_line(new_sock,buff);
         if(buff[0]=='\n')
         {
             break;
         }
         if(strncasecmp(buff,"Content-Length: ",strlen("Content-Length: ") )==0)
         {
              header->content_length=atoi(&buff[strlen("Content_Length: ")]);
         }

       }
       if(header->content_length>0)
       {
           header->body=(char*)malloc(header->content_length);
           read(new_sock,header->body,header->content_length);

       }
}

     


void parse_header(int64_t new_sock,Http_Header * header)
{
     
     parse_first_line(new_sock,header);
     printf("after parse_first_line\n"); 
     get_the_body(new_sock,header);
     printf("after get body\n");
}


int  CGI_Handle(int64_t new_sock,Http_Header* header,char* url)
{
    int fd1[2]; 
    int fd2[2];

    if(pipe(fd1)<0)
    {
       perror("pipe fd1");
       return 404;
    }

    if(pipe(fd2)<0)
    {
      perror("pipe fd2");
      return 404;
    }
    
    int father_write=fd1[1]; 
    int child_read=fd1[0];

    int father_read=fd2[0];
    int child_write=fd2[1];
   
    //处理header里body空间的内容，如果有的话.
    if(header->method=="GET" && header->content_length>0)
    {
      free(header->body);
    }
    if(header->method=="POST")
    {
      write(father_write,header->body,header->content_length);
      free(header->body);
    }
   
    printf("putenv\n");

    

    pid_t pid=fork();

    if(pid<0)
    {
       perror("pid");
       return 404;
    }
    else if(pid==0)
    {
        close(father_read);
        close(father_write);
       
        char method[10]={"METHOD="};
        strcat(method,header->method);
        putenv(method);
         
        if(header->parameter!=NULL)
        {
            char query_string[1024]={"QUERY_STRING="};
            strcat(query_string,header->parameter);
            putenv(query_string);   
        }
    
        char content_length[1024]={0};
        sprintf(content_length,"CONTENT_LENGTH=%d",header->content_length);
        putenv(content_length);

        int ret; 
        ret=dup2(child_read,0);
        if(ret<0)
        {
          perror("dup2 child_read");
          return 1;
        }
        ret=dup2(child_write,1);
        if(ret<0) 
        {
          perror("dup2 father_write");
          return 2;
        }
       
        //printf("hello worlds\n");
        ret= execl(url,NULL);
        printf("ret:%d\n",ret); 
        _exit(1);

    }
    else
    {
      close(child_read);
      close(child_write);
      int ret=waitpid(pid,NULL,0);
      if(ret==-1)
      {
           printf("waitpid faile\n");
           return 1;
      }
       
      printf("wait pid successful.\n"); 
      
      char buff[100]={0};
      read(father_read,buff,sizeof(buff));
     
      char body[SIZE]={0};
      sprintf(body,"<header><meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\"></header> <h1>The result is %s.</h1>",buff);
      
      char content_length[100]={0}; 
      sprintf(content_length,"Content-Length: %d\n",(int)strlen(body));

      write(new_sock,"HTTP/1.1 200 OK\n",strlen("HTTP/1.1 200 OK\n"));

      write(new_sock,content_length,strlen(content_length));
      write(new_sock,"\n",strlen("\n"));
      
    // while(1) 
    // {
       write(new_sock,body,strlen(body));
      // write(new_sock,buff,s);
       
     //  len = len-(int)s;     
     //  if(len<=0)
     //  {
     //    break;
     //  }

    // }
     printf("after write\n");

      
    }
    
    return 0; 
}





void ConnectRequest(int64_t new_sock)
{
       
       printf("in content request\n");
       int  errcode=0;
       char url[SIZE]={0};
      
       Http_Header header;
       memset(&header,0,sizeof(header));
       printf("before pares_header\n");
       parse_header(new_sock,&header); 
       
       printf("header_method:%s\n",header.method);    
       printf("header_url:%s\n",header.url);
       printf("Content_Length:%d\n",header.content_length);
      
       //parse url and judge if it exist

       sprintf(url, "webroot%s",header.url);

       if(header.url[strlen(header.url)-1]=='/')
       {
           strcat(url,HOME_PAGE);
       }
        printf("%s\n",url); 
        struct stat st;
      
       if(stat(url,&st)<0)
       {
          perror("url");
          errcode=404;
          goto END;
       }

       //handle by method 
       if(strcasecmp(header.method,"GET")==0)
       {        
             printf("in GET\n");
             if(header.parameter==NULL)
             {
                printf("Is an regular file\n");
                int fd=open(url,O_RDONLY);
                 
                if(fd<0)
                {
                   printf("open html faile\n");
                   errcode=404;
                   goto END;
                }
                printf("open right\n");
                char content_length[1024]={0}; 
                sprintf(content_length,"Content-Length: %d\n",(int)st.st_size);

                write(new_sock,"HTTP/1.1 200 OK\n",strlen("HTTP/1.1 200 OK\n"));
                
                if( strcasecmp(&header.url[strlen(header.url)-3],"jpg")==0)
                {
                  printf("in jpg request\n");
                  write(new_sock,"Content-Type: image/jpg\n",strlen("Content-Type: image/jpg\n"));
                }
                else
                {
                   printf("in html request\n");
                   write(new_sock,"Content-Type: text/html;charset:utf-8;\n",strlen("Content-Type: text/html;charset:utf-8;\n"));
                }
                
                write(new_sock,content_length,strlen(content_length));
                write(new_sock,"\n",strlen("\n"));
              
                sendfile(new_sock,fd,NULL,st.st_size); 
                printf("after write body\n");

             }
             else if(st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH )
             {    
                  printf("In Get cgi\n");
                  int ret= CGI_Handle(new_sock,&header,url);
                  if(ret>=400)
                  {
                    perror("CGI");
                    goto END;
                  }
                  
             }
                     
       }
       else if(strcasecmp(header.method,"POSE")==0)
       {
              //动态页面 
            if(st.st_mode & S_IXUSR || st.st_mode & S_IXGRP || st.st_mode & S_IXOTH)
            {
              int ret=CGI_Handle(new_sock,&header,url);
              if(ret>=400)
              {
                perror("CGI");
                goto END;
              }

            }
            else
            {
                perror("POSE url unable execute");
                errcode=404;
                goto END;
            }             


       }
       else
       {
             printf("method faile\n");
             errcode=404;
       }

END:
       if(errcode==404)
       {
           char buff[1024]={"<meta http-equiv=\"content-type\" content=\"text/html;charset=utf-8\"><h1>你的页面被喵星人吃掉了！</h1>\0"};
           
           write(new_sock,buff,strlen(buff));
       }

       close(new_sock);
       printf("///////////////One Thread end!//////////////////\n");       
}

void* thread(void* arg)
{
//   signal(SIGPIPE ,SIG_IGN);
   int64_t new_sock=(int64_t)arg;
   printf("pthread create\n");

//   char c='\0';
//   recv(new_sock,&c,1,MSG_PEEK);
//   if(c=='G'|| c=='P')
//   { 
//     printf("c:%c\n",c); 
     ConnectRequest(new_sock);
//   }
//   else
//   {
//     printf("error header %d\n",(int)new_sock);
//   }
   return NULL;
}


int main()
{
    daemon(1,1); 

    int fd=http_strat();
    if(fd<0)
    {
        perror("http_strat");
        return 1;
    }

    while(1)
    {
        struct sockaddr_in client;
        socklen_t len= sizeof(client);
        int64_t new_sock=accept(fd, (struct sockaddr*)&client, &len);
        if(new_sock<0)
        {
            perror("accept");
            continue;
        }
        int ret;        
        printf("get a new connect %d\n",(int)new_sock);
        
        pthread_t tid;
        ret= pthread_create(&tid,NULL,thread,(void*)new_sock);
        if(ret!=0)
        {
           perror("pthread_create");
           continue;
        }
        pthread_detach(tid);
        
    }
    return 0;
}
