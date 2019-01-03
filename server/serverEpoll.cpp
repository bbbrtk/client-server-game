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

#define NUMBER_OF_ROUNDS 2
#define GAME_REMOVING_DELAY 3000000 // 3 sec

class Client;
class Game;

int servFd;
int epollFd;

std::unordered_set<Client*> clients;
std::unordered_set<Game*> games;

typedef std::pair<int,int> intPair;

void ctrl_c(int);

void sendToAll(char * buffer);

void sendToAllBut(int fd, char * buffer);

void sendToAllInGame(int game, char * buffer);

void sendListOfGames(int clientFd);

void wantMasterTimer(Game game, Client& client);

int chooseGameNumber(Client& client, int _game, int chosenGame, char * str);

void startGameAndGetLetter(int _game, char * buffer);

void setClientsLetter(int _game, char letter);

uint16_t readPort(char * txt);

void setReuseAddr(int sock);

int wordCorrect(char * data, char letter);

int calculatePoints(int correct, char letter);

void setAndSendRank(int _game);


// *************** CLASSES ******************

class Game {
    int _gameNumber;
    char _letter;
    int _roundNumber;
    int _masterFd;

    public:
        Game(int number, int master) : _gameNumber(number) {
            _roundNumber = 0;
            _masterFd = master;
            _letter = '0';
        }

        virtual ~Game(){}

        int gameNumber() const {return _gameNumber;}
        char letter() const {return _letter;}
        int roundNumber() const {return _roundNumber;}
        int masterFd() const {return _masterFd;}

        void getNewLetter(){
            _roundNumber++;
            _letter = char ( rand()%(90-65+1)+65 ); // leter from ascii A to Z
        }

