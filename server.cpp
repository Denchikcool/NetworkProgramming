#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <stdlib.h>
#include <strings.h>
#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <time.h>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <optional>

using namespace std;

#define MAX_CLIENTS 3
#define BUFLEN 256

struct ClientInfo {
	int sockfd;
	optional<int> index;
	struct sockaddr_in addr;
	char message[BUFLEN];
	char name[BUFLEN];
	bool inLobby;
	bool isSent;
	time_t startTime;
	time_t endTime;
};

struct ClientInfo clients[MAX_CLIENTS];
int sockMain;
bool decisions[MAX_CLIENTS];

bool clientInArray(sockaddr_in clientAddr);

class Card {
	public: 
		enum rank { ACE = 1, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, TEN, JACK, QUEEN, KING };
		enum suit { CLUBS, DIAMONDS, HEARTS, SPADES };

		friend ostream& operator<<(ostream& os, const Card& aCard);
		Card(rank r = ACE, suit s = SPADES, bool ifu = true);

		int GetValue() const;

		void Flip();
		string to_string() const {
			if(m_IsFaceUp == false) return "XX";
			else return rank_to_string(m_Rank) + "(" + suit_to_string(m_Suit) + ")";
		}
		string rank_to_string(rank rank) const {
			switch (rank) {
				case ACE:
					return "A";
				case TWO:
					return "2";
				case THREE:
					return "3";
				case FOUR:
					return "4";
				case FIVE:
					return "5";
				case SIX:
					return "6";
				case SEVEN:
					return "7";
				case EIGHT:
					return "8";
				case NINE:
					return "9";
				case TEN:
					return "10";
				case JACK:
					return "J";
				case QUEEN:
					return "Q";
				case KING:
					return "K";
				default:
					return "X";
			}
		}

		string suit_to_string(suit suit) const {
			switch (suit) {
				case CLUBS:
					return "CLUBS";
				case DIAMONDS:
					return "DIAMONDS";
				case HEARTS:
					return "HEARTS";
				case SPADES:
					return "SPADES";
				default:
					return "X";
			}
		}

	private: 
		rank m_Rank;
		suit m_Suit;
		bool m_IsFaceUp;
};

Card::Card(rank r, suit s, bool ifu): m_Rank(r), m_Suit(s), m_IsFaceUp(ifu) {}

int Card::GetValue() const {
	int value = 0;
	if (m_IsFaceUp) {
		value = m_Rank;
		if (value > 10) {
			value = 10;
		}
	}
	return value;
}

void Card::Flip() {
	m_IsFaceUp = !(m_IsFaceUp);
}

class Hand {
	public: 
		Hand();
		virtual ~Hand();

		void Add(Card* pCard);
		void Clear();
		int GetTotal() const;

	protected: 
		vector<Card*> m_Cards;
};

Hand::Hand() {
	m_Cards.reserve(7);
}

Hand::~Hand() {
	Clear();
}

void Hand::Add(Card* pCard) {
	m_Cards.push_back(pCard);
}

void Hand::Clear() {
	vector <Card*>::iterator iter = m_Cards.begin();
	for (iter = m_Cards.begin(); iter != m_Cards.end(); ++iter) {
		delete *iter;
		*iter = 0;
	}
	m_Cards.clear();
}

int Hand::GetTotal() const {
	if (m_Cards.empty()) {
		return 0;
	}
	if (m_Cards[0]->GetValue() == 0) {
		return 0;
	}

	int total = 0;
	vector <Card*>::const_iterator iter;

	for (iter = m_Cards.begin(); iter != m_Cards.end(); ++iter) {
		total += (*iter)->GetValue();
	}

	bool containsAce = false;
	for (iter = m_Cards.begin(); iter != m_Cards.end(); ++iter) {
		if ((*iter)->GetValue() == Card::ACE) {
			containsAce = true;
		}
	}

	if (containsAce && total <= 11) {
		total += 10;
	}
	return total;
}

class GenericPlayer: public Hand {
	friend ostream& operator<<(ostream& os, const GenericPlayer& aGenericPlayer);
	public:
		GenericPlayer(const string name = "", const int socket = 0, const struct ClientInfo client = {});
		virtual ~GenericPlayer();

