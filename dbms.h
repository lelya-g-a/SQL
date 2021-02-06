#ifndef _DBMS_H_
#define _DBMS_H_

#define MAX_FIELD_NAME_LEN 15
#define MAX_TABLE_NAME_LEN 15
#define MAX_TEXT_LEN 20

#include <cstdio>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

// field_type --- types of field data
enum field_type
{
    TEXT,
    LONG
};

// field_struct --- structure for one field
struct field_struct
{
    char name [MAX_FIELD_NAME_LEN];
    field_type type;
    char text [MAX_TEXT_LEN];
    long l_num;
    unsigned long field_len;
};

// table_struct --- main info about table
struct table_struct
{
    char table_name [MAX_TABLE_NAME_LEN];
    unsigned long num_of_fields;
    unsigned long num_of_records;
    unsigned long title_length; // for easy pointer moving
};

// TableException --- exception class
class TableException 
{
public:
    string err_message;
    enum table_exception_code 
    {
        ESE_FILEOPEN,
        ESE_FILEWRITE,
        ESE_FILEREAD,
        ESE_FILESEEK,
        ESE_EMPTYFILE,
        ESE_FILEREMOVE,
        ESE_FILERENAME,
        ESE_FIELDNAME,
        ESE_LINEFIND,
        ESE_LINENUM,
        ESE_FIELDLEN,
        ESE_FILENAME
    };
    TableException (table_exception_code);
    void report (); // output the message
    ~ TableException () {}
};

// TableClass --- table structure
class TableClass
{
public:
    struct table_struct t_struct;
    vector <field_struct> fields;
    TableClass ();
    void add_text (const char *, const int); // add new field with text
    void add_long (const char *); // add new field with number
    ~ TableClass () {}
};

// Table --- class for work with tables
class Table : public TableClass
{
public:
    Table () {}
    void create_table (string);
    void open_table (string);
    void delete_table (string);
    field_struct * get_field (const char [MAX_FIELD_NAME_LEN]);
    void add_line ();
    unsigned long find_line (); // find line number with the data
    void delete_line ();
    void update_line (const unsigned long);
    void read_first ();
    void read_line (const unsigned long);
    void read_next ();
    void read_prev ();
    // 4 functions for printing to standard output
    void print_line_names (); // print names of fields
    void print_line (const unsigned long);
    void print_line (); // print line with the data
    void print_table (); // print whole table
    // output not full lines
    void print_short_line_names (vector <string>);
    void print_short_line (vector <string>, unsigned long);
    ~ Table () {}
};

/*--------------------------------------------------------------------*/

/*---------------TableException---------------*/
TableException :: TableException (table_exception_code errcode)
{
    switch (errcode) {
        case ESE_FILEOPEN:
            err_message = "ERROR: can't open file";
            break;
        case ESE_FILEWRITE:
            err_message = "ERROR: can't write to file";
            break;
        case ESE_FILEREAD:
            err_message = "ERROR: can't read from file";
            break;
        case ESE_FILESEEK:
            err_message = "ERROR: can't move the pointer";
            break;
        case ESE_EMPTYFILE:
            err_message = "ERROR: file you try to open is empty";
            break;
        case ESE_FILEREMOVE:
            err_message = "ERROR: can't delete file";
            break;
        case ESE_FILERENAME:
            err_message = "ERROR: can't rename file";
            break;
        case ESE_FIELDNAME:
            err_message = "ERROR: no such field name";
            break;
        case ESE_LINEFIND:
            err_message = "ERROR: no such record in the table";
            break;
        case ESE_LINENUM:
            err_message = "ERROR: no such line number in the table";
            break;
        case ESE_FIELDLEN:
            err_message = "ERROR: wrong field name or string lenth";
            break;
        case ESE_FILENAME:
            err_message = "ERROR: wrong table name lenth";
            break;
    }
}

void TableException :: report ()
{
    cout << err_message << endl;
}


/*---------------TableClass---------------*/
TableClass :: TableClass ()
{
    // setting initial values
    t_struct.num_of_fields = 0;
    t_struct.num_of_records = 0;
    t_struct.title_length = sizeof (struct table_struct);
}

void TableClass :: add_text (const char *f_name, const int f_length)
{
    if ((f_length <= MAX_TEXT_LEN) && (f_length > 0) &&
        (strlen(f_name) <= MAX_FIELD_NAME_LEN))
    {
        struct field_struct f_struct;
        strcpy (f_struct.name, f_name);
        f_struct.type = TEXT;
        strcpy (f_struct.text, "");
        f_struct.l_num = 0;
        f_struct.field_len = f_length;
        t_struct.num_of_fields += 1;
        t_struct.title_length += sizeof (struct field_struct);
        fields.push_back (f_struct);
    }
    else
    {
        throw TableException (TableException :: ESE_FIELDLEN);
    }
}

