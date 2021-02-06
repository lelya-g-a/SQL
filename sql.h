#ifndef _SQL_H_
#define _SQL_H_

#include "dbms.h"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <vector>

using namespace std;

// SQLExceptions --- exception class
class SQLException
{
public:
    std :: string err_message;
    enum sql_exception_code 
    {
        ESE_COMAND,
        ESE_FIELDNAME,
        ESE_FIELD,
        ESE_TEXT,
        ESE_NUM,
        ESE_LONGEXPR,
        ESE_TEXTEXPR,
        ESE_WHERE,
        ESE_LOGEXPR,
        ESE_STR
    };
    SQLException (sql_exception_code);
    void report ();
    ~ SQLException () {}
};

// Interpreter --- SQL-interpreter class
class Interpreter
{
private:
    void select_sentence (string &);
    void insert_sentence (string &);
    void update_sensence (string &);
    void delete_sentence (string &);
    void create_sentence (string &);
    void drop_sentence (string &);
    void field_description (string &);
    vector <unsigned long> where_clause (string &);
    Table bd_table;
public:
    Interpreter (string &);
    ~ Interpreter () {}
};

/*--------------------------------------------------------------------*/

// function for reading one word from line
string read_word (string & str)
{
    string word;
    int i = 0;
    while (!isspace (str.c_str() [i]) && !(str.empty()))
    {
        word = word + str.c_str() [i];
        str.erase (0, 1);
    }
    while (isspace (str.c_str() [i]) && !(str.empty()))
    {
        str.erase (0, 1);
    }
    return word;
}

/*--------------------------------------------------------------------*/

// lexical and syntactic parsers for long-expressions
enum long_type_t 
{
    START,
    PLUS,   // +
    MINUS,  // -
    MULT,   // *
    DIV,    // /
    MOD,    // %
    OPEN,   // (
    CLOSE,  // )
    NUMBER, // set of numbers
    L_NAME, // name of the field with type LONG
    END
};

namespace lexer_long_expr 
{
    enum long_type_t cur_lex_type;
    string cur_lex_text;
    string c;
    
    void init (string & str)
    {
        c = read_word (str);
        cur_lex_type = START;
    }

    void next (string & str)
    {
        cur_lex_text.clear();
        enum state_t {H, P, MI, MU, D, MO, O, C, N, L, OK} state = H;
        while (state != OK)
        {
            switch (state)
            {
                case H:
                    if (c == "+")
                    {
                        state = P;
                    }
                    else if (c == "-")
                    {
                        state = MI;
                    }
                    else if (c == "*")
                    {
                        state = MU;
                    }
                    else if (c == "/")
                    {
                        state = D;
                    }
                    else if (c == "%")
                    {
                        state = MO;
                    }
                    else if (c == "(")
                    {
                        state = O;
                    }
                    else if (c == ")")
                    {
                        state = C;
                    }
                    // words, that can follow long-expression
                    else if ((c == "NOT") || (c == "IN") || 
                             (c == "WHERE") || (c == "=") ||
                             (c == ">")  || (c == "<") ||
                             (c == ">=") || (c == "<=") ||
                             (c == "!=") || (c == "AND") ||
                             (c == "OR") || c.empty() )
                    {
                        cur_lex_type = END;
                        state = OK;
                    }
                    else
                    {
                        unsigned int i = 0;
                        int flag = 1;
                        while ((i < c.length()) && flag)
                        {
                            if (!isdigit(c[i]))
                            {
                                // not long
                                // => long_name
                                flag = 0;
                                state = L;
                            }
                            i++;
                        }
                        if (flag)
                        {
                            state = N;
                        }
                    }
                    break;

                case P:
                    cur_lex_type = PLUS;
                    state = OK;
                    break;
                    
                case MI:
                    cur_lex_type = MINUS;
                    state = OK;
                    break;

                case MU:
                    cur_lex_type = MULT;
                    state = OK;
                    break;
                    
                case D:
                    cur_lex_type = DIV;
                    state = OK;
                    break;
                    
                case MO:
                    cur_lex_type = MOD;
                    state = OK;
                    break;

                case O:
                    cur_lex_type = OPEN;
                    state = OK;
                    break;

                case C:
                    cur_lex_type = CLOSE;
                    state = OK;
                    break;
                
                case N:
                    cur_lex_type = NUMBER;
                    state = OK;
                    break;

                case L:
                    cur_lex_type = L_NAME;
                    state = OK;
                    break;

                case OK:
                    break;
            }

            if (state != OK)
            {
                if (cur_lex_type != END)
                {
                    cur_lex_text = c;
                }
                c = read_word (str);
            }
        }
    }
} // end of namespace lexer_long_expr

namespace parser_long_expr 
{

    void init (string & str)
    {
        lexer_long_expr :: init (str);
        lexer_long_expr :: next (str);
    }
    
    // functions for syntactic parser
    string A (string &);
    string B (string &);
    string C (string &);
    
    // functions to calculate the value
    long A (string &, Table);
    long B (string &, Table);
    long C (string &, Table);

    string A (string & str)
    {
        string s;
        s = B (str);
        while ( (lexer_long_expr::cur_lex_type == PLUS) ||
                (lexer_long_expr::cur_lex_type == MINUS) )
        {
            if (lexer_long_expr::cur_lex_type == PLUS)
            {
                lexer_long_expr::next(str);
                s = s + "+ "; 
                s = s + B (str);
            }
            else if (lexer_long_expr::cur_lex_type == MINUS)
            {
                lexer_long_expr::next(str);
                s = s + "- "; 
                s = s + B (str);
            }
        }
        return s;
    }

    string B (string & str)
    {
        string s;
        s = C (str);
        while ( (lexer_long_expr::cur_lex_type == MULT) ||
                (lexer_long_expr::cur_lex_type == DIV) ||
                (lexer_long_expr::cur_lex_type == MOD) )
        {
            if (lexer_long_expr::cur_lex_type == MULT)
            {
                lexer_long_expr::next(str);
                s = s + "* "; 
                s = s + C (str);
            }
            else if (lexer_long_expr::cur_lex_type == DIV)
            {
                lexer_long_expr::next(str);
                s = s + "/ "; 
                s = s + C (str);
            }
            else if (lexer_long_expr::cur_lex_type == MOD)
            {
                lexer_long_expr::next(str);
                s = s + "% "; 
                s = s + C (str);
            }
        }
        return s;
    }

    string C (string & str)
    {
        string s;
        if (lexer_long_expr::cur_lex_type == OPEN)
        {
            lexer_long_expr::next(str);
            s = "( "; 
            s = s + A (str);
            if (lexer_long_expr::cur_lex_type != CLOSE)
            {
                throw SQLException (SQLException :: ESE_LONGEXPR);
            }
            s = s + ") "; 
            lexer_long_expr::next (str);
        }
        else if (lexer_long_expr::cur_lex_type == NUMBER)
        {
            s = lexer_long_expr::cur_lex_text;
            s = s + " ";
            lexer_long_expr::next (str);
        }
        else if (lexer_long_expr::cur_lex_type == L_NAME)
        {
            s = lexer_long_expr::cur_lex_text;
            s = s + " ";
            lexer_long_expr::next (str);
        }
        else
        {
            throw SQLException (SQLException :: ESE_LONGEXPR);
        }
        return s;
    }
    
    
    