		virtual bool IsHitting() const = 0;
		bool IsBusted() const;
		void Burst() const;

	protected:
		string m_Name;
		int socketID;
		struct ClientInfo clientID;
};

GenericPlayer::GenericPlayer(const string name, const int socket, const struct ClientInfo client): m_Name(name), socketID(socket), clientID(client) {}

GenericPlayer::~GenericPlayer() {}

bool GenericPlayer::IsBusted() const {
	return (GetTotal() > 21);
}

void GenericPlayer::Burst() const {
	string information = m_Name + " перебрал карты!";
	int length = sizeof(clientID.addr);

	for (int i = 0; i < MAX_CLIENTS; i++) {
		sendto(sockMain, information.c_str(), information.size() + 1, 0, (struct sockaddr*)&clients[i].addr, length);
	}
}

class Player: public GenericPlayer {
	public: 
		Player(const string name = "", const int socket = 0, const struct ClientInfo client = {});
		virtual ~Player();

		virtual bool IsHitting() const;
		void Win() const;
		void Lose() const;
		void Push() const;
};

Player::Player(const string name, const int socket, const struct ClientInfo client): GenericPlayer(name, socket, client) {}

Player::~Player() {}

bool Player::IsHitting() const {
	string question = m_Name + " , ты хочешь сделать ход? (y/n): ";
	char answer[BUFLEN];
	int length = sizeof(clientID.addr);
	struct sockaddr_in temp;
	

	sendto(sockMain, question.c_str(), question.size() + 1, 0, (struct sockaddr * ) & clientID.addr, length);
	/*if (recvfrom(sockMain, answer, BUFLEN, 0, (struct sockaddr*)&clientID.addr, (socklen_t*)&length) < 0) {
		perror("Нельзя принять ответ от клиента.");
		exit(1);
	}*/
	while(1){
		if (recvfrom(sockMain, answer, BUFLEN, 0, (struct sockaddr*)&temp, (socklen_t*)&length) < 0) {
			perror("Нельзя принять ответ от клиента.");
			exit(1);
		}
		if(clientInArray(temp) == false){
			sendto(sockMain, "Лобби заполнено. Невозможно подключиться.", BUFLEN, 0, (struct sockaddr*)&temp, length);
		}
		else{
			break;
		}
	}
	return (answer[0] == 'y' || answer[0] == 'Y');
}

void Player::Win() const {
	string informationWins = m_Name + " победил!";
	int length = sizeof(clientID.addr);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		sendto(sockMain, informationWins.c_str(), informationWins.size() + 1, 0, (struct sockaddr*)&clients[i].addr, length);
	}
}

void Player::Lose() const {
	string informationLoses = m_Name + " проиграл!";
	int length = sizeof(clientID.addr);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		sendto(sockMain, informationLoses.c_str(), informationLoses.size() + 1, 0, (struct sockaddr*)&clients[i].addr, length);
	}
}

void Player::Push() const {
	string informationPushes = m_Name + " сыграл вничью!";
	int length = sizeof(clientID.addr);
	for (int i = 0; i < MAX_CLIENTS; i++) {
		sendto(sockMain, informationPushes.c_str(), informationPushes.size() + 1, 0, (struct sockaddr*)&clients[i].addr, length);
	}
}

class House: public GenericPlayer {
	public: 
		House(const string& name = "House");
		virtual ~House();

		virtual bool IsHitting() const;
		void FlipFirstCard();
};

House::House(const string& name): GenericPlayer(name) {}

House::~House() {}

bool House::IsHitting() const {
	return (GetTotal() <= 16);
}

void House::FlipFirstCard() {
	if (!(m_Cards.empty())) {
		m_Cards[0]->Flip();
	} else {
		cout << "Нет карты, чтоб перевернуть!" << endl;
	}
}

class Deck: public Hand {
	public: Deck();
		virtual ~Deck();

		void Populate();
		void Shuffle();
		void Deal(Hand& aHand);
		void AdditionalCards(GenericPlayer& aGenericPlayer);
};