void TableClass :: add_long (const char *f_name)
{
    if (strlen(f_name) <= MAX_FIELD_NAME_LEN)
    {
        struct field_struct f_struct;
        strcpy (f_struct.name, f_name);
        f_struct.type = LONG;
        strcpy (f_struct.text, "");
        f_struct.l_num = 0;
        f_struct.field_len = sizeof (long);
        t_struct.num_of_fields += 1;
        t_struct.title_length += sizeof (struct field_struct);
        fields.push_back (f_struct);
    }
    else
    {
        throw TableException (TableException :: ESE_FIELDLEN);
    }
}


/*---------------Table---------------*/
void Table :: create_table (string t_name)
{
    if (t_name.length() > MAX_TABLE_NAME_LEN)
    {
        throw TableException (TableException :: ESE_FILENAME);
    }
    strcpy (t_struct.table_name, t_name.c_str());
    string file_name = t_name + ".txt";
    // if file exists, its content is deleting
    FILE * f = fopen (file_name.c_str(), "wb+");
    if (f == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    // writing the main info about the table
    if (fwrite (&(t_struct), sizeof (struct table_struct), 1, f) == 0)
    {
        throw TableException (TableException :: ESE_FILEWRITE);
    }
    // writing the info about all fields with empty data
    // it's not a line of the table
    // it's a part of title info
    for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
    {
        strcpy (fields[i].text, "");
        fields[i].l_num = 0;
        if (fwrite (&(fields[i]), sizeof (fields[i]), 1, f) == 0)
        {
            throw TableException (TableException :: ESE_FILEWRITE);
        }
    }
    fclose (f);
}

void Table :: open_table (string t_name)
{
    string file_name = t_name + ".txt";
    // the file have to exist
    FILE * f = fopen (file_name.c_str(), "rb");
    if (f == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    if (fseek (f, 0, SEEK_SET) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    // check if there is no any information
    if (feof (f))
    {
        throw TableException (TableException :: ESE_EMPTYFILE);
    }
    // saving the info to the object info
    if (fread (&(t_struct), sizeof (struct table_struct), 1, f) == 0)
    {
        throw TableException (TableException :: ESE_FILEREAD);
    }
    for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
    {
        struct field_struct f_struct;
        if (fread (&(f_struct), sizeof(f_struct), 1, f) == 0)
        {
            throw TableException (TableException :: ESE_FILEREAD);
        }
        fields.push_back (f_struct);
    }
    fclose (f);
}

void Table :: delete_table (string t_name)
{
    // cleaning any info about the table
    t_struct.num_of_fields = 0;
    t_struct.num_of_records = 0;
    t_struct.title_length = sizeof (struct table_struct);
    fields.clear();
    string file_name = t_name + ".txt";
    // deleting the file with data
    if (remove (file_name.c_str()) != 0)
    {
        throw TableException (TableException :: ESE_FILEREMOVE);
    }
}

field_struct * Table :: get_field (const char n [MAX_FIELD_NAME_LEN])
{
    unsigned long i = 0; 
    while ((i < t_struct.num_of_fields) && 
            (strcmp (n, fields[i].name) != 0))
    {
        i++;
    }
    // if there is no such field or if there is no any fields
    if (i == t_struct.num_of_fields)
    {
        throw TableException (TableException :: ESE_FIELDNAME);
    }
    else
    {
        return &fields[i];
    }
}

void Table :: add_line ()
{
    string t_name = string (t_struct.table_name, 
                            strlen (t_struct.table_name));
    string file_name = t_name + ".txt";
    FILE * f = fopen (file_name.c_str(), "ab");
    if (f == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
    {
        if (fwrite (&(fields[i]), sizeof (fields[i]), 1, f) == 0)
        {
            throw TableException (TableException :: ESE_FILEWRITE);
        }
    }
    fclose (f);
    // changing the number of records in the table
    t_struct.num_of_records += 1;
    f = fopen (file_name.c_str(), "rb+");
    if (f == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    if (fseek (f, 0, SEEK_SET) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    if (fwrite (&(t_struct), sizeof (struct table_struct), 1, f) == 0)
    {
        throw TableException (TableException :: ESE_FILEWRITE);
    }
    fclose(f);

}

unsigned long Table :: find_line ()
{
    string t_name = string (t_struct.table_name, 
                            strlen (t_struct.table_name));
    string file_name = t_name + ".txt";
    FILE * f = fopen (file_name.c_str(), "rb");
    if (f == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    unsigned long line_num = 0;
    // lines counter
    unsigned long j = 0;
    if (fseek (f, t_struct.title_length, SEEK_SET) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    while (!line_num && (j < t_struct.num_of_records))
    {
        int flag_f = 1;
        // fields counter
        unsigned long i = 0;
        while (i < t_struct.num_of_fields)
        {
            struct field_struct f_struct;
            if (fread (&(f_struct), sizeof (f_struct), 1, f) == 0)
            {
                throw TableException (TableException :: ESE_FILEREAD);
            }
            if (f_struct.type == TEXT)
            {
                // if texts are not equal
                if (strcmp (f_struct.text, fields[i].text))
                {
                    flag_f = 0;
                }
            }
            if (f_struct.type == LONG)
            {
                // if numbers are not equal
                if (f_struct.l_num != fields[i].l_num)
                {
                    flag_f = 0;
                }
            }
            i++;
        }
        j++;
        // if there is necessary line
        if (flag_f)
        {
            line_num = j;
        }
    }
    fclose (f);
    // if there is no necessary line
    if (line_num == 0)
    {
        throw TableException (TableException :: ESE_LINEFIND);
    }
    else
    {
        return line_num;
    }
}

void Table :: delete_line ()
{
    unsigned long line_num = find_line();
    string t_name = string (t_struct.table_name, 
                            strlen (t_struct.table_name));
    string file_name = t_name + ".txt";
    FILE * f = fopen (file_name.c_str(), "rb");
    if (f == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    // a temporary file for table without the line
    FILE * tmp = fopen ("tmp.txt", "wb");
    if (tmp == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    // changing the number of records
    t_struct.num_of_records -= 1;
    // title in temporary file
    if (fwrite (&(t_struct), sizeof (struct table_struct), 1, tmp) == 0)
    {
        throw TableException (TableException :: ESE_FILEWRITE);
    }
    for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
    {
        strcpy (fields[i].text, "");
        fields[i].l_num = 0;
        if (fwrite (&(fields[i]), sizeof (fields[i]), 1, tmp) == 0)
        {
            throw TableException (TableException :: ESE_FILEWRITE);
        }
    }
    if (fseek (f, t_struct.title_length, SEEK_SET) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    // lines before the line we have to delete
    for (unsigned long j = 0; j < line_num - 1; j++)
    {
        for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
        {
            struct field_struct f_struct;
            if (fread (&(f_struct), sizeof (f_struct), 1, f) == 0)
            {
                throw TableException (TableException :: ESE_FILEREAD);
            }
            if (fwrite (&(f_struct), sizeof (f_struct), 1, tmp) == 0)
            {
                throw TableException (TableException :: ESE_FILEWRITE);
            }
        }
    }
    // moving the pointer through the line we have to delete
    if (fseek (f, sizeof (struct field_struct) * t_struct.num_of_fields, 
               SEEK_CUR) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    // lines after the line we have to delete
    for (unsigned long j = line_num; j < t_struct.num_of_records + 1; 
         j++)
    {
        for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
        {
            struct field_struct f_struct;
            if (fread (&(f_struct), sizeof (f_struct), 1, f) == 0)
            {
                throw TableException (TableException :: ESE_FILEREAD);
            }
            if (fwrite (&(f_struct), sizeof (f_struct), 1, tmp) == 0)
            {
                throw TableException (TableException :: ESE_FILEWRITE);
            }
        }
    }
    fclose (f);
    // deleting the file we used
    if (remove (file_name.c_str()) != 0)
    {
        throw TableException (TableException :: ESE_FILEREMOVE);
    }
    fclose (tmp);
    // rename the temporary file
    // it becomes a main file we work
    if (rename ("tmp.txt", file_name.c_str()) != 0)
    {
        throw TableException (TableException :: ESE_FILERENAME);
    }
}

void Table :: update_line (const unsigned long line_num)
{
    if ((line_num > t_struct.num_of_records) || 
        (line_num <= 0))
    {
        throw TableException (TableException :: ESE_LINENUM);
    }
    string t_name = string (t_struct.table_name, 
                            strlen (t_struct.table_name));
    string file_name = t_name + ".txt";
    FILE * f = fopen (file_name.c_str(), "rb+");
    if (f == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    if (fseek (f, t_struct.title_length, SEEK_SET) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    // moving through lines before the line we need
    if (fseek (f, sizeof (struct field_struct) * t_struct.num_of_fields 
               * (line_num - 1) , SEEK_CUR) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    // rewrite data
    for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
    {
        if (fwrite (&(fields[i]), sizeof (fields[i]), 1, f) == 0)
        {
            throw TableException (TableException :: ESE_FILEWRITE);
        }
    }
    fclose (f);
}

void Table :: read_first ()
{
    // check if the table is empty
    if (t_struct.num_of_records == 0)
    {
        throw TableException (TableException :: ESE_LINEFIND);
    }
    string t_name = string (t_struct.table_name, 
                            strlen (t_struct.table_name));
    string file_name = t_name + ".txt";
    FILE * f = fopen (file_name.c_str(), "rb+");
    if (f == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    if (fseek (f, t_struct.title_length, SEEK_SET) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
    {
        if (fread (&(fields[i]), sizeof (fields[i]), 1, f) == 0)
        {
            throw TableException (TableException :: ESE_FILEREAD);
        }
    }
    fclose (f);
}

void Table :: read_line (const unsigned long line_num)
{
    if ((line_num > t_struct.num_of_records) || 
        (line_num <= 0))
    {
        throw TableException (TableException :: ESE_LINENUM);
    }
    string t_name = string (t_struct.table_name, 
                            strlen (t_struct.table_name));
    string file_name = t_name + ".txt";
    FILE * f = fopen (file_name.c_str(), "rb+");
    if (f == NULL)
    {
        throw TableException (TableException :: ESE_FILEOPEN);
    }
    if (fseek (f, t_struct.title_length, SEEK_SET) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    // moving through lines before the line we need
    if (fseek (f, sizeof (struct field_struct) * t_struct.num_of_fields
               * (line_num - 1) , SEEK_CUR) != 0)
    {
        throw TableException (TableException :: ESE_FILESEEK);
    }
    for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
    {
        if (fread (&(fields[i]), sizeof (fields[i]), 1, f) == 0)
        {
            throw TableException (TableException :: ESE_FILEREAD);
        }
    }
    fclose (f);
}

void Table :: read_next ()
{
    read_line (find_line () + 1);
}

void Table :: read_prev ()
{
    read_line (find_line () - 1);
}

void Table :: print_line_names ()
{
    for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
    {
        unsigned long wid;
        wid = MAX_FIELD_NAME_LEN;
        if (fields[i].field_len > wid)
        {
            wid = fields[i].field_len;
        }
        cout.width (wid + 2);
        cout << fields[i].name;
    }
    cout.width (0);
    cout << endl;
}

void Table :: print_line (const unsigned long line_num)
{
    read_line (line_num);
    for (unsigned long i = 0; i < t_struct.num_of_fields; i++)
    {
        unsigned long wid;
        wid = MAX_FIELD_NAME_LEN;
        if (fields[i].field_len > wid)
        {
            wid = fields[i].field_len;
        }
        cout.width (wid + 2);
        if (fields[i].type == TEXT)
        {
            cout << fields[i].text;
        }
        if (fields[i].type == LONG)
        {
            cout << fields[i].l_num;
        }
    }
    cout.width (0);
    cout << endl;
}

void Table :: print_line ()
{
    print_line (find_line());
}

void Table :: print_table ()
{
    cout << endl;
    cout << t_struct.table_name << endl;
    print_line_names ();
    for (unsigned long i = 1; i <= t_struct.num_of_records; i++)
    {
        print_line (i);
    }
    cout << endl;
}

void Table :: print_short_line_names (vector <string> vect)
{
    for (unsigned long i = 0; i < vect.size(); i++)
    {
        unsigned long j = 0;
        while ((j < t_struct.num_of_fields) && 
                (strcmp (vect[i].c_str(), fields[j].name) != 0))
        {
            j++;
        }
        // if there is no such field or if there is no any fields
        if (j == t_struct.num_of_fields)
        {
            throw TableException (TableException :: ESE_FIELDNAME);
        }
        else
        {
            unsigned long wid;
            wid = MAX_FIELD_NAME_LEN;
            if (fields[j].field_len > wid)
            {
                wid = fields[j].field_len;
            }
            cout.width (wid + 2);
            cout << fields[j].name;
        }
    }
    cout.width (0);
    cout << endl;
}

void Table :: print_short_line (vector <string> vect, unsigned long num)
{
    read_line (num);
    for (unsigned long i = 0; i < vect.size(); i++)
    {
        field_struct * f = get_field (vect[i].c_str());
        unsigned long wid;
        wid = MAX_FIELD_NAME_LEN;
        if (f -> field_len > wid)
        {
            wid = f -> field_len;
        }
        cout.width (wid + 2);
        if (f -> type == TEXT)
        {
            cout << f -> text;
        }
        else
        {
            cout << f -> l_num;
        }
    }
    cout.width (0);
    cout << endl;
}

#endif
