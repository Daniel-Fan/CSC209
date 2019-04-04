#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <signal.h>

#include "socket.h"
#include "gameplay.h"


#ifndef PORT
    #define PORT 50265
#endif
#define MAX_QUEUE 5

// add player to linked list
void add_player(struct client **top, int fd, struct in_addr addr);

// remove player from linked list
void remove_player(struct client **top, int fd);

// find the new line from network input
int find_network_newline(const char *buf, int n);

// read the message from the network
int read_msg(struct game_state *game, int fd, char *msg);

// check whether the name is valid
int check_name(struct client **top, char *msg, int fd);

/* broadcast game status to all clients */
void broadcast(struct game_state *game);

//announce turn for particular client.
void announce_turn(struct game_state *game, int cur_fd);

// announce winner to all the clients.
void announce_winner(struct game_state *game, struct client *winner);

/* Move the has_next_turn pointer to the next active client */
void advance_turn(struct game_state *game);

// announce who is exit to all clients
void announce_exit(struct game_state *game, char *exiter);

// announce who is exit to all clients
void announce_join(struct game_state *game, int cur_fd, char *name);

// check whether the guess is valid
int check_valid(char *guess);

// check whether the letter from valid guess is in the word
int guess_word(struct game_state *game, char guess);

// announce that the guess is invalid.
void not_valid(struct game_state *game, int cur_fd);

// check whether is the client's turn
int check_turn(struct game_state *game, int cur_fd);

// announce it is not your turn
void not_your_turn(struct game_state *game, int cur_fd);

// announce that the letter is not in the word
void not_letter(struct game_state *game, int cur_fd, char guess);

// announce the letter guessed to all client
void announce_guess(struct game_state *game, char *guesser, char guess);

// broadcast the turn to all clients
void broadcast_turn(struct game_state *game);

// check whether it win
int is_win(struct game_state *game);

// check whether it lose
int is_lose(struct game_state *game);

// anounce the lose to all client
void announce_lose(struct game_state *game);

// restart a new game
void restart_game(struct game_state *game, char *dict_name);

// disconnect the client from active players
void is_disconnect(struct game_state *game, int cur_fd);


void announce_exit(struct game_state *game, char *exiter){
    //The msg to announce that who exits
    char result[MAX_BUF];
    sprintf(result, "Goodbye %s\r\n", exiter);
    printf("Goodbye %s\n", exiter);

    // got through all clients to write the announcement
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, result, strlen(result));
        if(num_written == -1){
            is_disconnect(game, fd);
        }
    }

    // if there is no player in the game, no need to announce the turn.
    if(game->head != NULL){
        broadcast_turn(game);
    }
}


void is_disconnect(struct game_state *game, int cur_fd){
    //find the location in the linked list where the client is
    struct client **p;

    for (p = &game->head; *p && (*p)->fd != cur_fd; p = &(*p)->next)
        ;
    
    // get the name of client who exits
    char name[MAX_BUF] = {"\0"};
    strcpy(name, (*p)->name);

    // if the exiter has next turn, change the turn to next player
    if(cur_fd == game->has_next_turn->fd){
        if((game->has_next_turn == game->head) && (game->head->next == NULL)){
            game->has_next_turn = NULL;
        }
        else{
            advance_turn(game);
        }
    }

    //remove the player and announce
    remove_player(&(game->head), cur_fd);
    announce_exit(game, name);
}


void restart_game(struct game_state *game, char *dict_name){
    // the msg to announce
    char result[MAX_BUF] = {"\0"};
    sprintf(result, "\r\n"
            "\r\n"
            "Let's start a new game\r\n");
    printf("Start a new Game.\n");

    // go through all clients
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, result, strlen(result));
        if(num_written == -1){
            is_disconnect(game, fd);
        }
    }

    //initialize the game and broadcast the status and turn to all clients
    init_game(game, dict_name);
    broadcast(game);
    broadcast_turn(game);
}


void announce_lose(struct game_state *game){
    // the msg to announce
    char result[MAX_BUF] = {"\0"};
    sprintf(result, "No more Guesses. The word was %s\r\n", game->word);
    printf("No more Guesses. The word was %s\n", game->word);

    // go through all clients
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, result, strlen(result));
        if(num_written == -1){
            is_disconnect(game, fd);
        }
    }
}

int is_lose(struct game_state *game){
    // if lose, return 1
    if(game->guesses_left == 0){
        return 1;
    }
    return 0;
}

