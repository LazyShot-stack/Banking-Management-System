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

#define MAX 1001

extern void error(char *);
extern char *client_ip;


// used to lock and unlock file
// type = F_RDLCK: for locking in read mode 
// type = F_WRLCK: for locking in write mode 
// type = F_UNLCK: for unlocking 
int lock_file(int fd, short type) {
    struct flock fl;
    fl.l_type = type;       // F_RDLCK for read, F_WRLCK for write
    fl.l_whence = SEEK_SET; // Start from the beginning of the file
    fl.l_start = 0;         // Offset from l_whence
    fl.l_len = 0;           // 0 means lock the whole file
    fl.l_pid = getpid();    // Process ID for locking

    // Apply the lock
    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        perror("Error locking file");
        return -1;
    }
    return 0;
}

void credit_amt(int sockfd, char *cust_id, double amount)
{
    //credit amount
    int n;
    char filename[MAX], filenameh[MAX];
    char *endptr;
    sprintf(filename,"%s",cust_id);
    strcat(filename,"_balance.txt");
    sprintf(filenameh, "%s", cust_id);
    strcat(filenameh, "_history.txt");
    
    FILE *fp = fopen(filename,"r+");
    if(fp == NULL)
        error("Error in opening balance user file for credit.");

    FILE *fph = fopen(filenameh, "a+");
    if(fph == NULL)
        error("Error in opening history user file for credit.");

/*
    if (lock_file(fp, F_WRLCK) == -1) {
        close(fp);
        return -1;
    }
*/

    size_t len = 0;
    char *balance = NULL;
    while(getline(&balance, &len, fp) != -1);
    double new_balance = strtod(balance, &endptr) + amount;
    printf("curr balance %s, new balance %f\n", balance, new_balance);

    // writing to balance file
    freopen(filename, "w+", fp); 
    fprintf(fp, "%f", new_balance);

    // writing to history file 
    time_t c_t = time(NULL);
    struct tm tm = *localtime(&c_t);
    fprintf(fph, "\n%.2d-%.2d-%.4d %s %f", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, "credit", amount);

    // writing to server 
    fprintf(stdout, "Credit amount of %f for customer '%s' to client with ip '%s'. \n", amount, cust_id, client_ip);

    // sending updated balance
    char new_bal[20];
    sprintf(new_bal, "%f", new_balance);
    n = write(sockfd, new_bal, strlen(new_bal));
    if (n < 0)  
        error("ERROR writing to socket");
    
    free(balance);
    fclose(fp);
    fclose(fph);
}

void debit_amt(int sockfd, char *cust_id, double amount)
{
    //debit amount
    int n;
    char filename[MAX], filenameh[MAX];
    char buffer[MAX];
    char *endptr;
    sprintf(filename,"%s",cust_id);
    strcat(filename,"_balance.txt");
    sprintf(filenameh, "%s", cust_id);
    strcat(filenameh, "_history.txt");

    FILE *fp = fopen(filename,"r+");
    if(fp == NULL)
        error("Error in opening balance user file for credit.");

    FILE *fph = fopen(filenameh, "a");
    if(fph == NULL)
        error("Error in opening history user file for credit.");

    size_t len = 0;
    char *balance = NULL;
    while(getline(&balance, &len, fp) != -1);
    double new_balance = strtod(balance, &endptr) - amount;
    printf("curr balance %s, new balance %f\n", balance, new_balance);
    if(new_balance < 0)
    {
        bzero(buffer, MAX);
        strcpy(buffer,"no"); 
        n = write(sockfd,buffer,strlen(buffer));
        if(n < 0)
            error("ERROR writing to socket");
        return;
    }
    else
    {
        bzero(buffer, MAX);
        strcpy(buffer,"yes");
        n = write(sockfd,buffer,strlen(buffer));
        if(n < 0)
            error("ERROR writing to socket");
    }

    // writing to balance file
    freopen(filename, "w+", fp);
    fprintf(fp, "%f", new_balance);

    // writing to history file 
    time_t c_t = time(NULL);
    struct tm tm = *localtime(&c_t);
    fprintf(fph, "\n%.2d-%.2d-%.4d %s %f", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, "debit", amount);

    // writing to server 
    fprintf(stdout, "Debit amount of %f for customer '%s' to client with ip '%s'. \n", amount, cust_id, client_ip);

    // sending updated balance
    char new_bal[20];
    sprintf(new_bal, "%f", new_balance);
    n = write(sockfd, new_bal, strlen(new_bal));
    if (n < 0)
        error("ERROR writing to socket");

    free(balance);
    fclose(fp);
    fclose(fph);
}

