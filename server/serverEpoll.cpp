#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <sys/epoll.h>
#include <unordered_set>
#include <signal.h>


class Client;
class Game;

int servFd;
int epollFd;

std::unordered_set<Client*> clients;
std::unordered_set<Game*> games;

void ctrl_c(int);

void sendToAll(char * buffer);

void sendToAllBut(int fd, char * buffer);

void sendToAllInGame(int game, char * buffer);

void sendListOfGames(int clientFd);

int chooseGameNumber(int _fd, int _game, int chosenGame, char * str);

void startGameAndGetLetter(int _game, char * buffer);

uint16_t readPort(char * txt);

void setReuseAddr(int sock);

bool isWordCorrect(char data[20], char letter);


class Game {
    int _gameNumber;
    char _letter;
    int _roundNumber;
    int _masterFd;

    public:
        Game(int number, int master) : _gameNumber(number) {
            _roundNumber = 0;
            _masterFd = master;
        }

        virtual ~Game(){}

        int gameNumber() const {return _gameNumber;}
        char letter() const {return _letter;}
        int roundNumber() const {return _roundNumber;}
        int masterFd() const {return _masterFd;}

        void getNewLetter(){
            _roundNumber++;
            _letter = char ( rand()%(90-65+1)+65 );
        }
};


struct Handler {
    virtual ~Handler(){}
    virtual void handleEvent(uint32_t events) = 0;
};


class Client : public Handler{
    int _fd;
    
public:
    int _game;

    Client(int fd) : _fd(fd) {
        epoll_event ee {EPOLLIN|EPOLLRDHUP, {.ptr=this}};
        epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);
        _game = 0;
    }
    virtual ~Client(){
        epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
        shutdown(_fd, SHUT_RDWR);
        close(_fd);
    }

    int fd() const {return _fd;}
    int game() const {return _game;}

    virtual void handleEvent(uint32_t events) override {
        if(events & EPOLLIN) {
            read();
        }
        if(events & ~EPOLLIN){
            remove();
        }
    }

    // char * data = new char[10];
    // void read(char* data){
    //     int len = ::read(_fd, data, sizeof(data)-1);
    //     if (len < 0) perror("read failed");
    //     //printf("client: %d \t text: %s \n", _fd, data);
    // }

    void read(){
        char dataFromRead[100];
        char dataFirst4[3];
        int len = ::read(_fd, dataFromRead, sizeof(dataFromRead)-1);
        if (len < 0) perror("read failed");

        for (int i=0; i<4; i++) dataFirst4[i] = dataFromRead[i];
        printf("client: %d \t READ: %s \n", _fd, dataFirst4);

        // przypisanie numeru gry
        //  dodac sprawdzanie czy odebrano poprawny numer
        if (dataFirst4[0] == 'G'){
            int chosenGame = (dataFirst4[1] - '0')*10 + (dataFirst4[2] - '0');
            char str[4];
            _game = chooseGameNumber(_fd, _game, chosenGame, str);
            write(str);            
        }
        // wystartowanie gry - inkrementacja numeru rundy i wyslanie litery
        else if (dataFirst4[0] == 'S'){
            char buffer[2];
            startGameAndGetLetter(_game, buffer);
            sendToAllInGame(_game, buffer); 
        }

        //sendToAllInGame(_fd, _game, dataFirst4);
    }

    // naprawic tak jak bylo domyslnie
    void write(char * buffer){
        int check = ::write(_fd, buffer, strlen(buffer));
        if(check != (int) strlen(buffer)) perror("write failed");
        printf("client: %d \t WRITE: %s \n", _fd, buffer);
    }

    // dodac eleganckie usuwanie gry, ktorej gracz byl masterem
    void remove() {
        printf("removing %d\n", _fd);
        clients.erase(this);
        delete this;
    }
};


class : Handler {
    public:
    virtual void handleEvent(uint32_t events) override {
        if(events & EPOLLIN){
            sockaddr_in clientAddr{};
            socklen_t clientAddrSize = sizeof(clientAddr);
            
            auto clientFd = accept(servFd, (sockaddr*) &clientAddr, &clientAddrSize);
            if(clientFd == -1) error(1, errno, "accept failed");
            
            printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);
            
            clients.insert(new Client(clientFd));
            sendListOfGames(clientFd);
        }

        if(events & ~EPOLLIN){
            error(0, errno, "Event %x on server socket", events);
            ctrl_c(SIGINT);
        }
    }
} servHandler;


