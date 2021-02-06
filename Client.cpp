#include "sock_wrap.h"

using namespace std;
using namespace ModelSQL;

const char * address = "mysocket";

int main (int argc, char* argv[])
{
    try 
    {
        // connecting to the server socket
        ClientSocket sock (address);
        // output server message
        cout << sock.get_string_();
        
        string str;
        // input file name for result
        getline(cin, str);
        sock.put_string_ (str);
        // output server message
        cout << sock.get_string_() << endl;
        
        // END - the end of the work
        while (str != "END")
        {
            // input a comand to server
            getline(cin, str);
            sock.put_string_ (str);
            // recieving answer
            cout << sock.get_string_() << endl;
        }
    } 
    catch (SocketException & e) 
    {
        // error --- output the message
        e.report();
    }
    return 0;
}
