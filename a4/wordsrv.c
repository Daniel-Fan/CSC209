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

#include "socket.h"
#include "gameplay.h"


#ifndef PORT
    #define PORT 50265
#endif
#define MAX_QUEUE 5


void add_player(struct client **top, int fd, struct in_addr addr);
void remove_player(struct client **top, int fd);
int find_network_newline(const char *buf, int n);
char *read_msg(int fd);
int check_name(struct client **top, char *msg, int fd);


/* These are some of the function prototypes that we used in our solution 
 * You are not required to write functions that match these prototypes, but
 * you may find the helpful when thinking about operations in your program.
 */
/* Send the message in outbuf to all clients */
void broadcast(struct game_state *game);
void announce_turn(struct game_state *game, int cur_fd);
void announce_winner(struct game_state *game, struct client *winner);
/* Move the has_next_turn pointer to the next active client */
void advance_turn(struct game_state *game);
void announce_exit(struct game_state *game, struct client *exiter);
void announce_join(struct game_state *game, char *joiner);
int check_valid(char *guess);
int guess_word(struct game_state *game, char guess);
void not_valid(int cur_fd);
int check_turn(struct game_state *game, int cur_fd);
void not_your_turn(int cur_fd);
void not_letter(int cur_fd, char guess);
void announce_guess(struct game_state *game, char *guesser, char guess);
void broadcast_turn(struct game_state *game);

void broadcast(struct game_state *game){
    char *status = malloc(sizeof(char) * MAX_BUF);
    status = status_message(status, game);
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, status, strlen(status));
        if(num_written == -1){
            perror("write");
            exit(1);
        }
    }
}
void not_your_turn(int cur_fd){
    char result[MAX_BUF] = {'\0'};
    strcpy(result,"It is not your turn to guess.\n");
    int num_written = write(cur_fd, result, strlen(result));
        if(num_written == -1){
            perror("write");
            exit(1);
        }
}

int check_turn(struct game_state *game, int cur_fd){
    if(game->has_next_turn->fd == cur_fd){
        return 1;
    }
    else{
        return 0;
    }
}

void not_valid(int cur_fd){
    char result[MAX_BUF] = {'\0'};
    strcpy(result,"Your Guess is not valid\nPlease enter another guess\n");
    int num_written = write(cur_fd, result, strlen(result));
        if(num_written == -1){
            perror("write");
            exit(1);
        }
}

int check_valid(char *guess){
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

void not_letter(int cur_fd, char guess){
    char result[MAX_BUF] = {'\0'};
    strcpy(result, &guess);
    strcat(result, " is not in the word.\n");
    int num_written = write(cur_fd, result, strlen(result));
        if(num_written == -1){
            perror("write");
            exit(1);
        }
}


int guess_word(struct game_state *game, char guess){
    int index_letter = guess - 'a';
    int result = 2;
    //the letter has benn guessed
    if(game->letters_guessed[index_letter] == 1){
        result = 0;
    }else{
        for(int i=0; i < strlen(game->word); i++){
            //the letter is in the words
            if(guess == game->word[i]){
                result = 1;
                game->guess[i] = guess;
            }
        }
        if(result == 2){
            game->guesses_left -= 1;
        }
    }
    //set this letter to guessed
    game->letters_guessed[index_letter] = 1;
    return result;
}
void broadcast_turn(struct game_state *game){
    char result[MAX_BUF] = {'\0'};
    strcpy(result, "It's ");
    strcat(result, game->has_next_turn->name);
    strcat(result, "'s turn.\n");
    char *your_turn = "YOUR GUESS?\n";
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        if(fd == game->has_next_turn->fd){
            int num_written = write(fd, your_turn, strlen(your_turn));
            if(num_written == -1){
            perror("write");
            exit(1);
            }
        }else{
            int num_written = write(fd, result, strlen(result));
            if(num_written == -1){
            perror("write");
            exit(1);
            }
        }
    }
}


void announce_turn(struct game_state *game, int cur_fd){
    char result[MAX_BUF] = {'\0'};
    strcpy(result, "It's ");
    strcat(result, game->has_next_turn->name);
    strcat(result, "'s turn.\n");

    char *your_turn = "YOUR GUESS?\n";
    if(cur_fd == game->has_next_turn->fd){
        int num_written = write(cur_fd, your_turn, strlen(your_turn));
        if(num_written == -1){
            perror("write");
            exit(1);
        }
    }else{
        int num_written = write(cur_fd, result, strlen(result));
        if(num_written == -1){
            perror("write");
            exit(1);
        }
    }
}

