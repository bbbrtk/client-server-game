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
#define GAME_REMOVING_DELAY 2000000 // 2 sec

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

void chooseGameNumber(Client& client, int chosenGame, char rounds);

void startGameAndGetLetter(Client& client, char * buffer);

void setClientsLetter(int game, char letter);

int wordCorrect(char * data, char letter);

int calculatePoints(int correct, char letter);

void setAndSendRank(int game);


void handleReloadGame(Client& client);

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
    int _roundNumber;
    int _lastRound;
    char _letter;
    bool _firstAnswerSent;

    public:
        Game(){
            _gameNumber = 0;
            _roundNumber = 0;
            _lastRound = 0;
            _masterFd = 0;
            _letter = '0';
            _firstAnswerSent = false;
        }

        Game(int number, int master, int rounds) : _gameNumber(number) {
            _roundNumber = 0;
            _lastRound = rounds;
            _masterFd = master;
            _letter = '0';
            _firstAnswerSent = false;
        }

        virtual ~Game(){}

        // getters
        int gameNumber() const {return _gameNumber;}
        char letter() const {return _letter;}
        int lastRound() const {return _lastRound;}
        int roundNumber() const {return _roundNumber;}
        int masterFd() const {return _masterFd;}
        bool firstAnswerSent() const {return _firstAnswerSent;}

        // setters
        void gameNumber(int a) {_gameNumber = a;}
        void letter(char a) {_letter = a;}
        void lastRound(int a) {_lastRound = a;}
        void roundNumber(int a) {_roundNumber = a;}
        void masterFd(int a) {_masterFd = a;}
        void firstAnswerSent(bool a) {_firstAnswerSent = a;}

        void getNewLetter(){
            _roundNumber++;
            _letter = char ( rand()%(90-65+1)+65 ); // leter from ascii A to Z
        }

        void remove(){
            if (_gameNumber != 0){
                printf("removing game %d\n", _gameNumber);
                games.erase(this);   
                delete this;
                printf("game REMOVED \n");
            }
        }
};


Game tempGame = Game();

struct Handler {
    virtual ~Handler(){}
    virtual void handleEvent(uint32_t events) = 0;
};

class Client : public Handler{
    int _fd;
    int _points;
    int _rank;
    int _correct;
    
public:
    Game * myGame;

    Client(int fd) : _fd(fd) {
        epoll_event ee {EPOLLIN|EPOLLRDHUP, {.ptr=this}};
        epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);
        myGame =  &tempGame;
        _points = 0;
        _rank = 0;
        _correct = 0;
    }

    virtual ~Client(){
        epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
        shutdown(_fd, SHUT_RDWR);
        close(_fd);
    }

    // getters
    int fd() const {return _fd;}
    int points() const {return _points;}
    int rank() const {return _rank;}
    int correct() const {return _correct;}

    // setters
    void points(int a) {_points = a;}
    void rank(int a) {_rank = a;}
    void correct(int a) {_correct = a;}

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

        if (data[0] == 'L') handleReloadGame(*this);

        else if (data[0] == 'G') handleSetGameNumber(data, *this);

        else if (data[0] == 'H') handleLateClient(data);

        else if (data[0] == 'S') handleStartGame(*this);

        else if (data[0] == 'A') handleGetAnswersSendTimer(data, *this);

        else if (data[0] == 'P') handleAllowScoreRequest(myGame->gameNumber());

        else if (data[0] == 'W') handleSendPoints(*this);

        else if (data[0] == 'B') handleStartNewRoundOrFinishGame(*this);

    }

    void write(char * buffer){
        if ( ::write(_fd, buffer, strlen(buffer)) != (int) strlen(buffer) ) perror("write failed");
        printf("client: %d \t WRITE: %s \n", _fd, buffer);
    }

    void remove() {
        printf("removing client %d\n", _fd);

        if (myGame->masterFd() == _fd){
            char buffer[6];
            sprintf(buffer, "X");    
            sendToAllInGame(myGame->gameNumber(), buffer);
            setAndSendRank(myGame->gameNumber());

            Game * deleteGame = myGame;
            myGame = &tempGame;
            deleteGame->remove();
        }

        myGame = &tempGame;
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
    for(Game * game : games)
        delete game;
    close(servFd);
    printf(" \n Closing server \n");
    exit(0);
}


// *************** SEND TO CLIENTS ******************