        void remove() {
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
    int _game;
    int _points;
    int _rank;
    int _correct;
    char _currentLetter;

    Client(int fd) : _fd(fd) {
        epoll_event ee {EPOLLIN|EPOLLRDHUP, {.ptr=this}};
        epoll_ctl(epollFd, EPOLL_CTL_ADD, _fd, &ee);
        _game = 0;
        _points = 0;
        _rank = 0;
        _correct = 0;
    }
    virtual ~Client(){
        epoll_ctl(epollFd, EPOLL_CTL_DEL, _fd, nullptr);
        shutdown(_fd, SHUT_RDWR);
        close(_fd);
    }

    int fd() const {return _fd;}
    int game() const {return _game;}
    int points() const {return _points;}

    virtual void handleEvent(uint32_t events) override {
        if(events & EPOLLIN) {
            read(events);
        }
        if(events & ~EPOLLIN){
            remove();
        }
    }

    void read(uint32_t events){
        char dataFromRead[60];
        char data[9];

        ssize_t count  = ::read(_fd, dataFromRead, sizeof(dataFromRead)-1);
        if(count <= 0) events |= EPOLLERR;

        // get only first 7 most important chars
        for (int i=0; i<8; i++) data[i] = dataFromRead[i];
        printf("client: %d \t READ: %s \n", _fd, data);

        // handle ReloadGameBtn
        if (data[0] == 'L'){
            usleep(GAME_REMOVING_DELAY);
            sendListOfGames(_fd);
        }

        // set game number
        else if (data[0] == 'G'){
            int chosenGame = (data[1] - '0')*10 + (data[2] - '0');
            char str[4];
            _game = chooseGameNumber(*this, _game, chosenGame, str);
            write(str);            
        }

        // late client
        else if (data[0] == 'H'){
            int receivedFd = (data[2] - '0')*10 + (data[3] - '0') - 10;
            printf("late client id: %d \n", receivedFd);

            for(Client * client : clients){
                if (receivedFd == client->fd() ){
                    client->_currentLetter = data[1];
                    int timerValue = (data[4] - '0')*10 + (data[5] - '0');

                    printf("TIMER VALUE : %d LETTER %c \n", timerValue, data[1]);
                    char buffer[6];
                    sprintf(buffer, "V%c%d", data[1], timerValue);
                    client->write(buffer);
                    break;
                }
            }
        }

        // start game
        else if (data[0] == 'S'){
            char buffer[2];           
            startGameAndGetLetter(_game, buffer);
            setClientsLetter(_game, buffer[1]);
            sendToAllInGame(_game, buffer); 
        }

        // get answers, send 10-sec-timer msg
        else if (data[0] == 'A'){
            _correct = wordCorrect(data, _currentLetter);
            _points += calculatePoints(_correct, data[1]);

            if (data[1] == 'F'){
                char buffer[2];
                sprintf(buffer, "T");
                sendToAllInGame(_game, buffer);
            }
        }

        // send allowance to request score 
        else if (data[0] == 'P'){       
            char buffer[2];
            sprintf(buffer, "W");    
            sendToAllInGame(_game, buffer);
        }

        // send points
        else if (data[0] == 'W'){
            char buffer[6];
            int points = _points + 100;
            sprintf(buffer, "P%d%d", _correct, points);
            write(buffer);
        }

        // start new round or finish game
        else if (data[0] == 'B'){
            for(Game * game : games){
                if(_game == game->gameNumber()){
                    if (game->roundNumber() == NUMBER_OF_ROUNDS){
                        setAndSendRank(_game);
                        game->remove();
                    } 
                    else{
                        char buffer[2];           
                        startGameAndGetLetter(_game, buffer);
                        setClientsLetter(_game, buffer[1]);
                        sendToAllInGame(_game, buffer); 
                    }
                    break;
                }
            }
        }

        // reset game after removing master
        else if (data[0] == 'Y'){
            setAndSendRank(_game);
        }
    }

    // fix to default option
    void write(char * buffer){
        int check = ::write(_fd, buffer, strlen(buffer));
        if(check != (int) strlen(buffer)) perror("write failed");
        printf("client: %d \t WRITE: %s \n", _fd, buffer);
    }

    void remove() {
        printf("removing client %d\n", _fd);

        for(Game * game : games){
            if(game->masterFd() == _fd){
                char buffer[2];
                sprintf(buffer, "X");    
                sendToAllInGame(_game, buffer);
                game->remove();
                break;
            }
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

        // LOL
        ((Handler*)ee.data.ptr)->handleEvent(ee.events);

        printf("_____next_while_iteration_____: \n");
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
        ::write(client->fd(), &buffer, sizeof(buffer));       
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
        if(client->game()==game)
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

    int check = write(clientFd, tab, strlen(tab));
    if(check != (int) strlen(tab)) perror("write failed");
}


// *************** GAME ******************
void wantMasterTimer(Game game, Client& client){
    char buffer[6];
    sprintf(buffer, "H%c%d", game.letter(), client.fd()+10); 
    int check = ::write(game.masterFd(), buffer, sizeof(buffer));
    //if(check != (int) strlen(buffer)) perror("write failed");
}


int chooseGameNumber(Client& client, int _game, int chosenGame, char * str){
    int maxGameNumber = 10;
    // check if game exist
    for(Game * game : games){
        // game exist
        if(chosenGame == game->gameNumber()){
            _game = chosenGame;
            sprintf(str, "G%dU", _game );
            if (game->letter() != '0') wantMasterTimer(*game, client); // if game is pending
            break;
        }
        if (maxGameNumber < game->gameNumber()) maxGameNumber = game->gameNumber();
    }

    // if new game
    if (chosenGame==0 && _game==0){
        games.insert( new Game(maxGameNumber+1, client.fd()) );
        _game = maxGameNumber+1;
        sprintf(str, "G%dM", _game );
    }

    // if number game is incorrect
    else if (chosenGame!=0 && _game==0) sendListOfGames(client.fd()); 

    printf("client: %d \t myGame: %d \n", client.fd(), _game);
    return _game;
}


void startGameAndGetLetter(int _game, char * buffer){
    for(Game * game : games){
        if (_game == game->gameNumber() ){
            game->getNewLetter();
            char letter = game->letter();
            sprintf(buffer, "R%c", letter);
            break;
        }
    }
}


void setClientsLetter(int _game, char letter){
    for(Client * client : clients){
        if (_game == client->game() ){
            client->_currentLetter = letter;
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


void setAndSendRank(int _game){
    std::map <int, int> rankMap;
    std::vector<intPair> rankVector;
    int rank = 0;

    for(Client * client : clients){
        if ( _game == client->game() ){
            rank++;
            rankMap[client->fd()] = client->points();           
            client->_game = 0;
            client->_points = 0;
            client->_rank = 0;
            client->_correct = 0;
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



    // iterate through map and send score
    // for (std::map<int, int>::iterator i = rankMap.begin(); i != rankMap.end(); i++){
    //     printf("key: %d, value %d \n", i->first, i->second);
    //     char str[8];
    //     int points = i->first+100;
    //     sprintf(str, "K%dR%dX", rank, points);
    //     rank--;
    //     if (write(i->second, str, strlen(str)) != (int) strlen(str)) perror("write failed");
    // }