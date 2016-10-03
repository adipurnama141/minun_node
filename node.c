#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/sysinfo.h>
#include <arpa/inet.h>
 
int main() {
    int socket_node, socket_master, sockaddr_size, read_size;
    struct sockaddr_in node, master;
    char master_message[32], hostname[32], listhostname[37];
    FILE* fd;

    //initialization
    bzero(hostname, 32);
    gethostname(hostname, 31);
    bzero(listhostname, 37);
    strcpy(listhostname, "list ");
    strcat(listhostname, hostname);
    bzero(master_message, 32);

    //create socket
    socket_node = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_node == -1) {
        printf("Could not create socket.\n");
        return 1;
    } else {
        printf("Socket created.\n");
    }

    //prepare the sockaddr_in structure
    node.sin_family = AF_INET;
    node.sin_addr.s_addr = INADDR_ANY;
    node.sin_port = htons(8888);

    //bind
    int retbind = bind(socket_node, (struct sockaddr*) &node, sizeof(node));
    if(retbind < 0) {
        printf("Bind failed.\n");
        return 1;
    } else {
        printf("Bind done.\n");
    }
	while(1){
	    //listen
	    listen(socket_node, 3);
	    printf("Waiting for incoming connections...\n");

	    //accept connection from an incoming master
	    sockaddr_size = sizeof(struct sockaddr_in);
	    socket_master = accept(socket_node, (struct sockaddr*) &master, (socklen_t*) &sockaddr_size);
	    if(socket_master < 0) {
	        printf("Accept failed.\n");
	        return 1;
	    } else {
	        printf("Connection accepted.\n");
	    }
	    fd = fdopen(socket_master, "r+");
	    fprintf(fd, "# munin at %s\n", hostname);
	    fflush(fd);

	    //receive a message from master
	    while((read_size = recv(socket_master, master_message, 32, 0)) > 0) {
	        //identify
	        if(compare(master_message, "cap")) {
	            fprintf(fd, "cap multigraph dirtyconfig\n");
	        } else if(compare(master_message, "nodes")) {
	            fprintf(fd, "%s\n.\n", hostname);
	        } else if(compare(master_message, listhostname)) {
	            fprintf(fd, "memory\n");
	        } else if(compare(master_message, "config memory")) {
	            fprintf(fd, "graph_args --base 1024 -l 0 --upper-limit 8271892480\n");
	            fprintf(fd, "graph_vlabel Bytes\n");
	            fprintf(fd, "graph_title Memory usage\n");
	            fprintf(fd, "graph_info This graph shows this machine memory.\n");
	            fprintf(fd, "graph_order used free\n");
	            fprintf(fd, "used.label used\n");
	            fprintf(fd, "used.draw STACK\n");
	            fprintf(fd, "used.info Used memory.\n");
	            fprintf(fd, "free.label free\n");
	            fprintf(fd, "free.draw STACK\n");
	            fprintf(fd, "free.info Free memory.\n.\n");
	        } else if(compare(master_message, "fetch memory")) {
	            struct sysinfo si;
	            sysinfo(&si);
	            fprintf(fd, "used.value %ld\n", si.bufferram);
	            fprintf(fd, "free.value %ld\n.\n", si.freeram);
	        } else if(compare(master_message, "version")) {
	            fprintf(fd, "lovely node on %s version: 8.48\n", hostname);
	        } else if(compare(master_message, "quit")) {
	            break;
	        } else {
	            fprintf(fd, "# Unknown command. Try cap, list, nodes, config, fecth, version or quit\n");
	        }
	        fflush(fd);
	        bzero(master_message, 32);
	    }
	    if(read_size == -1) {
	        printf("Recv failed.\n");
	        return 1;
	    }
	}

    //finish
    printf("Master disconnected.\n"); //read_size == 0
    return 0;
}

int compare(char* str, char* substr) {
    int i = 0;
    int n = strlen(substr);
    while(str[i] == substr[i]) {
        i++;
        if(i == n) { //sejauh ini sama
        	//karakter setelahnya ENTER (0xd telnet, 0xa nc) atau bzero
            if(str[i] == 0xd || str[i] == 0xa || str[i] == 0x0) {
                return 1;
            } else {
                return 0;
            }
        }
    }
    return 0; //beda
}