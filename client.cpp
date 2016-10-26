#include <zmq.hpp>
#include <iostream>
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <memory>

int main ()
{
    size_t  cnum_samples = 5000;
    size_t cbuf_size = 4000*3000;
    std::shared_ptr<char> buf(new char[cbuf_size]);
    //ZeroMq
    zmq::context_t context (1);
    zmq::socket_t socket (context, ZMQ_REQ);

    std::cout << "Connecting to hello world server…" << std::endl;
    socket.connect ("tcp://localhost:5555");

    for (int request_nbr = 0; request_nbr != cnum_samples; request_nbr++) {
        zmq::message_t request (5);
        memcpy (request.data (), "Hello", 5);
        std::cout << "Sending Hello " << request_nbr << "…" << std::endl;
        socket.send (request);

        //  Get the reply.
        zmq::message_t reply;
        socket.recv (&reply);
        std::cout << "Received World " << reply.size() << std::endl;
    }
    zmq::message_t end(2);
    memcpy(end.data(), "  ", 2);
    socket.send(end);

    //NanoMsg


    int sock = nn_socket (AF_SP, NN_PAIR);
    nn_connect (sock, "tcp://localhost:5556");
    for (int request_nbr = 0; request_nbr != cnum_samples; request_nbr++) {
        std::cout << "Sending Hello " << request_nbr << "…" << std::endl;
        nn_send(sock, "Hello", 5, 0);

        //  Get the reply.
        int bytes = nn_recv(sock, buf.get(), cbuf_size, 0);
        std::cout << "Received World " <<bytes << std::endl;
    }
    nn_send(sock, "  ", 2, 0);
    nn_shutdown (sock, 0);
    return 0;
}