#include <bits/stdc++.h>
#include <windows.h>
#include <ctype.h>

using namespace std;

double p;
int m, sz;
bool is_toogle_bit[222];

string message, pl;
queue <int> temp_crc, modulo, temp;

vector <int> rblock[111], rcb_block[111];
vector <int> block[111], cb_block[111], crc, mark[111];

void setColor(int val)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), val);
}

void input()
{
    cout << "enter string: ";
    getline(cin, message);

    cout << "enter number of data byte in a row: ";
    cin >> m;

    cout << "enter probability: ";
    cin >> p;

    cout << "enter generator polynomial: ";
    cin >> pl;

    cout << endl << endl;

    if (message.size() % m != 0)
    {
        for (int i = 0; i < (message.size() % m); ++i)
            message += '~';
    }

    cout << "data string after padding: " << message << endl << endl;

    int idx = 0;

    cout << "data block <ascii code of m characters per row>:" << endl;

    for (int i = 0; i < message.size(); ++i)
    {
        int val = message[i];

        for (int j = 7; j >= 0; --j)
        {
            if (val & (1 << j))
                block[idx].push_back(1);
            else
                block[idx].push_back(0);
        }

        if ((i + 1) % m == 0)
        {
            for (int j = 0; j < block[idx].size(); ++j)
                cout << block[idx][j];

            ++idx, ++sz, cout << endl;
        }
    }

    cout << endl;
}

void addCheckBits()
{
    cout << "data block after adding check bits:" << endl;

    for (int i = 0; i < sz; ++i)
    {
        int j = 0, power = 0, idx = 1;

        while (j < block[i].size())
        {
            if (idx == (1 << power))
                cb_block[i].push_back(0), ++power;
            else
                cb_block[i].push_back(block[i][j]), ++j;

            ++idx;
        }
    }

    for (int i = 0; i < sz; ++i)
    {
        for (int j = 0; (1 << j) < cb_block[i].size(); ++j)
        {
            int parity = 0;

            for (int k = (1 << j) - 1; k < cb_block[i].size(); ++k)
            {
                if (((k + 1) & (1 << j)) && cb_block[i][k])
                    ++parity;
            }

            if (parity % 2)
                cb_block[i][(1 << j) - 1] = 1;
        }
    }

    for (int i = 0; i < sz; ++i)
    {
        int power = 0;

        for (int j = 0; j < cb_block[i].size(); ++j)
        {
            if ((j + 1) == (1 << power))
            {
                setColor(2);
                cout << cb_block[i][j];
                setColor(15);
                ++power;
            }
            else
                cout << cb_block[i][j];
        }

        cout << endl;
    }

    cout << endl;
}

void columnWiseSerialize()
{
    cout << "data bits after column-wise serialization:" << endl;

    for (int i = 0; i < cb_block[0].size(); ++i)
    {
        for (int j = 0; j < sz; ++j)
            cout << cb_block[j][i], crc.push_back(cb_block[j][i]);
    }

    cout << endl << endl;
}

void modulo2Division()
{
    for (int i = 0; i < pl.size(); ++i)
        modulo.push(temp_crc.front()), temp_crc.pop();

    while (true)
    {
        for (int i = 0; i < pl.size(); ++i)
        {
            if (modulo.front() == (int) (pl[i] - '0') && temp.size())
                temp.push(0);
            else if (modulo.front() != (int) (pl[i] - '0'))
                temp.push(1);

            modulo.pop();
        }

        while (temp.size())
            modulo.push(temp.front()), temp.pop();

        while (temp_crc.size() && modulo.size() < pl.size())
            modulo.push(temp_crc.front()), temp_crc.pop();

        if (temp_crc.size() + modulo.size() < pl.size())
            break;
    }
}

void computeCRC()
{
    cout << "data bits after appending CRC checksum <sent frame>:" << endl;

    for (int i = 0; i < crc.size(); ++i)
        temp_crc.push(crc[i]);

    for (int i = 0; i < pl.size() - 1; ++i)
        temp_crc.push(0);

    modulo2Division();

    for (int i = 0; i < crc.size(); ++i)
        cout << crc[i];

    setColor(3);

    for (int i = 0; i < pl.size() - modulo.size() - 1; ++i)
        crc.push_back(0), cout << 0;


    while (modulo.size())
    {
        crc.push_back(modulo.front());
        cout << modulo.front();
        modulo.pop();
    }

    setColor(15), cout << endl << endl;
}