void sendToAll(char * buffer){
    auto it = clients.begin();
    while(it!=clients.end()){
        Client * client = *it;
        it++;
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
        if(client->myGame->gameNumber()==game)
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


// *************** GAME *****************

void wantMasterTimer(Client& client){
    char buffer[8];
    int sent;
    if (client.myGame->firstAnswerSent()) sent = 1;
    else sent = 0;

    sprintf(buffer, "H%c%d%d", client.myGame->letter(), client.fd()+10, sent); 
    int check = ::write(client.myGame->masterFd(), buffer, sizeof(buffer));
}


void chooseGameNumber(Client& client, int chosenGame, char rounds){
    char str[6];
    int maxGameNumber = 10;

    // check if game exist
    for(auto & game : games){
        // game exist
        if(chosenGame == game->gameNumber()){
            client.myGame = game;
            sprintf( str, "G%dU", client.myGame->gameNumber() );
            if (game->letter() != '0') wantMasterTimer(client); // if game is pending
            break;
        }
        if (maxGameNumber < game->gameNumber()) maxGameNumber = game->gameNumber();
    }

    // if new game
    if (chosenGame==0 ){
        games.insert(new Game(maxGameNumber+1, client.fd(), rounds-'0'));
        for(auto & game : games){
            if(maxGameNumber+1 == game->gameNumber()) client.myGame = game;
        }
        sprintf(str, "G%dM", client.myGame->gameNumber() );       
    }

    // if game number is incorrect
    else if (chosenGame!=0 && client.myGame==&tempGame){
        char buffer[6];
        sprintf(buffer, "E"); 
        sendListOfGames(client.fd());
    }  

    client.write(str);
    printf("client: %d \t myGame: %d \n", client.fd(), client.myGame->gameNumber());
}


void startGameAndGetLetter(Client& client, char * buffer){
    client.myGame->getNewLetter();
    sprintf(buffer, "R%c%d", client.myGame->letter(), client.myGame->roundNumber());
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
        if ( game == client->myGame->gameNumber() ){
            rank++;
            rankMap[client->fd()] = client->points();        
            client->points(0);
            client->rank(0);
            client->correct(0);
            if (client->myGame->masterFd() != client->fd()) { client->myGame = &tempGame;  printf("client %d RESETED \n", client->fd()); }
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
        ::write(intPair.first, str, sizeof(str));
        //if (::write(intPair.first, str, sizeof(str)) != (int) sizeof(str)) perror("write failed");
    }

    rankVector.clear();
    rankMap.clear();
}

// *************** HANDLERS ******************

void handleReloadGame(Client& client){
    client.myGame = &tempGame;
    sendListOfGames(client.fd());
}


void handleSetGameNumber(char * data, Client& client){
    int chosenGame = (data[1] - '0')*10 + (data[2] - '0');
    chooseGameNumber(client, chosenGame, data[3]);
}


void handleLateClient(char * data){
    int receivedFd = (data[2] - '0')*10 + (data[3] - '0') - 10;
    int sent;

    for(Client * client : clients){
        if (receivedFd == client->fd() ){
            client->myGame->letter(data[1]);
            int timerValue = (data[4] - '0')*10 + (data[5] - '0');  

            if (data[6] == '1') client->myGame->firstAnswerSent(true);
            else client->myGame->firstAnswerSent(false);      

            char buffer[6];
            sprintf(buffer, "V%c%d%d", data[1], timerValue, client->myGame->roundNumber());
            client->write(buffer);

            break;
        }
    }
}


void handleStartGame(Client& client){
    char buffer[6]; 
    startGameAndGetLetter(client, buffer);          
    sendToAllInGame(client.myGame->gameNumber(), buffer); 
}


void handleGetAnswersSendTimer(char * data, Client& client){
    client.correct( wordCorrect(data, client.myGame->letter()) );

    if (data[1] == 'S' && client.myGame->firstAnswerSent() == false){
        client.myGame->firstAnswerSent(true);       
        char buffer[6];
        sprintf(buffer, "T");
        sendToAllInGame(client.myGame->gameNumber(), buffer);
        data[1] = 'F';
    }
    
    client.points( client.points() + calculatePoints(client.correct(), data[1]) );
}


void handleAllowScoreRequest(int game){
    char buffer[6];
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
    if (client.myGame->roundNumber() == client.myGame->lastRound()){
        setAndSendRank(client.myGame->gameNumber());
        client.myGame->remove();
    } 
    else{
        char buffer[6];           
        startGameAndGetLetter(client, buffer);
        sendToAllInGame(client.myGame->gameNumber(), buffer); 
        client.myGame->firstAnswerSent(false); 
    }
}