    long A (string & str, Table line)
    {
        long num;
        num = B (str, line);
        while ( (lexer_long_expr::cur_lex_type == PLUS) ||
                (lexer_long_expr::cur_lex_type == MINUS) )
        {
            if (lexer_long_expr::cur_lex_type == PLUS)
            {
                lexer_long_expr::next(str);
                num = num + B (str, line);
            }
            else if (lexer_long_expr::cur_lex_type == MINUS)
            {
                lexer_long_expr::next(str);
                num = num - B (str, line);
            }
        }
        return num;
    }

    long B (string & str, Table line)
    {
        long num;
        num = C (str, line);
        while ( (lexer_long_expr::cur_lex_type == MULT) ||
                (lexer_long_expr::cur_lex_type == DIV) ||
                (lexer_long_expr::cur_lex_type == MOD) )
        {
            if (lexer_long_expr::cur_lex_type == MULT)
            {
                lexer_long_expr::next(str);
                num = num * C (str, line);
            }
            else if (lexer_long_expr::cur_lex_type == DIV)
            {
                lexer_long_expr::next(str);
                num = num / C (str, line);
            }
            else if (lexer_long_expr::cur_lex_type == MOD)
            {
                lexer_long_expr::next(str);
                num = num % C (str, line);
            }
        }
        return num;
    }

    long C (string & str, Table line)
    {
        long num;
        if (lexer_long_expr::cur_lex_type == OPEN)
        {
            lexer_long_expr::next(str);
            num = A (str, line);
            if (lexer_long_expr::cur_lex_type != CLOSE)
            {
                throw SQLException (SQLException :: ESE_LONGEXPR);
            }
            lexer_long_expr::next (str);
        }
        else if (lexer_long_expr::cur_lex_type == NUMBER)
        {
            try
            {
                // convert string to long
                num = stol (lexer_long_expr::cur_lex_text); 
            }
            catch (...)
            {
                throw SQLException (SQLException :: ESE_NUM);
            }
            lexer_long_expr::next (str);
        }
        else if (lexer_long_expr::cur_lex_type == L_NAME)
        {
            // check if such field exists and recieve its value
            field_struct * f = line.get_field 
            (lexer_long_expr::cur_lex_text.c_str());
            num = f -> l_num;
            lexer_long_expr::next (str);
        }
        else
        {
            throw SQLException (SQLException :: ESE_LONGEXPR);
        }
        return num;
    }
} // end of namespace parser_long_expr
/*--------------------------------------------------------------------*/

// lexical and syntactic parsers for where-clause
enum where_type_t 
{
    START_w,
    PLUS_w,     // +
    MINUS_w,    // -
    OR_w,       // OR
    MULT_w,     // *
    DIV_w,      // /
    MOD_w,      // %
    AND_w,      // AND
    NOT_w,      // NOT
    OPEN_w,     // (
    CLOSE_w,    // )
    REL_w,      // =, >, <, !=, >=, <=
    NUMBER_w,   // set of numbers
    L_NAME_w,   // name of the field with the type LONG
    T_NAME_w,   // name of the field with the type TEXT
    STR_w,      // line
    LIKE_w,     // LIKE
    IN_w,       // IN
    COM_w,      // ,
    ALL_w,      // ALL
    END_w
};

namespace lexer_where
{
    enum where_type_t cur_lex_type_w;
    string cur_lex_text_w;
    string c_w;
    
    void init (string & str)
    {
        c_w = read_word (str);
        cur_lex_type_w = START_w;
    }

    void next (string & str, Table bd)
    {
        cur_lex_text_w.clear();
        enum state_t_w {H, P, MI, O, MU, D, MO, AN, NO, OP,
                        CL, R, N, L, T, S, LI, I, C, A, OK} state_w = H;
        while (state_w != OK)
        {
            switch (state_w)
            {
                case H:
                    if (c_w == "+")
                    {
                        state_w = P;
                    }
                    else if (c_w == "-")
                    {
                        state_w = MI;
                    }
                    else if (c_w == "OR")
                    {
                        state_w = O;
                    }
                    else if (c_w == "*")
                    {
                        state_w = MU;
                    }
                    else if (c_w == "/")
                    {
                        state_w = D;
                    }
                    else if (c_w == "%")
                    {
                        state_w = MO;
                    }
                    else if (c_w == "AND")
                    {
                        state_w = AN;
                    }
                    else if (c_w == "NOT")
                    {
                        state_w = NO;
                    }
                    else if (c_w == "(")
                    {
                        state_w = OP;
                    }
                    else if (c_w == ")")
                    {
                        state_w = CL;
                    }
                    else if ( (c_w == "=") || (c_w == "!=") || 
                              (c_w == ">") || (c_w == ">=") ||
                              (c_w == "<") || (c_w == "<=") )
                    {
                        state_w = R;
                    }
                    else if (c_w == "LIKE")
                    {
                        state_w = LI;
                    }
                    else if (c_w == "IN")
                    {
                        state_w = I;
                    }
                    else if (c_w == ",")
                    {
                        state_w = C;
                    }
                    else if (c_w == "ALL")
                    {
                        state_w = A;
                    }
                    else if (c_w.empty())
                    {
                        cur_lex_type_w = END_w;
                        state_w = OK;
                    }
                    else
                    {
                        unsigned int i = 0;
                        int flag = 1;
                        while ((i < c_w.length()) && flag)
                        {
                            if (!isdigit(c_w[i]))
                            {
                                // not long
                                flag = 0;
                            }
                            i++;
                        }
                        if (flag)
                        {
                            state_w = N;
                        }
                        
                        else if (c_w[0] == '\'') // line
                        {
                            state_w = S;
                            if (c_w[c_w.length() - 1] == '\'')
                            {
                                cur_lex_type_w = STR_w;
                                cur_lex_text_w = cur_lex_text_w + c_w;
                                cur_lex_text_w = cur_lex_text_w + " ";
                                c_w = read_word (str);
                                state_w = OK;
                            }
                        }
                        else 
                        {
                            // chack if the word without '...' is
                            // the field and its type 
                            field_struct * f;
                            try
                            {
                                f = bd.get_field (c_w.c_str());
                            }
                            // if no such field name
                            catch (...)
                            {
                                throw SQLException (SQLException::
                                                    ESE_WHERE);
                            }
                            if (f -> type == TEXT)
                            {
                                state_w = T;
                            }
                            else
                            {
                                state_w = L;
                            }
                        }
                    }
                    break;

                case P:
                    cur_lex_type_w = PLUS_w;
                    state_w = OK;
                    break;
                    
                case MI:
                    cur_lex_type_w = MINUS_w;
                    state_w = OK;
                    break;
                    
                case O:
                    cur_lex_type_w = OR_w;
                    state_w = OK;
                    break;

                case MU:
                    cur_lex_type_w = MULT_w;
                    state_w = OK;
                    break;
                    
                case D:
                    cur_lex_type_w = DIV_w;
                    state_w = OK;
                    break;
                    
                case MO:
                    cur_lex_type_w = MOD_w;
                    state_w = OK;
                    break;
                
                case AN:
                    cur_lex_type_w = AND_w;
                    state_w = OK;
                    break;
                
                case NO:
                    cur_lex_type_w = NOT_w;
                    state_w = OK;
                    break;

                case OP:
                    cur_lex_type_w = OPEN_w;
                    state_w = OK;
                    break;

                case CL:
                    cur_lex_type_w = CLOSE_w;
                    state_w = OK;
                    break;
                
                case R:
                    cur_lex_type_w = REL_w;
                    state_w = OK;
                    break;
                
                case N:
                    cur_lex_type_w = NUMBER_w;
                    state_w = OK;
                    break;

                case L:
                    cur_lex_type_w = L_NAME_w;
                    state_w = OK;
                    break;
                    
                case T:
                    cur_lex_type_w = T_NAME_w;
                    state_w = OK;
                    break;
                    
                case S:
                    // searching fot the line end
                    if (c_w.empty())
                    {
                        throw SQLException (SQLException::ESE_STR);
                    }
                    if (c_w[c_w.length() - 1] != '\'')
                    {
                        //stay in S
                    }
                    else
                    {
                        cur_lex_type_w = STR_w;
                        cur_lex_text_w = cur_lex_text_w + c_w;
                        cur_lex_text_w = cur_lex_text_w + " ";
                        c_w = read_word (str);
                        state_w = OK;
                    }
                    break;
                    
                case LI:
                    cur_lex_type_w = LIKE_w;
                    state_w = OK;
                    break;
                
                case I:
                    cur_lex_type_w = IN_w;
                    state_w = OK;
                    break;
                
                case C:
                    cur_lex_type_w = COM_w;
                    state_w = OK;
                    break;
                    
                case A:
                    cur_lex_type_w = ALL_w;
                    state_w = OK;
                    break;

                case OK:
                    break;
            }

            if (state_w != OK)
            {
                if (cur_lex_type_w != END_w)
                {
                    cur_lex_text_w = cur_lex_text_w + c_w;
                    cur_lex_text_w = cur_lex_text_w + " ";
                }
                c_w = read_word (str);
            }
        }
    }
} // end of namespace lexer_where

