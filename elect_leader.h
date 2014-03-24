#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <time.h>

typedef struct pkg_s {
	int id;
	int phase;
	int hops;
} pkg;

typedef struct info_s {
	/* global setting info */
	int rank;
	int size;
	int prime;

	/*constructed basic info*/
	int id;
	int right;
	int left;
	int my_phase_no; //how far i got as leader

	/*collected info*/
	int *phase_info; //this will be dynamically allocated given world size, and indexed
	int no_sent;
	int no_recv;
	int leader_id;

} Info;


#define ELECTION 1
#define REPLY 2
#define LEADER 3
#define NO_SENT 18
#define NO_RECV 36
#define DONE 100
#define NEIGHBOURS 2

int elected_message(int primary, int secondary, pkg *primary_pkg);
int reply_message(int primary, int secondary, pkg *primary_pkg);
int power(int base, int exp);