int is_win(struct game_state *game){
    // if win, return 1
    if(strcmp(game->guess, game->word) == 0){
        return 1;
    }
    return 0;
}

void announce_winner(struct game_state *game, struct client *winner){
    // msg to announce
    char result[MAX_BUF] = {"\0"};
    sprintf(result, "The word was %s\r\n"
            "Game Over! %s Won\r\n", game->word, winner->name);
    char win[MAX_BUF] = {"\0"};
    sprintf(win, "The word was %s\r\n"
            "Game Over! You Won\r\n", game->word);
    printf("The word was %s. Game Over! %s Won\n", game->word, winner->name);

    // go through all clients
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;

        //the winner and others have the different msg
        if(fd == winner->fd){
            int num_written = write(fd, win, strlen(win));
            if(num_written == -1){
                is_disconnect(game, fd);
            }
        }else{
            int num_written = write(fd, result, strlen(result));
            if(num_written == -1){
                is_disconnect(game, fd);
            }
        }
    }
}

void broadcast(struct game_state *game){
    // get status msg from status_message
    char *status = malloc(sizeof(char) * MAX_BUF);
    status = status_message(status, game);\

    // go through all clients
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, status, strlen(status));
        if(num_written == -1){
            is_disconnect(game, fd);
        }
    }
}

void not_your_turn(struct game_state *game, int cur_fd){
    // msg to announce to particular client
    char result[MAX_BUF] = {'\0'};
    sprintf(result, "It is not your turn to guess.\r\n");
    printf("[%d] not in the turn.\n", cur_fd);

    // write the msg to this client
    int num_written = write(cur_fd, result, strlen(result));
        if(num_written == -1){
            is_disconnect(game, cur_fd);
        }
}

int check_turn(struct game_state *game, int cur_fd){
    // if it is the turn, return 1
    if(game->has_next_turn->fd == cur_fd){
        return 1;
    }
    else{
        return 0;
    }
}

void not_valid(struct game_state *game, int cur_fd){
    // msg to announce to particular client
    char result[MAX_BUF] = {'\0'};
    sprintf(result, "Your Guess is not valid\nPlease enter another guess\r\n");
    printf("[%d] receive invaild guess.\n", cur_fd);

    // write the msg to this client
    int num_written = write(cur_fd, result, strlen(result));
        if(num_written == -1){
            is_disconnect(game, cur_fd);
        }
}

int check_valid(char *guess){
    // if valid, return 1
    if(strlen(guess) != 1){
        return 0;
    }
    else if('a' <= (int)guess[0] && (int)guess[0]<= 'z'){
        return 1;
    }
    else
    {
        return 0;
    }
}

void not_letter(struct game_state *game, int cur_fd, char guess){
    // msg to announce to particular client
    char result[MAX_BUF] = {'\0'};
    sprintf(result, "%c is not in the word.\r\n", guess);
    printf("[%d] guess wrong letter.\n", cur_fd);

    //write the msg to client
    int num_written = write(cur_fd, result, strlen(result));
        if(num_written == -1){
            is_disconnect(game, cur_fd);
        }
}

int guess_word(struct game_state *game, char guess){
    //find the index of word guessed
    int index_letter = guess - 'a';

    int result = 2;

    //the letter has been guessed
    if(game->letters_guessed[index_letter] == 1){
        result = 0;
    }
    else{
        for(int i=0; i < strlen(game->word); i++){
            //the letter is in the words
            if(guess == game->word[i]){
                result = 1;
                game->guess[i] = guess;
            }
        }
        // wrong guess
        if(result == 2){
            game->guesses_left -= 1;
        }
    }

    //set this letter to guessed
    game->letters_guessed[index_letter] = 1;
    return result;
}

void broadcast_turn(struct game_state *game){
    // msg to announce to all clients
    char result[MAX_BUF] = {'\0'};
    sprintf(result, "It's %s's turn.\r\n", game->has_next_turn->name);
    char *your_turn = "YOUR GUESS?\r\n";

    // write the msg to all clients
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        // the client who is in the turn.
        if(fd == game->has_next_turn->fd){
            int num_written = write(fd, your_turn, strlen(your_turn));
            if(num_written == -1){
                is_disconnect(game, fd);
            }
        }
        // the client who is not in the turn
        else{
            int num_written = write(fd, result, strlen(result));
            if(num_written == -1){
                is_disconnect(game, fd);
            }
        }
    }
}