// modes of where_clause
enum mode_type
{
    LIKE_alt,
    IN_alt_T,
    IN_alt_L,
    LOG_alt,
    ALL_alt
};

namespace parser_where
{
    int flag_log = 0;
    int flag_expr = 0;
    enum mode_type mode;
    multiset <long> mst_l;
    multiset <string> mst_s;

    void init (string & str, Table bd)
    {
        lexer_where :: init (str);
        lexer_where :: next (str, bd);
    }
    
    // functions for sintactic parser
    string W0 (string &, Table); // beginning of where-clause
    string W1 (string &, Table); // LIKE-alternative
    string W2 (string &, Table); // IN-alternative
    string W3 (string &, Table); // expressions ...
    string W4 (string &, Table);
    string W5 (string &, Table);
    string W6 (string &, Table);
    string W7 (string &, Table);
    void W8 (string &, Table);   // list of constants
    
    // functions to calculate the value of logic-expression
    long W31 (string &, Table);
    long W41 (string &, Table);
    long W51 (string &, Table);
    long W71 (string &, Table);
    
    string W0 (string & str, Table bd)
    {
        string s;
        if (lexer_where::cur_lex_type_w == ALL_w)
        {
            // to do for all records
            s = "ALL";
            mode = ALL_alt;
            lexer_where::next(str, bd);
        }
        else if (lexer_where::cur_lex_type_w == T_NAME_w)
        {
            // if the first word is name of the field with type TEXT,
            // it can lead to LIKE-alternative or to IN-alternative
            s = lexer_where::cur_lex_text_w;
            lexer_where::next(str, bd);
            // if there is the word "NOT"
            if (lexer_where::cur_lex_type_w == NOT_w)
            {
                s = s + lexer_where::cur_lex_text_w;
                lexer_where::next(str, bd);
            }
            if (lexer_where::cur_lex_type_w == LIKE_w)
            {
                s = s + lexer_where::cur_lex_text_w;
                lexer_where::next(str, bd);
                // processing of LIKE-alternative
                s = s + W1 (str, bd);
            }
            else if (lexer_where::cur_lex_type_w == IN_w)
            {
                s = s + lexer_where::cur_lex_text_w;
                mode = IN_alt_T;
                // processing of IN-alternative
                s = s + W2 (str, bd);
            }
            else
            {
                throw SQLException (SQLException :: ESE_WHERE);
            }
        }
        else if (lexer_where::cur_lex_type_w == STR_w)
        {
            // if the first word is a line,
            // it can lead to IN-alternative,
            // because it is the text-expression
            s = lexer_where::cur_lex_text_w;
            lexer_where::next(str, bd);
            // if there is the word "NOT"
            if (lexer_where::cur_lex_type_w == NOT_w)
            {
                s = s + lexer_where::cur_lex_text_w;
                lexer_where::next(str, bd);
            }
            if (lexer_where::cur_lex_type_w == IN_w)
            {
                s = s + lexer_where::cur_lex_text_w;
                mode = IN_alt_T;
                // processing of IN-alternative 
                s = s + W2 (str, bd);
            }
            else
            {
                throw SQLException (SQLException :: ESE_WHERE);
            }
        }
        else if ( (lexer_where::cur_lex_type_w == L_NAME_w) ||
                  (lexer_where::cur_lex_type_w == NUMBER_w) )
        {
            // if the first word is a set of numbers or
            // name of the field with type LONG,
            // it can lead to IN-alternative,
            // because it is the beginning of long-expression
            str.insert (0, lexer_where::cur_lex_text_w);
            lexer_where::c_w = lexer_where::c_w + " ";
            str.insert (lexer_where::cur_lex_text_w.length(), 
                        lexer_where::c_w);
            // processing of long-expression
            parser_long_expr::init (str);
            s = parser_long_expr::A (str);
            // if the expression is not right
            if (lexer_long_expr::cur_lex_type != END)
            {
                throw SQLException (SQLException :: ESE_LONGEXPR);
            }
            s = s + " ";
            lexer_where::c_w = lexer_long_expr::c;
            lexer_where::next(str, bd);
            if (lexer_where::cur_lex_type_w == NOT_w)
            {
                s = s + lexer_where::cur_lex_text_w;
                s = s + " ";
                lexer_where::next(str, bd);
            }
            if (lexer_where::cur_lex_type_w == IN_w)
            {
                s = s + lexer_where::cur_lex_text_w;
                s = s + " ";
                mode = IN_alt_L;
                // processing of IN-alternative 
                s = s + W2 (str, bd);
            }
            else
            {
                throw SQLException (SQLException :: ESE_WHERE);
            }
        }
        else if ( (lexer_where::cur_lex_type_w == NOT_w) ||
                  (lexer_where::cur_lex_type_w == OPEN_w) )
        {
            // "NOT" and "(" can lead
            // to long-expression or to logic-expression
            s = W3 (str, bd);
            // if it is long-expression
            if (!flag_log)
            {
                if (lexer_where::cur_lex_type_w == NOT_w)
                {
                    s = s + "NOT ";
                    lexer_where::next(str, bd);
                }
                if (lexer_where::cur_lex_type_w == IN_w)
                {
                    s = s + "IN ";
                    mode = IN_alt_L;
                    // processing of IN-alternative 
                    s = s + W2 (str, bd);
                }
            }
            // if it is logic-expression
            else
            {
                mode = LOG_alt;
            }
        }
        else
        {
            throw SQLException (SQLException :: ESE_WHERE);
        }
        return s;
    }
    
