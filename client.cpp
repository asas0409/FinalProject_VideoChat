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
#include <opencv2/opencv.hpp>
#include <portaudio.h>

#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 2048
#define BUF_SIZE 30000

void error_handling(char *message);

typedef struct {
    int type;
    char buf[BUF_SIZE];
} Packet;

class Client : public std::enable_shared_from_this<Client> {
public:
    int sock;
    char id[20];
    int join = 0;
    std::string message;

    Packet packet_snd;
    Packet packet_rcv;
    Packet tmp;

    PaStreamParameters inputParameters;
    PaStreamParameters outputParameters;
    PaStream *in_stream;
    PaStream *out_stream;

    Client(int socket, char* id_i){
        sock = socket;
        strcpy(id, id_i);
        memset(&packet_snd, 0, sizeof(packet_snd));

        memset(&inputParameters, 0, sizeof(inputParameters));
        inputParameters.channelCount = 2;
        inputParameters.device = 0;
        inputParameters.hostApiSpecificStreamInfo = NULL;
        inputParameters.sampleFormat = paFloat32;
        inputParameters.suggestedLatency = Pa_GetDeviceInfo(0)-> defaultLowInputLatency;
        
        memset(&outputParameters, 0, sizeof(outputParameters));
        outputParameters.channelCount = 2;
        outputParameters.device = 1;
        outputParameters.hostApiSpecificStreamInfo = NULL;
        outputParameters.sampleFormat = paFloat32;
        outputParameters.suggestedLatency = Pa_GetDeviceInfo(1)-> defaultLowInputLatency;

        Pa_OpenStream(&in_stream,
        &inputParameters,
        NULL,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paNoFlag,
        patestCallback,
        this);

        Pa_OpenStream(&out_stream,
        NULL,
        &outputParameters,
        SAMPLE_RATE,
        FRAMES_PER_BUFFER,
        paNoFlag,
        NULL,
        this);
    }

    static int patestCallback(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
    void* userData) {
        memset(((Client*)userData)->packet_snd.buf, 0, sizeof(packet_snd));
        ((Client*)userData) -> packet_snd.type = 2;
        memcpy(((Client*)userData)->packet_snd.buf, inputBuffer, FRAMES_PER_BUFFER*2*4);
        write(((Client*)userData)->sock, &(((Client*)userData)->packet_snd), sizeof(packet_snd));

        return 0;
    }

    bool read_all() {
        int sum = 0;
        Packet * p = &packet_rcv;
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
        bool isOut = false;
        // send id after connection
        memset(&packet_snd, 0, sizeof(packet_snd));
        packet_snd.type = 0;
        memcpy(packet_snd.buf, id, strlen(id));
        write(sock, &packet_snd, sizeof(packet_snd));

        // print usage
        memset(&packet_rcv, 0, sizeof(packet_rcv));
        read_all();
        std::cout << packet_rcv.buf << std::endl;

        //start seding thread
        std::thread sending_thread{&Client::sending,this};
        sending_thread.detach();

        while(1){
            //command
            if(join == 0){
                if(isOut) {
                    while(true) {
                        if(!read_all()) {std::cout << "read_all() problem\n"; break;}
                        if(packet_rcv.type == 5) break;
                    }
                    std::cout << "You can do again\n";
                    isOut = false;
                }
                //send command
                std::getline(std::cin, message);
                memset(&packet_snd, 0 , sizeof(packet_snd));
                packet_snd.type = 0;
                strcpy(packet_snd.buf, message.c_str());
                write(sock, &packet_snd, sizeof(packet_snd));
                //get reply from server and take proper action
                memset(&packet_rcv, 0, sizeof(packet_rcv));
                // get packet
                read_all();

                if(packet_rcv.type == 4){
                    printf("join to room\n");
                    join = 1;
                    Pa_StartStream(in_stream);
                    Pa_StartStream(out_stream);
                } else if(packet_rcv.type == 3) {
                    // Show list
                    std::cout << packet_rcv.buf << std::endl;
                } else {
                    std:: cout << "wrong : " << packet_rcv.buf << std::endl;
                }
            }
            // if joined -> work as receiver
            else if(join == 1){
                memset(&packet_rcv, 0, sizeof(packet_rcv));
                read_all();

                if(packet_rcv.type == 1){
                    std::vector<uchar> da(packet_rcv.buf, packet_rcv.buf+BUF_SIZE);
                    cv::Mat jpegimage = cv::imdecode(da, cv::IMREAD_COLOR);
                    if(jpegimage.data) {
                        cv::imshow(id,jpegimage);
                    } else {
                        std::cout << "img empty\n";
                    }
                    if(cv::waitKey(1) == 27){
                        memset(&packet_snd, 0, sizeof(packet_snd));
                        packet_snd.type = 7;
                        write(sock, &packet_snd, sizeof(packet_snd));
                        join = 0;
                        Pa_StopStream(in_stream);
                        Pa_StopStream(out_stream);
                        cv::destroyAllWindows();
                        cv::waitKey(1);
                        isOut = true;
                        std::cout << "chatroom exit\n";
                    }
                }else if(packet_rcv.type==2){
                    Pa_WriteStream(out_stream, packet_rcv.buf, FRAMES_PER_BUFFER);
                } else if(packet_rcv.type == 6) {
                    std::cout << "opponent out\n";
                    cv::destroyAllWindows();
                    cv::waitKey(1);
                }
            }
        }
    }

    void sending(){
        cv::VideoCapture cap(0);
        cv::Mat frame;
        Packet packet_send;
        int num = 0;
        while(1){
            if(join == 1){
                if(num++ == 0)
                    std :: cout << "start to send" << std ::endl;
                cap >> frame;
                cv::resize(frame, frame, cv::Size(640, 480));
                // Compression
                std::vector<uchar> buff;
                std::vector<int> param = std::vector<int>(2);
                param[0] = cv::IMWRITE_JPEG_QUALITY;
                param[1] = 50;//default(95) 0-100
                cv::imencode(".jpg", frame, buff, param);
                //std::cout << "coded file size(jpg)" << frame.size() << "->" << buff.size() << std::endl;

                memset(&packet_send, 0, sizeof(packet_send));
                packet_send.type = 1;
                memcpy(packet_send.buf, buff.data(), BUF_SIZE);
                write(sock, &packet_send, sizeof(packet_send));
            }
        }
    }
};

static void checkErr(PaError err){
    if(err != paNoError){
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_adr;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));
    connect(sock, (struct sockaddr*) &serv_adr, sizeof(serv_adr));

    PaError err;
    err = Pa_Initialize();
    checkErr(err);

    Client cli{sock, argv[3]};
    cli.run();
    return 0;

    
}