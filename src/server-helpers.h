void get(char *path, char buffer[1024], void (^handler)());
int listenServer(int port);
int acceptClient(int servSock);
FILE *matchFile(char *pattern, char buffer[1024]);
void writeFile(FILE *fp, int fd);
