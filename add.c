#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main()
{  
//       putenv("SRCIPT_NAME=GET");
//       putenv("QUERY_STRING=a=13&b=12");
//       putenv("CONTENT_LENGTH=9");
  char* method=getenv("METHOD");
   char* paramere=getenv("QUERY_STRING");
   int   length=atoi(getenv("CONTENT_LENGTH"));
  // printf("%s\n",paramere);
   char buff[1024]={0};
   char content_length[1024]={0};

   //write(1,"hello world",strlen("hello world"));  
   if(strcmp(method,"GET")==0) 
   {
       // printf("in GET\n");
       if(paramere==NULL)
       {
           putenv("CONTENT_LENGTH=0");
           return 0;
       }
       strcpy(buff,paramere);
       
       int a,b;
       sscanf(buff,"a=%d&b=%d",&a,&b);
       int ret=a+b;      
       char result[20]={0}; 
       sprintf(result,"%d",ret); 
    //   write(1,"size=%d\n",6+strlen(result));
       write(1,result,strlen(result));
   }
   else if(strcmp(method,"POST")==0)
   {
       ssize_t s=read(0,buff,length);
       int a,b;
       sscanf(buff,"a=%d&b=%d",&a,&b);
       int ret=a+b;
        
       char result[20]={0}; 
       sprintf(result,"%d",ret); 
   //    printf("size=%d\n",strlen(result));
       write(1,result,strlen(result));
   }
   else 
   {
        putenv("CONTENT_LENGTH=0");
   }
   


  return 0;
}
