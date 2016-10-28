#include <zmq.hpp>
#include <iostream>
#include <memory>
#ifndef D3TEST
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <udt/udt.h>
#endif
#ifndef WIN32
#include <arpa/inet.h>
#endif
#include <chrono>
#include <thread>
#include <boost/asio.hpp>

using namespace boost::asio::ip;
std::string server_addr = "192.168.10.158";

int main (int argc, char* argv[])
{
    size_t  cnum_samples = 50;
    size_t cbuf_size = 4000*3000;
    if(argc >= 2)
    {
        server_addr = argv[1];
    }
    if(argc >= 3)
    {
        cnum_samples = atol(argv[2]);
    }
    std::shared_ptr<char> buf(new char[cbuf_size]);

    //ZeroMq
    std::this_thread::sleep_for(std::chrono::seconds(1));

    zmq::context_t context (1);
    zmq::socket_t zero_socket (context, ZMQ_REQ);

    std::cout << "Connecting to hello world server…" << std::endl;
    zero_socket.connect ((std::string("tcp://") + server_addr + std::string(":5555")).c_str());

    for (int request_nbr = 0; request_nbr != cnum_samples; request_nbr++) {
        zmq::message_t request (5);
        memcpy (request.data (), "Hello", 5);
        std::cout << "ZeroMq Sending Hello " << request_nbr << "…" << std::endl;
        zero_socket.send (request);

        //  Get the reply.
        zmq::message_t reply;
        zero_socket.recv (&reply);
        std::cout << "ZeroMq Received World " << reply.size() << std::endl;
    }
    zmq::message_t end(2);
    memcpy(end.data(), "  ", 2);
    zero_socket.send(end);
    zero_socket.close();

#ifndef D3TEST
    //NanoMsg
    std::this_thread::sleep_for(std::chrono::seconds(1));

    int nano_socket = nn_socket (AF_SP, NN_PAIR);
    nn_connect (nano_socket, (std::string("tcp://") + server_addr + std::string(":5556")).c_str());
    for (int request_nbr = 0; request_nbr != cnum_samples; request_nbr++) {
        std::cout << "NanoMSG Sending Hello " << request_nbr << "…" << std::endl;
        nn_send(nano_socket, "Hello", 5, 0);

        //  Get the reply.
        int bytes = nn_recv(nano_socket, buf.get(), cbuf_size, 0);
        std::cout << "NanoMSG Received World " <<bytes << std::endl;
    }
    nn_send(nano_socket, "  ", 2, 0);
    nn_shutdown (nano_socket, 0);

    //UDT
    std::this_thread::sleep_for(std::chrono::seconds(1));
    UDTSOCKET udt_socket = UDT::socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5557);
    inet_pton(AF_INET, server_addr.c_str(), &serv_addr.sin_addr);
    memset(&(serv_addr.sin_zero), '\0', 8);
    if (UDT::ERROR == UDT::connect(udt_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)))
    {
        std::cout << "connect: " << UDT::getlasterror().getErrorMessage();
        return 0;
    }
    for (int request_nbr = 0; request_nbr != cnum_samples; request_nbr++) {
        std::cout << "UDT Sending Hello " << request_nbr << "…" << std::endl;
        UDT::send(udt_socket, "Hello", 5, 0);


        size_t data_size = 0;
        int chunk_size = 0;
        while(data_size != cbuf_size) {
            chunk_size = UDT::recv(udt_socket, &buf.get()[data_size], cbuf_size, 0);
            if (UDT::ERROR == chunk_size) {
                std::cout<< UDT::getlasterror().getErrorMessage() << std::endl;
                return 0;
            }
            data_size += chunk_size;

        }

        std::cout << "UDT Received World " << data_size << std::endl;


    }
    UDT::send(udt_socket, "  ", 2, 0);
    UDT::close(udt_socket);
#endif
    //Boost.Asio
    std::this_thread::sleep_for(std::chrono::seconds(5));
    boost::asio::io_service io_service;
    tcp::socket s(io_service);
    tcp::resolver resolver(io_service);
    boost::asio::connect(s, resolver.resolve({server_addr.c_str(), "5558"}));

    for (int request_nbr = 0; request_nbr != cnum_samples; request_nbr++) {
        std::cout << "Boost.Asio Sending Hello " << request_nbr << "…" << std::endl;
        boost::asio::write(s, boost::asio::buffer("Hello", 5));
        //  Get the reply.
        size_t reply_length = boost::asio::read(s,
                                                boost::asio::buffer(buf.get(), cbuf_size));
        std::cout << "Boost.Asio Received World " << reply_length << std::endl;
    }
    boost::asio::write(s, boost::asio::buffer("  ", 2));



    return 0;
}