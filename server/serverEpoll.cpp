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
#include <map>
#include <vector>
#include <algorithm>

#define NUMBER_OF_ROUNDS 1
#define GAME_REMOVING_DELAY 3000000 // 3 sec

typedef std::pair<int,int> intPair;

class Client;
class Game;

int servFd;
int epollFd;

std::unordered_set<Client*> clients;
std::unordered_set<Game*> games;


void ctrl_c(int);

uint16_t readPort(char * txt);

void setReuseAddr(int sock);


void sendToAll(char * buffer);

void sendToAllBut(int fd, char * buffer);

void sendToAllInGame(int game, char * buffer);

void sendListOfGames(int clientFd);


void wantMasterTimer(Client& client);

void chooseGameNumber(Client& client, int chosenGame);

void startGameAndGetLetter(Client& client, char * buffer);

void setClientsLetter(int game, char letter);

int wordCorrect(char * data, char letter);

int calculatePoints(int correct, char letter);

void setAndSendRank(int game);


void handleReloadGame(int fd);

void handleSetGameNumber(char * data, Client& client);

void handleLateClient(char * data);

void handleStartGame(Client& client);

void handleGetAnswersSendTimer(char * data, Client& client);

void handleAllowScoreRequest(int game);

void handleSendPoints(Client& client);

void handleStartNewRoundOrFinishGame(Client& client);


// *************** CLASSES ******************

class Game {
    int _gameNumber;
    int _masterFd;

    public:
        int _roundNumber;
        char _letter;
        bool _firstAnswerSent;

        Game(){_gameNumber = 0;}

        Game(int number, int master) : _gameNumber(number) {
            _roundNumber = 0;
            _masterFd = master;
            _letter = '0';
            _firstAnswerSent = false;
        }

        virtual ~Game(){}

        int gameNumber() const {return _gameNumber;}
        char letter() const {return _letter;}
        int roundNumber() const {return _roundNumber;}
        int masterFd() const {return _masterFd;}
        bool firstAnswerSent() const {return _firstAnswerSent;}

        void getNewLetter(){
            _roundNumber++;
            _letter = char ( rand()%(90-65+1)+65 ); // leter from ascii A to Z
        }

        void remove(){
            printf("removing game %d\n", _gameNumber);
            usleep(GAME_REMOVING_DELAY);
            games.erase(this);   
            delete this;
            printf("game REMOVED \n");
        }
};


struct Handler {
    virtual ~Handler(){}
    virtual void handleEvent(uint32_t events) = 0;
};


class Client : public Handler{
    int _fd;
    
public:
    Game myGame;
    int _points;
    int _rank;
    int _correct;
    char _order;

    Client(int fd) : _fd(fd) {
        epoll_event ee {EPOLLIN|EPOLLRDHUP, {.ptr=this}};
        epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);
        myGame = Game();
        _points = 0;
        _rank = 0;
        _correct = 0;
        _order = 'F';
    }

    virtual ~Client(){
        epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
        shutdown(_fd, SHUT_RDWR);
        close(_fd);
    }

    int fd() const {return _fd;}
    int points() const {return _points;}
    int rank() const {return _rank;}
    int correct() const {return _correct;}
    char order() const {return _order;}

    virtual void handleEvent(uint32_t events) override {
        if(events & EPOLLIN) {
            read(events);
        }
        if(events & ~EPOLLIN){
            remove();
        }
    }

    void read(uint32_t events){
        char dataFromRead[60], data[9];

        ssize_t count  = ::read(_fd, dataFromRead, sizeof(dataFromRead)-1);
        if(count <= 0) events |= EPOLLERR;

        for (int i=0; i<8; i++) data[i] = dataFromRead[i];
        printf("client: %d \t READ: %s \n", _fd, data);

        if (data[0] == 'L') handleReloadGame(_fd);

        else if (data[0] == 'G') handleSetGameNumber(data, *this);

        else if (data[0] == 'H') handleLateClient(data);

        else if (data[0] == 'S') handleStartGame(*this);

        else if (data[0] == 'A') handleGetAnswersSendTimer(data, *this);

        else if (data[0] == 'P') handleAllowScoreRequest(myGame.gameNumber());

        else if (data[0] == 'W') handleSendPoints(*this);

        else if (data[0] == 'B') handleStartNewRoundOrFinishGame(*this);

        else if (data[0] == 'Y') setAndSendRank(myGame.gameNumber()); // reset game after removing master

    }

    void write(char * buffer){
        if ( ::write(_fd, buffer, strlen(buffer)) != (int) strlen(buffer) ) perror("write failed");
        printf("client: %d \t WRITE: %s \n", _fd, buffer);
    }

    void remove() {
        printf("removing client %d\n", _fd);

        if (myGame.masterFd() == _fd){
            char buffer[2];
            sprintf(buffer, "X");    
            sendToAllInGame(myGame.gameNumber(), buffer);
            myGame.remove();
            myGame = Game();
        }

        clients.erase(this);
        delete this;

        printf("client REMOVED \n");
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


// *************** MAIN  ******************

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

        ((Handler*)ee.data.ptr)->handleEvent(ee.events);

        printf("\t ----------------- \n");
    }
}


