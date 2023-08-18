#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <thread>
#include <algorithm>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <sys/time.h>

#define MAXLINE 1024
#define SIZE 1024

long GetFileSize(std::string filename)
{
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}


void send_file(std::string filename, int sockfd, long file_size){
    //FILE *fp2 = fopen("/temp_files/tempfile1.txt","w");

    int n;
    char data[SIZE] = {0};
    int s = 0;
    char ack2[2];

    FILE *fp = fopen(filename.c_str(), "rb");

    //int descript = open(filename.c_str(),0);
    //sendfile(sockfd,descript,0,file_size);
    //std::cout<<"File Sent\n";
 
    while(fread(data,1,SIZE, fp) != 0){//&&file_size>0){//&&file_size>0) {
        sleep(0.01);
        //std::cout<<data<<"\n";
        if (send(sockfd, data, sizeof(data), 0) == -1) {
            std::cout<<"Error\n";
        perror("[-]Error in sending file.");
        exit(1);
        }
        //file_size-=SIZE;
        //s+=SIZE;
        //std::cout<<s<<"\n";
        file_size-=sizeof(data);
        //std::cout<<data<<"\n"<<file_size<<"\n";
        //fprintf(fp2, "%s", data);
        bzero(data, SIZE);
        //recv(sockfd, ack2, 2,0);
        //if(ack2[0]=='o'&&ack2[1]=='k') continue;
        //else break;
        bzero(ack2,2);
    }
    //if(file_size<=0)
    fclose(fp);
}

void write_file(int sockfd, char* filename, long file_size){
    int n;
    FILE *fp;
    //char *filename = "recv.txt";
    char buffer[SIZE];
    //int s = 0;
    char ack2[2]={'o','k'};
    fp = fopen(filename, "wb");
    //std::string f_name(filename);
    //std::ofstream o_file;
    /*o_file.open(f_name, std::ios::trunc | std::ios::out);
    char input[SIZE];
    while(file_size>0){
        int len = recv(sockfd, input, SIZE,0);
        std::cout<<len<<"\n";
        o_file.write(input, len);
        std::cout<<input;
        file_size-=len;
    }*/
    //o_file.open(filename, std::ofstream::binary);
    //std::cout<<filename<<" From inside write_file_func\n";
    std::string check_ack = "!@#$%^&*!@#$%^&*";
    while (true) {
        n = recv(sockfd, buffer, sizeof(buffer), 0);
        
        //std::cout <<"Received " << n << "\n";
        //std::cout << "aaaaaaaaaaaaa " << strlen(buffer) << "\n";
        //std::cout<<buffer;

        if (n < 0){
            std::cout<<"Error\n";
            break;
            //return;
        }
        if(n==0) {
            //close(sockfd);
            std::cout<<"Nothing recieved\n";
            break;
        }
        //send(sockfd, ack2, 2,0);
        if(file_size<=0) break;
        std::string a(buffer);
        if(a==check_ack) break;
        //std::cout<<buffer;
        //s+=SIZE;
        
        //o_file<<buffer;
        //std::cout<<s<<"\n";
        long write_bytes = 0;
        if(n>file_size){write_bytes = file_size;}
        else write_bytes = n;
        fwrite(buffer,1,write_bytes, fp);
        // std::cout<<buffer;//<<"\n";
        //printf(buffer,1,n);
        
        file_size-=n;
        //std::cout<<file_size<<"\n";
        //fprintf(fp, "%s", buffer);
        bzero(buffer, SIZE);
    }
    fclose(fp);
    //o_file.close();

    return;
}

std::vector<std::string> split_file_str(std::string buf){
    std::vector<std::string> spl_f_str = {};
    std::stringstream s(buf);          
    std::string s2;  
    while (std:: getline (s, s2, '#') )  
    {  
        spl_f_str.push_back(s2); // store the string in s2  
    }  
    return spl_f_str;
}