    // processing of LIKE-alternative
    string W1 (string & str, Table bd)
    {
        string s;
        // saving sample string
        if (lexer_where::cur_lex_type_w == STR_w)
        {
            s = lexer_where::cur_lex_text_w;
            lexer_where::next (str, bd);
        }
        else 
        {
            throw SQLException (SQLException :: ESE_WHERE);
        }
        mode = LIKE_alt;
        return s;
    }

    // processing of IN-alternative
    string W2 (string & str, Table bd)
    {
        lexer_where::next(str, bd);
        string s = " ";
        if (lexer_where::cur_lex_type_w == OPEN_w)
        {
            lexer_where::next(str, bd);
            // list of constants
            W8 (str, bd);
            if (lexer_where::cur_lex_type_w != CLOSE_w)
            {
                throw SQLException (SQLException :: ESE_WHERE);
            }
            lexer_where::next (str, bd);
        }
        return s;
    }
    
    // the beginning og long- or logic-expression
    // flag_log == 1 => there is logic operators
    // flag_expr == 1 => processing long-expr
    
    string W3 (string & str, Table bd)
    {
        string s;
        s = W4 (str, bd);
        while ( (lexer_where::cur_lex_type_w == PLUS_w)  ||
                (lexer_where::cur_lex_type_w == MINUS_w) ||
                (lexer_where::cur_lex_type_w == OR_w)    )
        {
            // processing long-expression now
            if (flag_expr)
            {
                if (lexer_where::cur_lex_type_w == PLUS_w)
                {
                    lexer_where::next(str, bd);
                    s = s + "+ "; 
                }
                else if (lexer_where::cur_lex_type_w == MINUS_w)
                {
                    lexer_where::next(str, bd);
                    s = s + "- ";
                }
            }
            // processing logic-expression now
            else
            {
                if (lexer_where::cur_lex_type_w == OR_w)
                {
                    lexer_where::next(str, bd);
                    s = s + "OR "; 
                }
            }
            s = s + W4 (str, bd);
        }
        return s;
    }

    string W4 (string & str, Table bd)
    {
        string s;
        s = W5 (str, bd);
        while ( (lexer_where::cur_lex_type_w == MULT_w) ||
                (lexer_where::cur_lex_type_w == DIV_w)  ||
                (lexer_where::cur_lex_type_w == MOD_w)  ||
                (lexer_where::cur_lex_type_w == AND_w)  )
        {
            // processing long-expression now
            if (flag_expr)
            {
                if (lexer_where::cur_lex_type_w == MULT_w)
                {
                    lexer_where::next(str, bd);
                    s = s + "* ";
                }
                else if (lexer_where::cur_lex_type_w == DIV_w)
                {
                    lexer_where::next(str, bd);
                    s = s + "/ ";
                }
                else if (lexer_where::cur_lex_type_w == MOD_w)
                {
                    lexer_where::next(str, bd);
                    s = s + "% ";
                }
                s = s + W5 (str, bd);
            }
            // processing logic-expression now
            else 
            {
                if (lexer_where::cur_lex_type_w == AND_w)
                {
                    lexer_where::next(str, bd);
                    s = s + "AND ";
                }
                // (..) after logic operators
                s = s + W7 (str, bd);
            }
        }
        return s;
    }

    string W5 (string & str, Table bd)
    {
        string s;
        if (lexer_where::cur_lex_type_w == OPEN_w)
        {
            lexer_where::next(str, bd);
            s = "( "; 
            s = s + W3 (str, bd);
            if (lexer_where::cur_lex_type_w != CLOSE_w)
            {
                throw SQLException (SQLException :: ESE_WHERE);
            }
            s = s + ") "; 
            lexer_where::next (str, bd);
        }
        else if (lexer_where::cur_lex_type_w == NOT_w)
        {
            s = W7 (str, bd);
        }
        else if (lexer_where::cur_lex_type_w == NUMBER_w)
        {
            flag_expr = 1; // processing long-expression now
            s = lexer_where::cur_lex_text_w;
            s = s + " ";
            lexer_where::next (str, bd);
            if (lexer_where::cur_lex_type_w == REL_w)
            {
                flag_log = 1;
                s = s + lexer_where::cur_lex_text_w;
                s = s + " ";
                lexer_where::next (str, bd);
                s = s + W3 (str, bd);
                flag_expr = 0; // end of processing long-expression
            }
        }
        else if (lexer_where::cur_lex_type_w == L_NAME_w)
        {
            flag_expr = 1; // processing long-expression now
            s = lexer_where::cur_lex_text_w;
            s = s + " ";
            lexer_where::next (str, bd);
            if (lexer_where::cur_lex_type_w == REL_w)
            {
                flag_log = 1;
                s = s + lexer_where::cur_lex_text_w;
                s = s + " ";
                lexer_where::next (str, bd);
                s = s + W3 (str, bd);
                flag_expr = 0; // end of processing long-expression
            }
        }
        else if ( (lexer_where::cur_lex_type_w == T_NAME_w) ||
                  (lexer_where::cur_lex_type_w == STR_w)    )
        {
            flag_log = 1;
            s = lexer_where::cur_lex_text_w;
            s = s + " ";
            lexer_where::next (str, bd);
            // processing text-expression for logic-expression
            s = s + W6 (str, bd);
        }
        else
        {
            throw SQLException (SQLException :: ESE_WHERE);
        }
        return s;
    }
    
