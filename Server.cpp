#include "sock_wrap.h"
#include "sql.h"
#include <fstream>

using namespace std;
using namespace ModelSQL;

const char * address = "mysocket";

class MyServerSocket : public ServerSocket 
{
    public:
        MyServerSocket () : ServerSocket (address) {}
        void on_accept (BaseSocket * pConn)
        {
            // sending the request for the file name
            pConn->put_string_ ("Input full file name for result");
            string str;
            string f_name;
            f_name = pConn->get_string_();
            f_name.pop_back();
            
            pConn->put_string_ ("If you want to stop, input - END");
            
            // END - the end of the work
            while ((str = pConn->get_string_()) != "END\n")
            {
                //change standard output for .h-files working
                ofstream out(f_name);
                streambuf * coutbuf = cout.rdbuf();
                cout.rdbuf(out.rdbuf());
                try
                {
                    Interpreter obj (str);
                    // answer after successful work
                    pConn->put_string_ ("OK");
                }
                // answer after unsuccessful work
                catch (SQLException & e)
                {
                    e.report();
                    pConn->put_string_ (e.err_message);
                }
                catch (TableException & e)
                {
                    e.report();
                    pConn->put_string_ (e.err_message);
                }
                cout.rdbuf(coutbuf);
            }
            pConn->put_string_ ("END");
            delete pConn;
        }
};

int main (int argc, char* argv[])
{
    try 
    {
        // create socket
        MyServerSocket sock;
        
        // listening for connection request
        sock.on_accept (sock.accept_());
    } 
    catch (SocketException & e) 
    {
        // error --- input to the screen
        e.report();
    }
    return 0;
}
