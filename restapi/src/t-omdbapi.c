// This is test program which takes an command line argument
// as name of movie and then feedback to user the rotten
// tomatoes scoring.
// It uses the RESTAPI of omdbapi to fetch the scoring, if
// doesn't find it uses the imdi ratings and classify them
// into rotten tomatoes scoring.
// This program needs connectivity with internet to reach
// out www.omdbiapi.com

// Author:  Yogendra Pal
// Email :  jntupal@gmail.com
// Year  :  Dec, 2018


/** Sample output of restapi query
 *
{"Title":"Titanic","Year":"1997","Rated":"PG-13","Released":"19 Dec 1997","Runtime":"194 min","Genre":"Drama, Romance","Director":"James Cameron","Writer":"James Cameron","Actors":"Leonardo DiCaprio, Kate Winslet, Billy Zane, Kathy Bates","Plot":"A seventeen-year-old aristocrat falls in love with a kind but poor artist aboard the luxurious, ill-fated R.M.S. Titanic.","Language":"English, Swedish","Country":"USA","Awards":"Won 11 Oscars. Another 111 wins & 77 nominations.","Poster":"https://m.media-amazon.com/images/M/MV5BMDdmZGU3NDQtY2E5My00ZTliLWIzOTUtMTY4ZGI1YjdiNjk3XkEyXkFqcGdeQXVyNTA4NzY1MzY@._V1_SX300.jpg","Ratings":[{"Source":"Internet Movie Database","Value":"7.8/10"},{"Source":"Rotten Tomatoes","Value":"89%"},{"Source":"Metacritic","Value":"75/100"}],"Metascore":"75","imdbRating":"7.8","imdbVotes":"919,766","imdbID":"tt0120338","Type":"movie","DVD":"10 Sep 2012","BoxOffice":"N/A","Production":"Paramount Pictures","Website":"http://www.titanicmovie.com/","Response":"True"}
*/


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>


#undef   DEBUG
#define  RECV_BUFF_SIZE 1024
#define  TRUE 1
#define  FALSE 0

#define MSG1 "GET /?t="
#define MSG2 "&apikey=bd761d92 HTTP/1.1\r\nHost: www.omdbapi.com\r\n\r\n\r\n"
#define GET_REQ_LEN strlen (GET_REQ)
#define DST_HOSTNAME    "www.omdbapi.com"
#define DST_PORT        80

// Record for holding the verdict information
// for requested movie.
typedef struct verdict_rec_s 
{
    // Reviews rating
    char rtt_score[5];
    char imdb_score[7];
    char other_score[8];

    // Which review response received ?
    unsigned int rt_score:1;
    unsigned int im_score:1;
    unsigned int ot_score:1;

    // Timed out
    unsigned int tmo:1;
} verdict_rec_t;

verdict_rec_t verdict;

// Performs connect operation over TCP socket
int sock_connect (char *host, in_port_t port) 
{
	struct hostent *hp;
	struct sockaddr_in addr;
	int on = 1, sock;     

	if ((hp = gethostbyname(host)) == NULL) {
		herror("gethostbyname");
		exit(1);
	}
	bcopy(hp->h_addr, &addr.sin_addr, hp->h_length);
	addr.sin_port = htons(port);
	addr.sin_family = AF_INET;
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&on, sizeof(int));

	if (sock == -1) {
		perror("setsockopt");
		exit(1);
	}
	
	if (connect(sock, (struct sockaddr *)&addr, 
                sizeof(struct sockaddr_in)) == -1) {
		perror("connect");
		exit(1);
	}
	return sock;
}