Deck::Deck() {
	m_Cards.reserve(52);
}

Deck::~Deck() {}

void Deck::Populate() {
	Clear();

	for (int s = Card::CLUBS; s <= Card::SPADES; s++) {
		for (int r = Card::ACE; r <= Card::KING; r++) {
			Add(new Card(static_cast<Card::rank> (r), static_cast<Card::suit> (s)));
		}
	}
}

void Deck::Shuffle() {
	random_shuffle(m_Cards.begin(), m_Cards.end());
}

void Deck::Deal(Hand& aHand) {
	if (!m_Cards.empty()) {
		aHand.Add(m_Cards.back());
		m_Cards.pop_back();
	} else {
		cout << "Не осталось карт, нечего раздавать." << endl;
	}
}

void Deck::AdditionalCards(GenericPlayer& aGenericPlayer) {
	cout << endl;
	while (!(aGenericPlayer.IsBusted()) && aGenericPlayer.IsHitting()) {
		Deal(aGenericPlayer);
		cout << aGenericPlayer << endl;
		if (aGenericPlayer.IsBusted()) {
			aGenericPlayer.Burst();
		}
	}
}

class Game {
	public: 
		Game(const vector<string>& names, const vector<int>& sockets);
		~Game();

		void Play();
	private: 
		Deck m_Deck;
		House m_House;
		vector <Player> m_Players;
};

Game::Game(const vector<string>& names, const vector<int>& sockets) {

	for (int i = 0; i < MAX_CLIENTS; i++) {
		m_Players.push_back(Player(names[i], sockets[i], clients[i]));
	}
	
	srand(static_cast<unsigned int> (time(0)));
	m_Deck.Populate();
	m_Deck.Shuffle();
}

Game::~Game() {}

void Game::Play() {
	vector <Player>::iterator pPlayer;
	for (int i = 0; i < 2; ++i) {
		for (pPlayer = m_Players.begin(); pPlayer != m_Players.end(); ++pPlayer) {
			m_Deck.Deal(*pPlayer);
		}
		m_Deck.Deal(m_House);
	}
	m_House.FlipFirstCard();
	

	for (pPlayer = m_Players.begin(); pPlayer != m_Players.end(); ++pPlayer) {
		cout << *pPlayer << endl;
	}
	cout << m_House << endl;

	for (pPlayer = m_Players.begin(); pPlayer != m_Players.end(); ++pPlayer) {
		m_Deck.AdditionalCards(*pPlayer);
	}

	m_House.FlipFirstCard();
	cout << m_House << endl;

	m_Deck.AdditionalCards(m_House);
	if (m_House.IsBusted()) {
		for (pPlayer = m_Players.begin(); pPlayer != m_Players.end(); ++pPlayer) {
			if (!(pPlayer->IsBusted())) {
				pPlayer->Win();
			}
		}
	} else {
		for (pPlayer = m_Players.begin(); pPlayer != m_Players.end(); ++pPlayer) {
			if (!(pPlayer->IsBusted())) {
				if (pPlayer->GetTotal() > m_House.GetTotal()) {
					pPlayer->Win();
				} else if (pPlayer->GetTotal() < m_House.GetTotal()) {
					pPlayer->Lose();
				} else {
					pPlayer->Push();
				}
			}
		}
	}

	for (pPlayer = m_Players.begin(); pPlayer != m_Players.end(); ++pPlayer) {
		pPlayer->Clear();
	}
	m_House.Clear();
}

ostream& operator<<(ostream& os, const Card& aCard);
ostream& operator<<(ostream& os, const GenericPlayer& aGenericPlayer);


bool clientInArray(sockaddr_in clientAddr) {
	for(int i = 0; i < MAX_CLIENTS; i++) { 
		if (memcmp(&(clients[i].addr.sin_addr), &(clientAddr.sin_addr), sizeof(clientAddr.sin_addr)) 
			|| memcmp(&(clients[i].addr.sin_port), &(clientAddr.sin_port), sizeof(clientAddr.sin_port))) {
		} else {
			return true;
		}
	}

	return false;
}

