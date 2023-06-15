/*
    <type>
    0 : command
        .[num] : enter
        +[num] : create
    1 : video
    2 : audio
    3 : Chatroom list
    4 : Chatroom request - make or enter
    5 : Exit ACK
    6 : Opponent out
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <iostream>
#include <vector>
#include <thread>

#define BUF_SIZE 30000

class Client{
    public:
        char id[20];
        int my_sock;
        int current_room = -1;
};

typedef struct{
    int type;
    char buf[BUF_SIZE];
}Packet;

class Server : public std::enable_shared_from_this<Server>{
    public:
    int serv_sock, clnt_sock;
    int optval = 1;
    Packet packet_rcv;
    Packet packet_snd;
    Packet packet_tmp;
    struct sockaddr_in serv_adr;
    struct sockaddr_in clnt_adr;
    socklen_t clnt_adr_sz;
    std::vector<Client*> client_list;
    std::vector<int> rooms;
    int port_;

    Server(int port){
        port_ = port;
    }

    void display_room_list(Client *client){
        Packet packet_send;
        memset(&packet_send, 0, sizeof(packet_send));
        packet_send.type = 3;
        std::string list = "==========\n";
        for(auto room : rooms){
            list = list + "[" + std::to_string(room) + "] ";
        }
        list = list + "\n=========\n";
        strcpy(packet_send.buf, list.c_str());
        write(client->my_sock, &packet_send, sizeof(packet_send));
    }

    bool read_all(Packet &rcv, Packet &tmp, int sock) {
        int sum = 0;
        Packet * p = &rcv;
        while(sum < BUF_SIZE + 4) {
            int res = read(sock, &tmp, sizeof(tmp)-sum);
            if(res == 0) {
                return false;
            }
            memcpy((char*)p+sum, &tmp, res);
            sum += res;
        }
        return true;
    }

    void run(){

        serv_sock = socket(PF_INET, SOCK_STREAM, 0);   
        if (serv_sock == -1) {
            printf("socket() error\n");
            exit(1);
        }
   
        memset(&serv_adr, 0, sizeof(serv_adr));
        serv_adr.sin_family = AF_INET;
        serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
        serv_adr.sin_port = htons(port_);

        setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
            printf("bind() error\n");
            exit(1);
        }
   
        if (listen(serv_sock, 5) == -1) {
            printf("listen() error\n");
            exit(1);
        }
   
        clnt_adr_sz = sizeof(clnt_adr);

        while(true){
            clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
            Client* client = new Client();
            client -> my_sock = clnt_sock;
            printf("New client connected\n");

            // get id
            read_all(packet_rcv, packet_tmp, clnt_sock);
            strcpy(client->id, packet_rcv.buf);
            // printf("get id\n");
            client_list.push_back(client);

            // usage
            memset(&packet_snd, 0 ,sizeof(packet_snd));
            strcpy(packet_snd.buf,
            "==========USAGE========\n"
            "\'show\' to get room list\n"
            ". to enter room (ex : .0)\n"
            "+ to create toom (ex : +0)\n"
            );
            write(clnt_sock, &packet_snd, sizeof(packet_snd));

            std::thread session_thread{&Server::session, this, client};
            session_thread.detach();
        }
    }

    void session(Client* client){
        Packet tmp;
        Packet packet_r;
        Packet packet_s;

        while(1){
            memset(&packet_r, 0, sizeof(packet_r));
            try{
                if(!read_all(packet_r, tmp, client->my_sock)) {
                    std::cout << "Client connection end\n";
                    close(client->my_sock);
                    break;
                }
            }catch(...){
                printf("read_all() error\n");
            }
            //printf("%s %d\n", client->id, packet_r.type);
            //message type이 0 : command인 경우
            if(packet_r.type == 0){
                //for show list
                if(strncmp(packet_r.buf, "show", 4) == 0) {
                    // printf("show list");
                    display_room_list(client);
                //for entering room
                } else if(packet_r.buf[0] == '.' && client->current_room == -1){
                    // printf("try to join\n");
                    std::string num(packet_r.buf);
                    int want;
                    memset(&packet_s, 0, sizeof(packet_s));
                    try{
                        want = std::stoi(num.substr(1));
                        if(std::find(rooms.begin(), rooms.end(), want) != rooms.end()){
                            client -> current_room = want;
                            packet_s.type = 4;
                        }else{
                            strcpy(packet_s.buf, ("There is no room with no." + std::to_string(want)).c_str());
                        }
                    }catch(...){
                        strcpy(packet_s.buf, "wrong room number");
                    }
                    write(client->my_sock, &packet_s, sizeof(packet_s));
                }
                //for making room
                else if(packet_r.buf[0] == '+' && client->current_room == -1){
                    std::string num(packet_r.buf);
                    memset(&packet_s, 0, sizeof(packet_s));
                    int want = std::stoi(num.substr(1));
                    if(std::find(rooms.begin(), rooms.end(), want)!=rooms.end()){
                        strcpy(packet_s.buf, ("Room " + std::to_string(want) + " already exist!\n").c_str());
                    }else{
                        // printf("make room\n");
                        rooms.push_back(want);
                        client->current_room = want;
                        packet_s.type = 4;
                    }
                    write(client->my_sock, &packet_s, sizeof(packet_s));
                // for exit
                }
                // error
                else{
                    memset(&packet_s, 0, sizeof(packet_s));
                    strcpy(packet_s.buf,"wrong input");
                    write(client->my_sock, &packet_s, sizeof(packet_s));
                }

            // media data processing
            } else if((packet_r.type == 1 || packet_r.type == 2) && client->current_room != -1){
                for(auto other : client_list){
                    if(client->current_room == other->current_room && (other->my_sock != client->my_sock)){
                        write(other->my_sock, &packet_r, sizeof(packet_r));
                    }
                }
            }else if((packet_r.type == 7)&& (client->current_room != -1)){
                    printf("client has been left\n");
                    memset(&packet_s, 0, sizeof(packet_s));
                    packet_s.type = 5;
                    write(client->my_sock, &packet_s, sizeof(packet_s));
                    packet_s.type = 6;
                    for(auto other : client_list){
                        if(client->current_room == other->current_room && (other->my_sock != client->my_sock)){
                            write(other->my_sock, &packet_s, sizeof(packet_s));
                        }
                    }
                    client->current_room = -1;
                    
                }
        }
    }
};


int main(int argc, char *argv[])
{
   Server a_server{atoi(argv[1])};
   a_server.run();
   return 0;
}