    string W6 (string & str, Table bd)
    {
        string s;
        // processing text-expression for logic-expression
        if (lexer_where::cur_lex_type_w == REL_w)
        {
            s = lexer_where::cur_lex_text_w;
            s = s + " ";
            lexer_where::next (str, bd);
        }
        else 
        {
            throw SQLException (SQLException :: ESE_WHERE);
        }
        if ( (lexer_where::cur_lex_type_w == T_NAME_w) ||
             (lexer_where::cur_lex_type_w == STR_w)    )
        {
            s = s + lexer_where::cur_lex_text_w;
            s = s + " ";
            lexer_where::next (str, bd);
        }
        else 
        {
            throw SQLException (SQLException :: ESE_WHERE);
        }
        return s;
    }
    
    string W7 (string & str, Table bd)
    {
        string s;
        // (...) after logic operators
        if (lexer_where::cur_lex_type_w == OPEN_w)
        {
            lexer_where::next(str, bd);
            s = "( "; 
            s = s + W3 (str, bd);
            if ((lexer_where::cur_lex_type_w != CLOSE_w) || flag_expr)
            {
                throw SQLException (SQLException :: ESE_WHERE);
            }
            s = s + ") "; 
            lexer_where::next (str, bd);
        }
        else if (lexer_where::cur_lex_type_w == NOT_w)
        {
            flag_log = 1;
            s = "NOT ";
            lexer_where::next (str, bd);
            s = s + W7 (str, bd);
        }
        else
        {
            throw SQLException (SQLException :: ESE_WHERE);
        }
        return s;
    }
    
    // list of constants for IN-alternative
    void W8 (string & str, Table bd)
    {
        // list of strings
        if (lexer_where::cur_lex_type_w == STR_w)
        {
            lexer_where::cur_lex_text_w.pop_back(); // " "
            lexer_where::cur_lex_text_w.pop_back(); // "'"
            lexer_where::cur_lex_text_w.erase(0, 1); // "'"
            mst_s.insert(lexer_where::cur_lex_text_w);
            lexer_where::next (str, bd);
            
            while (lexer_where::cur_lex_type_w == COM_w)
            {
                lexer_where::next (str, bd);
                if (lexer_where::cur_lex_type_w == STR_w)
                {
                    lexer_where::cur_lex_text_w.pop_back(); // " "
                    lexer_where::cur_lex_text_w.pop_back(); // "'"
                    lexer_where::cur_lex_text_w.erase(0, 1);// "'"
                    mst_s.insert(lexer_where::cur_lex_text_w);
                    lexer_where::next (str, bd);
                }
                else
                {
                    throw SQLException (SQLException :: ESE_WHERE);
                }
            }
        }
        // list of numbers
        else if (lexer_where::cur_lex_type_w == NUMBER_w)
        {
            lexer_where::cur_lex_text_w.pop_back();
            long num;
            try
            {
                // convert string to long
                num = stol (lexer_where::cur_lex_text_w);
            }
            catch (...)
            {
                throw SQLException (SQLException :: ESE_WHERE);
            }
            mst_l.insert (num);
            lexer_where::next (str, bd);
            while (lexer_where::cur_lex_type_w == COM_w)
            {
                lexer_where::next (str, bd);
                if (lexer_where::cur_lex_type_w == NUMBER_w)
                {
                    lexer_where::cur_lex_text_w.pop_back();
                    try
                    {
                        num = stol (lexer_where::cur_lex_text_w);
                    }
                    catch (...)
                    {
                        throw SQLException (SQLException :: ESE_WHERE);
                    }
                    mst_l.insert (num);
                    lexer_where::next (str, bd);
                }
                else
                {
                    throw SQLException (SQLException :: ESE_WHERE);
                }
            }
        }
        else
        {
            throw SQLException (SQLException :: ESE_WHERE);
        }
    }
    
    
    
    long W31 (string & str, Table bd)
    {
        long res;
        res = W41 (str, bd);
        while ( (lexer_where::cur_lex_type_w == PLUS_w)  ||
                (lexer_where::cur_lex_type_w == MINUS_w) ||
                (lexer_where::cur_lex_type_w == OR_w)    )
        {
            // long-expression
            if (flag_expr)
            {
                if (lexer_where::cur_lex_type_w == PLUS_w)
                {
                    lexer_where::next(str, bd);
                    res = res + W41 (str, bd); 
                }
                else if (lexer_where::cur_lex_type_w == MINUS_w)
                {
                    lexer_where::next(str, bd);
                    res = res - W41 (str, bd);
                }
            }
            // logic-expression
            else
            {
                if (lexer_where::cur_lex_type_w == OR_w)
                {
                    lexer_where::next(str, bd);
                    if (res || W41 (str, bd))
                    {
                        res = 1;
                    }
                    else
                    {
                        res = 0;
                    }
                }
            }
        }
        return res;
    }

    long W41 (string & str, Table bd)
    {
        long res;
        res = W51 (str, bd);
        while ( (lexer_where::cur_lex_type_w == MULT_w) ||
                (lexer_where::cur_lex_type_w == DIV_w)  ||
                (lexer_where::cur_lex_type_w == MOD_w)  ||
                (lexer_where::cur_lex_type_w == AND_w)  )
        {
            // long-expression
            if (flag_expr)
            {
                if (lexer_where::cur_lex_type_w == MULT_w)
                {
                    lexer_where::next(str, bd);
                    res = res * W51 (str, bd);
                }
                else if (lexer_where::cur_lex_type_w == DIV_w)
                {
                    lexer_where::next(str, bd);
                    res = res / W51 (str, bd);
                }
                else if (lexer_where::cur_lex_type_w == MOD_w)
                {
                    lexer_where::next(str, bd);
                    res = res % W51 (str, bd);
                }
            }
            // logic-expression
            else 
            {
                if (lexer_where::cur_lex_type_w == AND_w)
                {
                    lexer_where::next(str, bd);
                    if (res && W71 (str, bd))
                    {
                        res = 1;
                    }
                    else
                    {
                        res = 0;
                    }
                }
            }
        }
        return res;
    }