void announce_turn(struct game_state *game, int cur_fd){
    //msg to announce to particular client
    char result[MAX_BUF] = {'\0'};
    sprintf(result, "It's %s's turn.\r\n", game->has_next_turn->name);
    char *your_turn = "YOUR GUESS?\r\n";

    // it is turn of this client
    if(cur_fd == game->has_next_turn->fd){
        int num_written = write(cur_fd, your_turn, strlen(your_turn));
        if(num_written == -1){
            is_disconnect(game, cur_fd);
        }
    }
    // it is not the turn of this client
    else{
        int num_written = write(cur_fd, result, strlen(result));
        if(num_written == -1){
            is_disconnect(game, cur_fd);
        }
    }
}

void advance_turn(struct game_state *game){
    //the client who has next turn has the next client
    if(game->has_next_turn->next != NULL){
        game->has_next_turn = game->has_next_turn->next;
    }
    //the client who has next turn is the last one in the list
    else{
        game->has_next_turn = game->head;
    }
}

void announce_guess(struct game_state *game, char *guesser, char guess){
    // msg to announce to all clients
    char result[MAX_BUF];
    sprintf(result, "%s guesses: %c\r\n", guesser, guess);

    // go through all clients
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, result, strlen(result));
        if(num_written == -1){
            is_disconnect(game, fd);
        }
    }
}

void announce_join(struct game_state *game, int cur_fd, char *name){
    // msg to announce to all client
    char result[MAX_BUF];
    sprintf(result, "%s has just joined.\r\n", name);

    // go through all clients
    struct client *cur_cl;
    printf("%s", result);
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, result, strlen(result));
        if(num_written == -1){
            is_disconnect(game, fd);
        }
        // if it is not the client who just joined, announce the turn to other clients
        if(fd != cur_fd){
            announce_turn(game, fd);
        }
    }
}

/* The set of socket descriptors for select to monitor.
 * This is a global variable because we need to remove socket descriptors
 * from allset when a write to a socket fails.
 */
fd_set allset;


/* Add a client to the head of the linked list
 */
void add_player(struct client **top, int fd, struct in_addr addr) {
    struct client *p = malloc(sizeof(struct client));

    if (!p) {
        perror("malloc");
        exit(1);
    }

    printf("Adding client %s\n", inet_ntoa(addr));

    p->fd = fd;
    p->ipaddr = addr;
    p->name[0] = '\0';
    p->in_ptr = p->inbuf;
    p->inbuf[0] = '\0';
    p->next = *top;
    *top = p;
}

/* Removes client from the linked list and closes its socket.
 * Also removes socket descriptor from allset 
 */
void remove_player(struct client **top, int fd) {
    struct client **p;

    for (p = top; *p && (*p)->fd != fd; p = &(*p)->next)
        ;
    // Now, p points to (1) top, or (2) a pointer to another client
    // This avoids a special case for removing the head of the list
    if (*p) {
        struct client *t = (*p)->next;
        printf("Removing client %d %s\n", fd, inet_ntoa((*p)->ipaddr));
        FD_CLR((*p)->fd, &allset);
        close((*p)->fd);
        free(*p);
        *p = t;
    } else {
        fprintf(stderr, "Trying to remove fd %d, but I don't know about it\n",
                 fd);
    }
}

/*
 * Search the first n characters of buf for a network newline (\r\n).
 * Return one plus the index of the '\n' of the first network newline,
 * or -1 if no network newline is found.
 * Definitely do not use strchr or other string functions to search here. (Why not?)
 */
int find_network_newline(const char *buf, int n) {
    for(int i=1; i<n; i++){
        if(buf[i-1] == '\r'){
            if(buf[i] == '\n'){
                return i + 1;
            }
        }
    }
    return -1;
}

int read_msg(struct game_state *game, int fd, char *msg){
    // Receive messages
    char buf[MAX_BUF] = {'\0'};

    int nbytes;
    nbytes = read(fd, buf, MAX_BUF);
    if(nbytes == -1){
        return fd;
    }
    printf("[%d] read %d bytes\n", fd, nbytes);
    
    // find out the newline character
    int where = find_network_newline(buf, MAX_BUF);
    while(where == -1 && nbytes != 0){
        nbytes = read(fd, buf+nbytes, MAX_BUF);
        if(nbytes == -1){
            return fd;
        }
        printf("[%d] read %d bytes\n", fd, nbytes);
        where = find_network_newline(buf, MAX_BUF);
    }

    buf[where-2] = '\0';
    
    if(nbytes == 0){
        return fd;
    }
    
    strcpy(msg, buf);
    msg[where-2] = '\0';
    return 0;
}

