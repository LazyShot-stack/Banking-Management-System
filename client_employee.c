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

#define MAX 3000

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


void employee(int sockfd)
{
    char buffer[MAX];
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
        printf(" * add_customer \n * loan_applications \n * customer_transactions \n * process_loan \n");
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
            if(!strcmp(operation,"add_customer"))
            {
                bzero(buffer,MAX);
                fprintf("Enter customer id: ");
                fgets(buffer, MAX, stdin);
                // send customer id to server to check uniqueness
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)  
                    error("ERROR writing to socket");
                // reading feedback for customer id
                bzero(buffer,MAX);
                n = read(sockfd,buffer,MAX-1);
                if (n < 0) 
                    error("ERROR reading from socket");
                // checking feedback 
                if(!strcmp(buffer, "no")){
                    fprintf(stdout, "Customer with this ID already exist! Please try again. \n\n");
                    continue;
                }
                else{
                    fprintf(stdout, "Adding a new customer..");
                }
                bzero(buffer, MAX);
                fprintf("Please enter password: ");
                fgets(buffer, MAX, stdin);
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)  
                    error("ERROR writing to socket");

                bzero(buffer,MAX);
                n = read(sockfd,buffer,MAX-1);
                if (n < 0) 
                    error("ERROR reading from socket");
                if(!strcmp(buffer, "processed")){
                    fprintf(stdout, "Successfully added new customer!\n\n");
                }   
                else{
                    fprintf(stdout, "ERROR with adding new customer!\n\n");
                }    
            }
            else if(!strcmp(operation,"loan_applications"))
            {   
                // loan applications
                printf("Loan applications: \n");
                bzero(buffer,MAX);
                n = read(sockfd,buffer,MAX-1);
                if (n < 0) 
                    error("ERROR reading from socket");
                fprintf(stdout, buffer);
                printf("\n\n");
            }
            else if(!strcmp(operation,"customer_transactions"))
            {
                // getting customer transactions
                bzero(buffer,MAX);
                printf("Enter customer id to see transactions: ");
                fgets(buffer, MAX, stdin);
                // send customer id to server 
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)  
                    error("ERROR writing to socket");
                
                char c_id[MAX];
                strcpy(c_id, buffer);

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
                printf("MINI STATEMENT of customer %s: \n", c_id);
                bzero(buffer,MAX);
                while ((remain_data > 0) && ((n = read(sockfd, buffer, MAX)) > 0))
                {
                    printf("%s", buffer);
                    remain_data -= n;
                    bzero(buffer,MAX);
                }
                printf("\n\n");
                
            }
            else if(!strcmp(operation,"process_loan"))
            {
  
            }
            else{
                printf("Invalid operation! Please try again!");
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
