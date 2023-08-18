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

#define MAXLINE 1024

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

void checkfiles(char* buf, std::string* req_files, int* file_found, int file_num){
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
                }
            }
        }
    }
}


void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void server_imp(int port, int neigh_num, std::string* req_files, int* file_found, int file_num){

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

    

    newfd  = 0;
    fdmax = 0;

    // clear the descriptor set
    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    FD_SET(server_fd,&master);
    fdmax = server_fd;

    

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::vector<std::string> received_msgs;

    while(true){


        

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
                        //std::cout<<buf<<"\n";
                        std::vector<std::string> t = split_file_str(std::string(buf));
                        std::string a  = "Connected to " + t[0] + " with unique-ID " + t[1] + " on port " + t[2];
                        received_msgs.push_back(a);
                        std::string b = "";
                        for(unsigned int i = 3;i<t.size();i++){
                            b+=t[i];
                            b+="#";
                        }
                        char *cstr = new char[b.length() + 1];
                        strcpy(cstr, b.c_str());
                        checkfiles(cstr, req_files, file_found, file_num);
                        close(i); // bye!
                        FD_CLR(i, &master); // remove from master set
                        total_recv_connections++;
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
 

    /*if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }*/

    
        if(total_recv_connections == neigh_num){
            std::sort(recevied_msgs.begin(), recevied_msgs.end());
            for(unsigned int i = 0; i<recevied_msgs.size();i++){
                std::cout<<recevied_msgs[i]<<"\n";
            }

            for(int i=0;i<file_num;i++){
                if(file_found[i]>0){
                    std::cout<<"Found "<<req_files[i]<<" at "<<file_found[i]<<" with MD5 0 at depth 1\n";
                }
                else{
                    std::cout<<"Found "<<req_files[i]<<" at "<<file_found[i]<<" with MD5 0 at depth 0\n";
                }
            }
            return;
        }


    }
}

void client_imp(std::string cl_id, std::string cl_uni_id, int port, int neigh_num,std::string* neigh_port_arr, std::string* neigh_id_arr, std::string owned_files){

    short int* neigh_connections;
    struct sockaddr_in address;
    int client_fd[neigh_num];
    int newfd = 0, opt = 1, fdmax = 0;
    fd_set master;
    FD_ZERO(&master);
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
                std::string hello = cl_id+"#"+cl_uni_id + "#" + std::to_string(port)+"#"+ owned_files;//"Connected to " + cl_id + " with unique-ID " + cl_uni_id + " on port " + std::to_string(port);
                if(send(client_fd[i], hello.c_str(), strlen(hello.c_str()), 0)<0){
                        perror("send");
                        continue;
                }
                neigh_connections[i]++;
                total_conf_msgs++;
                std::cout<<"";
                    //cl_id++;
                    //break;
            }
            
        }
        if(total_conf_msgs==neigh_num) break;
    }

        

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
    std::vector<std::string> files;
    DIR *d;
    struct dirent *dir;
    d = opendir(argv[2]);
    if (d!=NULL)
    {
        while ((dir = readdir(d)) != NULL)
        {
            std::string y(dir->d_name);
            if(y[0]=='.'||y=="Downloaded") continue;
            files.push_back(y);
            owned_files+=y;
            owned_files+="#";
            //printf("%s\n", dir->d_name);
        }
        closedir(d);
    }
    std::sort(files.begin(), files.end());
    for(unsigned int i=0;i<files.size();i++){
        std::cout<<files[i]<<"\n";
    }

    std::thread s(server_imp, cl_port3, neigh_num, req_files, file_found, file_num);
    std::thread c(client_imp, cl_id, cl_uni_id, cl_port3, neigh_num, neigh_port_arr, neigh_id_arr, owned_files);

    s.join();
    c.join();

        

    


        //set server_fd in readset
        

 // END for(;;)--and you thought it would never end!
 

    return(0);

}