int check_name(struct client **top, char *msg, int fd){
    // the name is too long
    if(strlen(msg) >= 30){
        char *error_msg = "The name is too long.\n";
        if(write(fd, error_msg, strlen(error_msg)) < 0){
            perror("write to user in check name:");
            exit(1);
        }
        return 1;
    }
    // the name has been taken
    else{
        struct client *temp;
        temp = *top;
        while(temp != NULL){
            if(strcmp(temp->name, msg) == 0){
                char *error_msg = "The name hes been taken.\n";
                if(write(fd, error_msg, strlen(error_msg)) < 0){
                    perror("write to user in check name:");
                    exit(1);
                }
                return 1;
            }
            temp = temp->next;
        }
        return 0;
    }
}

int main(int argc, char **argv) {
    int clientfd, maxfd, nready;
    struct client *p;
    struct sockaddr_in q;
    fd_set rset;
    
    if(argc != 2){
        fprintf(stderr,"Usage: %s <dictionary filename>\n", argv[0]);
        exit(1);
    }
    

    // Add the following code to main in wordsrv.c:
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    if(sigaction(SIGPIPE, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    // Create and initialize the game state
    struct game_state game;

    srandom((unsigned int)time(NULL));
    // Set up the file pointer outside of init_game because we want to 
    // just rewind the file when we need to pick a new word
    game.dict.fp = NULL;
    game.dict.size = get_file_length(argv[1]);

    init_game(&game, argv[1]);
    
    // head and has_next_turn also don't change when a subsequent game is
    // started so we initialize them here.
    game.head = NULL;
    game.has_next_turn = NULL;
    
    /* A list of client who have not yet entered their name.  This list is
     * kept separate from the list of active players in the game, because
     * until the new playrs have entered a name, they should not have a turn
     * or receive broadcast messages.  In other words, they can't play until
     * they have a name.
     */
    struct client *new_players = NULL;
    
    struct sockaddr_in *server = init_server_addr(PORT);
    int listenfd = set_up_server_socket(server, MAX_QUEUE);
    
    // initialize allset and add listenfd to the
    // set of file descriptors passed into select
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    // maxfd identifies how far into the set to search
    maxfd = listenfd;

    while (1) {
        // make a copy of the set before we pass it into select
        rset = allset;
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
        if (nready == -1) {
            perror("select");
            continue;
        }

        if (FD_ISSET(listenfd, &rset)){
            printf("A new client is connecting\n");
            clientfd = accept_connection(listenfd);

            FD_SET(clientfd, &allset);
            if (clientfd > maxfd) {
                maxfd = clientfd;
            }
            printf("Connection from %s\n", inet_ntoa(q.sin_addr));
            add_player(&new_players, clientfd, q.sin_addr);
            char *greeting = WELCOME_MSG;
            if(write(clientfd, greeting, strlen(greeting)) == -1) {
                fprintf(stderr, "Write to client %s failed\n", inet_ntoa(q.sin_addr));
                remove_player(&new_players, p->fd);
            };
        }
        
        /* Check which other socket descriptors have something ready to read.
         * The reason we iterate over the rset descriptors at the top level and
         * search through the two lists of clients each time is that it is
         * possible that a client will be removed in the middle of one of the
         * operations. This is also why we call break after handling the input.
         * If a client has been removed the loop variables may not longer be 
         * valid.
         */
        int cur_fd;
        for(cur_fd = 0; cur_fd <= maxfd; cur_fd++) {
            if(FD_ISSET(cur_fd, &rset)) {
                // Check if this socket descriptor is an active player
                for(p = game.head; p != NULL; p = p->next) {
                    if(cur_fd == p->fd) {
                        //TODO - handle input from an active client
                        char *rec_msg;
                        if((rec_msg = malloc(sizeof(char)*MAX_BUF)) == NULL){
                            perror("malloc");
                            exit(1);
                        }
                        // check whether the cilent is disconnected
                        int client_closed = read_msg(&game, cur_fd, rec_msg);
                        if (client_closed > 0){
                            is_disconnect(&game, cur_fd);
                            free(rec_msg);
                            break;
                        }
                        // get the msg from players
                        else{
                            //empty msg ignored
                            if(strlen(rec_msg) == 0){
                                free(rec_msg);
                                continue;
                            }
                            //it is the current turn
                            else if(check_turn(&game, cur_fd)){
                                //check whether the input is valid
                                if(check_valid(rec_msg)){
                                    int guess = guess_word(&game, rec_msg[0]);
                                    // invalid input
                                    if(guess == 0){
                                        not_valid(&game, cur_fd);
                                        free(rec_msg);
                                    }
                                    // the valid input but the letter is not in the word
                                    else if(guess == 2){
                                        not_letter(&game, cur_fd, rec_msg[0]);
                                        // they lose the game
                                        if(is_lose(&game) == 1){
                                            announce_lose(&game);
                                            advance_turn(&game);
                                            restart_game(&game, argv[1]);
                                            free(rec_msg);
                                        }
                                        // continue the game to guess and change turn
                                        else{
                                            announce_guess(&game, game.has_next_turn->name, rec_msg[0]);
                                            advance_turn(&game);
                                            free(rec_msg);
                                            broadcast(&game);
                                            broadcast_turn(&game);
                                        }
                                    }
                                    //the letter is in the word
                                    else if(guess == 1){
                                        // win the game
                                        if(is_win(&game) == 1){
                                            announce_winner(&game, game.has_next_turn);
                                            restart_game(&game, argv[1]);
                                            free(rec_msg);
                                        }
                                        // continue the game to guess and not change turn
                                        else{
                                            announce_guess(&game, game.has_next_turn->name, rec_msg[0]);
                                            free(rec_msg);
                                            broadcast(&game);
                                            broadcast_turn(&game);
                                        }
                                        
                                    }
                                }
                                // the guess input is invalid
                                else{
                                    not_valid(&game, cur_fd);
                                    free(rec_msg);
                                }
                            }
                            //not current turn
                            else{
                                not_your_turn(&game, cur_fd);
                                free(rec_msg);
                            }
                        }
                    }
                }
                struct client *prev = NULL;
                // Check if any new players are entering their names
                for(p = new_players; p != NULL; p = p->next) {
                    if(cur_fd == p->fd) {
                        // TODO - handle input from an new client who has
                        // not entered an acceptable name.
                        char *name;
                        if((name = malloc(sizeof(char)*MAX_BUF)) == NULL){
                            perror("malloc");
                            exit(1);
                        }

                        // check the disconnection
                        int client_closed = read_msg(&game, cur_fd, name);
                        if (client_closed > 0){
                            printf("remove new client [%d]\n", cur_fd);
                            // the last player in the list
                            if(new_players->next == NULL){
                                FD_CLR(cur_fd, &allset);
                                close(cur_fd);
                                new_players = NULL;
                            }
                            else{
                                FD_CLR(cur_fd, &allset);
                                close(cur_fd);
                                if(prev){
                                    prev->next = p->next;
                                }
                                else{
                                    new_players = p->next;
                                }
                            }
                            free(name);
                            continue;
                        }
                        // get the input from client
                        else{
                            // empty input from network
                            if(strlen(name) == 0){
                                char *error_msg = "The name can't be empty.\n";
                                if(write(cur_fd, error_msg, strlen(error_msg)) < 0){
                                    perror("write to user in check name:");
                                    exit(1);
                                }
                                prev = p;
                                free(name);
                            }
                            // check whether the name is valid
                            else if(check_name(&game.head, name, cur_fd) == 1){
                                free(name);
                                prev = p;
                                continue;
                            }
                            else{
                                // invalid name, add the player to active list and remove from new_player list
                                if(prev){
                                    prev->next = p->next;
                                }
                                else{
                                    new_players = p->next;
                                }
                                add_player(&game.head, cur_fd, q.sin_addr);
                                strcpy(game.head->name, name);
                                if(game.has_next_turn == NULL){
                                    game.has_next_turn = game.head;
                                }
                                // write the game status to client and announce the join
                                announce_join(&game, p->fd, name);
                                char *status = malloc(sizeof(char) * MAX_BUF);
                                status = status_message(status, &game);
                                int num_written = write(cur_fd, status, strlen(status));
                                if(num_written == -1){
                                    perror("write");
                                    exit(1);
                                }
                                announce_turn(&game, cur_fd);
                                free(name);
                                free(status);
                            }
                        }
                    }
                    // get the previous client in the list
                    prev = p;
                }
            }
        }
    }
    return 0;
}