void transfer_amt(int sockfd, char *src_cust_id, double amount)
{
    //transfer amount
    int n;
    char filename[MAX], filenameh[MAX];
    char buffer[MAX];
    char *endptr;
    sprintf(filename,"%s", src_cust_id);
    strcat(filename,"_balance.txt");
    sprintf(filenameh, "%s", src_cust_id);
    strcat(filenameh, "_history.txt");

    FILE *fp = fopen(filename,"r+");
    if(fp == NULL)
        error("Error in opening balance user file for credit.");

    FILE *fph = fopen(filenameh, "a");
    if(fph == NULL)
        error("Error in opening history user file for credit.");

    // checking validity of transfer 
    size_t len = 0;
    char *balance = NULL;
    while(getline(&balance, &len, fp) != -1);
    double new_balance = strtod(balance, &endptr) - amount;
    if(new_balance < 0)
    {
        bzero(buffer, MAX);
        strcpy(buffer,"no"); 
        n = write(sockfd,buffer,strlen(buffer));
        if(n < 0)
            printf("ERROR writing to socket");
        return;
    }
    else
    {
        bzero(buffer, MAX);
        strcpy(buffer,"yes");
        n = write(sockfd,buffer,strlen(buffer));
        if(n < 0)
            printf("ERROR writing to socket");
    }

    // getting receiver customer id 
    bzero(buffer,MAX);
    n = read(sockfd, buffer, MAX-1);
    if (n < 0)  
        printf("ERROR reading from socket");
    char rec_custid[MAX];
    strcpy(rec_custid, buffer);

    // checking for validity of user 
    printf("Validating receiver user id %s for fund transfer", buffer);
    FILE *fp_l = fopen("login_file.txt","r");
    if(fp_l == NULL)
        printf("Error in opening login_file.");
    
    char *cred = NULL;
    len = 0;
    int flag = 0;
    while(getline(&cred,&len,fp_l)!=-1)
    {
        size_t ll = strlen(cred);
        if(cred[ll-1] == '\n')
            cred[ll-1] = '\0';
        ll = strlen(buffer);
        if(buffer[ll-1] == '\n')
            buffer[ll-1] = '\0';
        char *username = strtok(cred," ");
        char *password = strtok(NULL," ");
        char *usertype = strtok(NULL, " ");
        printf("checking details: username %s, usertype: %s, buffer: %s", username, usertype, buffer);
        if(!strcmp(username,buffer)&&!strcmp(usertype,"C"))
        {
            flag = 1;
        } 
        else if(!strcmp(username,buffer))
        {
            flag = 2;
        }
    }
    free(cred);
    fclose(fp_l);
    bzero(buffer,MAX);
    if(flag == 1)
    {
        // valid user id 
        strcpy(buffer, "1");
    }
    else if(flag == 2)
    {
        // invalid user type
        strcpy(buffer, "2");
    }
    else
    {
        // invalid customer id 
        strcpy(buffer, "3");
    }
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
        error("ERROR writing to socket");
    // continue only for valid transactions
    if(flag != 1)
    {
        printf("Exiting transfer funds for customer %s because of incorrect receiver id\n", src_cust_id);
        return;
    }

    // writing to balance file of sender  
    freopen(filename, "w+", fp);
    fprintf(fp, "%f", new_balance);
    printf("Updated balance of %s\n", src_cust_id);

    // writing to history file of sender
    time_t c_t = time(NULL);
    struct tm tm = *localtime(&c_t);
    fprintf(fph, "\n%.2d-%.2d-%.4d %s %f", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, "debit", amount);

    fclose(fp);
    fclose(fph);

    // Updating balance file of receiver
    sprintf(filename,"%s", rec_custid);
    strcat(filename,"_balance.txt");
    sprintf(filenameh, "%s", rec_custid);
    strcat(filenameh, "_history.txt");

    fp = fopen(filename,"r+");
    if(fp == NULL)
        printf("Error in opening balance user file %s for credit.", filename);

    fph = fopen(filenameh, "a");
    if(fph == NULL)
        error("Error in opening history user file for credit.");
    // reading balance 
    free(balance);
    len = 0;
    balance = NULL;
    while(getline(&balance, &len, fp) != -1);
    double new_balance_recv = strtod(balance, &endptr) + amount;
    // writing new balance of receiver to its balance file 
    freopen(filename, "w+", fp);
    fprintf(fp, "%f", new_balance_recv);
    printf("Updated balance of %s\n", rec_custid);
    // adding this transaction to the history of receiver
    c_t = time(NULL);
    tm = *localtime(&c_t);
    fprintf(fph, "\n%.2d-%.2d-%.4d %s %f", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, "credit", amount);

    // writing to server 
    fprintf(stdout, "Fund transfer initiated of amount %f from customer '%s' to customer %s with client ip '%s'. \n", amount, src_cust_id, rec_custid, client_ip);
    // sending updated balance of client
    char new_bal[20];
    sprintf(new_bal, "%f", new_balance);
    n = write(sockfd, new_bal, strlen(new_bal));
    if (n < 0)
        error("ERROR writing to socket");

    free(balance);
    fclose(fp);
    fclose(fph);
}