// *************** SOCKETS ******************

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


// *************** SEND TO CLIENTS ******************

void sendToAll(char * buffer){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        //::write(client->fd(), &buffer, sizeof(buffer));       
        client->write(buffer);
    }
}


void sendToAllBut(int fd, char * buffer){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->fd()!=fd)
            client->write(buffer);
    }
}


void sendToAllInGame(int game, char * buffer){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
        if(client->myGame.gameNumber()==game)
            client->write(buffer);
    }
}


void sendListOfGames(int clientFd){
    std::string buffer = "C" ;
    for(Game * game : games){
        char str[3];
        sprintf(str, "%d,", game->gameNumber() );
        buffer += str;
    }
    char tab[100];
    strcpy(tab, buffer.c_str());
    if ( ::write(clientFd, tab, strlen(tab)) != (int) strlen(tab) ) perror("write failed");
}


// *************** GAME ******************

void wantMasterTimer(Client& client){
    char buffer[6];
    sprintf(buffer, "H%c%d", client.myGame.letter(), client.fd()+10); 
    int check = ::write(client.myGame.masterFd(), buffer, sizeof(buffer));
    //if(check != (int) strlen(buffer)) perror("write failed");
}


void chooseGameNumber(Client& client, int chosenGame){
    char str[4];
    int maxGameNumber = 10;

    // check if game exist
    for(Game * game : games){
        // game exist
        if(chosenGame == game->gameNumber()){
            client.myGame = *game;
            sprintf(str, "G%dU", client.myGame.gameNumber() );
            if (game->letter() != '0') wantMasterTimer(client); // if game is pending
            break;
        }
        if (maxGameNumber < game->gameNumber()) maxGameNumber = game->gameNumber();
    }

    // if new game
    if (chosenGame==0 && client.myGame.gameNumber()==0){
        // Game * newgame = new Game(maxGameNumber+1, client.fd()) ;
        // games.insert(newgame);
        // client.myGame = *newgame;
        games.insert(new Game(maxGameNumber+1, client.fd()));
        for(Game * game : games){
            if(maxGameNumber+1 == game->gameNumber()){
                client.myGame = *game;
            }
        }

        sprintf(str, "G%dM", client.myGame.gameNumber() );       
    }

    // if game number is incorrect
    else if (chosenGame!=0 && client.myGame.gameNumber()==0){
        char buffer[2];
        sprintf(buffer, "E"); 
        sendListOfGames(client.fd());
    }  

    client.write(str);

    printf("client: %d \t myGame: %d \n", client.fd(), client.myGame.gameNumber());
}


void startGameAndGetLetter(Client& client, char * buffer){
    client.myGame.getNewLetter();
    sprintf(buffer, "R%c", client.myGame.letter());

    for(Client * anotherClient : clients){
        if (client.myGame.gameNumber() == anotherClient->myGame.gameNumber() ){
            anotherClient->myGame._letter = client.myGame.letter();
        }
    }
}