int main() {
	socklen_t length;
	struct sockaddr_in servAddr, clientAddr, temp;;
	char buf[BUFLEN];
	char names[BUFLEN];
	int activity, difference;
	int numClients = 0;
	bool gameStarted, playAgain, isLobbyCompleted = false;
	double totalTime;
	char informationConnecting[BUFLEN], informationDisconnecting[BUFLEN];
	string lateWaiting = "Вы находились на сервере более 5 секунд без действий. До свидания!";
	

	if ((sockMain = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Сервер не может открыть socket для UDP.");
		exit(1);
	}
	bzero((char*)&servAddr, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = 0;
	length = sizeof(servAddr);
	if (bind(sockMain, (struct sockaddr*)&servAddr, length)) {
		perror("Связывание сервера неудачно.");
		exit(1);
	}
	if (getsockname(sockMain, (struct sockaddr*)&servAddr, &length)) {
		perror("Вызов getsockname неудачен.");
		exit(1);
	}
	printf("СЕРВЕР: номер порта - %d\n", ntohs(servAddr.sin_port));

	length = sizeof(clientAddr);

	while (1) {
		fd_set readfdsClients;
		struct timeval timeout;

		FD_ZERO(&readfdsClients);
		FD_SET(sockMain, &readfdsClients);
		timeout.tv_sec = 1;
    		timeout.tv_usec = 0;

		activity = select(sockMain + 1, &readfdsClients, NULL, NULL, &timeout);
		
		
		
		if (activity > 0) {
			
			bzero(names, sizeof(BUFLEN));
			if(FD_ISSET(sockMain, &readfdsClients)) {
				if (recvfrom(sockMain, names, BUFLEN, 0, (struct sockaddr*)&clientAddr, &length) < 0) {
					perror("Плохой socket клиента1.");
					exit(1);
				}
				if (clientInArray(clientAddr) == false) {
					if (numClients < MAX_CLIENTS) {
						for(int i = 0; i < MAX_CLIENTS; i++){
							
							if(clients[i].addr.sin_addr.s_addr == 0 && clients[i].addr.sin_port == 0){
								bzero(buf, sizeof(BUFLEN));
								if (sendto(sockMain, "Добро пожаловать в игровое лобби по игре BlackJack! Введите символ, хотите ли вы играть? (y/n): ", BUFLEN, 0, (struct sockaddr*)&clientAddr, length) < 0) {
										perror("Плохой socket клиента2.");
										exit(1);
								}
								
								clients[i].addr = clientAddr;
								clients[i].sockfd = sockMain;
								clients[i].index = i + 1;
								strcpy(clients[i].name, names);
								strcpy(clients[i].message, buf);
								clients[i].inLobby = true;
								clients[i].startTime = time(NULL);
								clients[i].isSent = false;
								decisions[i] = false;
								
								printf("Клиент %d\n", numClients);
								printf("IP адрес: %s\n", inet_ntoa(clients[numClients].addr.sin_addr));
								printf("Порт: %d\n", ntohs(clients[numClients].addr.sin_port));
								printf("INDEX: %d\n", clients[i].index);
								printf("Имя клиента: %s\n", clients[numClients].name);
								printf("Сообщение: %s\n", clients[numClients].message);
								printf("Boolean in lobby = %s\n", clients[numClients].inLobby ? "true" : "false");
								printf("DESICION: %s\n\n", decisions[numClients] ? "true" : "false");
								numClients++;
								break;
							}
						}
					} else {
						if (sendto(sockMain, "Лобби заполнено. Невозможно подключиться.", BUFLEN, 0, (struct sockaddr*)&clientAddr, length) < 0) {
							perror("Проблемы с sendto.\n");
							exit(1);
						}
					}
				} else {
					for(int i = 0; i < numClients; i++){
						if(clients[i].addr.sin_addr.s_addr == clientAddr.sin_addr.s_addr && clients[i].addr.sin_port == clientAddr.sin_port){
							clients[i].message[0] = names[0];
							decisions[i] = true;
							break;
						}
					}
					
					for(int i = 0; i < MAX_CLIENTS; i++){
						printf("CLIENTS NAME: %s AND ANSWER FROM CLIENT: %c\n", clients[i].name, clients[i].message[0]);
						printf("COUNTER = %d\n", i);
						if(clients[i].message[0] == 'n'){
							numClients--; 
							bzero(informationDisconnecting, sizeof(BUFLEN));
							informationDisconnecting[0] = '8';
							strcat(informationDisconnecting, clients[i].name);
							for (int j = 0; j < MAX_CLIENTS; j++) {
								if (clients[j].sockfd != 0 && clients[j].message[0] != NULL) {
									if (sendto(sockMain, informationDisconnecting, strlen(informationDisconnecting) + 1, 0, (struct sockaddr*)&clients[j].addr, length) < 0) {
										perror("Плохой socket клиента5.");
										exit(1);
									}
								}
							}
							if (sendto(sockMain, "Вы решили завершить игру. До свидания!", BUFLEN, 0, (struct sockaddr*)&clientAddr, length) < 0) {
								perror("Плохой socket клиента5.");
								exit(1);
							}

							bzero(&clients[i], sizeof(struct ClientInfo));
							bzero(&names[i], sizeof(BUFLEN));
							printf("Количество клиентов: %d\n", numClients);
							decisions[i] = false;
						}
						if(clients[i].message[0] == 'y'){
							bzero(informationConnecting, sizeof(BUFLEN));
							informationConnecting[0] = '9';
							strcat(informationConnecting, clients[i].name);
							
							printf("NAME: %s\n", informationConnecting);

							for (int j = 0; j < numClients, j != i; j++) {
								if (clients[j].sockfd != 0 && clients[j].isSent == false && clients[j].message[0] != NULL) {
									if (sendto(sockMain, informationConnecting, strlen(informationConnecting) + 1, 0, (struct sockaddr*)&clients[j].addr, length) < 0) {
										perror("Плохой socket клиента3.");
										exit(1);
									}
									
									
									clients[i].isSent = true;
								}
								
							}
							
						}
						if(i == numClients-1) break;
					}
					sendto(sockMain, "Вы подключились на сервер. Ожидайте подключения других игроков!", BUFLEN, 0, (struct sockaddr*)&clientAddr, length);

					for(int i = 0; i < numClients; i++){
						if(decisions[i] == true){
							isLobbyCompleted = true;
						}
						else{
							isLobbyCompleted = false;
							break;
						}
					}
					if(numClients == MAX_CLIENTS && isLobbyCompleted){
						gameStarted = false;
						playAgain = true;
						if (!gameStarted) {
							for (int i = 0; i < numClients; i++) {
								if (sendto(sockMain, "Игра началась!", BUFLEN, 0, (struct sockaddr*)&clients[i].addr, length) < 0) {
									perror("Плохой socket клиента6.");
									exit(1);
								}
								printf("Клиент %d\n", i);
								printf("IP адрес: %s\n", inet_ntoa(clients[i].addr.sin_addr));
								printf("Порт: %d\n", ntohs(clients[i].addr.sin_port));
								printf("Имя клиента: %s\n", clients[i].name);
								printf("Сообщение: %s\n\n", clients[i].message);
							}
							printf("Игра началась!\n");
							vector<string> namesClients;
							vector<int> sockets;
							string name;
							for (int i = 0; i < MAX_CLIENTS; ++i) {
								namesClients.push_back(clients[i].name);
								sockets.push_back(ntohs(clients[i].addr.sin_port));
								cout << "NAME: " << namesClients[i] << endl << "SOCKET: " << sockets[i] << endl;
							}
							cout << endl;

							Game aGame(namesClients, sockets);
							vector<char> answers;

							while (playAgain && numClients == MAX_CLIENTS) {
								aGame.Play();
								int counterAnswers = 0;
								for (int j = 0; j < MAX_CLIENTS; j++) {
									bzero(buf, sizeof(BUFLEN));
									if (sendto(sockMain, "Вы хотите сыграть ещё раз? (y/n)", BUFLEN, 0, (struct sockaddr*)&clients[j].addr, length) < 0) {
										perror("Плохой socket клиента6.");
										exit(1);
									}
									bzero(buf, sizeof(BUFLEN));
									
										/*if (recvfrom(sockMain, buf, BUFLEN, 0, (struct sockaddr*)&clients[j].addr, &length) < 0) {
											perror("Плохой socket клиента7.");
											exit(1);
										}*/
									while(1){
										if (recvfrom(sockMain, buf, BUFLEN, 0, (struct sockaddr*)&temp, &length) < 0) {
											perror("Нельзя принять ответ от клиента.");
											exit(1);
										}
										if(clientInArray(temp) == false){
											sendto(sockMain, "Лобби заполнено. Невозможно подключиться.", BUFLEN, 0, (struct sockaddr*)&temp, length);
										}
										else{
											break;
										}
									}
									
									auto iter = answers.cbegin();
									answers.insert(iter + j, buf[0]);
									printf("ANSWERS = %c\n", answers[j]);
								}

								for (int j = 0; j < answers.size(); j++) {
									if (answers[j] == 'n') {
										gameStarted = true;
										
										numClients--;
										clients[j].inLobby = false;
										counterAnswers++;
									}
									if(counterAnswers == MAX_CLIENTS) playAgain = false;
								}
								if (!playAgain) {
									for (int j = 0; j < MAX_CLIENTS; j++) {
										bzero(buf, sizeof(BUFLEN));
										if (sendto(sockMain, "Лобби закрывается. Никто не хочет играть.", BUFLEN, 0, (struct sockaddr*)&clients[j].addr, length) < 0) {
											perror("Плохой socket клиента111.");
											exit(1);
										}
										numClients = 0;
										bzero(&clients[j], sizeof(struct ClientInfo));
									}
								} 
								else{
									if(numClients < MAX_CLIENTS){
										for (int j = 0; j < MAX_CLIENTS; j++) {
											if(clients[j].inLobby == true){
												bzero(buf, sizeof(BUFLEN));
												if (sendto(sockMain, "Ожидайте подключения других игроков!", BUFLEN, 0, (struct sockaddr*)&clients[j].addr, length) < 0) {
													perror("Плохой socket клиента113.");
													exit(1);
												}
											}
											else{
												for(int k = 0; k < MAX_CLIENTS; k++){
													if(clients[k].inLobby == true){
														string infoDisconnecting = "";
														infoDisconnecting = "От сервера отключился игрок: " + string(clients[j].name);
														if (sendto(sockMain, infoDisconnecting.c_str(), infoDisconnecting.size() + 1, 0, (struct sockaddr*)&clients[k].addr, length) < 0) {
															perror("Плохой socket клиента110.");
															exit(1);
														}
													}
												}
												if (sendto(sockMain, "Вы решили закончить игру, до свидания!", BUFLEN, 0, (struct sockaddr*)&clients[j].addr, length) < 0) {
													perror("Плохой socket клиента110.");
													exit(1);
												}
												bzero(&clients[j], sizeof(struct ClientInfo));
												bzero(&names[j], sizeof(BUFLEN));
												
											}	
										}
									}
									if(numClients == MAX_CLIENTS) {
										for (int j = 0; j < MAX_CLIENTS; j++) {
											if(clients[j].inLobby == true){
												bzero(buf, sizeof(BUFLEN));
												if (sendto(sockMain, "Все решили играть снова, начнем!", BUFLEN, 0, (struct sockaddr*)&clients[j].addr, length) < 0) {
													perror("Плохой socket клиента112.");
													exit(1);
												}
											}
										}
									}
								
								}
								
								
							printf("\nPrintf after game!\n");
							bzero(names, sizeof(BUFLEN));
							}
						}
					}
				}
			}
		}
		else{
			totalTime = 0;
			
			for(int i = 0; i < MAX_CLIENTS; i++){
				clients[i].endTime = time(NULL);
				if(clients[i].message[0] == NULL && clients[i].addr.sin_addr.s_addr != 0 && clients[i].addr.sin_port != 0){
					totalTime = clients[i].endTime - clients[i].startTime;
					printf("TOTALTIME = %f\n", totalTime);
					if(totalTime >= 5){
						printf("Клиент %d\n", i);
						printf("IP адрес: %s\n", inet_ntoa(clients[i].addr.sin_addr));
						printf("Порт: %d\n", ntohs(clients[i].addr.sin_port));
						printf("Имя клиента: %s\n", clients[i].name);
						printf("Сообщение: %s\n\n", clients[i].message);
						if (sendto(sockMain, lateWaiting.c_str(), lateWaiting.size() + 1, 0, (struct sockaddr*)&clients[i].addr, length) < 0) {
							perror("Плохой socket клиента4.");
							exit(1);
						}
						bzero(&clients[i], sizeof(struct ClientInfo));
						numClients--;
					}
				}
			}
		}


		if (numClients == 0) {
			gameStarted = false;
			//break;
		}
	}
	printf("\n===================================");
	for (int i = 0; i < numClients; i++) {
		printf("111Клиент %d\n", i);
		printf("IP адрес: %s\n", inet_ntoa(clients[i].addr.sin_addr));
		printf("Порт: %d\n", ntohs(clients[i].addr.sin_port));
		printf("Сообщение: %s\n\n", clients[i].message);
	}
	printf("\n===================================");
	close(sockMain);
	return 0;
}

ostream& operator<<(ostream& os, const Card& aCard) {
	const string RANKS[] = { "0", "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K" };
	const string SUITS[] = { "clubs", "diamonds", "hearts", "spades" };

	string informationCards = RANKS[aCard.m_Rank] + "(" + SUITS[aCard.m_Suit] + ")";
	string informationUnknown = "XX";
	int length = sizeof(clients[0].addr);
	if (aCard.m_IsFaceUp) {
		for (int i = 0; i < MAX_CLIENTS; i++) {
			sendto(sockMain, informationCards.c_str(), informationCards.size() + 1, 0, (struct sockaddr*)&clients[i].addr, length);
		}
	} else {
		for (int i = 0; i < MAX_CLIENTS; i++) {
			sendto(sockMain, informationUnknown.c_str(), informationUnknown.size() + 1, 0, (struct sockaddr*)&clients[i].addr, length);
		}
	}
	return os;
}

ostream& operator<<(ostream& os, const GenericPlayer& aGenericPlayer) {
	string information = "@ " + aGenericPlayer.m_Name + ":\t";
	string result = "- " + to_string(aGenericPlayer.GetTotal());
	string emptyCards = "<empty>";
	vector<Card*>::const_iterator pCard;
	vector<string> cards;
	int length = sizeof(aGenericPlayer.clientID.addr);

	if (!aGenericPlayer.m_Cards.empty()) {
		for (pCard = aGenericPlayer.m_Cards.begin(); pCard != aGenericPlayer.m_Cards.end(); ++pCard) {
			cards.push_back((*(*pCard)).to_string());
		}
		for (auto & card: cards) {
			information += card + "\t";
		}
		if(aGenericPlayer.m_Name.compare("House") == 0){
			for(int i = 0; i < MAX_CLIENTS; i++) sendto(sockMain, information.c_str(), information.size() + 1, 0, (struct sockaddr * ) &clients[i].addr, length);	
		}
		sendto(sockMain, information.c_str(), information.size() + 1, 0, (struct sockaddr * ) & aGenericPlayer.clientID.addr, length);

		if (aGenericPlayer.GetTotal() != 0) {
			if(aGenericPlayer.m_Name.compare("House") == 0){
				for(int i = 0; i < MAX_CLIENTS; i++) sendto(sockMain, result.c_str(), result.size() + 1, 0, (struct sockaddr*)&clients[i].addr, length);	
			}
			sendto(sockMain, result.c_str(), result.size() + 1, 0, (struct sockaddr*)&aGenericPlayer.clientID.addr, length);
		}
	} else {
		for(int i = 0; i < MAX_CLIENTS; i++) sendto(sockMain, emptyCards.c_str(), emptyCards.size() + 1, 0, (struct sockaddr*)&clients[i].addr, length);
	}
	return os;
}

