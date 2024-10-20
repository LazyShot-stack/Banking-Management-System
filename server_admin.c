#include <stdio.h>
#include <time.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/sendfile.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>  // for file control 

#define MAX 3000

extern void error(char *);
extern char *client_ip;

void show_cust_history(int sockfd, char *cust_id)
{
    int n;
    char filename[MAX];
    sprintf(filename,"%s",cust_id);
    strcat(filename,"_history.txt");
    struct stat file_stat;
    char buffer[MAX];

    int fd = open(filename, O_RDONLY);
    if(fd == -1)
        error("Error in opening user file for mini_statement");
    
    // finding stats of file
    if (fstat(fd, &file_stat) < 0)
        error("Error in getting statistics of file.");

    // writing size of file
    bzero(buffer,MAX);
    sprintf(buffer, "%d", (int)file_stat.st_size);
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0) 
        error("ERROR writing to socket");
    
    // delimeter string
    bzero(buffer,MAX);
    n = read(sockfd,buffer,MAX-1);
    if (n < 0) 
        error("ERROR reading from socket");
    
    // sending mini statement        
    fprintf(stdout, "Sending mini statement of customer '%s' to client with ip '%s'. \n", cust_id, client_ip);
    while (1) 
    {
        bzero(buffer,MAX);
        int bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read == 0) 
            break;
        if (bytes_read < 0) 
            error("ERROR reading from file.");
        
        void *ptr = buffer;
        while (bytes_read > 0) {
            int bytes_written = write(sockfd, ptr, bytes_read);
            if (bytes_written <= 0) 
                error("ERROR writing to socket");
            bytes_read -= bytes_written;
            ptr += bytes_written;
        }
    }
    close(fd);         
}


void list_loan_applications(int sockfd, char *emp_id)
{
    int n;
    char filename[MAX];
    strcpy(filename, "loans.txt");
    char buffer[MAX];
    FILE *fd = fopen(filename, O_RDONLY);
    if(fd == -1)
        error("Error in opening user file for loan applications");
       
    fprintf(stdout, "Sending loan applications of employee '%s' to client with ip '%s'. \n", emp_id, client_ip);
    char str1[MAX];
    while (1) 
    {
        bzero(buffer,MAX);
        int bytes_read = read(fd, buffer, sizeof(buffer));
        if (bytes_read == 0) 
            break;
        if (bytes_read < 0) 
            error("ERROR reading from file.");
        
        char *time = strtok(buffer," ");
        char *cust_id = strtok(NULL," ");
        char *loan_amt = strtok(NULL, " ");
        char *status = strtok(NULL, " ");
        char *owner_id = strtok(NULL, " ");
        int l = strlen(owner_id);
        if(owner_id[l-1] == '\n');
            owner_id[l-1] = '\0';
        if(!strcmp(owner_id, emp_id)){
            strcat(str1, cust_id);
            strcat(str1,  " ");
            strcat(str1, loan_amt);
            strcat(str1, " ");
            if(!strcmp(status, "P")){
                strcat(str1, "Pending");
                strcat(str1, " ");
            }
            else if(!strcmp(status, "A")){
                strcat(str1, "Approved");
                strcat(str1, " ");
            }
            else if(!strcmp(status, "R")){
                strcat(str1, "Rejected");
                strcat(str1, " ");
            }
        }
    }
    n = write(sockfd,str1,strlen(str1));
    if (n < 0) 
        error("ERROR writing to socket");
    fclose(fd);         
}

void add_customer(int sockfd, char *cust_id){
    int n;
    char buffer[MAX];
    bzero(buffer,MAX);
    n = read(sockfd,buffer,MAX-1);
    if (n < 0) 
        error("ERROR reading from socket");
    char password[MAX];
    strcpy(password, buffer);
    // appending to the login file 
    char filename[MAX] = "login_file.txt";
    FILE *fp = fopen(filename,"a+");
    if(fp == NULL)
    {
        strcpy(buffer, "processed");
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0)  
            error("ERROR writing to socket");
        fprintf(fp, "%s %s C", cust_id, password);
        fclose(fp);
        free(buffer);
        free(password);
    }
    else 
    {
        strcpy(buffer, "processed");
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0)  
            error("ERROR writing to socket");
        free(buffer);
        free(password);
        fprintf(stdout, "Error in opening login file for add customer.");
    }
}

void admin(int sockfd, char* emp_id)
{
    int n;
    char buffer[MAX];
    char id[MAX];
    sprintf(id,"%d",emp_id);

    /* Reading flag */
    bzero(buffer,MAX);
    n = read(sockfd,buffer,MAX-1);
    if (n < 0) 
        error("ERROR reading from socket");
    
    while(buffer[0]=='y')
    {   
        // reading command
        bzero(buffer,MAX);
        n = read(sockfd,buffer,MAX-1);
        if (n < 0) 
            error("ERROR reading from socket");
        
        buffer[strlen(buffer)-1] = '\0';
        if(!strcmp(buffer,"add_customer"))
        {
            // sending true
            bzero(buffer,MAX);
            strcpy(buffer,"true");
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");

            // delimeter string
            bzero(buffer,MAX);
            n = read(sockfd,buffer,MAX-1);
            if (n < 0)
                error("ERROR reading from socket");
            // check customer id uniqueness
            char cust_id[MAX];
            strcpy(cust_id, buffer);
            if(check_new_cust(buffer) == 0){
                // return no 
                bzero(buffer, MAX);
                strcpy("no", buffer);
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)  
                    error("ERROR writing to socket");
                continue;
            }
            else{
                // return yes 
                bzero(buffer, MAX);
                strcpy("yes", buffer);
                n = write(sockfd,buffer,strlen(buffer));
                if (n < 0)  
                    error("ERROR writing to socket");
            }
            add_customer(sockfd, id);
        }
        else if(!strcmp(buffer,"loan_applications"))
        {
            // sending true
            bzero(buffer,MAX);
            strcpy(buffer,"true");
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");

            list_applications(sockfd, id);
        }
        else if(!strcmp(buffer,"customer_transactions"))
        {
            // sending true
            bzero(buffer,MAX);
            strcpy(buffer,"true");
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");

            // delimeter string
            bzero(buffer,MAX);
            n = read(sockfd,buffer,MAX-1);
            if (n < 0)
                error("ERROR reading from socket");
            char cust_id[MAX];
            show_cust_history(sockfd, cust_id);
        }
        else if(!strcmp(buffer,"process_loan"))
        {
            // sending true
            bzero(buffer,MAX);
            strcpy(buffer,"true");
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");

            // delimeter string
            bzero(buffer,MAX);
            n = read(sockfd,buffer,MAX-1);
            if (n < 0)
                error("ERROR reading from socket");
            double amount = 0;
            char *endptr;
            amount = strtod(buffer, &endptr);
            request_loan(sockfd, id, amount);
        }
        else
        {
            fprintf(stdout, "Request from client with ip '%s' declined. \n", client_ip);    
            // sending false
            bzero(buffer,MAX);
            strcpy(buffer,"false");
            n = write(sockfd,buffer,strlen(buffer));  
            if (n < 0) 
                error("ERROR writing to socket");
        } 
        /* Reading flag */
        bzero(buffer,MAX);
        n = read(sockfd,buffer,MAX-1);
        if (n < 0) 
            error("ERROR reading from socket");
    }
}