bool checkfiles(char* buf, std::string* req_files, int* file_found, int file_num, int sockfd, int* file_transfer_sock){
    bool x = false;
    std::string buf2(buf);
    std::vector<std::string> avail_files = split_file_str(buf2);
    for(unsigned int i=1; i<avail_files.size();i++){
        for(int j = 0;j<file_num;j++){
            if(avail_files[i]==req_files[j]){
                int un_id2;
                std::stringstream un_id(avail_files[0]);
                un_id>>un_id2;
                if(file_found[j]==0||file_found[j]>un_id2){
                    file_found[j] = un_id2;
                    file_transfer_sock[j] = sockfd;
                    x = true;
                }
            }
        }
    }
    return x;
}


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void server_imp(int port, int neigh_num, std::string* req_files, int* file_found, int file_num, std::string directory, std::string owned_files){

    int server_fd, new_socket, valread, newfd, fdmax, nbytes;
    int connfd, nready, maxfdp1;
    int *file_transfer_sock = new int[file_num];
    for(int i=0;i<file_num;i++){
        file_transfer_sock[i] = 0;
    }
    directory+="Downloaded/";
    struct sockaddr_in address;
    struct sockaddr_in cliaddr;
    struct sockaddr_storage remoteaddr;
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    socklen_t addrlen;
    fd_set rset;
    char remoteIP[INET6_ADDRSTRLEN];
    char buf[MAXLINE];    // buffer for client data

    int opt = 1, total_recv_connections = 0;
    pid_t childpid;
    char buffer[MAXLINE];

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0))
        == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(port);
    // Forcefully attaching socket to the client port
    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }



    
    //std::cout<<address.sin_port<<"\n";

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    newfd  = 0;
    fdmax = 0;

    // clear the descriptor set
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(server_fd,&master);
    fdmax = server_fd;

    

    int files_transferred = 0;
    bool x1 = true;
    std::vector<int> empty_sockets = {};
    std::vector<int> connected_sockets = {};
    while(true){


        

        read_fds = master; // copy it
        int p = 0;
        if (p=select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }
        else if(p<0){
            std::cout<<"No ret_val\n";
        }
       //cl_id++; 
        //break;

        

        // run through the existing connections looking for data to read
        for(int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) { // we got one!!
                if (i == server_fd) {
                    // handle new connections
                    addrlen = sizeof (remoteaddr);
                    newfd = accept(server_fd,
                        (struct sockaddr *)&remoteaddr,
                        &addrlen);
                    //std::cout<<&remoteaddr<<"\n"<<&addrlen<<"\n"<<server_fd<<"\n";//<<remoteaddr<<"\n"<<addrlen<<"\n";
                    if (newfd == -1) {
                        std::cout<<"Hi3\n";
                        perror("accept");
                    } else {
                        FD_SET(newfd, &master); // add to master set
                        if (newfd > fdmax) {    // keep track of the max
                            fdmax = newfd;
                        }
                        //printf("selectserver: new connection from %s on "    "socket %d\n",);
                    }
                } else {
                    // handle data from a client
                    if ((nbytes = recv(i, buf, sizeof (buf), 0)) <= 0) {
                        // got error or connection closed by client
                        if (nbytes == 0) {
                            // connection closed
                            printf("selectserver: socket %d hung up\n", i);
                        } else {
                            perror("recv");
                        }
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                    }
                    else{
                        bool abcd = false;
                        for(unsigned int j  = 0;j<connected_sockets.size();j++){
                            if(i == connected_sockets[j]){
                                abcd = true;
                                break;
                            }
                        }
                        if(!abcd){
                            char ack5[2] = {'o','k'};
                            
                            
                            if(checkfiles(buf, req_files, file_found, file_num, i, file_transfer_sock)==false){
                                //send(i,std::to_string(0).c_str(),strlen(std::to_string(0).c_str()),0);
                                empty_sockets.push_back(i);
                                //close(i); // bye!
                                //FD_CLR(i, &master); // remove from master set

                            }
                            //fcntl(i, F_SETFL, O_NONBLOCK); 
                            if(send(i,ack5,2,0)<=0){
                                perror("send");
                            }
                            std::cout<<"ACK : "<<ack5<<"\n";
                            std::cout<<"Hi : "<<buf<<" "<<i<<"\n";
                            bzero(buf,SIZE);
                            
                            //for(int i=0;i<file_num;i++){
                                
                            //}
                            total_recv_connections++;
                            connected_sockets.push_back(i);

                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
            
        }
        if(total_recv_connections==neigh_num) break;
        
    }

    int transferrable_files = 0;

    std::vector<int> curr_connections = {};
    std::vector<std::string> curr_connections_files = {};
    bool file_transfer_sock_check[neigh_num];
    for(int i=0;i<file_num;i++){
        bool y1 = false;
        if(file_found[i]>0){
            for(unsigned int j=0;j<curr_connections.size();j++){
                if(curr_connections[j]==file_transfer_sock[i]){
                    y1 = true;
                    curr_connections_files[j]+="#"+req_files[i];
                    break;
                }
            }
            if(!y1){
                curr_connections.push_back(file_transfer_sock[i]);
                curr_connections_files.push_back(req_files[i]);
            }
            transferrable_files++;
        }
    }
    for(unsigned int i = 0;i<empty_sockets.size();i++){
        curr_connections.push_back(empty_sockets[i]);
        curr_connections_files.push_back("");

    }
    std::cout<<"Transferrable files : "<<transferrable_files<<"\n";

    //newfd  = 0;
    //fdmax = 0;

    // clear the descriptor set
    //FD_ZERO(&master);
    //FD_ZERO(&read_fds);

    //FD_SET(server_fd,&master);
    //fdmax = server_fd;

    

    //if (listen(server_fd, 10) < 0) {
    //    perror("listen");
    //    exit(EXIT_FAILURE);
    //}
    //newfd  = 0;
    //fdmax = 0;


    //if (listen(server_fd, 10) < 0) {
    //    perror("listen");
    //    exit(EXIT_FAILURE);
    //}
    // clear the descriptor set
    //FD_ZERO(&master);
    //FD_ZERO(&read_fds);

    //FD_SET(server_fd,&master);
    //fdmax = server_fd;

    int total_finished_connections = 0;
    int count = 0;
    
    while(true){ // END looping through file descriptors
        //sleep(0.00001);
        count++;
        std::cout<<" Entered while loop\n";
        /*if(x1){

            std::vector<int> transfer_files_sock ={};
            std::vector <int> num_files_sock = {};
            for(int i=0;i<file_num;i++){
                if(file_found[i]>0){
                    bool y1 = false;
                    for(unsigned int j = 0; j<transfer_files_sock.size();j++){
                        if(transfer_files_sock[j]==file_transfer_sock[i]){
                            y1 = true;
                            num_files_sock[j]++;
                            break;
                        }
                    }
                    if(!y1){
                        transfer_files_sock.push_back(file_found[i]);
                        num_files_sock.push_back(1);
                    }
                }
            }
            //char num_files[20];
            for(int i=0;i<transfer_files_sock.size();i++){
                if(send(transfer_files_sock[i],std::to_string(num_files_sock[i]).c_str(),strlen(std::to_string(num_files_sock[i]).c_str()),0)>0){
                    std::cout<<num_files_sock[i]<<" Files required"<<"\n";
                }
            }
                

            x1 = false;
        }*/
        //std::cout<<"\n";
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000;
        read_fds = master; // copy it
        std::cout<<"Here\n";
        int p=0;
        p=select(fdmax+1, &read_fds, NULL, NULL, &tv);
        if ( p<0) {
            perror("select");
            std::cout<<"Select Error\n";
            continue;;
        }
        else if(p>0){

            std::cout<<"\n";
            std::cout<<"Haahaaa\n";
            for(int k = 0 ; k<=fdmax; k++){
                std::cout<<"K : "<<k<<"\n";

                if(FD_ISSET(k, &read_fds)){

                    std::cout<<"yes\n";

                    if(k!=server_fd){

                        
                        for(unsigned int i=0;i<curr_connections.size();i++){
                            if(curr_connections[i]==k){
                                count = 0;

                                std::cout<<k<<"\n";
                                char ack4[3] = {0};
                                std::vector<std::string> curr_files_tranfer = split_file_str(curr_connections_files[i]);
                                char ack11[6] = {};
                                if(recv(curr_connections[i],ack11, sizeof(ack11),0)<=0){
                                    close(curr_connections[i]);
                                    FD_CLR(curr_connections[i], &master);
                                    break;

                                } 
                                total_finished_connections++;
                                std::cout<<"Received : "<<ack11<<"\n";
                                if(strcmp(ack11,"ready")==0){
                                    //std::cout<<curr_connections[i];
                                    uint32_t un = htonl(int(curr_files_tranfer.size()));
                                    std::cout<<"Sending : "<<un<<"\n";
                                    write(curr_connections[i], &un, sizeof(un));
                                    //send(curr_connections[i], &un, sizeof(uint32_t), 0);
                                    //send(curr_connections[i],std::to_string(curr_files_tranfer.size()).c_str(), strlen(std::to_string(curr_files_tranfer.size()).c_str()),0);
                                    std::cout<<std::to_string(curr_files_tranfer.size())<<" files to be transferred over the connection\n";
                                    recv(curr_connections[i],ack4,sizeof(ack4),0);
                                    std::cout<<"Received "<<ack4<<"\n";
                                    if(strcmp(ack4,"ok")==0){

                                        for(unsigned int j = 0; j<curr_files_tranfer.size(); j++){
                                            std::string directory2  = directory + curr_files_tranfer[j];
                                            char*filename = new char[directory2.length()+1];
                                            strcpy(filename, directory2.c_str());
                                            if((send(curr_connections[i], curr_files_tranfer[j].c_str(), strlen(curr_files_tranfer[j].c_str()),0))<0){
                                                perror("send");
                                            }
                                            char buf2[SIZE];
                                            //recv(file_transfer_sock[i],buf2,sizeof(buf2),0);
                                            //std::string b(buf2);
                                            //std::stringstream a(b);
                                            long file_size = 0;
                                            int recv_int = 0;
                                            int ret_val = read(curr_connections[i], &recv_int,sizeof(recv_int));
                                            file_size = ntohl(recv_int);
                                            //a>>file_size;
                                            char ack[5] = {'r','e','a','d','y'};
                                            send(curr_connections[i], ack11, 5, 0);
                                            if(file_size>0){
                                                std::cout<<file_size<<"\n";
                                                write_file(curr_connections[i], filename, file_size);
                                                //std::cout<<"Found "<<curr_connections_files[i]<<" at "<<file_found[i]<<" with MD5 0 at depth 1\n";
                                                //file_found[i]=-1;
                                                files_transferred++;
                                                std::cout<<"Received file : "<<filename<<"\n";
                                            }
                                        }
                                        close(curr_connections[i]);
                                        FD_CLR(curr_connections[i], &master);
                                        break;
                                    }
                                
                                }
                            }
                            //recv(curr_connections[i],ack4,2,0);
                            //if(ack4[0]=='o'&&ack4[1]=='k'){

                            //}
                        }

                    }
                    else if (k == server_fd) {
                        std::cout<<"WTF\n";
                    // handle new connections
                        //addrlen = sizeof (remoteaddr);
                        //newfd = accept(server_fd,
                        //    (struct sockaddr *)&remoteaddr,
                        //    &addrlen);
                        //std::cout<<&remoteaddr<<"\n"<<&addrlen<<"\n"<<server_fd<<"\n";//<<remoteaddr<<"\n"<<addrlen<<"\n";
                        if (newfd == -1) {
                            std::cout<<"Hi3\n";
                            perror("accept");
                        } else {
                            FD_SET(newfd, &master); // add to master set
                            if (newfd > fdmax) {    // keep track of the max
                                fdmax = newfd;
                            }
                            //printf("selectserver: new connection from %s on "    "socket %d\n",);
                        }
                    }

                }
                


            }
            
        }

        /*for(int i=0;i<file_num;i++){
            std::string directory2  = directory + "#"+req_files[i];
                char*filename = new char[directory2.length()+1];
                strcpy(filename, directory2.c_str());
                //std::cout<<filename<<"\n";
                //long file_size = GetFileSize(directory2);
            std::string sending_str = req_files[i];//;+"#"+std::to_string(file_size);
            if(file_found[i]>0){
                std::cout << "Request for "<<filename<<"\n";
                if((send(file_transfer_sock[i], sending_str.c_str(), strlen(sending_str.c_str()),0))<0){
                    perror("send");
                }
                char buf2[SIZE];
                recv(file_transfer_sock[i],buf2,sizeof(buf2),0);
                std::string b(buf2);
                std::stringstream a(b);
                long file_size = 0;
                a>>file_size;
                if(file_size>0){
                    std::cout<<file_size<<"\n";
                    write_file(file_transfer_sock[i], filename, file_size);
                    std::cout<<"Found "<<req_files[i]<<" at "<<file_found[i]<<" with MD5 0 at depth 1\n";
                    file_found[i]=-1;
                    files_transferred++;
                }
                else file_found[i]=0;
                //close(file_transfer_sock[i]);
                //FD_CLR(file_transfer_sock[i], &master);
            }
            else if(file_found[i]==0){
                std::cout<<"Found "<<req_files[i]<<" at "<<file_found[i]<<" with MD5 0 at depth 0\n";
            }
        }*/
        if(files_transferred==transferrable_files&&total_finished_connections==curr_connections.size()) break;
 
        std::cout<<"\n";
    /*if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }*/
        //if(total_recv_connections == neigh_num){
            
        //    return;
        //}
    //if(count > 10) return;

    }
    for(int i=0;i<file_num;i++){
        
        if(file_found>0){
            FILE *fh;
            long filesize;
            unsigned char *buf3;
            unsigned char *md5_result = NULL;
            std::string file_name = directory+req_files[i];
            fh = fopen(file_name.c_str(), "r");
            fseek(fh, 0L, SEEK_END);
            filesize = ftell(fh);
            fseek(fh, 0L, SEEK_SET);
            buf3 = (unsigned char*) malloc(filesize);
            fread(buf3, filesize, 1, fh);
            fclose(fh);
            md5_result = (unsigned char*)malloc(MD5_DIGEST_LENGTH);
            MD5(buf3, filesize, md5_result);
            std::cout<<"Found "<<req_files[i]<<" at "<<file_found[i]<<" with MD5 ";
            for (i=0; i < MD5_DIGEST_LENGTH; i++)
            {
                printf("%02x",  md5_result[i]);
            }
            free(md5_result);
            free(buf3);
            std::cout<<" at depth 1\n";
        }
        else{
            std::cout<<"Found "<<req_files[i]<<" at "<<file_found[i]<<" with MD5 "<<"0 at depth 0\n";
        }
    }
    //std::cout<<"Received Required Files, closing sockets from the server side\n";
    //for(int i=0;i<file_num;i++){
    //    if(file_found[i]==-1)close(file_transfer_sock[i]);
    //}
}

void client_imp(std::string cl_id, std::string cl_uni_id, int port, int neigh_num,std::string* neigh_port_arr, std::string* neigh_id_arr, std::string owned_files, std::string directory){

    short int* neigh_connections;
    struct sockaddr_in address;
    int client_fd[neigh_num];
    int newfd = 0, opt = 1, fdmax = 0;
    fd_set master;
    FD_ZERO(&master);
    char buf[MAXLINE]; 
    neigh_connections = new short int[neigh_num];
    struct sockaddr_in serv_addr[neigh_num];
    for(int i=0;i<neigh_num;i++){
        neigh_connections[i] = 0;
    }
    int total_conf_msgs = 0;

    for(int i=0;i<neigh_num;i++){
        if ((client_fd[i] = socket(AF_INET, SOCK_STREAM, 0))
        == 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        if (setsockopt(client_fd[i], SOL_SOCKET,
                   SO_REUSEADDR | SO_REUSEPORT, &opt,
                   sizeof(opt))) {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        memset(&address, 0, sizeof(address));
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = inet_addr("0.0.0.0");
        address.sin_port = htons(port);

        if (bind(client_fd[i], (struct sockaddr*)&address,
                sizeof(address))
            < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        memset(&serv_addr[i], 0, sizeof(serv_addr[i]));
        serv_addr[i].sin_family = AF_INET;
        serv_addr[i].sin_addr.s_addr = inet_addr("0.0.0.0");
        
        std::stringstream tmp(neigh_port_arr[i]);
        int neighport;
        tmp>>neighport;
        //std::cout<<neighport<<"\n";
        serv_addr[i].sin_port = htons(neighport);
        
        //std::cout<<serv_addr.sin_port<<"\n";
    }

    while(true){
        sleep(0.001);
        for(int i=0;i<neigh_num;i++){
            if(neigh_connections[i]==0){
                //std::cout<<i<<std::endl;
                //std::cout<<"Hi\n";
                
                
                //std::cout<<"hi1\n";

                if ((newfd=connect(client_fd[i], (struct sockaddr*)&serv_addr[i],
                        sizeof(serv_addr[i])))
                < 0) {
                    //perror("connect");
                    continue;
                    //printf("\nConnection Failed \n");
                    //return -1;
                }   
                else{
                    //std::cout<<"newfd : "<<newfd<<"\n";
                    neigh_connections[i] = 1;
                    FD_SET(newfd,&master);
                    if(newfd>fdmax){
                        fdmax = newfd;
                    }
                        //std::cout<<"Enter\n";
                    
                    
                }
                
            }
            else if(neigh_connections[i]==1){
                /*newfd=connect(server_fd, (struct sockaddr*)&serv_addr,
                        sizeof(serv_addr))*/
                std::cout<<"Connection haha\n";
                std::string hello = cl_uni_id + "#"+ owned_files;//"Connected to " + cl_id + " with unique-ID " + cl_uni_id + " on port " + std::to_string(port);
                if(send(client_fd[i], hello.c_str(), strlen(hello.c_str()), 0)<0){
                        perror("send");
                        continue;
                }
                else{
                    char bufff[2];
                    //while(
                    //int n=    recv(client_fd[i],bufff,2,0);//<=0){

                    //}
                    //if(n>0)//std::cout<<"bufff : "<<bufff<<"\n";
                    if(recv(client_fd[i],bufff,2,0)>0){
                        if(bufff[0]=='o'&&bufff[1]=='k'){
                            neigh_connections[i]++;
                            total_conf_msgs++;
                            std::cout<<"Hello: "<<hello<<"\n";
                            std::cout<<"";

                        }
                    }
                    
                        //cl_id++;
                }
                    //break;
            }
            
        }
        if(total_conf_msgs==neigh_num) break;
    } 
    std::vector<int> sock_to_send = {};
    std::vector<int> num_files_to_send = {};
    char buf3[sizeof(uint32_t)];
    char ack4[3] = {'o','k','\0'};
    char ack5[6] = {'r','e','a','d','y','\0'};
    total_conf_msgs = 0;
    int files_to_send = 0;
    for(int i=neigh_num-1;i>=0;i--){
        sleep(0.1);
        struct sockaddr_in sin;
        socklen_t len = sizeof(sin);
        if (getsockname(client_fd[i], (struct sockaddr *)&sin, &len) == -1)
            perror("getsockname");
        //else
        //    printf("port number %d\n", ntohs(sin.sin_port));
        if(send(client_fd[i],ack5,sizeof(ack5),0)<=0){
            std::cout<<"No\n";
            perror("send");
        }
        std::cout<<"";
        std::cout<<"Sent "<<ack5<<" to "<<ntohs(sin.sin_port)<< "\n";
        int recv_int = 0;
        int ret_val = read(client_fd[i], &recv_int, sizeof(recv_int));
        if(ret_val<=0){//recv(client_fd[i],buf3, sizeof(buf3), 0)<=0){
            std::cout<<"Error haha\n";
        }
        else{
            //std::cout<<ntohl(buf3)<<"\n";
            std::cout<<"Received :: "<<recv_int<<"\n";
            //std::string f_to_send((buf3));
            //std::stringstream sstr(f_to_send);
            int files_from_connection = 0;
            //long nl = 0;
            //sstr>>nl;
            files_from_connection = ntohl(recv_int);
            std::cout<<files_from_connection<<" files to be sent to "<<client_fd[i]<<"\n";
            //sock_to_send.push_back(client_fd[i]);
            //num_files_to_send.push_back(files_from_connection);
            files_to_send+=files_from_connection;
            send(client_fd[i],ack4,3,0);
            std::cout<<"Sent" << ack4 << "\n";
            for(int j = 0;j<files_from_connection;j++){
                char ack10[5];

                if(recv(client_fd[i], buf, sizeof(buf),0)>0){
                    std::cout<<"Buffer : "<<buf<<"\n";
                    total_conf_msgs++;
                    std::string buff2(buf);
                    //std::vector<std::string> spl_f_str = {};
                    //std::stringstream s(buff2);          
                    //std::string s2;  
                    //while (std:: getline (s, s2, '#') )  
                    //{  
                    //    spl_f_str.push_back(s2); // store the string in s2  
                    //}  
                    std::string directory2 = directory+buff2;
                    const char* filename = directory2.c_str();
                    //std::stringstream a(spl_f_str[1]);
                    long file_size = GetFileSize(directory2);
                    uint32_t un = htonl(file_size);
                    write(client_fd[i],&un, sizeof(un));
                    //a>>file_size;
                    //FILE* fp = fopen(filename, "rb");
                    std::cout<<directory2<<" "<<file_size<<"\n";
                    //std::cout<<file_size<<"\n";
                    //send(client_fd[i],std::to_string(file_size).c_str(),strlen(std::to_string(file_size).c_str()),0);
                    if(recv(client_fd[i], ack10,5,0)>0){

                    send_file(directory2, client_fd[i], file_size);
                    std::string ack = "!@#$%^&*!@#$%^&*";
                    send(client_fd[i],ack.c_str(),strlen(ack.c_str()),0);
                    std::cout<<"Hi, Sent : "<<filename<<"\n";
                    total_conf_msgs++;
                    }
                }
            }
            //close(client_fd[i]);
        }
        //if(total_conf_msgs==files_to_send) break;

    }

    std::cout<<"Closing Client Thread\n";
    /*while (true)
    {
        if(total_conf_msgs==files_to_send) break;
        //std::cout<<"Waiting for file requests\n";
        for(unsigned int i=0;i<sock_to_send.size();i++){
            for(int j = 0; j<num_files_to_send[i];j++){

                char ack10[5];

                if(recv(sock_to_send[i], buf, sizeof(buf),0)>0){
                    std::cout<<"Buffer : "<<buf<<"\n";
                    total_conf_msgs++;
                    std::string buff2(buf);
                    //std::vector<std::string> spl_f_str = {};
                    //std::stringstream s(buff2);          
                    //std::string s2;  
                    //while (std:: getline (s, s2, '#') )  
                    //{  
                    //    spl_f_str.push_back(s2); // store the string in s2  
                    //}  
                    std::string directory2 = directory+buff2;
                    const char* filename = directory2.c_str();
                    //std::stringstream a(spl_f_str[1]);
                    long file_size = GetFileSize(directory2);
                    //a>>file_size;
                    //FILE* fp = fopen(filename, "rb");
                    std::cout<<directory2<<" "<<file_size<<"\n";
                    //std::cout<<file_size<<"\n";
                    send(sock_to_send[i],std::to_string(file_size).c_str(),strlen(std::to_string(file_size).c_str()),0);
                    if(recv(sock_to_send[i], ack10,5,0)>0){

                    send_file(directory2, sock_to_send[i], file_size);
                    std::string ack = "!@#$%^&*!@#$%^&*";
                    send(client_fd[i],ack.c_str(),strlen(ack.c_str()),0);
                    std::cout<<"Hi, Sent : "<<filename<<"\n";
                    total_conf_msgs++;
                    }
                }

            }
        }
        

    }*/
    

        

}




int main(int argc, char* argv[]){

    //TODO handle all the arguments, input files and directory
    //std::cout<<argv[1]<<"\n"<<argv[2]<<"\n";

    int neigh_num,file_num;
    int* file_found;
    std::string cl_port2, cl_id, cl_uni_id, owned_files;
    owned_files = "";
    std::string *neigh_id_arr, *neigh_port_arr ;
    //bool* neigh_connections;
    std::string *req_files;
    std::ifstream config_file;
    config_file.open(argv[1]);
    config_file>>cl_id>>cl_port2>>cl_uni_id>>neigh_num;
    const char* cl_port = cl_port2.c_str();
    int cl_port3;
    std::stringstream client(cl_port2);
    client>>cl_port3;
    //std::cout<<cl_port3<<"\n";
    //std::cout<<cl_id<<"\n"<<cl_port<<"\n"<<cl_uni_id<<"\n"<<neigh_num<<"\n";
    neigh_id_arr = new std::string[neigh_num];
    neigh_port_arr = new std::string[neigh_num]; 
    //neigh_connections = new bool[neigh_num];
    
    for(int i=0;i<neigh_num;i++){
        config_file>>neigh_id_arr[i];
        config_file>>neigh_port_arr[i];
        //std::cout<<neigh_id_arr[i]<<"\n"<<neigh_port_arr[i]<<"\n";
    }
    config_file>>file_num;
    file_found = new int[file_num];
    req_files = new std::string[file_num];
    std::vector<std::string> req_files_temp={};
    std::string temp_str="";
    for(int i=0;i<file_num;i++){
        config_file>>temp_str;
        req_files_temp.push_back(temp_str);
        file_found[i] = 0;
        temp_str="";
    }

    std::sort(req_files_temp.begin(), req_files_temp.end());
    for(int i=0;i<file_num;i++){
        req_files[i] = req_files_temp[i];
    }

    //for(int i=0;i<neigh_num;i++){
    //    std::cout<<neigh_id_arr[i]<<" "<<neigh_port_arr[i]<<"\n";//<<neigh_connections[i]<<"\n";
    //}

    DIR *d;
    struct dirent *dir;
    d = opendir(argv[2]);
    if (d!=NULL)
    {
        while ((dir = readdir(d)) != NULL)
        {
            std::string y(dir->d_name);
            if(y[0]=='.'||y=="Downloaded") continue;
            owned_files+=y;
            owned_files+="#";
            //printf("%s\n", dir->d_name);
        }
        closedir(d);
    }


    std::string direc (argv[2]);
    direc+="/Downloaded";
    struct stat st = {0};

    if (stat(direc.c_str(), &st) == -1) {
        mkdir(direc.c_str(), 0700);
    }

    std::thread s(server_imp, cl_port3, neigh_num, req_files, file_found, file_num, argv[2]);
    std::thread c(client_imp, cl_id, cl_uni_id, cl_port3, neigh_num, neigh_port_arr, neigh_id_arr, owned_files, argv[2]);

    s.join();
    c.join();

        

    


        //set server_fd in readset
        

 // END for(;;)--and you thought it would never end!
 

    return(0);

}