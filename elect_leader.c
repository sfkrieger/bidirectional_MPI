/*
 * elect_leader.c
 *
 *  Created on: Mar 17, 2014
 *      Author: Samiam

 */
#include "elect_leader.h"  /* TCP echo server includes */

int id;
int world_rank;
int world_size;
int pnum;
pkg to_pass;
int *replies_rcvd;
int i_am_leader = 0;
int leader = -1;

/* info to print */
int no_sent = 0;
int no_recvd = 0;

int main(int argc, char** argv) {

	//============================== INITIALIZATION =================================

	/**
	 * First initilize the MPI environment
	 */

	MPI_Init(NULL, NULL); /* initializes the MPI world? */
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	if (argc < 2) {
		printf("Error, correct usage: <PRIME NUMBER>");
		exit(1);
	} else {
		pnum = atoi(argv[1]);
	}

	id = (world_rank + 1) * pnum % world_size;

	int right = ((world_rank == (world_size - 1)) ? 0 : world_rank + 1);
	int left = ((world_rank == 0) ? (world_size - 1) : world_rank - 1);

	/*
	 * deal with the struct creation
	 */
	const int nitems = 3;
	int blocklengths[3] = { 1, 1, 1 };
	MPI_Datatype types[3] = { MPI_INT, MPI_INT, MPI_INT };
	MPI_Datatype mpi_pkg;
	MPI_Aint offsets[3];

	offsets[0] = offsetof(pkg, id);
	offsets[1] = offsetof(pkg, phase);
	offsets[2] = offsetof(pkg, hops);

	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_pkg);
	MPI_Type_commit(&mpi_pkg);
	//============================== LEADER ELECTION =================================

	to_pass.id = id;
	to_pass.phase = 0;
	to_pass.hops = 0;

	MPI_Send(&to_pass, 1, mpi_pkg, right, ELECTION, MPI_COMM_WORLD);
	MPI_Send(&to_pass, 1, mpi_pkg, left, ELECTION, MPI_COMM_WORLD);
	no_sent = no_sent + 2;

	MPI_Status status;
	pkg result;
	replies_rcvd = (int*) calloc(world_size, sizeof(int));
	int reply_status = 0;

	while (leader == -1) {

		MPI_Recv(&result, 1, mpi_pkg, MPI_ANY_SOURCE, MPI_ANY_TAG,
				MPI_COMM_WORLD, &status);
		no_recvd++;

		//		MPI_Waitall(2, recv_req, status);

		if (status.MPI_TAG == ELECTION) {
			if (status.MPI_SOURCE == left) {
				leader = elected_message(left, right, &result);
			} else if (status.MPI_SOURCE == right) {
				leader = elected_message(right, left, &result);
			} else {
				printf(
						"============= ERROR - PROCESS %d GETTING MESSAGE FROM BAD SOURCE OF RANK %d =============",
						world_rank, status.MPI_SOURCE);
			}

			if (leader != -1) {
				i_am_leader = 1;
				//				printf("Process %d declares %d leader\n", world_rank, leader);
				result.hops = no_sent;
				result.phase = no_recvd;
				MPI_Send(&result, 1, mpi_pkg, right, LEADER, MPI_COMM_WORLD);
				MPI_Recv(&result, 1, mpi_pkg, left, LEADER, MPI_COMM_WORLD,
						&status);

			}
			//HERE! check if the result from the election made you win the phase! (or actually maybe not..)
		} else if (status.MPI_TAG == REPLY) {

			//			printf("Got a reply message... not dealing with it right now\n");
			if (status.MPI_SOURCE == left) {
				reply_status = reply_message(left, right, &result);
			} else if (status.MPI_SOURCE == right) {
				reply_status = reply_message(right, left, &result);
			} else {
				printf(
						"============= ERROR - PROCESS %d GETTING MESSAGE FROM BAD SOURCE OF RANK %d =============",
						world_rank, status.MPI_SOURCE);
			}

			if (replies_rcvd[result.phase] == 2) {
				//				printf(
				//						"Process %d aparently won phase %d (really %d). It had a distance of %d\n",
				//						world_rank, result.phase, to_pass.phase, result.hops);
				to_pass.phase++;
				to_pass.hops = 0;

				MPI_Send(&to_pass, 1, mpi_pkg, right, ELECTION, MPI_COMM_WORLD);
				MPI_Send(&to_pass, 1, mpi_pkg, left, ELECTION, MPI_COMM_WORLD);
				no_sent = no_sent + 2;
				printf(
						"Process %d sent out a new election for phase %d. The status of the counter at this phase is currently %d\n",
						world_rank, to_pass.phase, replies_rcvd[to_pass.phase]);
			}

			if (reply_status == -1) {
				printf(
						"========== ERROR - GOT BAD REPLY STATUS IN PROCESS %d ===========",
						world_rank);
			}
			//			here check if the result from the reply means you win the phase
		} else if (status.MPI_TAG == LEADER) {
			leader = result.id;

			if (status.MPI_SOURCE != left) {
				printf(
						"========== ERROR - THE SOURCE OF THIS LEADER TAG WAS NOT FROM THE LEFT IN PROCESS %d ===========\n",
						world_rank);
			} else {
				result.hops = result.hops + no_sent;
				result.phase = result.hops + no_recvd;
				MPI_Send(&result, 1, mpi_pkg, right, LEADER, MPI_COMM_WORLD);
			}
		} else {

			printf(
					"============= ERROR - PROCESS %d GETTING TAG OTHER THAN ELECTION: %d =============",
					world_rank, status.MPI_TAG);

		}

		//		if (leader != -1) {
		//			printf("Process %d declares %d leader\n", world_rank, leader);
		////			MPI_Send(&leader, 1, MPI_INT, left, LEADER, MPI_COMM_WORLD);
		////			MPI_Send(&leader, 1, MPI_INT, right, LEADER, MPI_COMM_WORLD);
		//			//			MPI_Recv()
		//
		//		}
	}

	printf("rank=%d, id=%d, leader=%d, mrcvd=%d, msent=%d\n", world_rank, id,
			leader, no_recvd, no_sent);

	if (i_am_leader == 1) {
		//i am leader
		//		result.hops = no_sent + 1;
		//		result.phase = no_recvd;
		printf("rank=%d, id=%d, trcvd=%d, tsent=%d\n", world_rank, id,
				result.phase + 1, result.hops);

	}
	MPI_Finalize();

}
int elected_message(int primary, int secondary, pkg *primary_pkg) {

	const int nitems = 3;
	int blocklengths[3] = { 1, 1, 1 };
	MPI_Datatype types[3] = { MPI_INT, MPI_INT, MPI_INT };
	MPI_Datatype mpi_pkg;
	MPI_Aint offsets[3];

	offsets[0] = offsetof(pkg, id);
	offsets[1] = offsetof(pkg, phase);
	offsets[2] = offsetof(pkg, hops);

	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_pkg);
	MPI_Type_commit(&mpi_pkg);

	primary_pkg->hops = primary_pkg->hops + 1;
	int distance = primary_pkg->hops;
	int max_dist = ((primary_pkg->phase == 0) ? 1 : (1 << primary_pkg->phase));
	int max = power(2, primary_pkg->phase);


	if ((primary_pkg->id > id) && (distance < max_dist)) {
		//keep passing it forward
		//		printf(
		//				"Process rank-%d id-%d, got < id-%d. Hops-%d, max_dist-%d, max-%d. It is forwarding it to %d\n",
		//				world_rank, id, primary_pkg->id, distance, max_dist, max,
		//				secondary);
		MPI_Send(primary_pkg, 1, mpi_pkg, secondary, ELECTION, MPI_COMM_WORLD);
		no_sent++;
	}

	if ((primary_pkg->id > id) && (distance == max_dist)) {
		//		printf(
		//				"Process rank-%d id-%d, got < id-%d. Distance-%d, max_dist-%d, max-%d. It is sending it back to %d\n",
		//				world_rank, id, primary_pkg->id, distance, max_dist, max,
		//				primary);
		MPI_Send(primary_pkg, 1, mpi_pkg, primary, REPLY, MPI_COMM_WORLD);
		no_sent++;
	}

	if (primary_pkg->id == id) {
		//		printf("Rank-%d, id-%d won the round\n", world_rank, id);
		return id;
	}

	return -1;
}

