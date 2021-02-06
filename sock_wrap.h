#ifndef __SOCK_WRAP_H__
#define __SOCK_WRAP_H__

#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>


namespace ModelSQL 
{    
    // SocketException --- Exception class
    class SocketException 
    {
    public:
        std :: string err_message;
        enum socket_exception_code 
        {
            ESE_SOCKCREATE,
            ESE_SOCKCONN,
            ESE_SOCKSEND,
            ESE_SOCKRECV,
            ESE_SOCKBIND,
            ESE_SOCKLISTEN,
            ESE_SOCKACCEPT
        };
        SocketException (socket_exception_code);
        void report ();
        ~ SocketException () {}
    };
    
    // BaseSocket --- basic class fot sockets
    class BaseSocket 
    {
    protected:
        int m_Socket = -1;
        struct sockaddr_un m_SockAddr;
    public:
        explicit BaseSocket (int sd = -1, const char *address = NULL);
        
        // sending some information
        void write_ (void *, int);
        void put_char_ (char);
        void put_string_ (const char *);
        void put_string_ (const std::string &);
        
        // recieving some information
        int read_ (void *, int);
        char get_char_ ();
        std::string get_string_ ();
        
        int get_sock_descriptor ();
        ~ BaseSocket () {}
    };
    
    // ServerSocket --- class for server sockets
    class ServerSocket : public BaseSocket 
    {
    public:
        ServerSocket (const char *);
        void bind_ ();
        void listen_ (const int);
        BaseSocket * accept_ ();
        ~ ServerSocket ();
    };

    // ClientSocket --- class for client sockets
    class ClientSocket: public BaseSocket 
    {
    public:
        ClientSocket (const char *);
        void connect_ ();
        ~ ClientSocket ();
    };
    
/*--------------------------------------------------------------------*/

    /*---------------SocketException---------------*/
    SocketException :: SocketException (socket_exception_code errcode)
    {
        switch (errcode) {
            case ESE_SOCKCREATE:
                err_message = "ERROR: server - unsuccessful 'socket'";
                break;
            case ESE_SOCKCONN:
                err_message = "ERROR: client - unsuccessful 'connect'";
                break;
            case ESE_SOCKSEND:
                err_message = "ERROR: unsuccessful 'send'";
                break;
            case ESE_SOCKRECV:
                err_message = "ERROR: unsuccessful 'recv'";
                break;
            case ESE_SOCKBIND:
                err_message = "ERROR: server - unsuccessful 'bind'";
                break;
            case ESE_SOCKLISTEN:
                err_message = "ERROR: server - unsuccessful 'listen'";
                break;
            case ESE_SOCKACCEPT:
                err_message = "ERROR: server - unsuccessful 'accept'";
                break;
        }
    }
    
    void SocketException :: report ()
    {
        std :: cout << err_message << std :: endl;
        std :: cout << "Code: " << errno << std :: endl;
    }
    
    
    /*---------------BaseSocket---------------*/ 
    BaseSocket :: BaseSocket (int sd, const char * address)
    {
        m_Socket = sd;
        if (address != NULL)
        {
            m_SockAddr.sun_family = AF_UNIX;
            strcpy (m_SockAddr.sun_path, address);
        }
    }
    
    void BaseSocket :: write_ (void * buf, int len)
    {
        int s_send;
        s_send = send (m_Socket, buf, len, 0);
        if (s_send == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKSEND);
        }
    }
    
    void BaseSocket :: put_char_ (char sym)
    {
        int s_send;
        s_send = send (m_Socket, (char *) &sym, sizeof (char), 0);
        if (s_send == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKSEND);
        }
    }
    
    void BaseSocket :: put_string_ (const char * str)
    {
        int s_send;
        s_send = send (m_Socket, str, strlen (str) * sizeof (char), 0);
        if (s_send == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKSEND);
        }
        if (str[strlen (str) - 1] != '\n')
            BaseSocket :: put_char_ ('\n');
    }
    
    void BaseSocket :: put_string_ (const std::string &str)
    {
        BaseSocket :: put_string_ (str.c_str());
    }
    
    int BaseSocket :: read_ (void * buf, int len)
    {
        int s_recv = 0;
        while (s_recv == 0)
        {
            s_recv = recv (m_Socket, buf, len, 0);
        }
        if (s_recv == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKRECV);
        }
        return s_recv;
    }
    
    char BaseSocket :: get_char_ ()
    {
        char sym;
        int s_recv = 0;
        while (s_recv == 0)
        {
            s_recv = recv (m_Socket, (char *) &sym, sizeof (char), 0);
        }
        if (s_recv == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKRECV);
        }
        return sym;
    }
    
    std::string BaseSocket :: get_string_ ()
    {
        std :: string str;
        char sym = ' ';
        while ((sym != EOF) && (sym != '\n'))
        {
            sym = BaseSocket :: get_char_ ();
            str.push_back (sym);
        }
        return str;
    }
    
    int BaseSocket :: get_sock_descriptor ()
    {
        return m_Socket;
    }
    
    
    /*---------------ServerSocket---------------*/
    ServerSocket :: ServerSocket (const char * address)
    {
        int sock_fd;
        // create sockets for one computer
        sock_fd = socket (AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKCREATE);
        }
        m_Socket = sock_fd;
        m_SockAddr.sun_family = AF_UNIX;
        strcpy (m_SockAddr.sun_path, address);
        bind_ ();
        listen_ (1);
    }
    
    void ServerSocket :: bind_ ()
    {
        int s_bind;
        s_bind = bind (m_Socket, (struct sockaddr *) &m_SockAddr, 
                    sizeof (m_SockAddr));
        if (s_bind == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKBIND);
        }
    }
    
    void ServerSocket :: listen_ (const int backlog)
    {
        int s_listen;
        s_listen = listen (m_Socket, backlog);
        if (s_listen == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKLISTEN);
        }
    }
    
    BaseSocket * ServerSocket :: accept_ () 
    {
        int sock_fd;
        sock_fd = accept (m_Socket, NULL, NULL);
        if (sock_fd == -1) 
        {
            throw SocketException(SocketException::ESE_SOCKACCEPT);
        }
        BaseSocket * p_sock = new BaseSocket (sock_fd);
        return p_sock;
    }
    
    ServerSocket :: ~ ServerSocket ()
    {
        // unlink with socket
        shutdown (m_Socket, 2);
        close (m_Socket);
    }


    /*---------------ClientSocket---------------*/
    ClientSocket :: ClientSocket (const char * address)
    {
        int sock_fd;
        sock_fd = socket (AF_UNIX, SOCK_STREAM, 0);
        if (sock_fd == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKCREATE);
        }
        m_Socket = sock_fd;
        m_SockAddr.sun_family = AF_UNIX;
        strcpy (m_SockAddr.sun_path, address);
        connect_ ();
    }
    
    void ClientSocket :: connect_ ()
    {
        int s_connect;
        s_connect = connect (m_Socket, (struct sockaddr *) &m_SockAddr, 
                    sizeof (m_SockAddr));
        if (s_connect == -1)
        {
            throw SocketException (SocketException :: ESE_SOCKCONN);
        }
    }
    
    ClientSocket :: ~ ClientSocket ()
    {
        // unlink with socket
        shutdown (m_Socket, 2);
        close (m_Socket);
    }

}; // the end of namespace ModelSQL

#endif
