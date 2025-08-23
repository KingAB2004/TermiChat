#include"P2P_TermiChat.h"

// It for sending a packet using the socket given along with the data you have to send 

// Scoket Structure  ->  Packet Type 1byte  | Payload Length() 4 Byte | Payload Data Payload.size()
static bool send_packet(int sock, PacketType t, const std::vector<unsigned char>& DAta) {
    // here the htonl is used to convert it into network Style
    uint32_t nsize = htonl((uint32_t)DAta.size());
    
    if (write(sock, &t, 1) != 1) return false;
    
    if (write(sock, &nsize, 4) != 4) return false;
    size_t off = 0, left = DAta.size();

    while (left) {

        ssize_t n = write(sock, DAta.data() + off, left);
        if (n <= 0) return false;
        off += (size_t)n; left -= (size_t)n;
    }
    return true;
}

static bool recv_fully(int sock, void* buf, size_t len) {
    char* p = (char*)buf;
//    Checking if received fully or not
    size_t got = 0;
    while (got < len) {
   
        ssize_t n = read(sock, p + got, len - got);
   
        if (n <= 0) return false;
        got += (size_t)n;
    }
    return true;
}

// Function for Receiving the Packet as ordered by the socket structure

static bool receivingPacket(int sock, PacketType& t, std::vector<unsigned char>& DAta) {
    unsigned char tbyte;
    uint32_t nsize_net;
    if (!recv_fully(sock, &tbyte, 1)) return false;
   
    if (!recv_fully(sock, &nsize_net, 4)) return false;
   
    uint32_t size = ntohl(nsize_net);
    DAta.resize(size);
    if (size) {
   
        if (!recv_fully(sock, DAta.data(), size)) return false;
    }
    t = (PacketType)tbyte;
    return true;
}
