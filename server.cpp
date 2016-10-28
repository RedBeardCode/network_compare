#include <zmq.hpp>
#include <iostream>
#include <unistd.h>
#include <memory>
#include <chrono>
#include <thread>
#include "sys/times.h"
#ifndef D3TEST
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <udt/udt.h>
#endif
#include <boost/asio.hpp>

using namespace boost::asio::ip;

static clock_t lastCPU, lastSysCPU, lastUserCPU;
static int numProcessors;

void init_cpu(){
    FILE* file;
    struct tms timeSample;
    char line[128];

    lastCPU = times(&timeSample);
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    file = fopen("/proc/cpuinfo", "r");
    numProcessors = 0;
    while(fgets(line, 128, file) != NULL){
        if (strncmp(line, "processor", 9) == 0) numProcessors++;
    }
    fclose(file);
}

double getCurrentValue(){
    struct tms timeSample;
    clock_t now;
    double percent;

    now = times(&timeSample);
    if (now <= lastCPU || timeSample.tms_stime < lastSysCPU ||
        timeSample.tms_utime < lastUserCPU){
        //Overflow detection. Just skip this value.
        percent = -1.0;
    }
    else{
        percent = (timeSample.tms_stime - lastSysCPU) +
                  (timeSample.tms_utime - lastUserCPU);
        percent /= (now - lastCPU);
        percent /= numProcessors;
        percent *= 100;
    }
    lastCPU = now;
    lastSysCPU = timeSample.tms_stime;
    lastUserCPU = timeSample.tms_utime;

    return percent;
}





bool gMeasure = true;
std::vector<long double> cpu_samples;

void report_statistics(const size_t cdata_size, const std::chrono::steady_clock::time_point &start_time,
                       const std::chrono::steady_clock::time_point &end_time, size_t num_packages)
{
    double duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count() / 1000.0;
    double trans_data_size = cdata_size * num_packages / 10e6;
    std::cout << "Transfered " << trans_data_size << "MBytes in " << duration << "s, Transfer rate  " << trans_data_size / duration;
    std::cout << "MByte/s, " << trans_data_size / duration * 8 << "MBit/s" << std::endl;
    std::cout << "Number of cpu samples :" << cpu_samples.size() << std::endl;
    long double cpu_average = accumulate(cpu_samples.begin(), cpu_samples.end(), 0) / cpu_samples.size();
    std::cout << "Average CPU load: " << cpu_average << "%" << std::endl;
}

void sample_cpu_load()
{
    init_cpu();
    gMeasure = true;
    while(gMeasure)
    {
        cpu_samples.push_back(getCurrentValue());
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

    }
}


void dummy_free(void *data, void *hint){};

