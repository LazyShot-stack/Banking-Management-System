#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX 1001

extern void error(char *);
int is_valid_number(char *amt)
{
   // checking validity of amount
   int i;
   int count=0;
   for(i=0; i<strlen(amt)-1; i++)
   {
     // printf("checking %c", amt[i]);
      if(amt[i]=='.')
      {
         count++;
         if(count>1)
            return 0;
      }
      else if(amt[i]<='9' && amt[i]>='0')
         continue;
      else if(amt[i] == '\0')
         return 1;
      else
         return 0;
   }
   return 1;
}


void customers(int sockfd)
{
    char buffer[MAX];
    char buffer2[MAX];
    char operation[MAX];
    char *endptr;
    int n;
    char flag;
    
    printf("Do u want to continue (y/n): ");
    scanf("%c",&flag);
    getchar();
    
    while(flag=='y')
    {
        bzero(buffer,MAX);
        buffer[0] = flag;
        buffer[1] = '\0';
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0) 
            error("ERROR writing to socket");
        printf("Supported operations:\n");
        printf(" * deposit\n * withdraw \n * balance  \n * mini_statement\n * transfer_funds \n * feedback \n * loan_status \n * loan_application\n");
        printf("Select operation to perform: ");
        bzero(operation,MAX);
        fgets (operation, MAX, stdin);
        
        // sending command
        n = write(sockfd,operation,strlen(operation));
        if (n < 0) 
            error("ERROR writing to socket");
        
        operation[strlen(operation)-1] = '\0';

        // true or false
        bzero(buffer,MAX);
        n = read(sockfd,buffer,MAX-1);
        if (n < 0) 
            error("ERROR reading from socket");
        
        if(!strcmp(buffer,"false"))
        {
            printf("Invalid Operation.\n\n");
        }    
        else if(!strcmp(buffer,"true"))
        {
            if(!strcmp(operation,"deposit"))
            {
                // delimiter string
                bzero(buffer,MAX);
                printf("Enter amount to credit: ");
                fgets(buffer, MAX, stdin);
                // check for valid amount
                if(!is_valid_number(buffer)) 
                {
                    printf("ERROR invalid number entered!\n\n");
                    continue;
                }
                printf("Requesting to credit %s", buffer);
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)  
                    error("ERROR writing to socket");
                // balance
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0)  
                    error("ERROR reading from socket");
                printf("Amount credited! Updated balance: %s\n\n", buffer);   
            }
            else if(!strcmp(operation,"withdraw"))
            {
                // delimiter string
                bzero(buffer,MAX);
                printf("Enter amount to withdraw: ");
                fgets(buffer, MAX, stdin);
                // check for valid amount
                if(!is_valid_number(buffer)) 
                {
                    printf("ERROR invalid number entered!\n\n");
                    continue;
                }
                printf("Requesting to withdraw %s", buffer);
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)
                    error("ERROR writing to socket");

                // getting validity of amount to be debited
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0)
                    error("ERROR reading from socket");
                if(!strcmp(buffer, "no"))
                {
                    printf("ERROR Not enough balance! Try again.\n\n");
                    continue;
                }
                
                // updated balance
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0)  
                    error("ERROR reading from socket");
                printf("Amount debited! Updated balance: %s\n\n", buffer);   
            }
            else if(!strcmp(operation,"transfer_funds"))
            {
                // delimiter string
                bzero(buffer,MAX);
                printf("Enter amount to transfer: ");
                fgets(buffer, MAX, stdin);
                // check for valid amount
                if(!is_valid_number(buffer)) 
                {
                    printf("ERROR invalid number entered!\n\n");
                    continue;
                }
                double amount = strtod(buffer, &endptr);
                printf("Requesting to transfer %s", buffer);
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)
                    error("ERROR writing to socket");

                // getting validity of amount to be debited
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0)
                    error("ERROR reading from socket");
                if(!strcmp(buffer, "no"))
                {
                    printf("ERROR Not enough balance! Try again.\n\n");
                    continue;
                }
                bzero(buffer, MAX);
                printf("Enter customer id of receiver: ");
                fgets(buffer, MAX, stdin);
                int cut_id = atoi(buffer);
                // sending receiver id to server 
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)
                    error("ERROR writing to socket");
                
                // checking if it is a valid receiver or not
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0)  
                    error("ERROR reading from socket");

                if(buffer[0] == '1')
                {
                    printf("Correct receiver ID. Proceeding with fund transfer!\n");
                }
                else if(buffer[0] == '0')
                {
                    printf("ERROR Invalid receiver customer id. Please try again!\n\n");
                    continue;
                }
                else
                {
                    printf("buffer %s", buffer);
                    printf("ERROR Receiver ID is not assigned to any customer. Please try again!\n\n");
                    continue;
                }

                // updated balance
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0)  
                    error("ERROR reading from socket");
                printf("Funds Transferred! Updated balance: %s\n\n", buffer);   
            }
            else if(!strcmp(operation,"loan_application"))
            {
                // delimiter string
                bzero(buffer,MAX);
                printf("Enter amount of loan: ");
                fgets(buffer, MAX, stdin);
                // check for valid amount
                if(!is_valid_number(buffer)) 
                {
                    printf("ERROR invalid number entered!\n\n");
                    continue;
                }
                printf("Requesting for loan of %s\n", buffer);
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)  
                    error("ERROR writing to socket");
                // balance
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0)  
                    error("ERROR reading from socket");
                if(!strcmp(buffer, "processed"))
                    printf("Successfully requested for loan! Thank you.\n");   
                else
                    printf("ERROR with requesting for loan. Please try again! \n");
            }
            else if(!strcmp(operation,"loan_status"))
            {
                // delimiter string
                printf("Fetching status of your loan\n");
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0)  
                    error("ERROR reading from socket");
                if(!strcmp(buffer, "accept")){
                    printf("Your loan has been accepted!\n");
                }
                else if(!strcmp(buffer, "reject")){
                    printf("Your loan has been rejected!\n");
                }
                else if(!strcmp(buffer, "pending")){
                    printf("Your loan is under processing!\n");
                }
                else{
                    printf("Couldn't find the status! Please try again.\n");
                }
            }
            else if(!strcmp(operation,"feedback"))
            {
                bzero(buffer,MAX);
                printf("Please provide a feedback: ");
                fgets(buffer, MAX, stdin);
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)  
                    error("ERROR writing to socket");
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0)  
                    error("ERROR reading from socket");
                if(!strcmp(buffer, "processed"))
                   printf("Your feedback has been added. Thank you.\n");        
                else
                   printf("ERROR with adding feedback. Please try again. Thank you.\n");
            }
            else if(!strcmp(operation,"balance"))
            {
                // delimiter string
                bzero(buffer,MAX);
                strcpy(buffer,"content");
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0) 
                    error("ERROR writing to socket");
                // balance
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0) 
                    error("ERROR reading from socket");
                printf("BALANCE: %s\n\n", buffer);   
            }
            else if(!strcmp(operation,"mini_statement"))
            {
                // delimeter string
                bzero(buffer,MAX);
                strcpy(buffer,"size");
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0) 
                    error("ERROR writing to socket");
                
                // file size 
                bzero(buffer,MAX);
                n = read(sockfd, buffer, MAX-1);
                if (n < 0) 
                    error("ERROR reading from socket");
                
                int file_size = atoi(buffer);
                int remain_data = file_size;
                
                // delimeter string
                bzero(buffer,MAX);
                strcpy(buffer,"content");
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0) 
                    error("ERROR writing to socket");

                // mini statement
                printf("MINI STATEMENT: \n");
                bzero(buffer,MAX);
                while ((remain_data > 0) && ((n = read(sockfd, buffer, MAX)) > 0))
                {
                    printf("%s", buffer);
                    remain_data -= n;
                    bzero(buffer,MAX);
                }
                printf("\n\n");
            }
        }
        printf("Do u want to continue (y/n): ");
        scanf("%c",&flag);
        getchar();
    }
    // sending flag
    bzero(buffer,MAX);
    buffer[0] = flag;
    buffer[1] = '\0';
    n = write(sockfd,buffer,strlen(buffer));  
}
