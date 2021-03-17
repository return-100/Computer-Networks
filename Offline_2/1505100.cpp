#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <string>
#include <bits/stdc++.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

const int inf = 11111111;

string ipname;
map <string, bool> adj, nadj;
map <string, pair<string, int> > mp;

int str_to_int(string str)
{
    int ret;
    stringstream ss;
    ss << str;
    ss >> ret;
    return ret;
}

string int_to_str(int val)
{
    string ret;
    stringstream ss;
    ss << val;
    ss >> ret;
    return ret;
}

void create_table(char *filename)
{
    ifstream fin(filename);
    string line;

    while (getline(fin, line))
    {
        string info[3];

        for (int i = 0, j = 0; i < 3; ++i)
        {
            info[i] = "";
            while (j < line.size())
            {
                if (line[j] == ' ')
                    break;

                info[i] += line[j], ++j;
            }

            ++j;
        }

        if (info[0] != ipname)
            swap(info[0], info[1]);

        if (ipname == info[0])
        {
            mp[info[1]] = {info[1], str_to_int(info[2])}, adj[info[1]] = true;

            if (nadj[info[1]])
                adj[info[1]] = true, nadj[info[1]] = false;
        }
        else
        {
            if (!adj[info[0]])
                nadj[info[0]] = true;

            if (!adj[info[1]])
                nadj[info[1]] = true;
        }
    }

    map <string, bool> :: iterator itr = nadj.begin();

    for (itr; itr != nadj.end(); ++itr)
    {
        if (itr->second)
            mp[itr->first] = {itr->first, inf};
    }
}

void show_routing_table()
{
    cout << "Destination            Next Hop            Cost" << endl;
    cout << "-----------            --------          --------" << endl;

    map <string, pair<string, int> > :: iterator itr = mp.begin();

    for (itr; itr != mp.end(); ++itr)
        cout << itr->first << "         " << itr->second.first << "            " << itr->second.second << endl;
}

void change_cost(string ip1, string ip2, int cost)
{
    if (ip1 != ipname)
        swap(ip1, ip2);

    mp[ip2] = {ip2, cost};
    adj[ip2] = true;
}

/*void send_table()
{
    int sockfd;
    int bind_flag;
    char *buffer = (char *) mp;

    struct sockaddr_in client_address;
    client_address.sin_family = AF_INET;
	client_address.sin_port = htons(4747);
	client_address.sin_addr.s_addr = inet_addr(argv[1]);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bind_flag = bind(sockfd, (struct sockaddr*) &client_address, sizeof(sockaddr_in));

	for (auto itr : mp)
	{
        if (itr->second.second != inf)
        {
            client_address.sin_addr.s_addr = inet_addr(itr->second.second);
            sendto(sockfd, buffer, 1024, 0, (struct sockaddr*) &client_address, sizeof(sockaddr_in));
        }
	}
}*/

int main(int argc, char *argv[])
{

	int bytes_received, sockfd, bind_flag;
   	struct sockaddr_in client_address;

	if(argc != 3)
	{
		printf("%s <ip address>\n", argv[1]);
		exit(1);
	}

	ipname = "";

	for (int i = 0; i < strlen(argv[1]); ++i)
        ipname += argv[1][i];

    client_address.sin_family = AF_INET;
    client_address.sin_port = htons(4747);
    inet_pton(AF_INET, argv[1], &client_address.sin_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    bind_flag = bind(sockfd, (struct sockaddr*) &client_address, sizeof(sockaddr_in));

	if(!bind_flag) cout<<"Connection successful!!"<<endl;
	else cout<<"Connection failed!!!"<<endl;

    cout<<"--------------------------------------"<<endl;

    struct sockaddr_in router_address;
    socklen_t addrlen;

    create_table(argv[2]);

	while (true)
	{
		char buffer[1024];

		bytes_received = recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr*) &router_address, &addrlen);

		cout << buffer << endl;

		if(bytes_received!=-1)
		{
            string command = "";

			for (int i = 0; i < 4; ++i)
				command += buffer[i];

            //if (command = "clk")
                //send_table();
            if (command == "show")
                show_routing_table();
            else if (command == "cost")
            {
                string ip1 = "";
                string ip2 = "";
                int arr[4], brr[4], var, cost;

                for (int i = 4; i < 12; ++i)
                {
                    var = (int) buffer[i];

                    if (var < 0)
                        var += 256;

                    if (i < 8)
                        arr[i - 4] = var;
                    else
                        brr[i - 8] = var;
                }

                ip1 = int_to_str(arr[0]) + "." + int_to_str(arr[1]) + "." + int_to_str(arr[2]) + "." + int_to_str(arr[3]);
                ip2 = int_to_str(brr[0]) + "." + int_to_str(brr[1]) + "." + int_to_str(brr[2]) + "." + int_to_str(brr[3]);
                cost = ((int) buffer[13]) * 8 + ((int) buffer[12]);

                change_cost(ip1, ip2, cost);
            }
			else if (command == "send")
			{
				string ip1 = "";
                string ip2 = "";
                string text = "";
                int arr[4], brr[4], var, len;

                for (int i = 4; i < 12; ++i)
                {
                    var = (int) buffer[i];

                    if (var < 0)
                        var += 256;

                    if (i < 8)
                        arr[i - 4] = var;
                    else
                        brr[i - 8] = var;
                }

                ip1 = int_to_str(arr[0]) + "." + int_to_str(arr[1]) + "." + int_to_str(arr[2]) + "." + int_to_str(arr[3]);
                ip2 = int_to_str(brr[0]) + "." + int_to_str(brr[1]) + "." + int_to_str(brr[2]) + "." + int_to_str(brr[3]);
                len = ((int) buffer[13]) * 8 + ((int) buffer[12]);

                for (int i = 14; i < strlen(buffer); ++i)
                    text += buffer[i];
			}
		}
	}

	return 0;

}