int main() {


    const size_t cdata_size = 4000*3000;
    std::shared_ptr<char> data(new char[cdata_size]);
    std::shared_ptr<std::thread> cpu_thread;
    std::chrono::steady_clock::time_point start_time, end_time;
    size_t num_packages=0;

    // ZEROMQ
    zmq::context_t context (1);
    zmq::socket_t zero_socket (context, ZMQ_REP);
    zero_socket.bind ("tcp://*:5555");
    while (true) {
        zmq::message_t request;

        //  Wait for next request from client
        zero_socket.recv (&request);
        if(cpu_thread == NULL)
        {
            cpu_thread = std::shared_ptr<std::thread>(new std::thread(sample_cpu_load));
            start_time = std::chrono::steady_clock::now();
        }
        if(reinterpret_cast<char*>(request.data())[0] == ' ')
        {
            gMeasure = false;
            cpu_thread->join();
            end_time = std::chrono::steady_clock::now();

            break;
        }
        //  Send reply back to client
        zmq::message_t reply(cdata_size);
        zmq_msg_init_data(reinterpret_cast<zmq_msg_t*>(&reply), data.get(), cdata_size, dummy_free, NULL);
        zero_socket.send (reply);
        num_packages++;
    }

    std::cout << "=== ZEROMq ===" << std::endl;
    report_statistics(cdata_size, start_time, end_time, num_packages);
#ifndef D3TEST
    // NANOMSG
    int nano_socket = nn_socket (AF_SP, NN_PAIR);
    num_packages = 0;
    nn_bind (nano_socket, "tcp://*:5556");
    cpu_thread = NULL;
    while (true) {

        char *buf = NULL;
        nn_recv (nano_socket, &buf, NN_MSG, 0);
        if(cpu_thread == NULL)
        {
            cpu_thread = std::shared_ptr<std::thread>(new std::thread(sample_cpu_load));
            start_time = std::chrono::steady_clock::now();
        }
        if(buf[0] == ' ')
        {
            gMeasure = false;
            cpu_thread->join();
            end_time = std::chrono::steady_clock::now();
            break;
        }
        //  Send
        nn_freemsg (buf);
        nn_send (nano_socket, data.get(), cdata_size, 0);
        num_packages++;
    }

    std::cout << "=== NANOMSG ===" << std::endl;
    report_statistics(cdata_size, start_time, end_time, num_packages);

    // UDT
    UDTSOCKET udt_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
    num_packages = 0;
    sockaddr_in my_addr;
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(5557);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8);
    UDT::ERROR == UDT::bind(udt_socket, (sockaddr*)&my_addr, sizeof(my_addr));
    int e = UDT::listen(udt_socket, 10);

    int namelen;
    sockaddr_in their_addr;

    UDTSOCKET recver = UDT::accept(udt_socket, (sockaddr*)&their_addr, &namelen);

    cpu_thread = NULL;
    while (true) {

        char buf[20];
        UDT::recv(recver, buf, 5, 0);
        if(cpu_thread == NULL)
        {
            cpu_thread = std::shared_ptr<std::thread>(new std::thread(sample_cpu_load));
            start_time = std::chrono::steady_clock::now();
        }
        if(buf[0] == ' ')
        {
            gMeasure = false;
            cpu_thread->join();
            end_time = std::chrono::steady_clock::now();
            break;
        }
        //  Send

        size_t data_size = 0;
        int chunk_size=0;
        while (data_size < cdata_size) {
            chunk_size = UDT::send(recver, &data.get()[data_size], cdata_size - data_size, 0);
            data_size += chunk_size;
            if (UDT::ERROR == chunk_size) {
                std::cout << "send: " << UDT::getlasterror().getErrorMessage();
                return 0;
            }
        }

        num_packages++;
    }

    std::cout << "=== UDT ===" << std::endl;
    report_statistics(cdata_size, start_time, end_time, num_packages);
#endif
    //Boost.ASIO
    boost::asio::io_service io_service;
    tcp::acceptor a(io_service, tcp::endpoint(tcp::v4(), 5558));
    num_packages = 0;
    cpu_thread = NULL;
    tcp::socket sock(io_service);
    a.accept(sock);
    while (true) {

        char buf[20];

        boost::system::error_code error;
        size_t length = sock.read_some(boost::asio::buffer(buf), error);
        if (error == boost::asio::error::eof)
            break; // Connection closed cleanly by peer.
        else if (error)
            throw boost::system::system_error(error); // Some other error.
        if(cpu_thread == NULL)
        {
            cpu_thread = std::shared_ptr<std::thread>(new std::thread(sample_cpu_load));
            start_time = std::chrono::steady_clock::now();
        }
        if(buf[0] == ' ')
        {
            gMeasure = false;
            cpu_thread->join();
            end_time = std::chrono::steady_clock::now();
            break;
        }
        boost::asio::write(sock, boost::asio::buffer(data.get(), cdata_size));

        num_packages++;
    }

    std::cout << "=== BOOST.ASIO ===" << std::endl;
    report_statistics(cdata_size, start_time, end_time, num_packages);




    return 0;
}