int wordCorrect(char * data, char letter){
    int correct = 0;
    for(int i=2; i<6; i++){
        if (data[i]==letter) correct++;
    }
    return correct;
}


int calculatePoints(int correct, char letter){
    int points = 0;
    if (letter=='F') points=correct*3;
    else if (letter=='S') points=correct*2;
    else if (letter=='T')points=correct;

    return points;
}


void setAndSendRank(int game){
    std::map <int, int> rankMap;
    std::vector<intPair> rankVector;
    int rank = 0;

    for(Client * client : clients){
        if ( game == client->myGame.gameNumber() ){
            rank++;
            rankMap[client->fd()] = client->points();        
            client->_points = 0;
            client->_rank = 0;
            client->_correct = 0;
            if (client->myGame.masterFd() != client->fd()) client->myGame = Game();   
        }
    }

    // copy from map
    std::copy(rankMap.begin(), rankMap.end(),
        std::back_inserter<std::vector<intPair>>(rankVector));

    // sort vector
    std::sort(rankVector.begin(), rankVector.end(),
        [](const intPair& l, const intPair& r) {
            if (l.second != r.second) return l.second < r.second;
            return l.first < r.first;
        }
    );

    for (auto const &intPair: rankVector) {
        //printf("points: %d, client %d \n", intPair.second, intPair.first);
        char str[8];
        int points = intPair.second+100;
        sprintf(str, "K%dR%dX", rank, points);
        rank--;
        if (write(intPair.first, str, strlen(str)) != (int) strlen(str)) perror("write failed");
    }

    rankVector.clear();
    rankMap.clear();
}

// *************** HANDLERS ******************

void handleReloadGame(int fd){
    sendListOfGames(fd);
}


void handleSetGameNumber(char * data, Client& client){
    int chosenGame = (data[1] - '0')*10 + (data[2] - '0');
    chooseGameNumber(client, chosenGame);
}


void handleLateClient(char * data){
    int receivedFd = (data[2] - '0')*10 + (data[3] - '0') - 10;

    for(Client * client : clients){
        if (receivedFd == client->fd() ){
            client->myGame._letter = data[1];
            int timerValue = (data[4] - '0')*10 + (data[5] - '0');
            
            char buffer[6];
            sprintf(buffer, "V%c%d", data[1], timerValue);
            client->write(buffer);

            break;
        }
    }
}


void handleStartGame(Client& client){
    char buffer[2]; 
    startGameAndGetLetter(client, buffer);          
    sendToAllInGame(client.myGame.gameNumber(), buffer); 
}


void handleGetAnswersSendTimer(char * data, Client& client){
    client._correct = wordCorrect(data, client.myGame.letter());
    client._points += calculatePoints(client.correct(), data[1]);

    if (data[1] == 'F' && !client.myGame.firstAnswerSent()){
        client.myGame._firstAnswerSent = true; 
        char buffer[2];
        sprintf(buffer, "T");
        sendToAllInGame(client.myGame.gameNumber(), buffer);
    }
}


void handleAllowScoreRequest(int game){
    char buffer[2];
    sprintf(buffer, "W");    
    sendToAllInGame(game, buffer);
}


void handleSendPoints(Client& client){
    char buffer[6];
    int points = client.points() + 100;
    sprintf(buffer, "P%d%d", client.correct(), points);
    client.write(buffer);
}


void handleStartNewRoundOrFinishGame(Client& client){

    if (client.myGame.roundNumber() == NUMBER_OF_ROUNDS){
        setAndSendRank(client.myGame.gameNumber());
        client.myGame.remove();
    } 
    else{
        char buffer[2];           
        startGameAndGetLetter(client, buffer);
        sendToAllInGame(client.myGame.gameNumber(), buffer); 
        client.myGame._firstAnswerSent = false; 
    }
}