#ifndef PARALUTION_UTILS_COMMUNICATOR_HPP_
#define PARALUTION_UTILS_COMMUNICATOR_HPP_

#include <mpi.h>

namespace paralution {

struct MRequest {
  MPI_Request req;
};

//TODO make const what ever possible

template <typename ValueType>
void communication_allreduce_single_sum(ValueType local, ValueType *global, const void *comm);

template <typename ValueType>
void communication_async_recv(ValueType *buf, int count, int source, int tag, MRequest *request, const void *comm);

template <typename ValueType>
void communication_async_send(ValueType *buf, int count, int dest, int tag, MRequest *request, const void *comm);

void communication_syncall(int count, MRequest *requests);

}

#endif // PARALUTION_UTILS_COMMUNICATOR_HPP_