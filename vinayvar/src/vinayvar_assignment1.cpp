/**
 * @vinayvar_assignment1
 * @author  vinay vardhaman <vinayvar@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <list>

#include "../include/global.h"
#include "../include/logger.h"

using namespace std;

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */

#define BACKLOG 10 // how many pending connections queue will hold


struct ipstruct
{
int fd;
char ip[INET_ADDRSTRLEN];
};

list<ipstruct> iplist;
list<ipstruct>::iterator it;


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) 
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

char* ExtractCommand(char* buffer)
{
	char* command;
	int k;
	for(k=0; buffer[k] != ' '; k++)
	{
		command[k] = buffer[k];
	}
	command[k] = '\0';
	
	return command;	
}


// Starting the Server.
int StartServer(char **argv)
{
	fd_set master;			// master file descriptor list
	fd_set read_fds;		// temp file descriptor list for select()
	int fdmax;				// maximum file descriptor number	
	
	int listener;			// listening socket descriptor	
	int newfd;				// newly accept()ed socket descriptor
	int tempfd;             // Socket descriptor for getting an IP address.
	
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	
	char buf[1024];			// buffer for client data
	int nbytes;
	
	char remoteIP[INET6_ADDRSTRLEN];
	int yes=1;				// for setsockopt() SO_REUSEADDR, below
	int i, j, rv;
	
	struct addrinfo hints, *ai, *p;
	FD_ZERO(&master);		// clear the master and temp sets
	FD_ZERO(&read_fds);
	
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, argv[2], &hints, &ai)) != 0) 
	{
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next)
	{
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0) {
			continue;
		}
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
		break;
	}
	if (p == NULL)
	{ 
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	freeaddrinfo(ai); // all done with this
	// listen
	if (listen(listener, 10) == -1) {
		perror("listen");
		exit(3);
	}
	
	FD_SET(0,&master); // Set to read from the terminal
	// add the listener to the master set
	FD_SET(listener, &master);
	// keep track of the biggest file descriptor
	fdmax = listener; // so far, it's this one
	// main loop
	for(;;) 
	{
		read_fds = master; // copy it
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(4);
		}
	// run through the existing connections looking for data to read
		for(i = 0; i <= fdmax; i++) 
		{
			if (FD_ISSET(i, &read_fds)) 
			{
				if(i == 0)
				{
					int blen,clen;
					fgets(buf,1024,stdin);
					blen = strlen(buf);
					buf[blen-1] = '\0';
					
					char command[256];
					
					int k;
					for(k=0; (buf[k] != ' ')&&(buf[k] != '\0'); k++)
					{
						command[k] = buf[k];
					}
					command[k] = '\0';
					
					clen = strlen(command);
					
					if(strcmp(command,"AUTHOR")==0)
					{
						char ubit_name[] = "vinayvar";		
						//cout<<"I vinayvar have read and understood the course  academic integrity policy.\n";
						 cse4589_print_and_log("[%s:SUCCESS]\n","AUTHOR");
                                                cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubit_name);

                                                cse4589_print_and_log("[%s:END]\n","AUTHOR");
					}
					
					if(strcmp(command,"IP")==0)
					{ 
						// we can get host's external ip address by connecting to any outside server
						// and analising the socket info connected to that server
						// here we are connecting to google DNS
						// reference http://jhshi.me/2013/11/02/how-to-get-hosts-ip-address/index.html 
						if ((rv = getaddrinfo("8.8.8.8", "53", &hints, &ai)) != 0)
						{
							fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
							return 1;
						}
						// loop through all the results and connect to the first we can
						for(p = ai; p != NULL; p = p->ai_next)
						{
							if ((tempfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
							{
								perror("get_ip: socket");
								continue;
							}
							if (connect(tempfd, p->ai_addr, p->ai_addrlen) == -1)
							{
								close(tempfd);
								perror("get_ip: connect");
								continue;
							}
						}
						freeaddrinfo(ai); // all done with this
						
						struct sockaddr_in local_addr;
						socklen_t addr_len = sizeof(local_addr);

						if(getsockname(tempfd, (struct sockaddr*)&local_addr, &addr_len) == -1)
						{
							perror("get_ip: getsockname");
							close(tempfd);
							return 1;
						}

						// get ip addr 
						char myip[INET_ADDRSTRLEN];
						if (inet_ntop(local_addr.sin_family, &(local_addr.sin_addr), myip, sizeof(myip)) == NULL)
						{
							perror("get_ip: inet_ntop");
							return 1;
						}
						
						
						//printf("IP:%s\n", myip);
						cse4589_print_and_log("[%s:SUCCESS]\n",command);
                                                cse4589_print_and_log("IP:%s\n",myip);
                                                cse4589_print_and_log("[%s:END]\n",command);
					
					/*
					if ((send(sockfd,buf, strlen(buf),0))== -1) 
					{
						fprintf(stderr, "Failure Sending Message\n");
						close(sockfd);
						exit(1);
					}*/
					}
					if(strcmp(command,"PORT")==0)
					{
						int port = atoi(argv[2]);
						//printf("PORT:%d\n", port);
						cse4589_print_and_log("[%s:SUCCESS]\n",command);
                                                cse4589_print_and_log("PORT:%d\n",port);
                                                cse4589_print_and_log("[%s:END]\n",command);
					}                          

                                
			
					
					continue;
				}
				if (i == listener) 
				{
					// handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
					if (newfd == -1) 
					{
						perror("accept");
					} 
					else
					{
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdmax) 
						{
							// keep track of the max
							fdmax = newfd;
						}
						//store in the list
						ipstruct newipst;
						newipst.fd = newfd;
						const char* ch = inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN);
						strcpy(newipst.ip,ch);
						iplist.push_back(newipst);
						printf("selectserver: new connection from: %s on socket: %d\n", ch,newfd);
					}
				}
				else
				{
					//clear bufer
					memset(buf,0,strlen(buf));
					
					// handle data from a client
					if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
					{
						// got error or connection closed by client
						if (nbytes == 0)
						{
							// connection closed
							printf("selectserver: socket %d hung up\n", i);
						}
						else
						{
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
					}
					else
					{
						buf[nbytes] = '\0';
						// print message on Server terminal
						for(int i=0;buf[i]!='\0';i++)
									cout<<buf[i];
						cout<<"\n";
						
						// we got some data from a client check for the command
						char command[256];
						int k;
						for(k=0; (buf[k] != ' ')&&(buf[k] != '\0'); k++)
						{
							command[k] = buf[k];
						}
					    command[k] = '\0';
						if(strcmp(command,"BROADCAST")==0)
						{
							for(j = 1; j <= fdmax; j++) 
							{
								// send to everyone!
								if (FD_ISSET(j, &master)) 
								{							
									// except the listener and ourselves
									if (j != listener && j != i)
									{
										string ip,msg,txt;
										it = iplist.begin();
										while(it!= iplist.end())
										{
										   if(it->fd == i)
										   {
											ip = it->ip;	
										        break;												
									           }
											it++;	
  										}
										msg = buf +10;
										txt = ip;
										txt += "\n[msg]:";
										txt += msg;
										char *cstr = new char[txt.length() + 1];
										strcpy(cstr,txt.c_str());				
										if (send(j, cstr,strlen(cstr)+1 , 0) == -1)    
										{
											perror("send");
										}
										delete [] cstr;									
										
									}
								}
							}
						}
						else if(strcmp(command,"SEND")==0)
						{
							int l,fd;
							string ip,msg,txt,ips;
							
							it = iplist.begin();
                                                      
                                                        for(l=5;buf[l]!=' ';l++)
                                                        {
                                                                 ip += buf[l];
                                                        }
                                                        for(l=l+1; buf[l]!= '\0'; l++)
                                                        {
                                                                msg += buf[l];
                                                        }                                                              
                                                        cout<<"\n"<<ip;
							cout<<"\n"<<msg;	
								
							
							it = iplist.begin();
                                                        while(it!= iplist.end())
                                                        {
        	                                                if(strcmp(it->ip,ip.c_str())==0)
                                                                {
                                                                       fd = it->fd;
                                                                       break;                                                  
                                                                }
                                                                 it++;
                                                        }
	
							it = iplist.begin();
                                                        while(it!= iplist.end())
                                                        {
                                                              if(it->fd == i)
                                                              {
                                                                   ips = it->ip;                                                                                                                                   break;                                                  
                                                              }
                                                              it++;
                                                        }

									
						
							txt = ips;
							txt += "\n[msg]:";
                                                        txt += msg;
                                                        char *cstr = new char[txt.length() + 1];
                                                        strcpy(cstr,txt.c_str());
							if(FD_ISSET(fd, &master))
							{
                                                        if (send(fd, cstr,strlen(cstr)+1 , 0) == -1)
                                                        {
                                                    	    perror("send");
                                                        }
							}	
                                                        delete [] cstr;
						}
					}					
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors
	} // END for(;;)--and you thought it would never end!
	
	return 0;  
	
}

int StartClient(char** argv)
{
	fd_set master;			// master file descriptor list
	fd_set read_fds;		// temp file descriptor list for select()
	int fdmax;				// maximum file descriptor number	
	
	int sockfd,tempfd;			    // socket descriptor for clients and terminal commands
	int newfd;				  // newly accept()ed socket descriptor
	
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	
	char buf[1024];			// buffer for client data
	int nbytes;
	
	char remoteIP[INET6_ADDRSTRLEN];
	int yes=1;				// for setsockopt() SO_REUSEADDR, below
	int i, j, rv;
	bool login = false;
	
	struct addrinfo hints, *ai, *p;
	FD_ZERO(&master);		// clear the master and temp sets
	FD_ZERO(&read_fds);
	
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	// add the Client Socket to the master set
	FD_SET(0, &master);
	//FD_SET(sockfd, &master);
	
	fdmax = 0;	
	
	
	// main loop
	for(;;) 
	{	
		
		
		read_fds = master; // copy it
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(4);
		}
	// run through the existing connections looking for data to read
		for(i = 0; i <= fdmax; i++) 
		{	
			
			if (FD_ISSET(i, &read_fds)) 
			{
				if(i==0)	
				{
					int blen,clen;
					fgets(buf,1024,stdin);
					blen = strlen(buf);
					buf[blen-1] = '\0';
					
					char command[256];
					
					int k;
					for(k=0; (buf[k] != ' ')&&(buf[k] != '\0'); k++)
					{
						command[k] = buf[k];
					}
					command[k] = '\0';
					
					clen = strlen(command);
					if(strcmp(command,"AUTHOR")==0)
					{
						char ubit_name[] = "vinayvar";
						//cout<<"I vinayvar have read and understood the course  academic integrity policy.\n";
						cse4589_print_and_log("[%s:SUCCESS]\n","AUTHOR");
                                                cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", ubit_name);
                                                cse4589_print_and_log("[%s:END]\n","AUTHOR"); 
					}
					if(strcmp(command,"IP")==0)
					{	
						// we can do the same analysis as we did at the server side							
						if ((rv = getaddrinfo("8.8.8.8", "53", &hints, &ai)) != 0)
						{
							fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
							return 1;
						}
						// loop through all the results and connect to the first we can
						for(p = ai; p != NULL; p = p->ai_next)
						{
							if ((tempfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
							{
								perror("get_ip: socket");
								continue;
							}
							if (connect(tempfd, p->ai_addr, p->ai_addrlen) == -1)
							{
								close(tempfd);
								perror("get_ip: connect");
								continue;
							}
						}
						
						freeaddrinfo(ai); // all done with this

						struct sockaddr_in local_addr;
						socklen_t addr_len = sizeof(local_addr);

						if(getsockname(tempfd, (struct sockaddr*)&local_addr, &addr_len) == -1)
						{
							perror("get_ip: getsockname");
							close(tempfd);
							return 1;
						}
						// get ip addr 
						char myip[INET_ADDRSTRLEN];
						if (inet_ntop(local_addr.sin_family, &(local_addr.sin_addr), myip, sizeof(myip)) == NULL)
						{
							perror("get_ip: inet_ntop");
							return 1;
						}
				
						//printf("IP:%s\n", myip);					
						cse4589_print_and_log("[%s:SUCCESS]\n",command);
						cse4589_print_and_log("IP:%s\n",myip);
						cse4589_print_and_log("[%s:END]\n",command);									
					}
					
					if(strcmp(command,"PORT")==0)
					{
						int port = atoi(argv[2]);
						//printf("PORT:%d\n", port);
						cse4589_print_and_log("[%s:SUCCESS]\n",command);
                                                cse4589_print_and_log("PORT:%d\n",port);
                                                cse4589_print_and_log("[%s:END]\n",command);  
					}
					if(strcmp(command,"EXIT")==0)
					{
						 exit(0);
					}	
					
					if(strcmp(command,"LOGIN")==0)
					{						
						char ipaddr[40];
						char port[40];
						for(k=clen+1; (buf[k] != ' ')&&(buf[k] != '\0'); k++)
						{
							ipaddr[k-clen-1] = buf[k];
						}
						ipaddr[k-clen-1] = '\0';				
						
						
						int l;
						for(l=k+1; (buf[l] != ' ')&&(buf[l] != '\0'); l++)
						{
							port[l-k-1] = buf[l];
						}
						port[l-k-1] = '\0';						
						
						
						if ((rv = getaddrinfo(ipaddr, port, &hints, &ai)) != 0) 
						{
							fprintf(stderr, "Client: getaddrinfo: %s\n", gai_strerror(rv));
							return 1;
						}
						
						// loop through all the results and connect to the first we can
						for(p = ai; p != NULL; p = p->ai_next)
						{
							if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
							{
								perror("client: socket");
								continue;
							}
							if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
							{
								close(sockfd);
								perror("client: connect");
								continue;
							}	
							break;
						}
						
						freeaddrinfo(ai); // all done with this
						
						FD_SET(sockfd, &master);						
						fdmax = sockfd;
						
						login = true;
						
					}
					
					else if(login == true)
					{
						if ((send(sockfd,buf, strlen(buf),0))== -1) 
						{
							fprintf(stderr, "Failure Sending Message\n");
							close(sockfd);
							exit(1);
						}
					}
					continue;
				}
					
				if(i == sockfd)
				{
				
					//clear bufer
					memset(buf,0,strlen(buf));
					
					// handle data from a client
					if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
					{
						// got error or connection closed by client
						if (nbytes == 0)
						{
							// connection closed
							printf("selectclient: socket %d hung up\n", i);
						}
						else
						{
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
					}
					else
					{
						buf[nbytes] = '\0';

						// print message on Client terminal
						 cse4589_print_and_log("[%s:SUCCESS]\n","RECEIVED");
						cse4589_print_and_log("msg from:%s\n",buf);
						  cse4589_print_and_log("[%s:END]\n","RECEIVED");
					}
				}	
			
			
		} 
        
		
	}
	}
	
	return 0;
}


int main(int argc, char **argv)
{
	/*Init. Logger*/
	 cse4589_init_log(argv[2]);
	/* Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));
	
	/*Start Here*/
	
	if(argc!=3 || (*argv[1]!= 's' && *argv[1]!= 'c'))
	{
		cout<<"Invalid Parameters";
		exit(0);
	}
	
	if(*argv[1] == 's')
	{
		 
		StartServer(argv);
	}
	else
	{
		StartClient(argv);
	}
	
		return 0;
}
	
	
