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

#define MAXLINE 1024


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}




int main(int argc, char* argv[]){

    //TODO handle all the arguments, input files and directory
    //std::cout<<argv[1]<<"\n"<<argv[2]<<"\n";

    int cl_id, cl_uni_id, neigh_num,file_num;
    std::string cl_port2;
    std::string *neigh_id_arr, *neigh_port_arr ;
    bool* neigh_connections;
    std::string *req_files;
    std::ifstream config_file;
    config_file.open(argv[1]);
    config_file>>cl_id>>cl_port2>>cl_uni_id>>neigh_num;
    const char* cl_port = cl_port2.c_str();
    int cl_port3;
    std::stringstream client(cl_port2);
    client>>cl_port3;
    std::cout<<cl_port3<<"\n";
    //std::cout<<cl_id<<"\n"<<cl_port<<"\n"<<cl_uni_id<<"\n"<<neigh_num<<"\n";
    neigh_id_arr = new std::string[neigh_num];
    neigh_port_arr = new std::string[neigh_num]; 
    neigh_connections = new bool[neigh_num];
    
    for(int i=0;i<neigh_num;i++){
        config_file>>neigh_id_arr[i];
        config_file>>neigh_port_arr[i];
        neigh_connections[i] = false;
        //std::cout<<neigh_id_arr[i]<<"\n"<<neigh_port_arr[i]<<"\n";
    }
    config_file>>file_num;
    req_files = new std::string[file_num];
    for(int i=0;i<file_num;i++){
        config_file>>req_files[i];
    }

    for(int i=0;i<neigh_num;i++){
        std::cout<<neigh_id_arr[i]<<" "<<neigh_port_arr[i]<<"\n";//<<neigh_connections[i]<<"\n";
    }

    DIR *d;
    struct dirent *dir;
    d = opendir(argv[2]);
    if (d!=NULL)
    {
        while ((dir = readdir(d)) != NULL)
        {
            if((dir->d_name)[0]=='.') continue;
            printf("%s\n", dir->d_name);
        }
        closedir(d);
    }

    int server_fd, new_socket, valread, newfd, fdmax, nbytes;
    int client_fd[neigh_num];
    int connfd, nready, maxfdp1;
    struct sockaddr_in address;
    struct sockaddr_in cliaddr;
    struct sockaddr_storage remoteaddr;
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    socklen_t addrlen;
    fd_set rset;
    char remoteIP[INET6_ADDRSTRLEN];
    char buf[256];    // buffer for client data

    int opt = 1;
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
    address.sin_port = htons(cl_port3);
    // Forcefully attaching socket to the client port
    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in serv_addr[neigh_num];

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
        address.sin_addr.s_addr = inet_addr("127.0.0.1");
        address.sin_port = htons(cl_port3);

        if (bind(client_fd[i], (struct sockaddr*)&address,
                sizeof(address))
            < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }
        memset(&serv_addr[i], 0, sizeof(serv_addr[i]));
        serv_addr[i].sin_family = AF_INET;
        serv_addr[i].sin_addr.s_addr = inet_addr("127.0.0.1");
        
        std::stringstream tmp(neigh_port_arr[i]);
        int neighport;
        tmp>>neighport;
        //std::cout<<neighport<<"\n";
        serv_addr[i].sin_port = htons(neighport);
        
        //std::cout<<serv_addr.sin_port<<"\n";
    }
    //std::cout<<address.sin_port<<"\n";

    

    newfd  = 0;
    fdmax = 0;

    // clear the descriptor set
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(server_fd,&master);
    fdmax = server_fd;


    /*for(int i=0;i<neigh_num;i++){
        if(neigh_connections[i]==false){
            std::cout<<i<<std::endl;
            std::cout<<"Hi\n";
            std::string hello = "Hello from client" + neigh_id_arr[i];
            struct sockaddr_in serv_addr;
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            memset(&serv_addr, 0, sizeof(serv_addr));
            std::cout<<"hi1\n";
            std::stringstream tmp(neigh_port_arr[i]);
            int neighport;
            tmp>>neighport;
            serv_addr.sin_port = htons(neighport);
            std::cout<<server_fd<<"\n";

            if ((newfd=connect(server_fd, (struct sockaddr*)&serv_addr,
                    sizeof(serv_addr)))
            < 0) {
                perror("connect");
                continue;
                //printf("\nConnection Failed \n");
                //return -1;
            }   
            else{
                
                neigh_connections[i] = true;
                FD_SET(newfd,&master);
                if(newfd>fdmax){
                    fdmax = newfd;
                }
                    std::cout<<"Enter\n";//<<newfd<<"\n";
                
                //if(send(server_fd, hello.c_str(), strlen(hello.c_str()), 0)<0){
                //    std::cout<<"err\n";
                //    perror("send");
                //}
                std::cout<<"hola!\n";
                //printf("Hello message sent\n");
            }
            
        }

    }*/

    

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }


    while(true){


        for(int i=0;i<neigh_num;i++){
            if(neigh_connections[i]==false){
                std::cout<<i<<std::endl;
                std::cout<<"Hi\n";
                
                
                std::cout<<"hi1\n";

                if ((newfd=connect(client_fd[i], (struct sockaddr*)&serv_addr[i],
                        sizeof(serv_addr[i])))
                < 0) {
                    perror("connect");
                    continue;
                    //printf("\nConnection Failed \n");
                    //return -1;
                }   
                else{
                    
                    neigh_connections[i] = true;
                    FD_SET(newfd,&master);
                    if(newfd>fdmax){
                        fdmax = newfd;
                    }
                        std::cout<<"Enter\n";
                    
                    
                }
                
            }
            else{
                /*newfd=connect(server_fd, (struct sockaddr*)&serv_addr,
                        sizeof(serv_addr))*/
                std::cout<<"haha\n";
            std::string hello = "Hello from client" + neigh_id_arr[i];
            if(send(client_fd[i], hello.c_str(), strlen(hello.c_str()), 0)<0){
                        perror("send");
                    }
                    printf("Hello message sent\n");
                    //cl_id++;
                    //break;
            }
            
        }

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
                    std::cout<<&remoteaddr<<"\n"<<&addrlen<<"\n"<<server_fd<<"\n";//<<remoteaddr<<"\n"<<addrlen<<"\n";
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
                    if ((nbytes = recv(newfd, buf, sizeof (buf), 0)) <= 0) {
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
                        std::cout<<buf<<"\n";
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
 

    /*if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }*/


        

    


        //set server_fd in readset
        

           } // END for(;;)--and you thought it would never end!
 

    return(0);

}