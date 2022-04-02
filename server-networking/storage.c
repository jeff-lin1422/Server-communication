#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>
#include "LList.c"
#include "strbuf.c"
#include <stdbool.h>
#define BACKLOG 5

int running = 1;
pthread_mutex_t lock;
// the argument we will pass to the connection-handler threads
struct connection {
    struct sockaddr_storage addr;
    socklen_t addr_len;
    int fd;
    LList* l;
};

int server(char *port);
void *echo(void *arg);

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s [port]\n", argv[0]);
		exit(EXIT_FAILURE);
	}

    (void) server(argv[1]);
    return EXIT_SUCCESS;
}

void handler(int signal)
{
	running = 0;
}


int server(char *port)
{
    struct addrinfo hint, *info_list, *info;
    struct connection *con;
    int error, sfd;
    pthread_t tid;
    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;
    	// setting AI_PASSIVE means that we want to create a listening socket

    // get socket and address info for listening port
    // - for a listening socket, give NULL as the host name (because the socket is on
    //   the local host)
    error = getaddrinfo(NULL, port, &hint, &info_list);
    if (error != 0) {

      //  fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // attempt to create socket
    for (info = info_list; info != NULL; info = info->ai_next) {
        sfd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

        // if we couldn't create the socket, try the next method
        if (sfd == -1) {
            continue;
        }

        // if we were able to create the socket, try to set it up for
        // incoming connections;
        //
        // note that this requires two steps:
        // - bind associates the socket with the specified port on the local host
        // - listen sets up a queue for incoming connections and allows us to use accept
        if ((bind(sfd, info->ai_addr, info->ai_addrlen) == 0) &&
            (listen(sfd, BACKLOG) == 0)) {
            break;
        }

        // unable to set it up, so try the next method
        close(sfd);
    }

    if (info == NULL) {
        // we reached the end of result without successfuly binding a socket
        //fprintf(stderr, "Could not bind\n");
        freeaddrinfo(info_list);
        return -1;
    }

    freeaddrinfo(info_list);

	struct sigaction act;
	act.sa_handler = handler;
	act.sa_flags = 0;
	sigemptyset(&act.sa_mask);
	sigaction(SIGINT, &act, NULL);

	sigset_t mask;

	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
  LList* l = malloc(sizeof(LList));
  if(l == NULL){
  //  write(con->fd, "ERR\nSRV\n", 8);
  //  close(con->fd);
  //  free(con);
    return 0;
  }
  init_LL(l);
  pthread_mutex_init(&lock, NULL);
    // at this point sfd is bound and listening
  //  printf("Waiting for connection\n");
	while (running) {
    	// create argument struct for child thread
		con = malloc(sizeof(struct connection));
    if(con == NULL){
        write(con->fd, "ERR\nSRV\n", 8);
        close(con->fd);
        free(con);
        return 0;
    }
        con->l = l;
        con->addr_len = sizeof(struct sockaddr_storage);
        	// addr_len is a read/write parameter to accept
        	// we set the initial value, saying how much space is available
        	// after the call to accept, this field will contain the actual address length

        // wait for an incoming connection
        con->fd = accept(sfd, (struct sockaddr *) &con->addr, &con->addr_len);
        	// we provide
        	// sfd - the listening socket
        	// &con->addr - a location to write the address of the remote host
        	// &con->addr_len - a location to write the length of the address
        	//
        	// accept will block until a remote host tries to connect
        	// it returns a new socket that can be used to communicate with the remote
        	// host, and writes the address of the remote hist into the provided location

        // if we got back -1, it means something went wrong
        if (con->fd == -1) {
          //  perror("accept");
            write(con->fd, "ERR\nSRV\n", 8);
            continue;
        }

        // temporarily block SIGINT (child will inherit mask)
        error = pthread_sigmask(SIG_BLOCK, &mask, NULL);
        if (error != 0) {
        	//fprintf(stderr, "sigmask: %s\n", strerror(error));
          write(con->fd, "ERR\nSRV\n", 8);
        	abort();
        }

		// spin off a worker thread to handle the remote connection
        error = pthread_create(&tid, NULL, echo, con);

		// if we couldn't spin off the thread, clean up and wait for another connection
        if (error != 0) {
            //fprintf(stderr, "Unable to create thread: %d\n", error);
            write(con->fd, "ERR\nSRV\n", 8);
            close(con->fd);
            free(con);
            continue;
        }

		// otherwise, detach the thread and wait for the next connection request
        pthread_detach(tid);

        // unblock SIGINT
        error = pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
        if (error != 0) {
        	//fprintf(stderr, "sigmask: %s\n", strerror(error));
          write(con->fd, "ERR\nSRV\n", 8);
        	abort();
        }

    }
  destroyLList(l);
  free(con);
  pthread_mutex_destroy(&lock);
	//puts("No longer listening.");
	pthread_detach(pthread_self());
	pthread_exit(NULL);

    // never reach here
    return 0;
}

#define BUFSIZE 1000

char* setMemories(char* origin, char* dest, int length){
  for(int i = 0; i < length; i++){
    dest[i] = origin[i];
  }
  dest[length-1] = '\0';
  return dest;
}

void *echo(void *arg)
{
    char host[100], port[10];
    struct connection *c = (struct connection *) arg;
    int error, nread;

	// find out the name and port of the remote host
    error = getnameinfo((struct sockaddr *) &c->addr, c->addr_len, host, 100, port, 10, NI_NUMERICSERV);
    	// we provide:
    	// the address and its length
    	// a buffer to write the host name, and its length
    	// a buffer to write the port (as a string), and its length
    	// flags, in this case saying that we want the port as a number, not a service name
    if (error != 0) {
        //fprintf(stderr, "getnameinfo: %s", gai_strerror(error));
        write(c->fd, "ERR\nSRV\n", 8);
        close(c->fd);
        return NULL;
    }
    //printf("[%s:%s] connection\n", host, port);
    strbuf_t *sb = calloc(sizeof(strbuf_t),1);
    if(sb == NULL){
      write(c->fd, "ERR\nSRV\n", 8);
      sb_destroy(sb);
      free(sb);
      close(c->fd);
      free(c);
      return NULL;
    }

    LList* l = c->l;
    //doesnt really matter what BUFSIZE is
    sb_init(sb, BUFSIZE);
    char bufchar = '?';
    int counter = 0;

    char* action = calloc(sizeof(char),4);
    if(action == NULL){
      write(c->fd, "ERR\nSRV\n", 8);
      sb_destroy(sb);
      free(sb);
      close(c->fd);
      free(c);
      free(action);
      return NULL;
    }
    char* length = calloc(sizeof(char),1);
    if(length == NULL){
      write(c->fd, "ERR\nSRV\n", 8);
      sb_destroy(sb);
      free(sb);
      close(c->fd);
      free(c);
      free(length);
      free(action);
      return NULL;
    }
    char* key;
    char* value;
    int lengthInt;
    // read user input letter by letter, each letter stored in bufchar
    while ((nread = read(c->fd, &bufchar, 1)) > 0) {
      if(bufchar == '\n'){
        //change it to a word once it hit a \n
        char* str = malloc(sizeof(char)* (sb->used+1));
        if(str == NULL){
          free(str);
          write(c->fd, "ERR\nSRV\n", 8);
          sb_destroy(sb);
          free(sb);
          close(c->fd);
          free(c);
          free(length);
          free(action);
          return NULL;
        }
        sb_toString(sb, str);
        str[sb->used] = '\0';
    //    printf("-------------------------------------\n");
        counter++;
    //    printf("this is counter: %d\n", counter);
        if(counter == 1){
          // set the action
          free(action);
          action = calloc(sizeof(char), 4);
          if(action == NULL){
            free(str);
            write(c->fd, "ERR\nSRV\n", 8);
            sb_destroy(sb);
            free(sb);
            close(c->fd);
            free(c);
            free(length);
            free(action);
            return NULL;
          }
          if(strlen(str) != 3){
            free(str);
            write(c->fd, "ERR\nBAD\n", 8);
            destroyLList(l);
            sb_destroy(sb);
            free(sb);
            close(c->fd);
            free(c);
            free(length);
            free(action);
            exit(0);
          }
          // just repopulates action with str
          action = setMemories(str, action, 4);
          if(strcmp(action, "GET") != 0 && strcmp(action, "DEL") != 0 && strcmp(action, "SET") != 0){
            free(str);
            write(c->fd, "ERR\nBAD\n", 8);
            sb_destroy(sb);
            free(sb);
            close(c->fd);
            free(c);
            free(length);
            free(action);
            return NULL;
          }
          //printf("set the action %s\n", action);

        }
        else if(counter == 2){
          // set the length
          free(length);
          length = calloc(sizeof(char), sb->used+1);
          if(length == NULL){
            free(str);
            write(c->fd, "ERR\nSRV\n", 8);
            sb_destroy(sb);
            free(sb);
            close(c->fd);
            free(c);
            free(length);
            free(action);
            return NULL;
          }
          length = setMemories(str, length, sb->used+1);
          lengthInt = atoi(length);
        //  printf("set the length %d\n", lengthInt);
          if(lengthInt == 0){
            free(str);
            write(c->fd, "ERR\nBAD\n", 8);
            sb_destroy(sb);
            free(sb);
            close(c->fd);
            free(c);
            free(length);
            free(action);
            //free(key);
            //free(value);
            //abort();
            return NULL;
          }
        }
        else if(counter == 3){
          // set the key
          // check the action that was stored, if it is SET, then keeeeep going
          // else set to 0
          if(strlen(str)+1 != lengthInt){
            //we need to free str, but if there is something already set it gives you a double free error if you have a ERR LEN
            free(str);
            write(c->fd, "ERR\nLEN\n", 8);
            sb_destroy(sb);
            free(sb);
            close(c->fd);
            free(c);
            free(length);
            free(action);
            return NULL;
          }
          key = calloc(sizeof(char), lengthInt);
          if(key == NULL){
            free(str);
            write(c->fd, "ERR\nSRV\n", 8);
            sb_destroy(sb);
            free(sb);
            close(c->fd);
            free(c);
            free(length);
            free(action);
            free(value);
            free(key);
            return NULL;
          }
          key = setMemories(str, key, lengthInt);
        //  printf("set the key %s\n", key);
          if(strcmp(action, "SET") != 0){
            counter = 0;
            // if the key is in the list, check the action
            // maybe pass in the file descriptor and have the library function to write it to the fd?
            if(inLLKey(l, key)){
              if(strcmp(action, "GET") == 0){
                getLLKey(l, key, c->fd);
              }
              else if(strcmp(action, "DEL") == 0){
                pthread_mutex_lock(&lock);
                l = deleteLLKey(l, key, c->fd);
                pthread_mutex_unlock(&lock);
              }
            }
            // key not in list
            else{
              write(c->fd, "KNF\n", 4);
            }
            // since its not in the list, we need to free it manually
            free(key);
          }
        }
        // set the values
        else if(counter == 4){
          // add linked list here too if its set
          value = calloc(sizeof(char), sb->used+1);
          if(value == NULL){
            free(str);
            write(c->fd, "ERR\nSRV\n", 8);
            sb_destroy(sb);
            free(sb);
            close(c->fd);
            free(c);
            free(length);
            free(action);
            free(value);
            free(key);
            return NULL;
          }
          value = setMemories(str, value, sb->used+1);
      //    printf("set the value %s\n", value);
          pthread_mutex_lock(&lock);
          l = insertLL(l, key, value, lengthInt);
          pthread_mutex_unlock(&lock);
          write(c->fd, "OKS\n", 4);
          counter = 0;
          //free(value);
        }
        free(str);
        sb_destroy(sb);
        sb_init(sb, BUFSIZE);
      }
      else{
        sb_append(sb, bufchar);
      }
        //uncomment to check what its reading:
        //printf("[%s:%s] read  |%c|\n", host, port, bufchar);
    }
  //  printLL(l);
    //printf("[%s:%s] got EOF\n", host, port);

    sb_destroy(sb);
    free(sb);
    close(c->fd);
    free(c);
    free(length);
    free(action);
    return NULL;
}
