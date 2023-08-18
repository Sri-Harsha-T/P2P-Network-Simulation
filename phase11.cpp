#include <iostream>
#include <fstream>
#include <string>
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
#include <vector>
#include <algorithm>

#define MAXLINE 1024


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void server_imp(int port, int neigh_num){

    int server_fd, new_socket, valread, newfd, fdmax, nbytes;
    int connfd, nready, maxfdp1;
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
    std::vector<int> recvsock = {};

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

    std::vector<std::string> recevied_msgs = {};

    while(true){

        sleep(0.1);
        

        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
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
                        //std::cout<<"Accepted "<< newfd << " " << fdmax << "\n";
                        //printf("selectserver: new connection from %s on "    "socket %d\n",);
                    }
                } else {
                    // handle data from a client
                    //std::cout << "Trying to receive from " << i<<"\n";
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
                        //if(nbytes>3){
                            std::string s(buf);
                            recevied_msgs.push_back(s);
                            //std::cout<<buf<<"\n";
                            //send(i,"ack",3,0);
                            //recvsock.push_back(i);
                            close(i); // bye!    
                            FD_CLR(i, &master); // remove from master set
                            total_recv_connections++;                        
                        //}
                        //close(i); // bye!    
                        //    FD_CLR(i, &master); // remove from master set
                        
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
        //std::cout<<total_recv_connections<<"\n";

    /*if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }*/
        if(total_recv_connections == neigh_num){
            break;
        }


    }
    std::sort(recevied_msgs.begin(), recevied_msgs.end());
    for(unsigned int i = 0; i<recevied_msgs.size();i++){
        std::cout<<recevied_msgs[i]<<"\n";
    }
    //for(unsigned int i=0;i<recvsock.size();i++){
    //    close(i);
    //    FD_CLR(i, &master);
    //}
}

void client_imp(std::string cl_id, std::string cl_uni_id, int port, int neigh_num,std::string* neigh_port_arr, std::string* neigh_id_arr){

    short int* neigh_connections;
    struct sockaddr_in address;
    int client_fd[neigh_num];
    int newfd = 0, opt = 1, fdmax = 0;
    fd_set master;
    FD_ZERO(&master);
    char buff[MAXLINE];
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

        /*if (bind(client_fd[i], (struct sockaddr*)&address,
                sizeof(address))
            < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }*/
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
        sleep(0.1);
        for(int i=0;i<neigh_num;i++){
            if(neigh_connections[i]==0){
                //std::cout<<i<<std::endl;
                //std::cout<<"Hi\n";
                
                
                //std::cout<<"hi1\n";

                if ((newfd=connect(client_fd[i], (struct sockaddr*)&serv_addr[i],
                        sizeof(serv_addr[i])))
                < 0) {
                    //perror("connect");
                    //std::cout<<"Connect Error\n";
                    break;
                    //continue;
                    //printf("\nConnection Failed \n");
                    //return -1;
                }   
                else{
                    //std::cout<<"Connected to "<<neigh_port_arr[i]<<"\n";
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
                //std::cout<<"haha\n";
                std::string hello = "Connected to " + cl_id + " with unique-ID " + cl_uni_id + " on port " + std::to_string(port);//+"\n";
                if(send(client_fd[i], hello.c_str(), strlen(hello.c_str()), 0)<0){
                        perror("send");
                        continue;;
                }
                neigh_connections[i]++;
                total_conf_msgs++;
                std::cout<<"";
                //if(recv(client_fd[i],buff,MAXLINE,0)==3){
                //    std::cout<<"ack recieved "<<buff<<"\n";
                //    close(client_fd[i]);
                //};
                //close(client_fd[i]);
                    //cl_id++;
                    //break;
            }
            
        }
        if(total_conf_msgs==neigh_num) break;
    }
    //for(int i=0;i<neigh_num;i++){
    //    close(client_fd[i]);
    //}
        
    //return;
}




int main(int argc, char* argv[]){

    //TODO handle all the arguments, input files and directory
    //std::cout<<argv[1]<<"\n"<<argv[2]<<"\n";

    int neigh_num,file_num;
    std::string cl_port2, cl_id, cl_uni_id;
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
    req_files = new std::string[file_num];
    for(int i=0;i<file_num;i++){
        config_file>>req_files[i];
    }

    //for(int i=0;i<neigh_num;i++){
    //    std::cout<<neigh_id_arr[i]<<" "<<neigh_port_arr[i]<<"\n";//<<neigh_connections[i]<<"\n";
    //}

    std::vector<std::string> files;

    DIR *d;
    struct dirent *dir;
    d = opendir(argv[2]);
    if (d!=NULL)
    {
        while ((dir = readdir(d)) != NULL)
        {
            std::string y(dir->d_name);
            if(y[0]=='.'|| y=="Downloaded") continue;
            //std::string x(dir->d_name);
            files.push_back(y);
            //printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
    std::sort(files.begin(), files.end());
    for(unsigned int i=0;i<files.size();i++){
        std::cout<<files[i]<<"\n";
    }
    std::thread s(server_imp, cl_port3, neigh_num);
    std::thread c(client_imp, cl_id, cl_uni_id, cl_port3, neigh_num, neigh_port_arr, neigh_id_arr);

    s.join();
    c.join();

        

    


        //set server_fd in readset
        

 // END for(;;)--and you thought it would never end!
 

    return(0);

}