void advance_turn(struct game_state *game){
    if(game->has_next_turn->next != NULL){
        game->has_next_turn = game->has_next_turn->next;

    }else{
        game->has_next_turn = game->head;
    }
}

void announce_guess(struct game_state *game, char *guesser, char guess){
    char result[MAX_BUF];
    strcpy(result, guesser);
    strcat(result, " guesses: ");
    strncat(result, &guess, sizeof(char));
    strcat(result, "\n\0");
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, result, strlen(result));
        if(num_written == -1){
            perror("write");
            exit(1);
        }
    }

}

void announce_join(struct game_state *game, char *joiner){
    char result[MAX_BUF];
    strcpy(result, joiner);
    strcat(result, " has just joined.\n");
    struct client *cur_cl;
    printf("%s", result);
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, result, strlen(result));
        if(num_written == -1){
                announce_exit(game, cur_cl);
            }
        announce_turn(game, cur_cl->fd);
    }
}

void announce_exit(struct game_state *game, struct client *exiter){
    char result[MAX_BUF];
    strcpy(result, "Goodbye ");
    strcat(result, exiter->name);
    struct client *cur_cl;
    for(cur_cl = game->head; cur_cl != NULL; cur_cl = cur_cl->next){
        int fd = cur_cl->fd;
        int num_written = write(fd, result, strlen(result));
        if(num_written != -1){
            announce_exit(game, cur_cl);
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

char *read_msg(int fd){
    // Receive messages
    char buf[MAX_BUF] = {'\0'};

    int nbytes;
    nbytes = read(fd, buf, MAX_BUF);
    if(nbytes == -1){
        perror("read:");
        exit(1);
    }

    int where = find_network_newline(buf, MAX_BUF);
    buf[where-2] = '\0';
    
    char *msg = malloc(sizeof(char) * MAX_BUF);
    if(msg == NULL){
        perror("malloc");
        exit(1);
    }
    strcpy(msg, buf);
    msg[where-2] = '\0';
    return msg;
}

int check_name(struct client **top, char *msg, int fd){
    // if(strlen(msg) == 0){
    //     char *error_msg = "The name can't be empty.\n";
    //     if(write(fd, error_msg, strlen(error_msg)) < 0){
    //         perror("write to user in check name:");
    //         exit(1);
    //     }
    //     return 1;
    // }
    if(strlen(msg) >= 30){
        char *error_msg = "The name is too long.\n";
        if(write(fd, error_msg, strlen(error_msg)) < 0){
            perror("write to user in check name:");
            exit(1);
        }
        return 1;
    }
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
                        rec_msg = read_msg(cur_fd);
                        //empty msg ignored
                        if(strlen(rec_msg) == 0){
                            free(rec_msg);
                            continue;
                        }
                        //it is the current turn
                        else if(check_turn(&game, cur_fd)){
                            if(check_valid(rec_msg)){
                                int guess = guess_word(&game, rec_msg[0]);
                                if(guess == 0){
                                    not_valid(cur_fd);
                                    free(rec_msg);
                                }else if(guess == 2){
                                    not_letter(cur_fd, rec_msg[0]);
                                    announce_guess(&game, game.has_next_turn->name, rec_msg[0]);
                                    advance_turn(&game);
                                    free(rec_msg);
                                    broadcast(&game);
                                    broadcast_turn(&game);
                                }else if(guess == 1){
                                    announce_guess(&game, game.has_next_turn->name, rec_msg[0]);
                                    free(rec_msg);
                                    broadcast(&game);
                                    broadcast_turn(&game);
                                }
                            }else{
                                not_valid(cur_fd);
                                free(rec_msg);
                            }
                        }
                        //not current turn
                        else{
                            not_your_turn(cur_fd);
                            free(rec_msg);
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
                        name = read_msg(cur_fd);
                        if(strlen(name) == 0){
                            char *error_msg = "The name can't be empty.\n";
                            if(write(cur_fd, error_msg, strlen(error_msg)) < 0){
                                perror("write to user in check name:");
                                exit(1);
                            }
                            free(name);
                        }
                        else if(check_name(&game.head, name, cur_fd) == 1){
                            free(name);
                            continue;
                        }
                        else{
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
                            announce_join(&game, name);
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
                    else{
                        prev = p;
                    }
                }
            }
        }
    }
    return 0;
}