void write_feedback(int sockfd, char *cust_id){
    int n;
    char filename[MAX] = "feedbacks.txt";
    char buffer[MAX];
    FILE* fp = fopen(filename, "a+");
    if(fp == -1)
        fprintf(stdout, "Error in opening user file for loan_status");

    bzero(buffer,MAX);
    n = read(sockfd, buffer, MAX-1);
    if (n < 0)  
        fprintf(stdout, "ERROR reading from socket");
    
    time_t c_t = time(NULL);
    struct tm tm = *localtime(&c_t);
    fprintf(fp, "\n%.2d-%.2d-%.4d %s %s", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, cust_id, buffer);
    fprintf(stdout, "Received feedback from customer %s\n", cust_id);
    fclose(fp);
    strcpy(buffer, "processed");
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
       error("ERROR writing to socket");

}

void check_loan_status(int sockfd, char *cust_id)
{
    int n;
    char filename[MAX] = "loans.txt";
    //sprintf(filename,"%s",cust_id);
    //strcat(filename,"_history.txt");
    char buffer[MAX];

    FILE* fd = fopen(filename, "r");
    if(fd == -1)
        error("Error in opening user file for loan_status");
    
    // finding details  
    fprintf(stdout, "Checking loan status of customer %s\n", cust_id);
    char *loan_details = NULL;
    int len = 0;
    while(getline(&loan_details,&len,fd)!=-1)
    {
        size_t ll = strlen(loan_details);
        if(loan_details[ll-1] == '\n')
            loan_details[ll-1] = '\0';
        char *time = strtok(loan_details," ");
        char *cust_id_f = strtok(NULL," ");
        char *amt = strtok(NULL, " ");
        char *status = strtok(NULL, " ");
        if(strcmp(cust_id, cust_id_f))
            continue;
        if(!strcmp(status, "P")){
            strcpy(buffer, "pending");
        }
        else if(!strcmp(status, "A")){
            strcpy(buffer, "accept");
        }
        else if(!strcmp(status, "R")){
            strcpy(buffer, "reject");
        }
        fprintf(stdout, "Sending loan status %s for customer %s\n", buffer, cust_id);
        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0)  
            error("ERROR writing to socket");
        break;
    }
    free(loan_details);
    fclose(fd);
    bzero(buffer,MAX);
    close(fd);         
}


void request_loan(int sockfd, char *cust_id, double loan_amount)
{
    int n;
    char filename[MAX] = "loans.txt";
    char buffer[MAX];
    //strcat(filename,"loans.txt");
    
    FILE *fp = fopen(filename,"a+");
    if(fp == NULL)
        error("Error in opening user file for balance.");
    
    // writing to lonas file 
    time_t c_t = time(NULL);
    struct tm tm = *localtime(&c_t);
    fprintf(fp, "\n%.2d-%.2d-%.4d %s %f P -1", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, cust_id, loan_amount);
    fprintf(stdout, "Received loan request of amount %f from customer %s\n", loan_amount, cust_id);
    fclose(fp);
    strcpy(buffer, "processed");
    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
       error("ERROR writing to socket");

}

void available_balance(int sockfd,char *cust_id)
{
    int n;
    char filename[MAX];
    sprintf(filename,"%s", cust_id);
    strcat(filename,"_balance.txt");
    
    FILE *fp = fopen(filename,"r");
    if(fp == NULL)
        error("Error in opening user file for balance.");
    
    size_t len = 0;
    char *balance = NULL;
    getline(&balance, &len, fp);
    fprintf(stdout, "Sending balance of customer '%s' to client with ip '%s'. \n", cust_id, client_ip);
    // balance
    n = write(sockfd,balance,strlen(balance));
    if (n < 0) 
        error("ERROR writing to socket");
    
    free(balance);
    fclose(fp);
}

void mini_statement(int sockfd, char *cust_id)
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


void customer(int sockfd,int cust_id)
{
    int n;
    char buffer[MAX];
    char id[MAX];
    sprintf(id,"%d",cust_id);

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
        if(!strcmp(buffer,"deposit"))
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
            credit_amt(sockfd, id, amount);
        }
        else if(!strcmp(buffer,"withdraw"))
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
            debit_amt(sockfd, id, amount);
        }
        else if(!strcmp(buffer,"transfer_funds"))
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
            transfer_amt(sockfd, id, amount);
        }
        else if(!strcmp(buffer,"loan_application"))
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
        else if(!strcmp(buffer,"loan_status"))
        {
            // sending true
            bzero(buffer,MAX);
            strcpy(buffer,"true");
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
            check_loan_status(sockfd, id);
        }
        else if(!strcmp(buffer,"feedback"))
        {
            // sending true
            bzero(buffer,MAX);
            strcpy(buffer,"true");
            n = write(sockfd,buffer,strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
            write_feedback(sockfd, id);
        }
        else if(!strcmp(buffer,"balance"))
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
            
            available_balance(sockfd,id);
        }
        else if(!strcmp(buffer,"mini_statement"))
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
            
            mini_statement(sockfd,id); 
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