// Parse the buffer and fill the struct with information
// to create the verdict.
void parse_recv_buffer (char *buffer)
{
    const char *s   = "Ratings";
    const char *rtt = "Rotten Tomatoes";
    const char *imd = "Internet Movie Database";
    const char *val = "Value";

    char *p, *p1;
    int i = 0;

    if (buffer != NULL) {
        p = strstr (buffer, s);
        if (p == NULL) {
            return;
        } else {
            // Search RTT first 
            p1 = p;
            p = strstr(p, rtt);

            if (p != NULL) {
                p = strstr(p, val);
                assert (p != NULL);
                while (*p != ':')
                    p++;
                
                p++; p++; // To reach "
                while (*p != '%')
                    verdict.rtt_score[i++] = *p++;
                verdict.rt_score = TRUE;
                return;
            }
            // Search IMD second
            p = p1;
            p = strstr(p, imd);

            if (p != NULL) {
                p = strstr(p, val);
                assert (p != NULL);
                while (*p != ':')
                    p++;
                
                p++; p++; // To reach "
                i = 0;
                while (*p != '"')
                    verdict.imdb_score[i++] = *p++;
                verdict.im_score = TRUE;
                return;
            }
        }
    }
    return;
}

// Gives the verdict
int get_verdict (char *dst_hostname, char *movie_name)
{
    fprintf(stdout,
            "Movie: %s\n", movie_name);
    if (verdict.rt_score == TRUE) {
        fprintf (stdout, 
                "Rotten Tomatoes rating: %s%s\n",
                verdict.rtt_score, "%");
    } else if (verdict.im_score == TRUE) {
        fprintf(stdout, 
                "IMDB rating: %s\n",
                verdict.imdb_score);
    } else if (verdict.ot_score == TRUE) {
        fprintf(stdout, 
                "Other rating: %s\n",
                verdict.other_score);
    } else {
        fprintf(stdout, "Rating: N/A\n");
        return 0;
    }

    return 0;
}

int main(int argc, char *argv[])
{
	int movie_namelen, get_reqlen, bytes = 0;
    int i, fd, rc;
	char buffer[RECV_BUFF_SIZE];
    char *get_req = NULL;
    char *p;
    
    // Usage description
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <Movie name, if multiple words, put into \" \">\n",
                argv[0]);
		exit(1);
	}

    // Alloc the GET request buffer
    movie_namelen = strlen (argv[1]);
    get_reqlen = (strlen(MSG1) + strlen(MSG2) + movie_namelen);
    get_req = calloc(1, get_reqlen);

    // Prepare the GET request
    bytes += snprintf(get_req + bytes, get_reqlen - bytes, "%s", MSG1);
    p = argv[1];
    i = 0;
    while (p[i] != '\0') {
        if (p[i] == ' ' || p[i] == '\n' || p[i] == '\t') {
            bytes += snprintf(get_req + bytes, get_reqlen - bytes, "%c", '+');
        } else {
            bytes += snprintf(get_req + bytes, get_reqlen - bytes, "%c", p[i]); 
        }
        i++;
    }
    bytes += snprintf(get_req + bytes, get_reqlen - bytes, "%s", MSG2); 
#ifdef DEBUG
    printf ("REQ:%s, len: %d\n", get_req, bytes);
#endif /* DEBUG */

    // Connect the TCP socket w/ DST_HOSTNAME
    fd = sock_connect(DST_HOSTNAME, DST_PORT);
    if (fd <= 0) {
        printf("Connect to socket failed, errno: %d\n", errno);
        exit(1);
    }

    // Send the GET request to DST_HOSTNAME
	rc = write(fd, get_req, get_reqlen);
    if (rc < 0) {
        printf("Connect to socket failed, errno: %d\n", errno);
        goto done;
    }
	bzero(buffer, RECV_BUFF_SIZE);
    memset(&verdict, 0, sizeof(verdict_rec_t));
	
    // Read the response and perform the parsing on it.
	while ((verdict.rt_score == FALSE) && 
           read(fd, buffer, RECV_BUFF_SIZE - 1) != 0) {
#ifdef DEBUG
        fprintf(stdout, "%s", buffer);
#endif /* DEBUG */
        parse_recv_buffer(buffer);
		bzero(buffer, RECV_BUFF_SIZE);
	}

    get_verdict(DST_HOSTNAME, p);

done:
    if (fd > 0) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }
    if (get_req != NULL) {
        free (get_req);
    }

	return 0;
}