/* __MAIN___ */
int main(int argc, char ** argv){
    if(argc != 2) error(1, 0, "Need 1 arg (port)");
    auto port = readPort(argv[1]);
    
    servFd = socket(AF_INET, SOCK_STREAM, 0);
    if(servFd == -1) error(1, errno, "socket failed");
    
    signal(SIGINT, ctrl_c);
    signal(SIGPIPE, SIG_IGN);
    
    setReuseAddr(servFd);
    
    sockaddr_in serverAddr{.sin_family=AF_INET, .sin_port=htons((short)port), .sin_addr={INADDR_ANY}};
    int res = bind(servFd, (sockaddr*) &serverAddr, sizeof(serverAddr));
    if(res) error(1, errno, "bind failed");
    
    res = listen(servFd, 1);
    if(res) error(1, errno, "listen failed");

    epollFd = epoll_create1(0);
    
    epoll_event ee {EPOLLIN, {.ptr=&servHandler}};
    epoll_ctl(epollFd, EPOLL_CTL_ADD, servFd, &ee);

    srand(time(NULL));

    while(true){
        if(-1 == epoll_wait(epollFd, &ee, 1, -1)) {
            error(0,errno,"epoll_wait failed");
            ctrl_c(SIGINT);
        }

        // LOL
        ((Handler*)ee.data.ptr)->handleEvent(ee.events);

        printf("____next_commands: \n");
    }
}


uint16_t readPort(char * txt){
    char * ptr;
    auto port = strtol(txt, &ptr, 10);
    if(*ptr!=0 || port<1 || (port>((1<<16)-1))) error(1,0,"illegal argument %s", txt);
    return port;
}


void setReuseAddr(int sock){
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(res) error(1,errno, "setsockopt failed");
}


void ctrl_c(int){
    for(Client * client : clients)
        delete client;
    close(servFd);
    printf("\n Closing server \n");
    exit(0);
}


/*  wyslij 'buffer' do wszystkich graczy */
void sendToAll(char * buffer){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        ::write(client->fd(), &buffer, sizeof(buffer));       
        client->write(buffer);
    }
}


/*  wyslij 'buffer' do wszystkich graczy oprocz gracza o danym fd */
void sendToAllBut(int fd, char * buffer){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->fd()!=fd)
            client->write(buffer);
    }
}


/*  wyslij 'buffer' do wszystkich graczy w grze o danym numerze*/
void sendToAllInGame(int game, char * buffer){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->game()==game)
            client->write(buffer);
    }
}


/*  wyslij liste wszystkich aktualnie dostepnych gier */
void sendListOfGames(int clientFd){
    std::string buffer = "C" ;
    for(Game * game : games){
        char str[3];
        sprintf(str, "%d,", game->gameNumber() );
        buffer += str;
    }
    char tab[100];
    strcpy(tab, buffer.c_str());
    int check = write(clientFd, tab, strlen(tab));
    if(check != (int) strlen(tab)) perror("write failed");
}


/*  przypisz graczowi numer gry w zaleznosci od tego 
*   czy tworzy nowa (staje sie masterem) czy jest dodawany do istniejacej 
*   dodac sprawdzanie czy wyslano poprawnie
*/
int chooseGameNumber(int _fd, int _game, int chosenGame, char * str){
    int maxGameNumber = 10;
    
    for(Game * game : games){
        // jesli gra istnieje
        if(chosenGame == game->gameNumber()){
            _game = chosenGame;
            sprintf(str, "G%dU", _game );
            break;
        }
        if (maxGameNumber < game->gameNumber()) maxGameNumber = game->gameNumber();
    }

    // jesli ma byc utworzona nowa gra
    if (chosenGame==0 && _game==0){
        games.insert(new Game(maxGameNumber+1, _fd) );
        _game = maxGameNumber+1;
        sprintf(str, "G%dM", _game );
    }

    // jesli numer gry jest niepoprawny
    else if (chosenGame!=0 && _game==0){
        sendListOfGames(_fd); 
        }

    printf("client: %d \t myGame: %d \n", _fd, _game);
    return _game;
}


/* wystartuj gre i wylosuj litere dla danej rundy */
void startGameAndGetLetter(int _game, char * buffer){
    for(Game * game : games){
        if (_game == game->gameNumber() ){
            game->getNewLetter();
            char letter = game->letter();
            sprintf(buffer, "R%c", letter);
        }
    }
}


/* sprawdz czy slowo zaczyna sie na odpowiednia litere */
bool isWordCorrect(char data[20], char letter){
    if (data[0] == letter) return true; 
    else return false; 
}


/* UNUSED FUNCTIONS

* * * * * *



* * * * * *

for(Client * client : clients){
    client->read();
    //printf("fd %d \t", client->fd());
}

* * * * * *

char * info;   
char data[21]{};

while(true){
    int len = read(clientFd, data, sizeof(data)-1);
    bool isCorrect = isWordCorrect(data,'p');
    if (isCorrect){
        printf("recieved:\t %s",data);
        printf("is correct?:\t %d\n",isCorrect);
        info = (char*)'c'; // due to warning: ISO C++ forbids converting a string constant to ‘char*
        ///sendToAll(info,strlen(info));

        //int count = write(clientFd, info, strlen(info));
        //if(count != (int) strlen(info)) perror("write failed");
    }
}

* * * * * *
* IN HANDLE EVENT *
char buffer[256];
ssize_t count = ::read(_fd, buffer, 256);
if(count > 0)
    sendToAllBut(_fd, buffer, count);
else
    events |= EPOLLERR;

* * * * * *


* * * * * *


* * * * * *
*/