void receivedFrame()
{
    cout << "received frame:" << endl;

    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    default_random_engine generator(seed);
    uniform_real_distribution<double> dist(0.0, 1.0);

    for (int i = 0; i < crc.size(); ++i)
    {
        if (dist(generator) <= p)
        {
            is_toogle_bit[i] = true;
            setColor(4);
            cout << (crc[i] ? 0 : 1);
            setColor(15);
            crc[i] = crc[i] ? 0 : 1;
        }
        else
            cout << crc[i];
    }

    cout << endl << endl;
}

void checkReceivedFrame()
{
    cout << "result of CRC checksum matching: ";

    for (int i = 0; i < crc.size(); ++i)
        temp_crc.push(crc[i]);

    modulo2Division();

    while (modulo.size())
    {
        if (modulo.front() == 1)
        {
            cout << "error detected" << endl << endl;
            return;
        }

        modulo.pop();
    }

    cout << "no error" << endl << endl;
}

void removingCRC()
{
    cout << "data block after removing CRC checksum bits:" << endl;

    int idx = 0, check_bit_sz = 0, power = 0;

    for (int i = 1; i <= m * 8; ++i)
    {
        if ((1 << power) & i)
            ++check_bit_sz, ++power;
    }

    for (int i = 0; i < (m * 8) + check_bit_sz; ++i)
    {
        for (int j = 0; j < sz; ++j)
        {
            rcb_block[j].push_back(crc[idx]);

            if (is_toogle_bit[idx])
                mark[j].push_back(1);
            else
                mark[j].push_back(0);

            ++idx;
        }
    }

    for (int i = 0; i < sz; ++i)
    {
        for (int j = 0; j < rcb_block[i].size(); ++j)
        {
            if (mark[i][j])
                setColor(4);

            cout << rcb_block[i][j];
            setColor(15);
        }

        cout << endl;
    }

    cout << endl;
}

void removeError()
{
    cout << "data block after removing check bits:" << endl;

    for (int i = 0; i < sz; ++i)
    {
        int corrupt_idx = 0;

        for (int j = 0; (1 << j) < rcb_block[i].size(); ++j)
        {
            int parity = 0;

            for (int k = (1 << j); k < rcb_block[i].size(); ++k)
            {
                if (((k + 1) & (1 << j)) && rcb_block[i][k])
                    ++parity;
            }

            if ((parity % 2) != rcb_block[i][(1 << j) - 1])
                corrupt_idx |= (1 << j);
        }

        if (corrupt_idx)
            rcb_block[i][corrupt_idx - 1] = (rcb_block[i][corrupt_idx - 1] ? 0 : 1);
    }

    for (int i = 0; i < sz; ++i)
    {
        int power = 0;

        for (int j = 0; j < rcb_block[i].size(); ++j)
        {
            if ((1 << power) == (j + 1))
                ++power;
            else
                rblock[i].push_back(rcb_block[i][j]);
        }
    }

    for (int i = 0; i < sz; ++i)
    {
        for (int j = 0; j < rblock[i].size(); ++j)
            cout << rblock[i][j];

        cout << endl;
    }

    cout << endl;
}

void outputMessage()
{
    cout << "output frame: ";

    int val = 0, idx = 7;

    for (int i = 0; i < sz; ++i)
    {
        for (int j = 0; j < rblock[i].size(); ++j)
        {
            if (rblock[i][j])
                val |= (1 << idx);

            if ((j + 1) % 8 == 0)
                cout << (char) val, idx = 7, val = 0;
            else
                --idx;
        }
    }

    cout << endl;
}

int main()
{
    input();
    addCheckBits();
    columnWiseSerialize();
    computeCRC();
    receivedFrame();
    checkReceivedFrame();
    removingCRC();
    removeError();
    outputMessage();

    return 0;
}

/*
Hamming Code
2
.05
10101
*/

/*
a
1
0
101
*/

/*
Computer Networks
4
.04
1010111
*/

/*
Error Detection
3
.02
10111
*/

/*
physical layer
3
.05
1010
*/

/*
Error Correction
4
.02
101011
*/

/*
no error
3
0
101
*/

/*
many errors
4
.1
10001
*/