    long W51 (string & str, Table bd)
    {
        long res;
        if (lexer_where::cur_lex_type_w == OPEN_w)
        {
            lexer_where::next(str, bd);
            res = W31 (str, bd);
            if (lexer_where::cur_lex_type_w != CLOSE_w)
            {
                throw SQLException (SQLException :: ESE_LOGEXPR);
            }
            lexer_where::next (str, bd);
        }
        else if (lexer_where::cur_lex_type_w == NOT_w)
        {
            res = W71 (str, bd);
        }
        else if ( (lexer_where::cur_lex_type_w == L_NAME_w) ||
                  (lexer_where::cur_lex_type_w == NUMBER_w) )
        {
            // processing long-expression now
            flag_expr = 1;
            long res1;
            long res2;
            string op;
            str.insert (0, lexer_where::cur_lex_text_w);
            lexer_where::c_w = lexer_where::c_w + " ";
            str.insert (lexer_where::cur_lex_text_w.length(), 
                        lexer_where::c_w);
            parser_long_expr::init (str);
            res1 = parser_long_expr::A (str, bd);
            lexer_where::c_w = lexer_long_expr::c;
            
            // logic operator
            lexer_where::next (str, bd);
            op = lexer_where::cur_lex_text_w;
            lexer_where::next (str, bd);
            
            lexer_where::c_w = lexer_where::c_w + " ";
            str.insert (0, lexer_where::c_w);
            str.insert (0, lexer_where::cur_lex_text_w);
            // processing long-expression now
            parser_long_expr::init (str);
            res2 = parser_long_expr::A (str, bd);
            lexer_where::c_w = lexer_where::c_w + " ";
            str.insert (0, lexer_where::c_w);
            str.insert(0, ") ");
            lexer_where::c_w = ")";
            lexer_where::next (str, bd);
            
            if (op == "= ")
            {
                if (res1 == res2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == "< ")
            {
                if (res1 < res2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == "> ")
            {
                if (res1 > res2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == "!= ")
            {
                if (res1 != res2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == "<= ")
            {
                if (res1 <= res2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == ">= ")
            {
                if (res1 >= res2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
            flag_expr = 0;
        }
        else if ( (lexer_where::cur_lex_type_w == T_NAME_w) ||
                  (lexer_where::cur_lex_type_w == STR_w)    )
        {
            // text-expression
            string f1;
            string f2;
            string op;
            if (lexer_where::cur_lex_type_w == T_NAME_w)
            {
                lexer_where::cur_lex_text_w.pop_back();
                field_struct * f;
                f = bd.get_field (lexer_where::cur_lex_text_w.c_str());
                f1 = f -> text;
            }
            else if (lexer_where::cur_lex_type_w == STR_w)
            {
                lexer_where::cur_lex_text_w.pop_back();
                lexer_where::cur_lex_text_w.pop_back();
                lexer_where::cur_lex_text_w.erase(0, 1);
                f1 = lexer_where::cur_lex_text_w;
            }
            lexer_where::next (str, bd);
            if (lexer_where::cur_lex_type_w == REL_w)
            {
                lexer_where::cur_lex_text_w.pop_back();
                op = lexer_where::cur_lex_text_w;
                lexer_where::next (str, bd);
            }
            if (lexer_where::cur_lex_type_w == T_NAME_w)
            {
                lexer_where::cur_lex_text_w.pop_back();
                field_struct * f;
                f = bd.get_field (lexer_where::cur_lex_text_w.c_str());
                f2 = f -> text;
            }
            else if (lexer_where::cur_lex_type_w == STR_w)
            {
                lexer_where::cur_lex_text_w.pop_back();
                lexer_where::cur_lex_text_w.pop_back();
                lexer_where::cur_lex_text_w.erase(0, 1);
                f2 = lexer_where::cur_lex_text_w;
            }
            lexer_where::next (str, bd);
            if (op == "=")
            {
                if (f1 == f2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == "<")
            {
                if (f1 < f2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == ">")
            {
                if (f1 > f2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == "!=")
            {
                if (f1 != f2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == "<=")
            {
                if (f1 <= f2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
                
            else if (op == ">=")
            {
                if (f1 >= f2)
                {
                    res = 1;
                }
                else
                {
                    res = 0;
                }
            }
        }
        else
        {
            throw SQLException (SQLException :: ESE_LOGEXPR);
        }
        return res;
    }
    
    long W71 (string & str, Table bd)
    {
        // (...) for logic operators
        long res;
        if (lexer_where::cur_lex_type_w == OPEN_w)
        {
            lexer_where::next(str, bd);
            res = W31 (str, bd);
            if ((lexer_where::cur_lex_type_w != CLOSE_w) || flag_expr)
            {
                throw SQLException (SQLException :: ESE_LOGEXPR);
            }
            lexer_where::next (str, bd);
        }
        else if (lexer_where::cur_lex_type_w == NOT_w)
        {
            lexer_where::next (str, bd);
            if (W71 (str, bd))
            {
                res = 0;
            }
            else
            {
                res = 1;
            }
        }
        else
        {
            throw SQLException (SQLException :: ESE_LOGEXPR);
        }
        return res;
    }
} // end of namespace parser_where

/*--------------------------------------------------------------------*/

/*---------------SQLException---------------*/
SQLException :: SQLException (sql_exception_code errcode)
{
    switch (errcode) {
        case ESE_COMAND:
            err_message = "ERROR: wrong comand";
            break;
        case ESE_FIELDNAME:
            err_message = "ERROR: wrong field name";
            break;
        case ESE_FIELD:
            err_message = "ERROR: wrong field description";
            break;
        case ESE_TEXT:
            err_message = "ERROR: wrong text format";
            break;
        case ESE_NUM:
            err_message = "ERROR: wrong number format";
            break;
        case ESE_LONGEXPR:
            err_message = "ERROR: wrong long-expression";
            break;
        case ESE_TEXTEXPR:
            err_message = "ERROR: wrong text-expression";
            break;
        case ESE_WHERE:
            err_message = "ERROR: wrong where-expression";
            break;
        case ESE_LOGEXPR:
            err_message = "ERROR: wrong logic-expression";
            break;
        case ESE_STR:
            err_message = "ERROR: not ended string";
            break;
    }
}

void SQLException :: report ()
{
    std :: cout << err_message << std :: endl;
}


/*---------------Interpreter---------------*/
Interpreter :: Interpreter (string & str)
{
    string cur_word;
    cur_word = read_word (str); // operation
    if (cur_word == "SELECT")
    {
        select_sentence (str);
    }
    else if (cur_word == "INSERT")
    {
        insert_sentence (str);
    }
    else if (cur_word == "UPDATE")
    {
        update_sensence (str);
    }
    else if (cur_word == "DELETE")
    {
        delete_sentence (str);
    }
    else if (cur_word == "CREATE")
    {
        create_sentence (str);
    }
    else if (cur_word == "DROP")
    {
        drop_sentence (str);
    }
    else
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
}

void Interpreter :: select_sentence (string & str)
{
    vector <string> vect;
    string cur_word;
    cur_word = read_word (str); // first field_name
    int fields_flag = 0;
    if (cur_word == "*")
    {
        // all fields
        fields_flag = 1;
        cur_word.clear();
        cur_word = read_word (str);
    }
    else // {, field_name}
    {
        vect.push_back (cur_word);
        cur_word.clear();
        cur_word = read_word (str);
        while (cur_word == ",")
        {
            cur_word.clear();
            cur_word = read_word (str); // next field_name
            vect.push_back (cur_word);
            cur_word.clear();
            cur_word = read_word (str); // "," or not
        }
    }
    if (cur_word != "FROM")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    cur_word.clear();
    cur_word = read_word (str); // table_name
    bd_table.open_table (cur_word);
    cur_word.clear();
    cur_word = read_word (str);
    if (cur_word != "WHERE")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    } 
    vector <unsigned long> v_where;
    v_where = where_clause (str); // where-clause
    // doing action for SELECT
    if (fields_flag)
    {
        bd_table.print_line_names ();
        for (unsigned long i = 0; i < v_where.size(); i++)
        {
            bd_table.print_line (v_where[i]);
        }
    }
    else
    {
        bd_table.print_short_line_names (vect);
        for (unsigned long i = 0; i < v_where.size(); i++)
        {
            bd_table.print_short_line (vect, v_where[i]);
        }
    }
    
}

void Interpreter :: insert_sentence (string & str)
{
    string cur_word;
    cur_word = read_word (str); 
    if (cur_word != "INTO")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    string table_name;
    table_name = read_word (str);
    bd_table.open_table (table_name); // open necessary table
    cur_word.clear();
    cur_word = read_word (str);
    if (cur_word != "(")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    for (unsigned long i = 0; i < bd_table.t_struct.num_of_fields; i++)
    {
        // if the field with type TEXT
        if (bd_table.fields[i].type == TEXT)
        {
            string t_str;
            if (cur_word.empty())
            {
                throw SQLException (SQLException::ESE_STR);
            }
            while (cur_word[cur_word.length() - 1] != '\'')
            {
                cur_word.clear();
                cur_word = read_word (str);
                if (cur_word.empty())
                {
                    throw SQLException (SQLException::ESE_STR);
                }
                t_str = t_str + cur_word;
                t_str = t_str + ' ';
            }
            if (t_str[0] != '\'')
            {
                throw SQLException (SQLException :: ESE_TEXT);
            }
            t_str.erase(0, 1);
            t_str.erase((t_str.length() - 2), 2);
            if (t_str.length() > bd_table.fields[i].field_len)
            {
                throw TableException (TableException :: ESE_FIELDLEN);
            }
            strcpy (bd_table.fields[i].text, t_str.c_str());
        }
        // if the field with type LONG
        else
        {
            cur_word.clear();
            cur_word = read_word (str);
            long num;
            try
            {
                num = stol (cur_word); // convert string to long
            }
            catch (...)
            {
                throw SQLException (SQLException :: ESE_NUM);
            }
            bd_table.fields[i].l_num = num;
        }
        cur_word.clear();
        cur_word = read_word (str);
    }
    if (cur_word != ")")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    cur_word.clear();
    // check if it is the end of the comand
    cur_word = read_word (str);
    if (!cur_word.empty())
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    // doing actions for INSERT
    bd_table.add_line ();
    bd_table.print_table();
}

void Interpreter :: update_sensence (string & str)
{
    string cur_word;
    cur_word = read_word (str); // table_name
    bd_table.open_table (cur_word); // open necessary table
    cur_word.clear();
    cur_word = read_word (str);
    if (cur_word != "SET")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    cur_word.clear();
    cur_word = read_word (str); // field_name
    // get information about the field, if it exists
    field_struct * f = bd_table.get_field (cur_word.c_str());
    cur_word.clear();
    cur_word = read_word (str);
    if (cur_word != "=")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    string expr_text;
    string t_f_name;
    field_struct * f1;
    string expr_num;
    // processing text-expression
    if (f -> type == TEXT)
    {
        cur_word.clear();
        cur_word = read_word (str);
        if (cur_word[0] != '\'')
        {
            try
            {
                // get information about the field, if it exists
                f1 = bd_table.get_field (cur_word.c_str());
            }
            catch (...)
            {
                throw SQLException (SQLException :: ESE_TEXTEXPR);
            }
            t_f_name = cur_word;
        }
        else 
        {
            string t_str;
            t_str = t_str + cur_word;
            t_str = t_str + ' ';
            while (cur_word[cur_word.length() - 1] != '\'')
            {
                cur_word.clear();
                cur_word = read_word (str);
                if (cur_word.empty())
                {
                    throw SQLException (SQLException::ESE_STR);
                }
                t_str = t_str + cur_word;
                t_str = t_str + ' ';
            }
            t_str.erase(0, 1);
            t_str.erase((t_str.length() - 2), 2);
            if (t_str.length() > f -> field_len)
            {
                throw TableException (TableException :: ESE_FIELDLEN);
            }
            expr_text = t_str;
        }
        cur_word.clear();
        cur_word = read_word (str);
    }
    // processing long-expression
    else
    {
        parser_long_expr::init (str);
        expr_num = parser_long_expr::A (str);
        if (lexer_long_expr::cur_lex_type != END)
        {
            throw SQLException (SQLException :: ESE_LONGEXPR);
        }
        cur_word = lexer_long_expr::c;
    }
    if (cur_word != "WHERE")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    vector <unsigned long> v_where;
    v_where = where_clause (str); // where-clause
    // doing actions for UPDATE
    for (unsigned long i = 0; i < v_where.size(); i++)
    {
        bd_table.read_line(v_where[i]);
        if (f -> type == TEXT)
        {
            if (!expr_text.empty())
            {
                strcpy (f -> text, expr_text.c_str());
            }
            else
            {
                f1 = bd_table.get_field (t_f_name.c_str());
                strcpy (f -> text, f1 -> text);
            }
        }
        else
        {
            string tmp = expr_num + "WHERE";
            parser_long_expr::init (tmp);
            f -> l_num = parser_long_expr::A (tmp, bd_table);
        }
        bd_table.update_line (v_where[i]);
    }
    bd_table.print_table ();
}

void Interpreter :: delete_sentence (string & str)
{
    string cur_word;
    cur_word = read_word (str);
    if (cur_word != "FROM")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    cur_word.clear();
    cur_word = read_word (str); // table_name
    bd_table.open_table (cur_word);
    cur_word.clear();
    cur_word = read_word (str);
    if (cur_word != "WHERE")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    vector <unsigned long> v_where;
    v_where = where_clause (str); // where-clause
    // doing actions for DELETE
    for (unsigned long i = v_where.size(); i > 0; i--)
    {
        bd_table.read_line (v_where[i-1]);
        bd_table.delete_line ();
    }
    bd_table.print_table ();
}

void Interpreter :: create_sentence (string & str)
{
    string cur_word;
    cur_word = read_word (str);
    if (cur_word != "TABLE")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    string table_name;
    table_name = read_word (str);
    cur_word.clear();
    cur_word = read_word (str);
    if (cur_word != "(")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    field_description (str); // first field
    cur_word.clear();
    cur_word = read_word (str); // "," or not
    while (cur_word == ",")
    {
        field_description (str); // next field
        cur_word.clear();
        cur_word = read_word (str); // "," or not
    }
    if (cur_word != ")")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    cur_word.clear();
    // check if it is the end of the comand
    cur_word = read_word (str);
    if (!cur_word.empty())
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    // doing actions for CREATE
    bd_table.create_table (table_name);
    bd_table.print_table();
}

void Interpreter :: drop_sentence (string & str)
{
    string cur_word;
    string t_name;
    cur_word = read_word (str);
    if (cur_word != "TABLE")
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    t_name = read_word (str); // table_name
    cur_word.clear();
    // check if it is the end of the comand
    cur_word = read_word (str);
    if (!cur_word.empty())
    {
        throw SQLException (SQLException :: ESE_COMAND);
    }
    // doing actions for DELETE
    bd_table.delete_table (t_name);
    cout << "The table " << t_name << " was deleted" << endl;
}

void Interpreter :: field_description (string & str)
{
    // creating field
    string f_name;
    f_name = read_word (str); // field_name
    string f_type;
    f_type = read_word (str); // field_type
    if (f_type == "TEXT")
    {
        string cur_word;
        cur_word = read_word (str); 
        if (cur_word != "(")
        {
            throw SQLException (SQLException :: ESE_FIELD);
        }
        cur_word.clear();
        cur_word = read_word (str); 
        unsigned long num;
        try
        {
            num = stoul (cur_word); // convert string to u long
        }
        catch (...)
        {
            throw SQLException (SQLException :: ESE_NUM);
        }
        cur_word.clear();
        cur_word = read_word (str); 
        if (cur_word != ")")
        {
            throw SQLException (SQLException :: ESE_FIELD);
        }
        bd_table.add_text (f_name.c_str(), num);
    }
    else if (f_type == "LONG")
    {
        bd_table.add_long (f_name.c_str());
    }
    else 
    {
        throw SQLException (SQLException :: ESE_FIELD);
    }
}

// list of record numbers need to be treated
vector <unsigned long> Interpreter :: where_clause (string & str)
{
    // doing actions for WHERE-clause
    vector <unsigned long> vect;
    // analisys of WHERE-clause
    parser_where :: init (str, bd_table);
    string s = parser_where :: W0 (str, bd_table);
    if (lexer_where::cur_lex_type_w != END_w)
    {
        throw SQLException (SQLException :: ESE_WHERE);
    }
    string f_name;
    string w;
    string s_log;
    // processing necesssary mode
    switch (parser_where::mode)
    {
        case LIKE_alt:
            f_name = read_word(s);
            w = read_word(s);
            if (w == "NOT")
            {
                w.clear();
                w = read_word(s);
                w.clear();
                w = read_word(s);
                w.pop_back();
                w.erase(0, 1);
                regex rx (w.c_str());
                // filling in the list
                // if NOT LIKE
                for (unsigned long i = 0; i < 
                     bd_table.t_struct.num_of_records; i++)
                {
                    bd_table.read_line (i+1);
                    field_struct * f;
                    f = bd_table.get_field (f_name.c_str());
                    if (!regex_match (f -> text, rx))
                    {
                        vect.push_back (i + 1);
                    }
                }
            }
            else if (w == "LIKE")
            {
                w.clear();
                w = read_word(s);
                w.pop_back();
                w.erase(0, 1);
                regex rx (w.c_str());
                // filling in the list
                // if LIKE
                for (unsigned long i = 0; i < 
                     bd_table.t_struct.num_of_records; i++)
                {
                    bd_table.read_line (i+1);
                    field_struct * f;
                    f = bd_table.get_field (f_name.c_str());
                    if (regex_match (f -> text, rx))
                    {
                        vect.push_back (i + 1);
                    }
                }
            }
            break;
            
        case IN_alt_L:
            for (unsigned long i = 0; i < 
                 bd_table.t_struct.num_of_records; i++)
            {
                string s_tmp = s;
                bd_table.read_line (i+1);
                long num;
                // calculating the value of long-expression
                parser_long_expr::init (s_tmp);
                num = parser_long_expr::A (s_tmp, bd_table);
                // filling in the list
                // if IN
                if (lexer_long_expr::c == "IN")
                {
                    if (parser_where::mst_l.count(num))
                    {
                        vect.push_back (i + 1);
                    }
                }
                // filling in the list
                // if NOT IN
                else if (lexer_long_expr::c == "NOT")
                {
                    if (!parser_where::mst_l.count(num))
                    {
                        vect.push_back (i + 1);
                    }
                }
            }
            break;
            
        case IN_alt_T:
            // if text-expression is name of the field with type TEXT
            if (s[0] != '\'')
            {
                f_name = read_word(s);
                w = read_word(s);
                for (unsigned long i = 0; i < 
                     bd_table.t_struct.num_of_records; i++)
                {
                    bd_table.read_line (i+1);
                    field_struct * f;
                    f = bd_table.get_field (f_name.c_str());
                    // filling in the list
                    // if IN
                    if (w == "IN")
                    {
                        if (parser_where::mst_s.
                            count(string(f -> text)))
                        {
                            vect.push_back (i + 1);
                        }
                    }
                    // filling in the list
                    // if NOT IN
                    else if (w == "NOT")
                    {
                        if (!parser_where::mst_s.
                            count(string(f -> text)))
                        {
                            vect.push_back (i + 1);
                        }
                    }
                }
            }
            // if text-expression is string
            else
            {
                string t_str;
                w = read_word(s);
                t_str = w;
                while (w[w.length() - 1] != '\'')
                {
                    w.clear();
                    w = read_word (s);
                    t_str = t_str + ' ';
                    t_str = t_str + w;
                }
                t_str.pop_back();
                w.clear();
                w = read_word (s);
                // filling in the list
                // if IN
                if (w == "IN")
                {
                    if (parser_where::mst_s.count(t_str))
                    {
                        for (unsigned long i = 0; i < 
                             bd_table.t_struct.num_of_records; i++)
                        {
                            vect.push_back (i + 1);
                        }
                    }
                }
                // filling in the list
                // if NOT IN
                else if (w == "NOT")
                {
                    if (!parser_where::mst_s.count(t_str))
                    {
                        for (unsigned long i = 0; i < 
                             bd_table.t_struct.num_of_records; i++)
                        {
                            vect.push_back (i + 1);
                        }
                    }
                }
            }
            break;
        
        case LOG_alt:
            long num;
            // filling in the list
            for (unsigned long i = 0; i < 
                 bd_table.t_struct.num_of_records; i++)
            {
                s_log = s;
                bd_table.read_line (i+1);
                // calculating the value of logic-expression
                parser_where :: init (s_log, bd_table);
                num = parser_where :: W31 (s_log, bd_table);
                if (num)
                {
                    vect.push_back (i + 1);
                }
            }
            break;
            
        case ALL_alt:
            // all records
            for (unsigned long i = 0; i < 
                 bd_table.t_struct.num_of_records; i++)
            {
                vect.push_back (i + 1);
            }
            break;
    }
    sort(vect.begin(), vect.end());
    vect.erase(unique(vect.begin(), vect.end()), vect.end());
    return vect;
}

#endif