int reply_message(int primary, int secondary, pkg *primary_pkg) {
	//	printf("In reply message...\n");
	const int nitems = 3;
	int blocklengths[3] = { 1, 1, 1 };
	MPI_Datatype types[3] = { MPI_INT, MPI_INT, MPI_INT };
	MPI_Datatype mpi_pkg;
	MPI_Aint offsets[3];

	offsets[0] = offsetof(pkg, id);
	offsets[1] = offsetof(pkg, phase);
	offsets[2] = offsetof(pkg, hops);

	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_pkg);
	MPI_Type_commit(&mpi_pkg);

	if (id != primary_pkg->id) {
		//		printf("Process %d is forwarding a reply message\n", world_rank);
		MPI_Send(primary_pkg, 1, mpi_pkg, secondary, REPLY, MPI_COMM_WORLD);
		no_sent++;
		return 0;

	} else {
		//then its my id
		replies_rcvd[primary_pkg->phase] = replies_rcvd[primary_pkg->phase] + 1;
		int temp = replies_rcvd[primary_pkg->phase];
		//		printf(
		//				"Process %d got its own id back for phase %d. That brings the toll to %d\n",
		//				world_rank, primary_pkg->phase, temp);
		return 1;
	}

	return -1;
}

int power(int base, int exp) {
	int i, result = 1;
	for (i = 0; i < exp; i++)
		result *= base;
	return result;
}
